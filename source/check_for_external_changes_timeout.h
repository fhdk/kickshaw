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

#ifndef __check_for_external_changes_timeout_h
#define __check_for_external_changes_timeout_h

extern ks_data ks;

extern void create_invalid_icon_imgs (void);
extern guint get_font_size (void);
extern gboolean set_icon (GtkTreeIter *icon_iter, gchar *icon_path, gboolean display_err_msg);
extern gchar *get_modification_time_for_icon (gchar *icon_path);
extern void wrong_or_missing (GtkWidget *widget, GtkCssProvider *css_provider);

#endif
