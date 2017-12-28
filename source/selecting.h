/*
   Kickshaw - A Menu Editor for Openbox

   Copyright (c) 2010-2017        Marcus Schaetzle

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

#ifndef __selecting_h
#define __selecting_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern gchar *execute_options[];
extern gchar *execute_displayed_txts[];

extern gchar *startupnotify_options[];
extern gchar *startupnotify_displayed_txts[];

extern GtkTreeModel *model;
extern GtkWidget *treeview;
extern GtkTreeIter iter;

extern GtkWidget *mb_file_menu_items[];
extern GtkWidget *mb_edit;
extern GtkWidget *mb_edit_menu_items[];
extern GtkWidget *mb_search;

extern GtkToolItem *tb[];

extern GtkWidget *add_image;
extern GtkWidget *bt_bar_label;
extern GtkWidget *bt_add[];
extern GtkWidget *bt_add_action_option_label;

extern GtkWidget *action_option_grid;

extern GtkWidget *find_entry_buttons[];
extern GList *rows_with_found_occurrences;

extern GtkWidget *entry_grid;
extern GtkWidget *entry_labels[], *entry_fields[];
extern GtkCssProvider *find_entry_css_provider;
extern GtkWidget *icon_chooser, *remove_icon;

extern GtkWidget *statusbar;
extern gboolean statusbar_msg_shown;

extern gchar *txt_fields[];

extern gboolean change_done;
extern gboolean autosort_options;
extern gchar *filename;

extern GtkTargetEntry enable_list[];
extern GSList *source_paths;

extern gint handler_id_action_option_button_clicked;

extern void add_new (gchar *new_menu_element);
extern void hide_action_option_grid (void);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
                                        gchar **options_array, gboolean *opts_exist);
extern gboolean check_if_invisible_descendant_exists (              GtkTreeModel *filter_model, 
                                                      G_GNUC_UNUSED GtkTreePath  *filter_path, 
                                                                    GtkTreeIter  *filter_iter, 
                                                                    gboolean     *at_least_one_descendant_is_invisible);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern void generate_action_option_combo_box (gchar *preset_choice);
extern void repopulate_txt_fields_array (void);
extern void set_status_of_expand_and_collapse_buttons_and_menu_items (void);
extern void show_msg_in_statusbar (gchar *message);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;
extern void wrong_or_missing (GtkWidget *widget, GtkCssProvider *provider);

#endif
