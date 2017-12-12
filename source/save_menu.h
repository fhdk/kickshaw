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
   GNU General Public License for more details.    g_free (filename);

   You should have received a copy of the GNU General Public License along 
   with Kickshaw. If not, see http://www.gnu.org/licenses/.
*/

#ifndef __save_menu_h
#define __save_menu_h

#define not_NULL_and_not_empty(string) (string && *string)
#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern GtkTreeModel *model;

extern GtkWidget *mb_file_menu_items[];
extern GtkToolItem *tb[];

extern gchar *filename;

extern gboolean change_done;

extern void create_file_dialog (GtkWidget **dialog, gboolean open);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern void set_filename_and_window_title (gchar *new_filename);
extern void show_errmsg (gchar *errmsg_raw_txt);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;

#endif
