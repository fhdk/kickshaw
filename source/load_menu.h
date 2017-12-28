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

#ifndef __load_menu_h
#define __load_menu_h

#define free_and_reassign(string, new_value) { g_free (string); string = new_value; }
#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern GtkTreeStore *treestore;
extern GtkTreeModel *model;
extern GtkWidget *treeview;
extern GtkTreeIter iter;

extern GtkWidget *mb_view_and_options[];

extern GSList *menu_ids;

extern GdkPixbuf *invalid_icon_imgs[];

extern gboolean autosort_options;
extern gboolean change_done;

extern gint font_size;

extern gint handler_id_row_selected;

extern guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
extern gchar *choose_icon (void);
extern void clear_global_data (void);
extern gboolean continue_despite_unsaved_changes (void);
extern GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *icon_name, gchar *label_txt, 
                                 gchar *button_txt_1, gchar *button_txt_2, gchar *button_txt_3, 
                                 gboolean show_immediately);
extern void create_file_dialog (GtkWidget **dialog, gboolean open);
extern void create_list_of_icon_occurrences (void);
extern gchar *extract_substring_via_regex (gchar *string, gchar *regex_str);
extern gchar *get_modification_time_for_icon (gchar *icon_path);
extern GtkWidget *new_label_with_formattings (gchar *label_txt, gboolean wrap);
extern void remove_rows (gchar *origin);
extern void row_selected (void);
extern void set_filename_and_window_title (gchar *new_filename);
extern void show_errmsg (gchar *errmsg_raw_txt);
extern gboolean sort_loop_after_sorting_activation (GtkTreeModel *foreach_model, GtkTreePath *foreach_path,
                                                    GtkTreeIter *foreach_iter);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;

#endif
