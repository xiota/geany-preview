#include "prefs.h"

// Global Variables
extern GeanyData *geany_data;
struct PreviewSettings settings;

// Functions
void init_settings() {
  settings.update_interval = 200;
  settings.background_interval = 5000;
  settings.html_processor = g_strdup("native");
  settings.markdown_processor = g_strdup("native");
  settings.asciidoc_processor = g_strdup("asciidoctor");
  settings.fountain_processor = g_strdup("screenplain");
  settings.wiki_default = g_strdup("mediawiki");
  settings.pandoc_disabled = FALSE;
  settings.pandoc_fragment = FALSE;
  settings.pandoc_toc = FALSE;
  settings.pandoc_markdown = g_strdup("gfm");
  settings.default_font_family = g_strdup("serif");
}

void open_settings() {
  init_settings();

  char *conf_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", NULL);
  char *conf_dn = g_path_get_dirname(conf_fn);
  g_mkdir_with_parents(conf_dn, 0755);

  GKeyFile *kf = g_key_file_new();

  // if file does not exist, create it with default settings
  if (!g_file_test(conf_fn, G_FILE_TEST_EXISTS)) {
    save_default_settings();
  }

  g_key_file_load_from_file(
      kf, conf_fn, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
      NULL);

  load_settings(kf);

  g_free(conf_fn);
  g_free(conf_dn);
  g_key_file_free(kf);
}

void save_settings() {
  GKeyFile *kf = g_key_file_new();
  char *contents;
  size_t length = 0;
  char *fn = g_build_filename(geany_data->app->configdir, "plugins", "preview",
                              "preview.conf", NULL);

  // Load old contents in case user changed file outside of GUI
  g_key_file_load_from_file(
      kf, fn, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

  // Update with new contents

  SET_KEY(integer, "update_interval", settings.update_interval);
  SET_KEY(integer, "background_interval", settings.background_interval);

  SET_KEY(string, "html_processor", settings.html_processor);
  SET_KEY(string, "markdown_processor", settings.markdown_processor);
  SET_KEY(string, "asciidoc_processor", settings.asciidoc_processor);
  SET_KEY(string, "fountain_processor", settings.fountain_processor);
  SET_KEY(string, "wiki_default", settings.wiki_default);

  SET_KEY(boolean, "pandoc_disabled", settings.pandoc_disabled);
  SET_KEY(boolean, "pandoc_fragment", settings.pandoc_fragment);
  SET_KEY(boolean, "pandoc_toc", settings.pandoc_toc);
  SET_KEY(string, "pandoc_markdown", settings.pandoc_markdown);
  SET_KEY(string, "default_font_family", settings.default_font_family);

  contents = g_key_file_to_data(kf, &length, NULL);
  if (contents) {
    // Store back on disk
    g_file_set_contents(fn, contents, length, NULL);
    g_free(contents);
  }

  g_free(fn);
  g_key_file_free(kf);
}

void load_settings(GKeyFile *kf) {
  if (!g_key_file_has_group(kf, "preview")) {
    return;
  }

  LOAD_KEY_INTEGER(update_interval, 200);
  LOAD_KEY_INTEGER(background_interval, 5000);

  LOAD_KEY_STRING(html_processor, "native");
  LOAD_KEY_STRING(markdown_processor, "native");
  LOAD_KEY_STRING(asciidoc_processor, "asciidoctor");
  LOAD_KEY_STRING(fountain_processor, "screenplain");
  LOAD_KEY_STRING(wiki_default, "mediawiki");

  LOAD_KEY_BOOLEAN(pandoc_disabled);
  LOAD_KEY_BOOLEAN(pandoc_fragment);
  LOAD_KEY_BOOLEAN(pandoc_toc);

  LOAD_KEY_STRING(pandoc_markdown, "gfm");
  LOAD_KEY_STRING(default_font_family, "serif");
}

void save_default_settings() {
  char *conf_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", NULL);
  char *conf_dn = g_path_get_dirname(conf_fn);
  g_mkdir_with_parents(conf_dn, 0755);

  if (!g_file_test(conf_fn, G_FILE_TEST_EXISTS)) {
    if (g_file_test(PREVIEW_CONFIG, G_FILE_TEST_EXISTS)) {
      char *contents = NULL;
      size_t length = 0;
      if (g_file_get_contents(PREVIEW_CONFIG, &contents, &length, NULL)) {
        g_file_set_contents(conf_fn, contents, length, NULL);
        g_free(contents);
      }
    }
  }
  g_free(conf_dn);
  g_free(conf_fn);
}