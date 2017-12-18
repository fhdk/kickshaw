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

#include <gtk/gtk.h>

#include "general_header_files/define__treeview_column_offset.h"
#include "general_header_files/enum__columns.h"
#include "general_header_files/enum__find_entry_buttons.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__menu_bar_items_view_and_options.h"
#include "find.h"

void show_or_hide_find_grid (void);
void find_buttons_management (gchar *column_check_button_clicked);
static inline void clear_list_of_rows_with_found_occurrences (void);
static gboolean add_occurrence_to_list (G_GNUC_UNUSED GtkTreeModel *local_model, 
													  GtkTreePath  *local_path, 
													  GtkTreeIter  *local_iter);
GList *create_list_of_rows_with_found_occurrences (void);
gboolean check_for_match (GtkTreeIter *local_iter, guint8 column_number);
static gboolean ensure_visibility_of_find (G_GNUC_UNUSED GtkTreeModel *foreach_or_local_model,  
														 GtkTreePath  *foreach_or_local_path, 
														 GtkTreeIter  *foreach_or_local_iter);
void run_search (void);
void jump_to_previous_or_next_occurrence (gpointer direction_pointer);

/* 

	Shows or hides (and resets the settings of the elements inside) the find grid.

*/

void show_or_hide_find_grid (void)
{
	if (gtk_widget_get_visible (find_grid)) {
		guint8 columns_cnt;

		gtk_widget_hide (find_grid);
		g_string_assign (search_term, "");
		clear_list_of_rows_with_found_occurrences ();
		gtk_entry_set_text (GTK_ENTRY (find_entry), "");
		gtk_style_context_remove_class (gtk_widget_get_style_context(find_entry), "bg_class");
		for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
			gtk_style_context_remove_class (gtk_widget_get_style_context(find_in_columns[columns_cnt]), "bg_class");
		}
		gtk_style_context_remove_class (gtk_widget_get_style_context(find_in_all_columns), "bg_class");
		gtk_widget_set_sensitive (find_entry_buttons[BACK], FALSE);
		gtk_widget_set_sensitive (find_entry_buttons[FORWARD], FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_in_all_columns), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_match_case), FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_regular_expression), TRUE);
		gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview.
	}
	else {
		gtk_widget_show (find_grid);
		gtk_widget_grab_focus (find_entry);
	}
}

/* 

	(De)activates all other column check buttons if "All columns" is (un)selected. 
	Search results are updated for any change of the chosen columns and criteria ("match case" and "regular expression").

*/

void find_buttons_management (gchar *column_check_button_clicked)
{
	// TRUE if any find_in_columns check button or find_in_all_columns check button clicked.
	if (column_check_button_clicked) {
		gboolean marking_active;
		gboolean all_columns_check_button_clicked = streq (column_check_button_clicked, "all");

		marking_active = gtk_style_context_has_class (gtk_widget_get_style_context(find_in_all_columns), "bg_class");

		if (marking_active || all_columns_check_button_clicked) {
			gboolean find_in_all_activated = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_all_columns));

			guint8 columns_cnt;

			for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
				if (all_columns_check_button_clicked) {
					g_signal_handler_block (find_in_columns[columns_cnt], handler_id_find_in_columns[columns_cnt]);
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt]), find_in_all_activated);
					g_signal_handler_unblock (find_in_columns[columns_cnt], handler_id_find_in_columns[columns_cnt]);
					gtk_widget_set_sensitive (find_in_columns[columns_cnt], !find_in_all_activated);
				}
				if (marking_active) {
					gtk_style_context_remove_class (gtk_widget_get_style_context(find_in_columns[columns_cnt]), "bg_class");
				}
			}
			if (marking_active) {
				gtk_style_context_remove_class (gtk_widget_get_style_context(find_in_all_columns), "bg_class");
			}
		}
	}

	if (*search_term->str) {
		if (create_list_of_rows_with_found_occurrences ()) {
			gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) ensure_visibility_of_find, NULL);
		}
    	row_selected (); // Reset status of forward and back buttons.
	}

	gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview (for highlighting of search results).
}

/* 

	Clears the list of rows so it can be rebuild later.

*/

static inline void clear_list_of_rows_with_found_occurrences (void) {
	g_list_free_full (rows_with_found_occurrences, (GDestroyNotify) gtk_tree_path_free);
	rows_with_found_occurrences = NULL;
}

/* 

	Adds a row that contains a column matching the search term to a list.

*/

static gboolean add_occurrence_to_list (G_GNUC_UNUSED GtkTreeModel *local_model, 
													  GtkTreePath  *local_path, 
													  GtkTreeIter  *local_iter)
{
	guint8 columns_cnt;

	for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt])) && 
			check_for_match (local_iter, columns_cnt)) {
			// Row references are not used here, since the list is recreated everytime the treestore is changed.
			rows_with_found_occurrences = g_list_prepend (rows_with_found_occurrences, gtk_tree_path_copy (local_path));
			break;
		}
	}

	return FALSE;
}

/* 

	Creates a list of all rows that contain at least one cell with the search term.

	rows_with_found_occurrences is a global GList, the purpose of returning the pointer is that the function to 
	which is returned to can immediately check whether a list has been created or not, in the latter case the 
	return value is NULL.

*/

GList *create_list_of_rows_with_found_occurrences (void)
{
	clear_list_of_rows_with_found_occurrences ();
	gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) add_occurrence_to_list, NULL);
	return rows_with_found_occurrences = g_list_reverse (rows_with_found_occurrences);
}

/* 

	Checks for each column if it contains the search term.

*/

gboolean check_for_match (GtkTreeIter *local_iter, 
                          guint8       column_number)
{
	gboolean match_found = FALSE; // Default
	gchar *current_column;
  
	gtk_tree_model_get (model, local_iter, column_number + TREEVIEW_COLUMN_OFFSET, &current_column, -1);
	if (current_column) {
		gchar *search_term_str_escaped = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_regular_expression))) ? 
										  NULL : g_regex_escape_string (search_term->str, -1);

		if (g_regex_match_simple ((search_term_str_escaped) ? search_term_str_escaped : search_term->str,
								  current_column, 
								  (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_match_case))) ? 
								  0 : G_REGEX_CASELESS, 
								  G_REGEX_MATCH_NOTEMPTY)) {
			match_found = TRUE;
		}

		// Cleanup
		g_free (current_column);
		g_free (search_term_str_escaped);
	}

	return match_found;
}

/* 

	If the search term is contained inside...
	...a row whose parent is not expanded expand the latter.
	...a column which is hidden show this column.

*/

static gboolean ensure_visibility_of_find (G_GNUC_UNUSED GtkTreeModel *foreach_or_local_model,  
														 GtkTreePath  *foreach_or_local_path, 
														 GtkTreeIter  *foreach_or_local_iter)
{
	guint8 columns_cnt;

	for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt])) && 
			check_for_match (foreach_or_local_iter, columns_cnt)) {
			if (gtk_tree_path_get_depth (foreach_or_local_path) > 1 && 
				!gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), foreach_or_local_path)) {
				gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), foreach_or_local_path);
				gtk_tree_view_collapse_row (GTK_TREE_VIEW (treeview), foreach_or_local_path);
			}
			if (!gtk_tree_view_column_get_visible (columns[columns_cnt])) {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[(columns_cnt == COL_MENU_ID) ? 
																						  SHOW_MENU_ID_COL : 
																						  SHOW_EXECUTE_COL]), 
												TRUE);
			}
		}
	}

	return FALSE;
}

/* 

	Runs a search on the entered search term.

*/

void run_search (void)
{
	gboolean no_find_in_columns_buttons_clicked = TRUE; // Default
	

	guint8 columns_cnt;

	g_string_assign (search_term, gtk_entry_get_text (GTK_ENTRY (find_entry)));

	if (*search_term->str) {
		gtk_style_context_remove_class (gtk_widget_get_style_context (find_entry), "bg_class");
	}
	else {
		wrong_or_missing (find_entry, find_entry_css_provider);
	}

	for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt]))) {
			no_find_in_columns_buttons_clicked = FALSE;
			break;
		}
	}

	if (!(*search_term->str) || no_find_in_columns_buttons_clicked) {
		if (no_find_in_columns_buttons_clicked) {
			for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
				wrong_or_missing (find_in_columns[columns_cnt], find_in_columns_css_providers[columns_cnt]);
			}
			wrong_or_missing (find_in_all_columns, find_in_all_columns_css_provider);
		}
		clear_list_of_rows_with_found_occurrences ();
	}
	else if (create_list_of_rows_with_found_occurrences ()) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

		gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) ensure_visibility_of_find, NULL);

		gtk_tree_selection_unselect_all (selection);
 		gtk_tree_selection_select_path (selection, rows_with_found_occurrences->data);
		/*
			There is no horizontically movement to a specific GtkTreeViewColumn; this is indicated by NULL.  
			Alignment arguments (row_align and col_align) aren't used; this is indicated by FALSE.
		*/
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), rows_with_found_occurrences->data, NULL, FALSE, 0, 0);
	}

	row_selected (); // Reset status of forward and back buttons.

	gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview (for highlighting of search results).
}

/* 

	Enables the possibility to move between the found occurrences.

*/

void jump_to_previous_or_next_occurrence (gpointer direction_pointer)
{
	gboolean forward = GPOINTER_TO_INT (direction_pointer);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	GtkTreePath *path = gtk_tree_model_get_path (model, &iter), *path_of_occurrence;
	GtkTreeIter iter_of_occurrence;

	GList *rows_with_found_occurrences_loop;

	for (rows_with_found_occurrences_loop = (forward) ? 
		 rows_with_found_occurrences : g_list_last (rows_with_found_occurrences); 
		 rows_with_found_occurrences_loop; 
		 rows_with_found_occurrences_loop = (forward) ? rows_with_found_occurrences_loop->next : 
		 rows_with_found_occurrences_loop->prev) {
		if ((forward) ? (gtk_tree_path_compare (path, rows_with_found_occurrences_loop->data) < 0) : 
			(gtk_tree_path_compare (path, rows_with_found_occurrences_loop->data) > 0)) {
			break;
		}
	}

	path_of_occurrence = rows_with_found_occurrences_loop->data;
	gtk_tree_model_get_iter (model, &iter_of_occurrence, path_of_occurrence);

	// Ensure_visibility_of_find is not called by gtk_tree_model_foreach; model is unused so this argument is set to NULL.
	ensure_visibility_of_find (NULL, path_of_occurrence, &iter_of_occurrence);

	gtk_tree_selection_unselect_all (selection);
	gtk_tree_selection_select_path (selection, path_of_occurrence);
	/*
		There is no horizontically movement to a specific GtkTreeViewColumn; this is indicated by NULL.  
		Alignment arguments (row_align and col_align) aren't used; this is indicated by FALSE.
	*/
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path_of_occurrence, NULL, FALSE, 0, 0);

	// Cleanup
	gtk_tree_path_free (path);
}
