// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "converter_preprocessor.h"
#include "converter_registrar.h"
#include "default_css.h"
#include "document_geany.h"
#include "document_local.h"
#include "preview_config.h"
#include "preview_context.h"
#include "renderers_pdf.h"
#include "util/file_utils.h"
#include "util/gtk_utils.h"
#include "util/string_utils.h"
#include "util/xdg_utils.h"
#include "webview.h"

PreviewPane::PreviewPane(PreviewContext *context)
    : context_(context),
      sidebar_notebook_(context_->geany_sidebar_),
      preview_config_(context_->preview_config_),
      webview_(context) {
  context_->webview_ = &webview_;

  page_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  offscreen_ = gtk_offscreen_window_new();
  gtk_widget_show(offscreen_);

  if (GTK_IS_NOTEBOOK(sidebar_notebook_)) {
    sidebar_page_number_ = gtk_notebook_append_page(
        GTK_NOTEBOOK(sidebar_notebook_), page_box_, gtk_label_new("Preview")
    );
    gtk_widget_show_all(page_box_);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar_notebook_), sidebar_page_number_);

    // for styling with geany.css
    gtk_widget_set_name(GTK_WIDGET(page_box_), "geany-preview-sidebar-page");

    // simulated :focus-within tracking for CSS styling
    GtkUtils::enableFocusWithinTracking(page_box_);
  }

  addWatchIfNeeded(preview_config_->configDir() / "preview.css");
  addWatchIfNeeded(preview_config_->configDir() / (previous_key_ + ".css"));

  // initial webview
  webview_.reset();
  connectWebViewSignals();

  DocumentLocal local_doc("/somewhere-out-there/over-the-rainbow.ftn");
  initWebView(local_doc);

  if (GeanyDocument *doc = document_get_current()) {
    DocumentGeany document(doc);
    triggerUpdate(document);
  }

  safeReparentWebView(page_box_, true);

  // config callback
  preview_config_->connectChanged([this]() {
    webview_.reset();
    connectWebViewSignals();

    DocumentLocal local_doc("/somewhere-out-there/over-the-rainbow.ftn");
    initWebView(local_doc);

    if (GeanyDocument *doc = document_get_current()) {
      DocumentGeany document(doc);
      triggerUpdate(document);
    }

    safeReparentWebView(page_box_, true);
  });

  sidebar_switch_page_handler_id_ = g_signal_connect(
      sidebar_notebook_,
      "switch-page",
      G_CALLBACK(
          +[](GtkNotebook *notebook, GtkWidget *page, guint /*page_num*/, gpointer user_data) {
            auto *self = static_cast<PreviewPane *>(user_data);
            DocumentGeany document(document_get_current());

            if (page == self->page_box_) {
              self->safeReparentWebView(self->page_box_, true);
              self->triggerUpdate(document);
            } else {
              int w = gtk_widget_get_allocated_width(self->page_box_);
              int h = gtk_widget_get_allocated_height(self->page_box_);
              if (w > 0 && h > 0) {
                gtk_window_resize(GTK_WINDOW(self->offscreen_), w, h);
              }
              self->safeReparentWebView(self->offscreen_, false);
            }
          }
      ),
      this
  );
}

PreviewPane::~PreviewPane() noexcept {
  if (page_box_ && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
    int idx = gtk_notebook_page_num(GTK_NOTEBOOK(sidebar_notebook_), page_box_);
    if (idx >= 0) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar_notebook_), idx);
    }
  }

  if (GtkWidget *wv = webview_.widget()) {
    if (GTK_IS_WIDGET(wv)) {
      if (GtkWidget *p = gtk_widget_get_parent(wv)) {
        gtk_container_remove(GTK_CONTAINER(p), wv);
      }
    }
  }

  if (sidebar_switch_page_handler_id_) {
    g_signal_handler_disconnect(sidebar_notebook_, sidebar_switch_page_handler_id_);
    sidebar_switch_page_handler_id_ = 0;
  }

  if (GTK_IS_WIDGET(offscreen_)) {
    gtk_widget_destroy(offscreen_);
    offscreen_ = nullptr;
  }

  stopAllWatches();
}

GtkWidget *PreviewPane::widget() const {
  return page_box_ ? page_box_ : webview_.widget();
}

PreviewPane &PreviewPane::initWebView(const Document &document) {
  previous_base_uri_.clear();
  previous_key_.clear();
  previous_theme_.clear();

  const std::string base_uri = calculateBaseUri(document);
  root_id_ = "geany-preview-" + StringUtils::randomHex(8);
  webview_.loadHtml("", base_uri, root_id_, nullptr);

  clearAndReloadCss();

  triggerUpdate(document);
  return *this;
}

void PreviewPane::connectWebViewSignals() {
  init_handler_id_ = g_signal_connect(
      webview_.widget(),
      "load-changed",
      G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent e, gpointer user_data) {
        auto *self = static_cast<PreviewPane *>(user_data);
        if (e == WEBKIT_LOAD_FINISHED) {
          self->scheduleUpdate();
          g_signal_handler_disconnect(self->webview_.widget(), self->init_handler_id_);
          self->init_handler_id_ = 0;
        }
      }),
      this
  );

  g_signal_connect(
      webview_.widget(),
      "load-changed",
      G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent e, gpointer user_data) {
        auto *self = static_cast<PreviewPane *>(user_data);
        if (e == WEBKIT_LOAD_FINISHED) {
          self->checkHealth();
        }
      }),
      this
  );
}

PreviewPane &PreviewPane::checkHealth() {
  if (!webview_healthy_) {
    return *this;
  }

  std::string js =
      "!("
      "document.documentElement && "
      "document.head && "
      "document.body && "
      "document.getElementById(`" +
      WebView::escapeForJsTemplateLiteral(root_id_) +
      "`)"
      ")";

  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(webview_.widget()),
      js.c_str(),
      -1,
      nullptr,
      nullptr,
      nullptr,
      [](GObject *source, GAsyncResult *res, gpointer user_data) {
        auto *self = static_cast<PreviewPane *>(user_data);
        GError *err = nullptr;
        JSCValue *val =
            webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, &err);

        bool needs_reinit = false;
        if (!err && jsc_value_is_boolean(val)) {
          needs_reinit = jsc_value_to_boolean(val);
        }
        if (G_IS_OBJECT(val)) {
          g_object_unref(val);
        }
        if (err) {
          g_error_free(err);
        }

        self->webview_healthy_ = !needs_reinit;
      },
      this
  );
  return *this;
}

PreviewPane &PreviewPane::scheduleUpdate() {
  if (update_pending_) {
    return *this;
  }

  update_pending_ = true;

  gint64 now = g_get_monotonic_time() / 1000;
  gint64 update_cooldown_ms_ = preview_config_->get<int>("update_cooldown");
  gint64 update_min_delay_ = preview_config_->get<int>("update_min_delay");

  gint64 delay_ms =
      std::max(update_min_delay_, update_cooldown_ms_ - (now - last_update_time_));

  g_timeout_add(
      delay_ms,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<PreviewPane *>(data);
        DocumentGeany document(document_get_current());
        self->triggerUpdate(document);
        return G_SOURCE_REMOVE;
      },
      this
  );
  return *this;
}

void PreviewPane::triggerUpdate(const Document &document) {
  update(document);
  last_update_time_ = g_get_monotonic_time() / 1000;
  update_pending_ = false;
}

void PreviewPane::exportHtmlToFileAsync(
    const std::filesystem::path &dest,
    std::function<void(bool)> callback
) {
  webview_.getDomSnapshot(
      root_id_, [dest, cb = std::move(callback)](const std::string &content_html) {
        bool success = false;
        if (!content_html.empty()) {
          std::error_code ec;
          if (!dest.parent_path().empty()) {
            std::filesystem::create_directories(dest.parent_path(), ec);
          }
          std::ofstream out(dest, std::ios::binary | std::ios::trunc);
          if (out) {
            out << "<!doctype html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n</head>\n"
                << "<body>\n"
                << content_html << "\n</body>\n</html>\n";
            out.flush();
            out.close();
            success = out.good();
          }
        }
        cb(success);
      }
  );
}

void PreviewPane::exportPdfToFileAsync(
    const std::filesystem::path &dest,
    std::function<void(bool)> callback
) {
  DocumentGeany document(document_get_current());

  // Ensure parent directories exist
  std::error_code ec;
  if (!dest.parent_path().empty()) {
    std::filesystem::create_directories(dest.parent_path(), ec);  // best-effort
  }

#ifdef HAVE_PODOFO
  // Fountain with PoDoFo
  if (registrar_.getConverterKey(document) == "fountain") {
    bool ok = Fountain::ftn2pdf(dest.string(), document.text());
    callback(ok);
    return;
  }
#endif

  // Fallback: WebKit print-to-PDF
  GtkPrintSettings *settings = gtk_print_settings_new();
  gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_PRINTER, "Print to File");
  gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");

  if (char *uri = g_filename_to_uri(dest.c_str(), nullptr, nullptr)) {
    gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_URI, uri);
    g_free(uri);
  } else {
    g_object_unref(settings);
    callback(false);
    return;
  }

  WebKitPrintOperation *op = webkit_print_operation_new(WEBKIT_WEB_VIEW(webview_.widget()));
  webkit_print_operation_set_print_settings(op, settings);

  GtkPageSetup *page_setup = gtk_page_setup_new();
  webkit_print_operation_set_page_setup(op, page_setup);

  // Connect to the "finished" signal to know when printing completes
  g_signal_connect(
      op,
      "finished",
      G_CALLBACK(+[](WebKitPrintOperation *, gboolean success, gpointer user_data) {
        auto *cb_ptr = static_cast<std::function<void(bool)> *>(user_data);
        (*cb_ptr)(success);
        delete cb_ptr;
      }),
      new std::function<void(bool)>(std::move(callback))
  );

  webkit_print_operation_print(op);

  g_object_unref(settings);
  g_object_unref(page_setup);
  g_object_unref(op);
}

bool PreviewPane::canPreviewFile(const Document &doc) const {
  return !registrar_.getConverterKey(doc).empty();
}

void PreviewPane::safeReparentWebView(GtkWidget *new_parent, bool pack_into_box) {
  GtkWidget *wv = webview_.widget();
  if (!GTK_IS_WIDGET(new_parent) || !GTK_IS_WIDGET(wv)) {
    return;
  }

  g_object_ref(wv);

  if (GtkWidget *old_parent = gtk_widget_get_parent(wv)) {
    gtk_container_remove(GTK_CONTAINER(old_parent), wv);
  }

  if (GTK_IS_BIN(new_parent)) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(new_parent));
    if (child && child != wv) {
      gtk_container_remove(GTK_CONTAINER(new_parent), child);
    }
  }

  if (pack_into_box && GTK_IS_BOX(new_parent)) {
    gtk_box_pack_start(GTK_BOX(new_parent), wv, true, true, 0);
  } else {
    gtk_container_add(GTK_CONTAINER(new_parent), wv);
  }

  gtk_widget_show(wv);
  g_object_unref(wv);
}

std::string PreviewPane::generateHtml(const Document &document) const {
  ConverterPreprocessor pre(document, preview_config_->get<int>("headers_incomplete_max"));

  Converter *converter = nullptr;
  if (!pre.type().empty()) {
    converter = registrar_.getConverter(pre.type());
  }
  if (!converter) {
    converter = registrar_.getConverter(document);
  }

  auto normalizedType = [](std::string_view t) {
    auto s = StringUtils::trimWhitespace(t);
    s = StringUtils::toLower(s);
    auto semi = s.find(';');
    if (semi != std::string::npos) {
      s = StringUtils::trimWhitespace(s.substr(0, semi));
    }
    return std::string{ s };
  };

  if (converter) {
    return pre.headersToHtml() + std::string{ converter->toHtml(pre.body()) };
  } else {
    std::string html = "<tt>";
    html += std::string{ context_->geany_plugin_->info->name } + " ";
    html += std::string{ context_->geany_plugin_->info->version } + "</br>";
    html += "&nbsp;&nbsp;&nbsp;";
    if (!document.filetypeName().empty()) {
      html += document.filetypeName() + ", " + document.encodingName();
    } else {
      html += "Unknown";
    }
    if (!pre.type().empty()) {
      html += ", " + normalizedType(pre.type());
    }
    html += "</tt>";
    return html;
  }
}

namespace {
std::string toUri(const std::filesystem::path &path, const std::string &fallback) {
  std::filesystem::path p = path;
  if (!p.is_absolute()) {
    p = std::filesystem::path("/") / p;
  }
  p /= "";
  std::string normalized = p.lexically_normal().string();

  gchar *uri = g_filename_to_uri(normalized.c_str(), nullptr, nullptr);
  if (uri) {
    std::string out(uri);
    g_free(uri);
    return out.empty() ? fallback : out;
  }
  return fallback;
}
}  // namespace

std::string PreviewPane::calculateBaseUri(const Document &document) const {
  std::string config_path = preview_config_->get<std::string>("preview_base_path", "sandbox");
  config_path = config_path.empty() ? "sandbox" : XdgUtils::expandEnvVars(config_path);

  std::string default_base_uri = toUri(config_path, "file:///sandbox/");

  auto file_path = std::filesystem::path(document.filePath()).lexically_normal();
  if (file_path.empty()) {
    return default_base_uri;
  }

  auto dir_path = file_path.parent_path();
  if (dir_path.empty()) {
    return default_base_uri;
  }

  return toUri(dir_path, default_base_uri);
}

PreviewPane &PreviewPane::update(const Document &document) {
  std::string html = generateHtml(document);

  // load new css on document type change
  auto key = registrar_.getConverterKey(document);
  if (key != previous_key_) {
    addWatchIfNeeded(preview_config_->configDir() / std::string{ key + ".css" });
    previous_key_ = key;
    clearAndReloadCss();
  }

  auto theme = preview_config_->get<std::string>("theme_mode", "system");
  if (theme != previous_theme_) {
    injectCssTheme();
  }

  auto file = document.filePath();
  std::string base_uri = calculateBaseUri(document);

  if (base_uri != previous_base_uri_) {
    previous_base_uri_ = base_uri;
    webview_.loadHtml(html, base_uri, root_id_, &scroll_by_file_[file]);
  } else if (!webview_healthy_) {
    webview_.loadHtml(html, base_uri, root_id_, &scroll_by_file_[file]);
    webview_healthy_ = true;
  } else {
    webview_.getScrollFraction([this, file, base_uri, html](double frac) {
      scroll_by_file_[file] = frac;
      webview_.updateHtml(html, base_uri, root_id_, &scroll_by_file_[file]);
    });
  }
  return *this;
}

void PreviewPane::addWatchIfNeeded(const std::filesystem::path &path) {
  if (watches_.find(path) != watches_.end()) {
    return;
  }

  // Construct the handle in-place in the map
  auto [it, inserted] = watches_.emplace(path, FileUtils::FileWatchHandle{});

  FileUtils::watchFile(it->second, path, [this, path]() {
    auto preview_css_path = preview_config_->configDir() / "preview.css";
    auto previous_css_path = preview_config_->configDir() / (previous_key_ + ".css");
    if (path == preview_css_path || path == previous_css_path) {
      clearAndReloadCss();
    } else {
      // do nothing; different file changed
    }
  });
}

void PreviewPane::stopAllWatches() {
  for (auto &[path, handle] : watches_) {
    FileUtils::stopWatching(handle);
  }
  watches_.clear();
}

PreviewPane &PreviewPane::clearAndReloadCss() {
  auto preview_css_path = preview_config_->configDir() / "preview.css";
  auto previous_css_path = preview_config_->configDir() / (previous_key_ + ".css");

  webview_.clearInjectedCss();
  if (FileUtils::fileExists(preview_css_path)) {
    webview_.injectCssFromFile(preview_css_path);
  } else {
    auto it = kDefaultCssMap.find("preview.css");
    std::string_view css = (it != kDefaultCssMap.end()) ? it->second : std::string_view{};
    webview_.injectCssFromLiteral(css.data());
  }

  if (FileUtils::fileExists(previous_css_path)) {
    webview_.injectCssFromFile(previous_css_path);
  } else {
    auto it = kDefaultCssMap.find(previous_key_ + ".css");
    std::string_view css = (it != kDefaultCssMap.end()) ? it->second : std::string_view{};
    webview_.injectCssFromLiteral(css.data());
  }

  injectCssTheme();
  return *this;
}

PreviewPane &PreviewPane::injectCssTheme() {
  previous_theme_ = preview_config_->get<std::string>("theme_mode", "system");
  if (previous_theme_ == "light") {
    webview_.injectCssFromString("html { color-scheme: light; }");
  } else if (previous_theme_ == "dark") {
    webview_.injectCssFromString("html { color-scheme: dark; }");
  } else {
    webview_.injectCssFromString("html { color-scheme: light dark; }");
  }
  return *this;
}
