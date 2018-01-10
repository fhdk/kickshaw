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

#ifndef __kickshaw_h
#define __kickshaw_h

extern void action_option_insert (gchar *origin);
extern void add_new (gchar *new_menu_element);
extern void boolean_toggled (void);
extern void cell_edited (G_GNUC_UNUSED GtkCellRendererText *renderer, 
                                       gchar               *path, 
                                       gchar               *new_text, 
                                       gpointer             column_number_pointer);
extern void change_row (void);
extern gboolean check_expansion_statuses_of_nodes (GtkTreeModel *foreach_or_filter_model, 
                                                   GtkTreePath  *foreach_or_filter_path, 
                                                   GtkTreeIter  *foreach_or_filter_iter, 
                                                   gboolean     *expansion_statuses_of_nodes);
extern gboolean check_for_match (GtkTreeIter *local_iter, guint8 column_number);
extern guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
extern void create_context_menu (GdkEventButton *event);
extern GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *icon_name, gchar *label_txt, 
                                 gchar *button_txt_1, gchar *button_txt_2, gchar *button_txt_3, 
                                 gboolean show_immediately);
extern void create_invalid_icon_imgs (void);
extern void create_list_of_icon_occurrences (void);
extern GList *create_list_of_rows_with_found_occurrences (void);
extern void drag_data_received_handler (G_GNUC_UNUSED GtkWidget      *widget, 
                                        G_GNUC_UNUSED GdkDragContext *drag_context, 
                                                      gint            x, 
                                                      gint            y);
extern gboolean drag_motion_handler (G_GNUC_UNUSED GtkWidget      *widget, 
                                                   GdkDragContext *drag_context, 
                                                   gint            x, 
                                                   gint            y, 
                                                   guint           time);
extern void find_buttons_management (gchar *column_check_button_clicked);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern guint get_font_size (void);
extern void get_menu_elements_from_file (gchar *new_filename);
extern void hide_action_option_grid (void);
extern void icon_choosing_by_button_or_context_menu (void);
extern void jump_to_previous_or_next_occurrence (gpointer direction_pointer);
extern void key_pressed (G_GNUC_UNUSED GtkWidget   *widget, 
                                       GdkEventKey *event);
extern void move_selection (gpointer direction_pointer);
extern gboolean multiple_str_replacement_callback_func (const GMatchInfo *match_info, 
                                                        GString *result, gpointer hash_table);
extern void one_of_the_change_values_buttons_pressed (gchar *origin);
extern void open_menu (void);
extern void option_list_with_headlines (G_GNUC_UNUSED GtkCellLayout   *cell_layout, 
                                                      GtkCellRenderer *action_option_combo_box_renderer, 
                                                      GtkTreeModel    *action_option_combo_box_model, 
                                                      GtkTreeIter     *action_option_combo_box_iter, 
                                        G_GNUC_UNUSED gpointer         data);
extern void remove_all_children (void);
extern void remove_icons_from_menus_or_items (void);
extern void remove_rows (gchar *origin);
extern void row_selected (void);
extern void run_search (void);
extern void save_menu (gchar *save_as_filename);
extern void save_menu_as (void);
extern void show_action_options (void);
extern void show_or_hide_find_grid (void);
extern void show_startupnotify_options (void);
extern void single_field_entry (void);
extern gboolean sort_loop_after_sorting_activation (GtkTreeModel *foreach_model, GtkTreePath *foreach_path,
                                                    GtkTreeIter *foreach_iter);
extern void stop_timeout (void);
G_GNUC_NULL_TERMINATED extern gboolean streq_any (const gchar *string, ...);
extern void visualise_menus_items_and_separators (gpointer recursively_pointer);

#endif
