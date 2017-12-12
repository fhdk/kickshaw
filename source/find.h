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

#ifndef __find_h
#define __find_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern GtkTreeModel *model;
extern GtkTreeView *treeview;
extern GtkTreeIter iter;

extern GtkTreeViewColumn *columns[];

extern GtkWidget *mb_view_and_options[];

extern GtkWidget *find_grid;
extern GtkWidget *find_entry_buttons[];
extern GtkWidget *find_entry;
extern GtkCssProvider *find_entry_css_provider;
extern GtkWidget *find_in_columns[], *find_in_all_columns;
extern GtkCssProvider *find_in_columns_css_providers[], *find_in_all_columns_css_provider;
extern GtkWidget *find_match_case, *find_regular_expression; 

extern GString *search_term;

extern GList *rows_with_found_occurrences;

extern gint handler_id_find_in_columns[];

extern void row_selected (void);
extern void wrong_or_missing (GtkWidget *widget, GtkCssProvider *css_provider);

#endif
