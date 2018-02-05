/*
   Kickshaw - A Menu Editor for Openbox

   Copyright (c) 2010–2018        Marcus Schätzle

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

#ifndef __editing_h
#define __editing_h

extern ks_data ks;

extern void activate_change_done (void);
extern guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
extern GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *icon_name, gchar *label_txt, 
                                 gchar *button_txt_1, gchar *button_txt_2, gchar *button_txt_3, 
                                 gboolean show_immediately);
extern gchar *get_modification_time_for_icon (gchar *icon_path);
extern void get_toplevel_iter_from_path (GtkTreeIter *local_iter, GtkTreePath *local_path);
extern void remove_menu_id (gchar *menu_id);
extern void remove_rows (gchar *origin);
extern void repopulate_txt_fields_array (void);
extern void row_selected (void);
extern void set_entry_fields (void);
extern void show_errmsg (gchar *errmsg_raw_txt);
G_GNUC_NULL_TERMINATED gboolean streq_any (const gchar *string, ...);

#endif
