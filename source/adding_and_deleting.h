/*
   Kickshaw - A Menu Editor for Openbox

   Copyright (c) 2010-2018        Marcus Schaetzle

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Kickshaw. If not, see http://www.gnu.org/licenses/.
*/

#ifndef __adding_and_deleting_h
#define __adding_and_deleting_h

extern ks_data ks;

extern void activate_change_done (void);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts,
                                        gchar **options_array, gboolean *opts_exist);
extern gboolean check_for_external_file_and_settings_changes (G_GNUC_UNUSED gpointer identifier);
extern guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
extern GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *icon_name, gchar *label_txt,
                                 gchar *button_txt_1, gchar *button_txt_2, gchar *button_txt_3,
                                 gboolean show_immediately);
extern void expand_row_from_iter (GtkTreeIter *local_iter);
extern gchar *extract_substring_via_regex (gchar *string, gchar *regex_str);
extern gchar *get_modification_time_for_icon (gchar *icon_path);
extern void repopulate_txt_fields_array (void);
extern void row_selected (void);
extern gboolean set_icon (GtkTreeIter *icon_iter, gchar *icon_path, gboolean display_err_msg);
extern void show_errmsg (gchar *errmsg_raw_txt);
extern void show_or_hide_find_grid (void);
extern void sort_execute_or_startupnotify_options_after_insertion (GtkTreeSelection *selection, GtkTreeIter *parent,
                                                                   gchar *execute_or_startupnotify, gchar *option);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;
extern void wrong_or_missing (GtkWidget *widget, GtkCssProvider *css_provider);

#endif
