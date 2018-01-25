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

#ifndef __selecting_h
#define __selecting_h

extern ks_data ks;

extern void add_new (gchar *new_menu_element);
extern void change_row (void);
extern void hide_action_option_grid (gchar *origin);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts,
                                        gchar **options_array, gboolean *opts_exist);
extern gboolean check_if_invisible_descendant_exists (              GtkTreeModel *filter_model,
                                                      G_GNUC_UNUSED GtkTreePath  *filter_path,
                                                                    GtkTreeIter  *filter_iter,
                                                                    gboolean     *at_least_one_descendant_is_invisible);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern void generate_items_for_action_option_combo_box (gchar *preset_choice);
extern void repopulate_txt_fields_array (void);
extern void set_status_of_expand_and_collapse_buttons_and_menu_items (void);
extern void show_msg_in_statusbar (gchar *message);
extern void stop_timeout (void);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;
extern void wrong_or_missing (GtkWidget *widget, GtkCssProvider *provider);

#endif
