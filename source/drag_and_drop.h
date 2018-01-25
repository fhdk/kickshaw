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

#ifndef __drag_and_drop_h
#define __drag_and_drop_h

extern ks_data ks;

extern void activate_change_done (void);
extern guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
extern void remove_rows (gchar *origin);
extern void row_selected (void);
extern gboolean selection_block_unblock (G_GNUC_UNUSED GtkTreeSelection *selection,
                                         G_GNUC_UNUSED GtkTreeModel *model,
                                         G_GNUC_UNUSED GtkTreePath  *path,
                                         G_GNUC_UNUSED gboolean  path_currently_selected,
                                         gpointer  block_state);
extern void show_msg_in_statusbar (gchar *message);
extern void sort_execute_or_startupnotify_options_after_insertion (GtkTreeSelection *selection, GtkTreeIter *parent,
   gchar *execute_or_startupnotify, gchar *option);
extern gboolean streq_any (const gchar *string, ...) G_GNUC_NULL_TERMINATED;

#endif
