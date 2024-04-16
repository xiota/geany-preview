/*

 * Preview Geany Plugin
 * Copyright 2024 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pv_config_dialog.h"

extern GeanyData *geany_data;

extern PreviewSettings *gSettings;

GtkWidget *getConfigDialog(GtkDialog *dialog) {
  GtkWidget *box, *btn;
  char *tooltip;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

  tooltip = g_strdup(_("Save the active settings to the config file."));
  btn = gtk_button_new_with_label(_("Save Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(saveConfig), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip =
      g_strdup(_("Reload settings from the config file.  May be used to "
                 "apply preferences after editing without restarting Geany."));
  btn = gtk_button_new_with_label(_("Reload Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(reloadConfig), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip =
      g_strdup(_("Delete the current config file and restore the default "
                 "file with explanatory comments."));
  btn = gtk_button_new_with_label(_("Reset Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(resetConfig), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip = g_strdup(_("Open the config file in Geany for editing."));
  btn = gtk_button_new_with_label(_("Edit Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(editConfig), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip = g_strdup(
      _("Open the config folder in the default file manager.  "
        "The config folder contains the stylesheets, which may be edited."));
  btn = gtk_button_new_with_label(_("Open Config Folder"));
  g_signal_connect(btn, "clicked", G_CALLBACK(openConfigFolder), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);
  return box;
}

void saveConfig(GtkWidget *self, GtkWidget *dialog) { gSettings->save(); }

void reloadConfig(GtkWidget *self, GtkWidget *dialog) {
  gSettings->load();
  // wv_apply_settings();
}

void resetConfig(GtkWidget *self, GtkWidget *dialog) { gSettings->reset(); }

void editConfig(GtkWidget *self, GtkWidget *dialog) {
  namespace fs = std::filesystem;

  gSettings->load();

  fs::path conf_fn = fs::path{geany_data->app->configdir} / "plugins" /
                     "preview" / "preview.conf";

  GeanyDocument *doc =
      document_open_file(conf_fn.c_str(), false, nullptr, nullptr);
  document_reload_force(doc, nullptr);

  if (dialog != nullptr) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}

void openConfigFolder(GtkWidget *self, GtkWidget *dialog) {
  namespace fs = std::filesystem;

  fs::path conf_dir =
      fs::path{geany_data->app->configdir} / "plugins" / "preview";
  std::string command = R"(xdg-open ")" + conf_dir.string() + R"(")";

  if (std::system(command.c_str())) {
    // ignore return value
  }
}
