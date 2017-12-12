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
#include <string.h>

#include "general_header_files/enum__columns.h"
#include "general_header_files/enum__entry_fields.h"
#include "general_header_files/enum__execute_options.h"
#include "general_header_files/enum__invalid_icon_img_statuses.h"
#include "general_header_files/enum__move_row.h"
#include "general_header_files/enum__startupnotify_options.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "editing.h"

enum { FILTER_SELECTED_PATH, VISUALISE_RECURSIVELY, NUMBER_OF_FILTER_VISUALISATION_ELEMENTS };

void sort_execute_or_startupnotify_options_after_insertion (GtkTreeSelection *selection, GtkTreeIter *parent, 
															gchar *execute_or_startupnotify, gchar *option);
static void sort_execute_or_startupnotify_options (GtkTreeIter *parent, gchar *execute_or_startupnotify);
gboolean sort_loop_after_sorting_activation (GtkTreeModel *foreach_model, GtkTreePath *foreach_path,
											 GtkTreeIter *foreach_iter);
void key_pressed (G_GNUC_UNUSED GtkWidget *widget, GdkEventKey *event);
void move_selection (gpointer direction_pointer);
static gboolean check_and_adjust_dependent_element_visibilities (GtkTreeModel *filter_model, GtkTreePath *filter_path, 
																 GtkTreeIter *filter_iter, 
																 gpointer *filter_visualisation);
void visualise_menus_items_and_separators (gpointer recursively_pointer);
static gboolean image_type_filter (const GtkFileFilterInfo *filter_info);
gchar *choose_icon (void);
void icon_choosing_by_button_or_context_menu (void);
gboolean set_icon (GtkTreeIter *icon_iter, gchar *icon_path, gboolean automated);
void remove_icons_from_menus_or_items (void);
void change_row (void);
void cell_edited (G_GNUC_UNUSED GtkCellRendererText *renderer, 
								gchar				*path, 
								gchar				*new_text, 
								gpointer			 column_number_pointer);
static void empty_label_msg (void);
void boolean_toogled (void);

/* 

	First initiates the sorting of Execute or startupnotify options, then selects the inserted option.

*/

void sort_execute_or_startupnotify_options_after_insertion (GtkTreeSelection *selection, 
															GtkTreeIter      *parent, 
															gchar			 *execute_or_startupnotify, 
															gchar			 *option)
{
	GtkTreeIter iter_loop;
	gint ch_cnt;

	gchar *menu_element_txt_loop;

	sort_execute_or_startupnotify_options (parent, execute_or_startupnotify);
	for (ch_cnt = 0; ch_cnt < gtk_tree_model_iter_n_children (model, parent); ch_cnt++) {
		gtk_tree_model_iter_nth_child (model, &iter_loop, parent, ch_cnt);
		gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
		if (streq (menu_element_txt_loop, option)) {
			// Cleanup
			g_free (menu_element_txt_loop);
			break;
		}
		// Cleanup
		g_free (menu_element_txt_loop);
	}
	gtk_tree_selection_select_iter (selection, &iter_loop);
}

/* 

	Sorts Execute or startupnotify options according to the order
	Execute: 1. prompt, 2. command, 3. startupnotify.
	startupnotify: 1. enabled, 2. name, 3. wmclass, 4. icon.

*/

static void sort_execute_or_startupnotify_options (GtkTreeIter *parent, 
												   gchar	   *execute_or_startupnotify)
{
	gboolean execute = (streq (execute_or_startupnotify, "Execute"));
	const guint number_of_options = (execute) ? NUMBER_OF_EXECUTE_OPTS : NUMBER_OF_STARTUPNOTIFY_OPTS;

	GtkTreeIter iter_loop1, iter_loop2;
	gint ch_cnt, foll_ch_cnt; // children counter, following children counter
	guint8 opt_cnt;

	gchar *menu_element_txt_loop1, *menu_element_txt_loop2;

	for (ch_cnt = 0; ch_cnt < gtk_tree_model_iter_n_children (model, parent) - 1; ch_cnt++) {
		gtk_tree_model_iter_nth_child (model, &iter_loop1, parent, ch_cnt);
		gtk_tree_model_get (model, &iter_loop1, TS_MENU_ELEMENT, &menu_element_txt_loop1, -1);
		for (opt_cnt = 0; opt_cnt < number_of_options; opt_cnt++) {
			if (streq (menu_element_txt_loop1, 
			    (execute) ? execute_options[opt_cnt] : startupnotify_options[opt_cnt])) {
				break;
			}
			else {
				for (foll_ch_cnt = ch_cnt + 1;
					 foll_ch_cnt < gtk_tree_model_iter_n_children (model, parent); 
					 foll_ch_cnt++) {
					gtk_tree_model_iter_nth_child (model, &iter_loop2, parent, foll_ch_cnt);
					gtk_tree_model_get (model, &iter_loop2, TS_MENU_ELEMENT, &menu_element_txt_loop2, -1);
					if (streq ((execute) ? execute_options[opt_cnt] : startupnotify_options[opt_cnt], 
						menu_element_txt_loop2)) {
						gtk_tree_store_swap (treestore, &iter_loop1, &iter_loop2);
						// Cleanup
						g_free (menu_element_txt_loop2);
						goto next_child; // Break out of nested loop.
					}
					// Cleanup
					g_free (menu_element_txt_loop2);
				}
			}
		}
		next_child:
		// Cleanup
		g_free (menu_element_txt_loop1);
	}
}

/* 

	This function is run after autosorting of options has been activated.
	All Execute and startupnotify options of the treestore are sorted by it.

*/

gboolean sort_loop_after_sorting_activation (GtkTreeModel *foreach_model, 
											 GtkTreePath  *foreach_path,
											 GtkTreeIter  *foreach_iter)
{
	if (gtk_tree_path_get_depth (foreach_path) == 1 || gtk_tree_model_iter_n_children (foreach_model, foreach_iter) < 2) {
		return FALSE;
	}

	gchar *menu_element_txt_loop;
	gchar *type_txt_loop;

	gtk_tree_model_get (foreach_model, foreach_iter, 
						TS_MENU_ELEMENT, &menu_element_txt_loop, 
						TS_TYPE, &type_txt_loop,
						-1);

	if ((streq (type_txt_loop, "action") && streq (menu_element_txt_loop, "Execute")) || 
		 streq (type_txt_loop, "option block")) {
		sort_execute_or_startupnotify_options (foreach_iter, menu_element_txt_loop);
	}

	// Cleanup
	g_free (menu_element_txt_loop);
	g_free (type_txt_loop);

	return FALSE;
}

/* 

	Function that deals with key press events, currently only used for "Delete" key.

	Because the callback function for the "key-press-event" event receives three arguments 
	(widget, event and user data), the function can't be called via g_signal_connect_swapped, 
	so the widget argument has to remain (it is marked as unused). The "user data" argument is omitted.

*/

void key_pressed (G_GNUC_UNUSED GtkWidget *widget, GdkEventKey *event)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_count_selected_rows (selection) > 0 && event->keyval == GDK_KEY_Delete) {
		remove_rows ("delete key");
	}
}

/* 

	Moves selection up or down, to the top or bottom.

*/

void move_selection (gpointer direction_pointer)
{
	guint8 direction = GPOINTER_TO_UINT (direction_pointer);
	GtkTreeIter iter_new_pos = iter;

	switch (direction) {
		case UP:
		case DOWN:
			if (direction == UP) {
				gtk_tree_model_iter_previous (model, &iter_new_pos);
			}
			else {
				gtk_tree_model_iter_next (model, &iter_new_pos);
			}
			gtk_tree_store_swap (GTK_TREE_STORE (model), &iter, &iter_new_pos);
			break;
		case TOP:
			gtk_tree_store_move_after (treestore, &iter, NULL);
			break;
		case BOTTOM:
			gtk_tree_store_move_before (treestore, &iter, NULL);
	}

	activate_change_done ();
	row_selected ();
}

/* 

	Sets element visibility of menus, pipe menus, items and separators 
	that are dependent on the menu element that has been visualised.

*/

static gboolean check_and_adjust_dependent_element_visibilities (GtkTreeModel *filter_model, 
																 GtkTreePath  *filter_path, 
																 GtkTreeIter  *filter_iter, 
																 gpointer	  *filter_visualisation)
{
	gchar *element_visibility_txt;

	gtk_tree_model_get (filter_model, filter_iter, TS_ELEMENT_VISIBILITY, &element_visibility_txt, -1);

	if (!element_visibility_txt) {
		return FALSE;
	}

	// Cleanup
	g_free (element_visibility_txt);

	GtkTreeIter model_iter;

	gchar *menu_element_txt_filter;
	gchar *type_txt_filter;

	/*
		Makes the following conditional statement more readable.
		If the selected path is on toplevel, filter_visualisation[FILTER_SELECTED_PATH] == NULL.
	*/
	GtkTreePath *filter_selected_path = (GtkTreePath *) filter_visualisation[FILTER_SELECTED_PATH];
	gboolean recursively_and_row_is_dsct = (GPOINTER_TO_UINT (filter_visualisation[VISUALISE_RECURSIVELY]) && 
											(!filter_selected_path || // Selected path is on toplevel = dsct. of NULL.
											gtk_tree_path_is_descendant (filter_path, filter_selected_path)));

	gtk_tree_model_get (filter_model, filter_iter, 
						TS_MENU_ELEMENT, &menu_element_txt_filter, 
						TS_TYPE, &type_txt_filter, 
						-1);
	gtk_tree_model_filter_convert_iter_to_child_iter ((GtkTreeModelFilter *) filter_model, &model_iter, filter_iter);

	/*
		Current row is an ancestor of the selected row or the selected row itself.

		A menu element can only be visible if its ancestors are visible as well, 
		so if a visualisation is done for a (pipe) menu, item or separator, the visibility statuses for all ancestors are 
		set to "visible". 
		To keep the code simple there is no check if the visibility status is already set to "visible", it is done anyway. 
 		Visualised menus and items get a "dummy" label, whilst separators don't, because they are always visible if they
		are not descendants of an invisible menu, whether they have a label or not.
	*/
	if (filter_selected_path && // Selected path is not on toplevel.
		(gtk_tree_path_is_ancestor (filter_path, filter_selected_path) || 
		gtk_tree_path_compare (filter_path, filter_selected_path) == 0)) {
		gtk_tree_store_set (treestore, &model_iter, TS_ELEMENT_VISIBILITY, "visible", -1);
		if (!menu_element_txt_filter && !streq (type_txt_filter, "separator")) {
			gtk_tree_store_set (treestore, &model_iter, TS_MENU_ELEMENT, "(Newly created label)",  -1);
		}
	}
	// Current row is a descendant of the selected row or not an ascendant of the selected row/the selected row itself.
	else {
		guint8 invisible_ancestor = check_if_invisible_ancestor_exists (filter_model, filter_path);
		gboolean unlabeled_menu_or_item = !menu_element_txt_filter && !streq (type_txt_filter, "separator");

		gchar *new_element_visibility_txt = 
			(invisible_ancestor || (!recursively_and_row_is_dsct && unlabeled_menu_or_item)) ? 
 			((invisible_ancestor) ? "invisible dsct. of invisible menu" : 
 			((streq (type_txt_filter, "item") ? "invisible item" : "invisible menu"))) : "visible";

		if (recursively_and_row_is_dsct && unlabeled_menu_or_item) {
			gtk_tree_store_set (treestore, &model_iter, TS_MENU_ELEMENT, "(Newly created label)", -1);
		}

		gtk_tree_store_set (treestore, &model_iter, TS_ELEMENT_VISIBILITY, new_element_visibility_txt, -1);
	}

	// Cleanup
	g_free (menu_element_txt_filter);
	g_free (type_txt_filter);

	return FALSE;
}

/* 

	Changes the status of one or more menus, pipe menus, items and separators to visible.

*/

void visualise_menus_items_and_separators (gpointer recursively_pointer)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

	GtkTreeModel *filter_model;
	GtkTreePath *path_toplevel;
	GtkTreeIter iter_toplevel;
	gpointer filter_visualisation[NUMBER_OF_FILTER_VISUALISATION_ELEMENTS];

	GList *selected_rows_loop;

	gchar *menu_element_txt_loop;

	for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
		get_toplevel_iter_from_path (&iter_toplevel, selected_rows_loop->data);
		gtk_tree_store_set (treestore, &iter_toplevel, TS_ELEMENT_VISIBILITY, "visible", -1);
		gtk_tree_model_get (model, &iter_toplevel, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
		if (!menu_element_txt_loop) {
			gtk_tree_store_set (treestore, &iter_toplevel, TS_MENU_ELEMENT, "(Newly created label)",  -1);
		}

		path_toplevel = gtk_tree_model_get_path (model, &iter_toplevel);
		filter_model = gtk_tree_model_filter_new (model, path_toplevel);

		/*
			-menu
			-menu
				-menu
					-item
					-menu --- invisible ---   Path 1:0:1   selected_rows_loop->data
				-item
				-menu
			-menu

			is converted to

			-menu
				-item
				-menu --- invisible ---   Path 1:0   filter_visualisation[FILTER_SELECTED_PATH]
			-item

			-----------------------------------------------------------------------------------

			-menu
			-menu --- invisible ---   Path 1
				-item
			-menu

			is converted to NULL.
		*/
		filter_visualisation[FILTER_SELECTED_PATH] =  
			(gpointer) gtk_tree_model_filter_convert_child_path_to_path ((GtkTreeModelFilter *) filter_model, 
																		 selected_rows_loop->data);

		filter_visualisation[VISUALISE_RECURSIVELY] = recursively_pointer;

		gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_and_adjust_dependent_element_visibilities, 
								filter_visualisation);

		// Cleanup
		g_object_unref (filter_model);
		g_free (menu_element_txt_loop);
		gtk_tree_path_free (path_toplevel);
		gtk_tree_path_free ((GtkTreePath *) filter_visualisation[FILTER_SELECTED_PATH]);
	}

	// Cleanup
	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

	activate_change_done ();
	/*
		If just the txt_fields array would be repopulated, 
		the menu bar sensivity for visualisation wouldn't be updated.
	*/
	row_selected ();
}

/* 

   File filter is limited to display only image files.

*/

static gboolean image_type_filter (const GtkFileFilterInfo *filter_info)
{
	return (g_regex_match_simple ("image/.*", filter_info->mime_type, 0, 0));
}

/* 

	Dialog for choosing an icon for a (pipe) menu or item.

	The returned filename is a newly allocated string that should be freed with g_free () after use.

*/

gchar *choose_icon (void)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open Image File",
													  GTK_WINDOW (window), 
													  GTK_FILE_CHOOSER_ACTION_OPEN,
													  "_Cancel", GTK_RESPONSE_CANCEL, 
													  "_Open", GTK_RESPONSE_ACCEPT, 
													  NULL);

	GtkFileFilter *open_image_file_filter = gtk_file_filter_new ();
	gchar *icon_filename = NULL; // Default

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), "/");

	gtk_file_filter_set_name (open_image_file_filter, "Image files");
	// The last two arguments (data and function to call to free data) aren't used.
	gtk_file_filter_add_custom (open_image_file_filter, GTK_FILE_FILTER_MIME_TYPE, (GtkFileFilterFunc) image_type_filter, 
								NULL, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), open_image_file_filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		icon_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	}

	gtk_widget_destroy (dialog);

	return icon_filename;
}

/* 

	Lets the user choose an icon after (s)he clicked on the icon chooser button or
	(s)he chose the corresponding action from the context menu.

*/

void icon_choosing_by_button_or_context_menu (void)
{
	gchar *icon_path;

	if (!(icon_path = choose_icon ())) {
		return;
	}
	set_icon (&iter, icon_path, FALSE); // FALSE = not automated, returned boolean value is ignored here.

	// Cleanup
	g_free (icon_path);

	activate_change_done ();
}

/* 

	Adds an icon, if possible, at a position passed to this function.

*/

gboolean set_icon (GtkTreeIter *icon_iter, 
		   gchar       *icon_path, 
		   gboolean     automated)
{
	GdkPixbuf *icon_in_original_size;
	GError *error = NULL;

	if (G_UNLIKELY (!(icon_in_original_size = gdk_pixbuf_new_from_file (icon_path, &error)))) {
		if (!automated) {
			gchar *error_message = g_strdup_printf ("The following error occurred while trying to "
													"add an icon to the %s '%s':\n\n"
													"<span foreground='#8a1515'>%s</span>", 
													txt_fields[TYPE_TXT], txt_fields[MENU_ELEMENT_TXT], 
													error->message);
			show_errmsg (error_message);

			// Cleanup
			g_free (error_message);
		}
		// Cleanup
		g_error_free (error);
		return FALSE;
	}

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	GdkPixbuf *icon;
	gchar *icon_modification_time;

	icon = gdk_pixbuf_scale_simple (icon_in_original_size, font_size + 10, font_size + 10, GDK_INTERP_BILINEAR);

	// Cleanup
	g_object_unref (icon_in_original_size);

	icon_modification_time = get_modification_time_for_icon (icon_path);

	gtk_tree_store_set (GTK_TREE_STORE (model), icon_iter, 
						TS_ICON_IMG, icon, 
						TS_ICON_IMG_STATUS, NONE_OR_NORMAL, 
						TS_ICON_PATH, icon_path, 
						TS_ICON_MODIFICATION_TIME, icon_modification_time, 
						-1);

	// Cleanup
	g_object_unref (icon);
	g_free (icon_modification_time);

	// This function might have been called from the timeout function, with currently no selection done at that time.
	if (gtk_tree_selection_count_selected_rows (selection)) {
		repopulate_txt_fields_array (); // There is no need to change the status of any menu- or toolbar widget.
		set_entry_fields ();
	} 

	return TRUE;
}

/* 

   Removes icons from menus or items.

*/

void remove_icons_from_menus_or_items (void)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
  
	GList *selected_rows_loop;
	GtkTreeIter iter_loop;

	for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
		gtk_tree_model_get_iter (model, &iter_loop, selected_rows_loop->data);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter_loop,
							TS_ICON_IMG, NULL,
							TS_ICON_IMG_STATUS, NONE_OR_NORMAL,
							TS_ICON_MODIFICATION_TIME, NULL, 
							TS_ICON_PATH, NULL, 
							-1);
	}

	if (gtk_tree_selection_count_selected_rows (selection) == 1) {
		repopulate_txt_fields_array (); // There is no need to change the status of any menu- or toolbar widget.
		set_entry_fields ();
	}

	// Cleanup
	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

	activate_change_done ();
}

/* 

	Changes one or more values of a row after at least one of the entry fields has been altered.

*/

void change_row (void)
{
	const gchar *entry_txt[NUMBER_OF_ENTRY_FIELDS];

	for (guint8 entry_field_cnt = 0; entry_field_cnt < NUMBER_OF_ENTRY_FIELDS; entry_field_cnt++) {
		entry_txt[entry_field_cnt] = gtk_entry_get_text (GTK_ENTRY (entry_fields[entry_field_cnt]));
	}

	if (txt_fields[ELEMENT_VISIBILITY_TXT]) { // menu, pipe menu, item or separator
		if (!streq (txt_fields[TYPE_TXT], "separator")) {
			if (!(*entry_txt[MENU_ELEMENT_OR_VALUE_ENTRY])) {
				empty_label_msg ();
				set_entry_fields ();
				return;
			}
			if (streq_any (txt_fields[TYPE_TXT], "menu", "pipe menu", NULL)) {
				if (!streq (txt_fields[MENU_ID_TXT], entry_txt[MENU_ID_ENTRY])) {
					if (G_UNLIKELY (g_slist_find_custom (menu_ids, entry_txt[MENU_ID_ENTRY], (GCompareFunc) strcmp))) {
						show_errmsg ("This menu ID already exists. Please choose another one.");
						return;
					}
					remove_menu_id (txt_fields[MENU_ID_TXT]);
					menu_ids = g_slist_prepend (menu_ids, g_strdup (entry_txt[MENU_ID_ENTRY]));
					gtk_tree_store_set (treestore, &iter, TS_MENU_ID, entry_txt[MENU_ID_ENTRY], -1);
				}

				if (streq (txt_fields[TYPE_TXT], "pipe menu")) {
					gtk_tree_store_set (treestore, &iter, TS_EXECUTE, entry_txt[EXECUTE_ENTRY], -1);
				}
			}
			if (txt_fields[ICON_PATH_TXT] && !(*entry_txt[ICON_PATH_ENTRY])) {
				remove_icons_from_menus_or_items ();
			}
			else if (*entry_txt[ICON_PATH_ENTRY] && !streq (entry_txt[ICON_PATH_ENTRY], txt_fields[ICON_PATH_TXT]) && 
					 set_icon (&iter, (gchar *) entry_txt[ICON_PATH_ENTRY], FALSE)) { // FALSE = not automated.
				gtk_style_context_remove_class (gtk_widget_get_style_context (entry_fields[ICON_PATH_ENTRY]), "bg_class");
				gtk_widget_set_sensitive (remove_icon, TRUE);
			}
		}
		gtk_tree_store_set (treestore, &iter, TS_MENU_ELEMENT, 
							(*entry_txt[MENU_ELEMENT_OR_VALUE_ENTRY]) ? entry_txt[MENU_ELEMENT_OR_VALUE_ENTRY] : NULL, -1);
	}
	// Option. Enabled is never shown, since it is edited directly inside the treeview.
	else {
		gtk_tree_store_set (treestore, &iter, TS_VALUE, entry_txt[MENU_ELEMENT_OR_VALUE_ENTRY], -1);
	}

	/*
		There is no need to change the status of any menu- or toolbar widget.
		A repopulation of the text fields array is also necessary for the case 
		the same entry field is modified at least twice in a row.
	*/
	repopulate_txt_fields_array ();

	activate_change_done ();
}

/* 

	Adopts changes from editable cells, in case it's a menu ID prevents a duplicate.

*/

void cell_edited (G_GNUC_UNUSED GtkCellRendererText *renderer, 
								gchar				*path, 
								gchar				*new_text, 
								gpointer			 column_number_pointer)
{
	guint column_number = GPOINTER_TO_UINT (column_number_pointer);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);

	gboolean erased_separator_label = FALSE; // Default

	guint8 treestore_pos, txt_fields_pos;

	if (G_UNLIKELY (!gtk_tree_model_get_iter_from_string (model, &iter, path))) {
		return;
	}

	treestore_pos = column_number + 4; // Columns start with TS_MENU_ELEMENT, which has index 4 inside the treestore.
	txt_fields_pos = column_number + 1; // ICON_PATH_TXT is ommitted.

	if (G_UNLIKELY (number_of_selected_rows > 1)) {
		gtk_tree_model_get (model, &iter, treestore_pos, &txt_fields[txt_fields_pos], -1);
	}

	if (G_UNLIKELY (streq (txt_fields[txt_fields_pos], new_text))) { // New == Old.
		return;
	}

	if (column_number == COL_MENU_ELEMENT && !(*new_text)) {
		if (G_UNLIKELY (!streq (txt_fields[TYPE_TXT], "separator"))) {
			empty_label_msg ();
			return;
		}
		erased_separator_label = TRUE;
	}

	if (column_number == COL_MENU_ID) {
		if (G_UNLIKELY (g_slist_find_custom (menu_ids, new_text, (GCompareFunc) strcmp))) {
			show_errmsg ("This menu ID already exists. Please choose another one.");
			return;
		}
		remove_menu_id (txt_fields[MENU_ID_TXT]);
		menu_ids = g_slist_prepend (menu_ids, g_strdup (new_text));
	}

	gtk_tree_store_set (treestore, &iter, treestore_pos, (!erased_separator_label) ? new_text : NULL, -1);
	repopulate_txt_fields_array (); // There is no need to change the status of any menu- or toolbar widget.
	if (number_of_selected_rows == 1) {
		set_entry_fields ();
	}

	activate_change_done ();
}

/* 

	Empty labels for menus, pipe menus or items are blocked, because they would make these elements invisible.

*/

static void empty_label_msg (void)
{
	GtkWidget *dialog;
	gchar *label_txt = g_strdup_printf ("<b>The creation of an empty label for this %s is blocked, "
										"since it would make the %s invisible.</b>\n\n"
										"Existing menus with invisible menu elements may be opened; "
										"these menu elements can be visualised or deleted at once, "
										"otherwise they are highlighted.", 
										txt_fields[TYPE_TXT], txt_fields[TYPE_TXT]);

	create_dialog (&dialog, "Entry of empty label blocked", "dialog-error", label_txt, "Close", NULL, NULL, TRUE);

	// Cleanup
	g_free (label_txt);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/* 

	Toggles value of toggle buttons in the treeview.

*/

void boolean_toogled (void)
{
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, TS_VALUE, 
						(streq (txt_fields[VALUE_TXT], "yes")) ? "no" : "yes", -1);
}
