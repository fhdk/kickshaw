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

#ifndef __auxiliary_h
#define __auxiliary_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern GtkWidget *window;
extern GtkTreeModel *model;
extern GtkWidget *treeview;

extern GdkPixbuf *invalid_icon_imgs[];

extern GSList *rows_with_icons;

extern guint font_size;

extern gboolean check_for_external_file_and_settings_changes (G_GNUC_UNUSED gpointer identifier);

#endif
