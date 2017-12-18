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

#include "general_header_files/enum__add_buttons.h"
#include "general_header_files/enum__entry_fields.h"
#include "general_header_files/enum__execute_options.h"
#include "general_header_files/enum__find_entry_buttons.h"
#include "general_header_files/enum__menu_bar_items_file_and_edit.h"
#include "general_header_files/enum__startupnotify_options.h"
#include "general_header_files/enum__toolbar_buttons.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "selecting.h"

static void all_options_have_been_set_msg (gchar *action_option);
static gboolean check_for_selected_dsct (			   GtkTreeModel	*filter_model,
													   GtkTreePath	*filter_path, 
										 G_GNUC_UNUSED GtkTreeIter	*filter_iter, 
													   gboolean		*selected_row_has_selected_dsct);
void row_selected (void);
static gboolean avoid_overlapping (void);
void set_entry_fields (void);

/* 

	Displays a message if all options for execute or startupnotify have been set.

*/

static void all_options_have_been_set_msg (gchar *action_option)
{
	gchar *msg_label_txt = g_strdup_printf (" All options for %s have been set", action_option);

	gtk_widget_hide (add_image);
	gtk_widget_hide (bt_add[ACTION_OR_OPTION]);

	gtk_label_set_text (GTK_LABEL (bt_bar_label), msg_label_txt);

	// Cleanup
	g_free (msg_label_txt);
}

/* 

	Checks if a selected node has a descendant that has also been selected.

*/

static gboolean check_for_selected_dsct (			   GtkTreeModel	*filter_model,
													   GtkTreePath	*filter_path, 
										 G_GNUC_UNUSED GtkTreeIter	*filter_iter, 
													   gboolean		*selected_row_has_selected_dsct)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	// The path of the model, not filter model, is needed to check whether the row is selected.
	GtkTreePath *model_path = gtk_tree_model_filter_convert_path_to_child_path ((GtkTreeModelFilter *) filter_model, 
																				filter_path);

	if (gtk_tree_selection_path_is_selected (selection, model_path)) {
		*selected_row_has_selected_dsct = TRUE;
	}

	// Cleanup
	gtk_tree_path_free (model_path);

	return *selected_row_has_selected_dsct; // Stop iteration if a selected descendant has been found.
}

/* 

	If one or more rows have been selected, all (in)appropriate actions for them are (de)activated according to 
	criteria as e.g. their type (menu, item, etc.), expansion and visibility status, position and the number of 
	selected rows.
	If there is currently no selected row left after an unselection, 
	all actions that would be eligible in case of a selection are deactivated.

*/

void row_selected (void)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	const gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);
	GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

	GtkTreeModel *filter_model;

	gboolean dragging_enabled = TRUE; // Default

	// Defaults
	gboolean at_least_one_selected_row_has_no_children = FALSE;
	gboolean at_least_one_descendant_is_invisible = FALSE;

	GList *selected_rows_loop;
	GtkTreeIter iter_loop;
	GtkTreePath *path_loop;

	gchar *menu_element_txt_loop, *type_txt_loop, *element_visibility_txt_loop;

	guint8 mb_menu_items_cnt, tb_cnt, buttons_cnt;

	gboolean treestore_is_empty = !gtk_tree_model_get_iter_first (model, &iter_loop); // using iter_loop spares one v.

	// Resets
	if (statusbar_msg_shown) {
 		gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), 1); // Only one context (indicated by 1) with one message.
 		statusbar_msg_shown = FALSE;
	}
	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (treeview), GDK_BUTTON1_MASK, enable_list, 1, GDK_ACTION_MOVE);

	// Check if dragging has to be blocked.
	if (number_of_selected_rows > 1) {
		// Defaults
		gboolean selected_row_has_selected_dsct = FALSE;
		gboolean menu_pipemenu_item_separator_selected = FALSE;
		gboolean at_least_one_prompt_command_startupnotify_selected = FALSE;
		gboolean at_least_one_enabled_name_wmclass_icon_selected = FALSE;
		gboolean action_selected = FALSE;
		gboolean option_selected = FALSE;
		gboolean at_least_one_option_more_than_once_selected = FALSE;

		gchar *all_options[] = { "prompt", "command", "startupnotify", "enabled", "name", "wmclass", "icon" }; 
		const guint NUMBER_OF_ALL_OPTIONS = G_N_ELEMENTS (all_options);
		G_GNUC_EXTENSION guint number_of_options_selected_per_opt[] = { [0 ... NUMBER_OF_ALL_OPTIONS - 1] = 0 };

		guint8 options_cnt;

		for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
			path_loop = selected_rows_loop->data;
			gtk_tree_model_get_iter (model, &iter_loop, path_loop);
			gtk_tree_model_get (model, &iter_loop, 
								TS_MENU_ELEMENT, &menu_element_txt_loop,
								TS_TYPE, &type_txt_loop, 
			  -					1);
			// Check if a selected row has a selected child.
			if (gtk_tree_model_iter_has_child (model, &iter_loop)) {
				filter_model = gtk_tree_model_filter_new (model, path_loop);
				gtk_tree_model_foreach (filter_model,
										(GtkTreeModelForeachFunc) check_for_selected_dsct, 
										&selected_row_has_selected_dsct);

				// Cleanup
				g_object_unref (filter_model);
			}
			if (streq_any (type_txt_loop, "menu", "pipe menu", "item", "separator", NULL)) {
				menu_pipemenu_item_separator_selected = TRUE;
			}
			else if (streq (type_txt_loop, "action")) {
				action_selected = TRUE;
			}
			else { // Option or option block
				option_selected = TRUE;
				for (options_cnt = 0; options_cnt < NUMBER_OF_ALL_OPTIONS; options_cnt++) {
					if (streq (menu_element_txt_loop, all_options[options_cnt])) {
						if (++number_of_options_selected_per_opt[options_cnt] > 1) {
							at_least_one_option_more_than_once_selected = TRUE;
						}
						if (streq_any (menu_element_txt_loop, "prompt", "command", "startupnotify", NULL)) {
							at_least_one_prompt_command_startupnotify_selected = TRUE;
						}
						else {
							at_least_one_enabled_name_wmclass_icon_selected = TRUE;
						}
					}
				}
			}
			// Cleanup
			g_free (menu_element_txt_loop);
			g_free (type_txt_loop);
		}

		if ((menu_pipemenu_item_separator_selected && (action_selected || option_selected)) || 
			(action_selected && option_selected) || 
			(at_least_one_prompt_command_startupnotify_selected && at_least_one_enabled_name_wmclass_icon_selected) || 
			at_least_one_option_more_than_once_selected || 
			selected_row_has_selected_dsct) { // Dragging is blocked
			GString *statusbar_msg = g_string_new ("");

			dragging_enabled = FALSE;
			if (menu_pipemenu_item_separator_selected && (action_selected || option_selected)) {
				g_string_append (statusbar_msg, "Menu/item/separator and action/option selected   ");
			}
			if (action_selected && option_selected) {
				g_string_append (statusbar_msg, "Action and option selected   ");
			}
			if (at_least_one_prompt_command_startupnotify_selected && at_least_one_enabled_name_wmclass_icon_selected) {
				g_string_append (statusbar_msg, "Action option and startupnotify option selected   ");
			}
			if (at_least_one_option_more_than_once_selected) {
				g_string_append (statusbar_msg, "More than one option of a kind selected   ");
			}
			// This message is only shown if the other cases don't apply.
			if (!(*statusbar_msg->str)) { // -> selected_row_has_selected_dsct == TRUE
				g_string_append (statusbar_msg, "Row and descendant of it selected   ");
			}
			g_string_prepend (statusbar_msg, "Dragging disabled --- ");
			show_msg_in_statusbar (statusbar_msg->str);

			// Cleanup
			g_string_free (statusbar_msg, TRUE);
			gtk_tree_view_unset_rows_drag_source (GTK_TREE_VIEW (treeview));
		}
	}

	if (dragging_enabled) {
		/*
			Create a list of row references of the selected paths for possible later drag and drop usage.
			An existing list is cleared first.
		*/
		g_slist_free_full (source_paths, (GDestroyNotify) gtk_tree_row_reference_free);
		source_paths = NULL;
		for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
 			source_paths = g_slist_prepend (source_paths, gtk_tree_row_reference_new (model, selected_rows_loop->data));
		}
		source_paths = g_slist_reverse (source_paths);
	}

	// Default settings
	if (gtk_widget_get_visible (action_option_grid)) {
		hide_action_option_grid ();
	}
	gtk_widget_set_sensitive (mb_edit, TRUE);
	gtk_widget_set_sensitive (mb_search, TRUE);
	gtk_widget_set_sensitive (mb_edit_menu_items[MB_VISUALISE], TRUE);
	for (tb_cnt = 0; tb_cnt < TB_QUIT; tb_cnt++) {
		gtk_widget_set_sensitive ((GtkWidget *) tb[tb_cnt], TRUE);
	}
	gtk_widget_show (add_image);
	gtk_label_set_text (GTK_LABEL (bt_bar_label), "Add new:  ");
	for (buttons_cnt = 0; buttons_cnt < ACTION_OR_OPTION; buttons_cnt++) {
		gtk_widget_show (bt_add[buttons_cnt]);
	}
	gtk_widget_hide (bt_add[ACTION_OR_OPTION]);
	gtk_widget_set_sensitive (find_entry_buttons[BACK], FALSE);
	gtk_widget_set_sensitive (find_entry_buttons[FORWARD], FALSE);
	gtk_widget_hide (entry_grid);

	gtk_widget_set_sensitive (mb_file_menu_items[MB_NEW], !(treestore_is_empty && !filename));
	gtk_widget_set_sensitive ((GtkWidget *) tb[TB_NEW], !(treestore_is_empty && !filename));

	gtk_widget_set_sensitive (mb_file_menu_items[MB_SAVE], filename && change_done);
	gtk_widget_set_sensitive ((GtkWidget *) tb[TB_SAVE], filename && change_done);

	for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
		path_loop = selected_rows_loop->data;
		gtk_tree_model_get_iter (model, &iter_loop, path_loop);
		gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);

		if (!element_visibility_txt_loop || streq (element_visibility_txt_loop, "visible")) {
			gtk_widget_set_sensitive (mb_edit_menu_items[MB_VISUALISE], FALSE);
		}
		if (gtk_tree_model_iter_has_child (model, &iter_loop)) {
			filter_model = gtk_tree_model_filter_new (model, path_loop);
			gtk_tree_model_foreach (filter_model,
									(GtkTreeModelForeachFunc) check_if_invisible_descendant_exists, 
									&at_least_one_descendant_is_invisible);

			// Cleanup
			g_object_unref (filter_model);
		}
		else {
			at_least_one_selected_row_has_no_children = TRUE;
		}

		// Cleanup
		g_free (element_visibility_txt_loop);
	}
	gtk_widget_set_sensitive (mb_edit_menu_items[MB_REMOVE_ALL_CHILDREN], !at_least_one_selected_row_has_no_children);
	gtk_widget_set_sensitive (mb_edit_menu_items[MB_VISUALISE_RECURSIVELY], 
							  (gtk_widget_get_sensitive (mb_edit_menu_items[MB_VISUALISE]) && 
							  at_least_one_descendant_is_invisible));

	gtk_widget_set_sensitive (mb_search, !treestore_is_empty);
	gtk_widget_set_sensitive ((GtkWidget *) tb[TB_FIND], !treestore_is_empty);

	set_status_of_expand_and_collapse_buttons_and_menu_items ();

	// Show/activate or hide/deactivate certain widgets depending on if there is no or more than one selection.
	if (number_of_selected_rows != 1) {
		free_elements_of_static_string_array (txt_fields, NUMBER_OF_TXT_FIELDS, TRUE);
		gtk_widget_set_sensitive ((GtkWidget *) tb[TB_MOVE_UP], FALSE);
		gtk_widget_set_sensitive ((GtkWidget *) tb[TB_MOVE_DOWN], FALSE);
		if (number_of_selected_rows == 0) {
			gtk_widget_set_sensitive (mb_edit, FALSE);
			gtk_widget_set_sensitive ((GtkWidget *) tb[TB_REMOVE], FALSE);
		}
		else { // number of selected rows > 1
			for (mb_menu_items_cnt = MB_MOVE_TOP; mb_menu_items_cnt <= MB_MOVE_BOTTOM; mb_menu_items_cnt++) {
				gtk_widget_set_sensitive (mb_edit_menu_items[mb_menu_items_cnt], FALSE);
			}
			gtk_widget_hide (add_image);
			for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_ADD_BUTTONS; buttons_cnt++) {
				gtk_widget_hide (bt_add[buttons_cnt]);
			}
			gtk_label_set_text (GTK_LABEL (bt_bar_label), 
								" Addition of new menu elements deactivated due to multiple selections.");
		}

		return;
	}


	// --- Everything from here is for the case that just one row is selected. ---


	GtkTreeIter iter_next, iter_previous;

	GtkTreePath *path = selected_rows->data;
	gint path_depth = gtk_tree_path_get_depth (path);
	GtkTreeIter parent;
	gchar *menu_element_txt_parent = NULL, *type_txt_parent = NULL;

	gtk_tree_model_get_iter (model, &iter, path);

	if (path_depth > 1) {
 		gtk_tree_model_iter_parent (model, &parent, &iter);
		gtk_tree_model_get (model, &parent, 
							TS_MENU_ELEMENT, &menu_element_txt_parent, 
							TS_TYPE, &type_txt_parent, 
							-1);
	}

	gboolean not_at_top, not_at_bottom, option_and_autosort;

	iter_previous = iter_next = iter;
	not_at_top = gtk_tree_model_iter_previous (model, &iter_previous);
	not_at_bottom = gtk_tree_model_iter_next (model, &iter_next);

	option_and_autosort = autosort_options && streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL);

	repopulate_txt_fields_array ();

	for (mb_menu_items_cnt = MB_MOVE_TOP; mb_menu_items_cnt <= MB_MOVE_BOTTOM; mb_menu_items_cnt++) {
		gtk_widget_set_sensitive (mb_edit_menu_items[mb_menu_items_cnt], 
								  ((mb_menu_items_cnt < MB_MOVE_DOWN) ? not_at_top : not_at_bottom) && !option_and_autosort);
	}
	gtk_widget_set_sensitive ((GtkWidget *) tb[TB_MOVE_UP], not_at_top && !option_and_autosort);
	gtk_widget_set_sensitive ((GtkWidget *) tb[TB_MOVE_DOWN], not_at_bottom && !option_and_autosort);

	if (rows_with_found_occurrences) {
		gtk_widget_set_sensitive (find_entry_buttons[BACK], 
								  gtk_tree_path_compare (path, rows_with_found_occurrences->data) > 0);
		gtk_widget_set_sensitive (find_entry_buttons[FORWARD], 
		gtk_tree_path_compare (path, g_list_last (rows_with_found_occurrences)->data) < 0);
	}

	// Cleanup
	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

	// Item selected?
	if (streq (txt_fields[TYPE_TXT], "item")) {
		gtk_widget_show (bt_add[ACTION_OR_OPTION]);
		gtk_label_set_text (GTK_LABEL (bt_add_action_option_label), "Action");
		g_signal_handler_disconnect (bt_add[ACTION_OR_OPTION], handler_id_action_option_button_clicked);
		handler_id_action_option_button_clicked = g_signal_connect_swapped (bt_add[ACTION_OR_OPTION], "clicked", 
																			G_CALLBACK (generate_action_option_combo_box), 
																			NULL);
	}

	// Action or option of it selected?
	if (streq_any (txt_fields[TYPE_TXT], "action", "option", "option block", NULL)) {
		gint number_of_children_of_parent = gtk_tree_model_iter_n_children (model, &parent);
		gint number_of_children_of_iter = gtk_tree_model_iter_n_children (model, &iter);

		// Defaults
		gboolean prompt_or_command_selected_and_action_is_Execute = FALSE;
		gboolean startupnotify_or_option_of_it_selected = FALSE;

		gchar *msg_label_txt;

		for (buttons_cnt = 0; buttons_cnt < ACTION_OR_OPTION; buttons_cnt++) {
			gtk_widget_hide (bt_add[buttons_cnt]);
		}
		gtk_widget_show (bt_add[ACTION_OR_OPTION]);

		// Default callback function
		g_signal_handler_disconnect (bt_add[ACTION_OR_OPTION], handler_id_action_option_button_clicked);
		handler_id_action_option_button_clicked = g_signal_connect_swapped (bt_add[ACTION_OR_OPTION], "clicked", 
																			G_CALLBACK (generate_action_option_combo_box), 
																			NULL);

		// Action selected
		if (streq (txt_fields[TYPE_TXT], "action")) {
			gtk_label_set_text (GTK_LABEL (bt_add_action_option_label), 
								(!streq (txt_fields[MENU_ELEMENT_TXT], "Reconfigure") && 
								 ((streq (txt_fields[MENU_ELEMENT_TXT], "Execute") &&
								  number_of_children_of_iter < NUMBER_OF_EXECUTE_OPTS) || number_of_children_of_iter == 0)) ? 
								"Action/Option" : "Action");
		}

		// "prompt" or "command" option selected and action is "Execute"?
		if (streq (txt_fields[TYPE_TXT], "option") && 
			streq_any (txt_fields[MENU_ELEMENT_TXT], "prompt", "command", NULL) && 
			streq (menu_element_txt_parent, "Execute")) {
			prompt_or_command_selected_and_action_is_Execute = TRUE;
		}

		// Startupnotify or option of it selected?
		if (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) && 
			!streq_any (txt_fields[MENU_ELEMENT_TXT], "prompt", "command", NULL)) {
			startupnotify_or_option_of_it_selected = TRUE;
		}

		// Default button text for a selected option
		if (prompt_or_command_selected_and_action_is_Execute || startupnotify_or_option_of_it_selected) {
			gtk_label_set_text (GTK_LABEL (bt_add_action_option_label), "Option");
		}

		if (prompt_or_command_selected_and_action_is_Execute || 
			(streq (txt_fields[TYPE_TXT], "option block") && number_of_children_of_iter == NUMBER_OF_STARTUPNOTIFY_OPTS)) {
			// All options of "Execute" are set.
			if (number_of_children_of_parent == NUMBER_OF_EXECUTE_OPTS) {
				all_options_have_been_set_msg ("this Execute action");
			}
			// One option of "Execute" is not set.
 			else if (number_of_children_of_parent == NUMBER_OF_EXECUTE_OPTS - 1) {
				gboolean execute_options_status[NUMBER_OF_EXECUTE_OPTS] = { 0 };

				guint8 execute_opts_cnt;

				check_for_existing_options (&parent, NUMBER_OF_EXECUTE_OPTS, execute_options, execute_options_status);

				for (execute_opts_cnt = 0; execute_opts_cnt < NUMBER_OF_EXECUTE_OPTS; execute_opts_cnt++) {
					if (!execute_options_status[execute_opts_cnt]) {
						g_signal_handler_disconnect (bt_add[ACTION_OR_OPTION], handler_id_action_option_button_clicked);
						handler_id_action_option_button_clicked = 
							g_signal_connect_swapped (bt_add[ACTION_OR_OPTION], "clicked", 
													  G_CALLBACK ((execute_opts_cnt != STARTUPNOTIFY) ? 
																  add_new : generate_action_option_combo_box), 
													  (execute_opts_cnt != STARTUPNOTIFY) ? 
													  execute_options[execute_opts_cnt] : "Startupnotify");
						gtk_label_set_text (GTK_LABEL (bt_add_action_option_label), execute_displayed_txts[execute_opts_cnt]);
					}
				}
			}
		}

		if (startupnotify_or_option_of_it_selected) {
			// "startupnotify" option block or option of it selected and all options of "startupnotify" are set.
			if ((streq (txt_fields[TYPE_TXT], "option block") && 
				number_of_children_of_iter == NUMBER_OF_STARTUPNOTIFY_OPTS && 
				number_of_children_of_parent == NUMBER_OF_EXECUTE_OPTS) || 
				(streq (txt_fields[TYPE_TXT], "option") && 
				number_of_children_of_parent == NUMBER_OF_STARTUPNOTIFY_OPTS)) {
				all_options_have_been_set_msg ("startupnotify");
			}
			// "startupnotify" option block or option of it selected and one option of "startupnotify" not set.
			else if ((streq (txt_fields[TYPE_TXT], "option block") && 
					 number_of_children_of_iter == NUMBER_OF_STARTUPNOTIFY_OPTS - 1 && 
					 number_of_children_of_parent == NUMBER_OF_EXECUTE_OPTS) || 
					 (streq (txt_fields[TYPE_TXT], "option") && 
					 number_of_children_of_parent == NUMBER_OF_STARTUPNOTIFY_OPTS - 1)) {
				gboolean startupnotify_options_status[NUMBER_OF_STARTUPNOTIFY_OPTS] = { 0 };

				guint8 snotify_opts_cnt;

				if (streq (txt_fields[TYPE_TXT], "option block")) {
					parent = iter;
				}

				check_for_existing_options (&parent, NUMBER_OF_STARTUPNOTIFY_OPTS, 
											startupnotify_options, startupnotify_options_status);

				for (snotify_opts_cnt = 0; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
					if (!startupnotify_options_status[snotify_opts_cnt]) {
						g_signal_handler_disconnect (bt_add[ACTION_OR_OPTION], handler_id_action_option_button_clicked);
						handler_id_action_option_button_clicked = g_signal_connect_swapped (bt_add[ACTION_OR_OPTION],
																  "clicked", 
																  G_CALLBACK (add_new), 
																  startupnotify_options[snotify_opts_cnt]);
						gtk_label_set_text (GTK_LABEL (bt_add_action_option_label), startupnotify_displayed_txts[snotify_opts_cnt]);
						break;
					}
				}
			}
		}

		/*
			"prompt" option selected and action is "Exit" or "SessionLogout" or
			"command" option selected and action is "Restart".
		*/
		if (streq (txt_fields[TYPE_TXT], "option") && streq_any (txt_fields[MENU_ELEMENT_TXT], "prompt", "command", NULL) && 
			streq_any (menu_element_txt_parent, "Exit", "Restart", "SessionLogout", NULL)) {
			msg_label_txt = g_strconcat ("this ", 
										 (streq (menu_element_txt_parent, "Exit")) ? 
										 "Exit" : ((streq (menu_element_txt_parent, "Restart")) ? 
												   "Restart" : "SessionLogout"), " action", NULL);
			all_options_have_been_set_msg (msg_label_txt);

			// Cleanup
			g_free (msg_label_txt);
		}
	}

	// Set entry fields if not one of the conditions below applies.
	if (!(streq_any (txt_fields[TYPE_TXT], "action", "option block", NULL) || 
		(streq (txt_fields[TYPE_TXT], "option") && 
		(streq (txt_fields[MENU_ELEMENT_TXT], "enabled") || 
		streq_any (menu_element_txt_parent, "Exit", "SessionLogout", NULL))))) {
		set_entry_fields ();
	}

	// Cleanup
	g_free (menu_element_txt_parent);
	g_free (type_txt_parent);

	gtk_widget_queue_draw (treeview); // Force redrawing of treeview.
}

/* 

	Avoids overlapping of a row by the entry grid.

*/

static gboolean avoid_overlapping (void)
{
	GtkTreePath *path = gtk_tree_model_get_path (model, &iter);

	// There is no horizontal move to a certain column (NULL). Row and column alignments (0,0) are not used (FALSE).
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path, NULL, FALSE, 0, 0);

	// Cleanup
	gtk_tree_path_free (path);

	return FALSE;
}

/* 

	Sets the entry fields that appear below the treeview when a modifiable menu element has been selected.

*/

void set_entry_fields (void)
{
	guint8 entry_fields_cnt;
	GtkStyleContext *entry_context = gtk_widget_get_style_context (entry_fields[ICON_PATH_ENTRY]);

	gtk_widget_show (entry_grid);

	// Default settings
	gtk_label_set_text (GTK_LABEL (entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY]), " Label: ");
	gtk_widget_set_sensitive (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], TRUE);
	gtk_widget_hide (icon_chooser);
	gtk_widget_hide (remove_icon);
	for (entry_fields_cnt = ICON_PATH_ENTRY; entry_fields_cnt < NUMBER_OF_ENTRY_FIELDS; entry_fields_cnt++) {
		gtk_widget_hide (entry_labels[entry_fields_cnt]);
		gtk_widget_hide (entry_fields[entry_fields_cnt]);
	}

	if (txt_fields[ELEMENT_VISIBILITY_TXT]) { // = menu, pipe menu, item or separator
		gtk_entry_set_text (GTK_ENTRY (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), 
							(txt_fields[MENU_ELEMENT_TXT]) ? txt_fields[MENU_ELEMENT_TXT] : "");
		if (!streq (txt_fields[TYPE_TXT], "separator")) {
			if (G_UNLIKELY (!txt_fields[MENU_ELEMENT_TXT])) {
				gtk_entry_set_text (GTK_ENTRY (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), "(No label)");
				gtk_widget_set_sensitive (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], FALSE);
 			}

			gtk_widget_show (icon_chooser);
			gtk_widget_show (remove_icon);
			gtk_widget_show (entry_labels[ICON_PATH_ENTRY]);
			gtk_widget_show (entry_fields[ICON_PATH_ENTRY]);
			gtk_widget_set_sensitive (remove_icon, txt_fields[ICON_PATH_TXT] != NULL);
			gtk_entry_set_text (GTK_ENTRY (entry_fields[ICON_PATH_ENTRY]), 
								(txt_fields[ICON_PATH_TXT]) ? txt_fields[ICON_PATH_TXT] : "");

			if (!txt_fields[ICON_PATH_TXT] || g_file_test (txt_fields[ICON_PATH_TXT], G_FILE_TEST_EXISTS)) {
				gtk_style_context_remove_class (entry_context, "bg_class");
			}
			else {
				wrong_or_missing (entry_fields[ICON_PATH_ENTRY], find_entry_css_provider);
			}

			if (!streq (txt_fields[TYPE_TXT], "item")) {
				gtk_widget_show (entry_labels[MENU_ID_ENTRY]);
				gtk_widget_show (entry_fields[MENU_ID_ENTRY]);
				gtk_entry_set_text (GTK_ENTRY (entry_fields[MENU_ID_ENTRY]), txt_fields[MENU_ID_TXT]);
				if (streq (txt_fields[TYPE_TXT], "pipe menu")) {
					gtk_widget_show (entry_labels[EXECUTE_ENTRY]);
					gtk_widget_show (entry_fields[EXECUTE_ENTRY]);
					gtk_entry_set_text (GTK_ENTRY (entry_fields[EXECUTE_ENTRY]), 
										(txt_fields[EXECUTE_TXT]) ? txt_fields[EXECUTE_TXT] : "");
				}
			}
		}
	}
	// Option other than "startupnotify" and "enabled".
	else {
		gchar *label_txt = g_strdup_printf (" %s: ", 
											(streq (txt_fields[MENU_ELEMENT_TXT], "wmclass")) ? 
											"WM_CLASS" : txt_fields[MENU_ELEMENT_TXT]);
		label_txt[1] = g_ascii_toupper (label_txt[1]);

		gtk_label_set_text (GTK_LABEL (entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY]), label_txt);
		gtk_entry_set_text (GTK_ENTRY (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), 
									   (txt_fields[VALUE_TXT]) ? txt_fields[VALUE_TXT] : "");

		// Cleanup
		g_free (label_txt);
	}

	// If the last row is clicked, an overlapping of it by entry fields is avoided by this. No data is passed.
	gdk_threads_add_idle ((GSourceFunc) avoid_overlapping, NULL);
}
