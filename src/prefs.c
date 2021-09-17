#include "prefs.h"

#define HAS_KEY(gp, key) g_key_file_has_key(kf, gp, key, NULL)
#define GET_KEY(T, gp, key) g_key_file_get_##T(kf, gp, key, NULL)
#define SET_KEY(T, gp, key, val) g_key_file_set_##T(kf, gp, key, val)

extern GeanyData *geany_data;

struct PreviewSettings settings;

void init_settings() {
  settings.update_interval = 200;
  settings.background_interval = 5000;

  settings.html_processor = g_strdup("native");
  settings.markdown_processor = g_strdup("native");
  settings.asciidoc_processor = g_strdup("asciidoctor");
  settings.fountain_processor = g_strdup("screenplain");
  settings.wiki_default = g_strdup("mediawiki");

  settings.pandoc_fragment = FALSE;
  settings.pandoc_toc = FALSE;
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
    save_settings();
    return;
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

  SET_KEY(integer, "preview", "update_interval", settings.update_interval);
  SET_KEY(integer, "preview", "background_interval",
          settings.background_interval);

  SET_KEY(string, "preview", "html_processor", settings.html_processor);
  SET_KEY(string, "preview", "markdown_processor",
          settings.markdown_processor);
  SET_KEY(string, "preview", "asciidoc_processor",
          settings.asciidoc_processor);
  SET_KEY(string, "preview", "fountain_processor",
          settings.fountain_processor);
  SET_KEY(string, "preview", "wiki_default", settings.wiki_default);

  SET_KEY(boolean, "preview", "pandoc_fragment", settings.pandoc_fragment);
  SET_KEY(boolean, "preview", "pandoc_toc", settings.pandoc_toc);

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

  if (HAS_KEY("preview", "update_interval")) {
    int val = GET_KEY(integer, "preview", "update_interval");
    if (val) {
      settings.update_interval = val;
    } else {
      settings.update_interval = 200;
    }
  }

  if (HAS_KEY("preview", "background_interval")) {
    int val = GET_KEY(integer, "preview", "background_interval");
    if (val) {
      settings.update_interval = val;
    } else {
      settings.update_interval = 5000;
    }
  }

  if (HAS_KEY("preview", "html_processor")) {
    char *val = GET_KEY(string, "preview", "html_processor");
    if (val) {
      settings.markdown_processor = g_strdup(val);
    } else {
      settings.markdown_processor = g_strdup("native");
    }
    g_free(val);
  }

  if (HAS_KEY("preview", "markdown_processor")) {
    char *val = GET_KEY(string, "preview", "markdown_processor");
    if (val) {
      settings.markdown_processor = g_strdup(val);
    } else {
      settings.markdown_processor = g_strdup("native");
    }
    g_free(val);
  }

  if (HAS_KEY("preview", "asciidoc_processor")) {
    char *val = GET_KEY(string, "preview", "asciidoc_processor");
    if (val) {
      settings.markdown_processor = g_strdup(val);
    } else {
      settings.markdown_processor = g_strdup("asciidoctor");
    }
    g_free(val);
  }

  if (HAS_KEY("preview", "fountain_processor")) {
    char *val = GET_KEY(string, "preview", "fountain_processor");
    if (val) {
      settings.markdown_processor = g_strdup(val);
    } else {
      settings.markdown_processor = g_strdup("screenplain");
    }
    g_free(val);
  }

  if (HAS_KEY("preview", "wiki_default")) {
    char *val = GET_KEY(string, "preview", "wiki_default");
    if (val) {
      settings.markdown_processor = g_strdup(val);
    } else {
      settings.markdown_processor = g_strdup("mediawiki");
    }
    g_free(val);
  }

  if (HAS_KEY("preview", "pandoc_fragment")) {
    settings.pandoc_fragment =
        GET_KEY(boolean, "preview", "pandoc_fragment");
  }

  if (HAS_KEY("preview", "pandoc_toc")) {
    settings.pandoc_toc = GET_KEY(boolean, "preview", "pandoc_toc");
  }
}
