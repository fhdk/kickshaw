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

#include "general_header_files/enum__ts_elements.h"
#include "drag_and_drop.h"

enum { SUBROWS_ICON_IMG, SUBROWS_ICON_IMG_STATUS, SUBROWS_ICON_MODIFICATION_TIME, SUBROWS_ICON_PATH, 
       SUBROWS_MENU_ELEMENT, SUBROWS_TYPE, SUBROWS_VALUE, SUBROWS_MENU_ID, SUBROWS_EXECUTE, SUBROWS_ELEMENT_VISIBILITY, 
       SUBROWS_EXPANSION_STATUS, SUBROWS_CURRENT_PATH_DEPTH, SUBROWS_MAX_PATH_DEPTH, SUBROWS_ROOT_VISIBILITY, 
       NUMBER_OF_SUBROW_ELEMENTS };

gboolean drag_motion_handler (G_GNUC_UNUSED GtkWidget      *widget,
                                            GdkDragContext *drag_context, 
                                            gint            x, 
                                            gint            y, 
                                            guint           time);
static gboolean subrows_creation_auxiliary (GtkTreeModel *filter_model, GtkTreePath *filter_path,
                                            GtkTreeIter *filter_iter, GPtrArray **subrows);
void drag_data_received_handler (G_GNUC_UNUSED GtkWidget      *widget, 
                                 G_GNUC_UNUSED GdkDragContext *drag_context, 
                                               gint            x, 
                                               gint            y);

/* 

    Allows a drop only under conditions which make sense.

*/

gboolean drag_motion_handler (G_GNUC_UNUSED GtkWidget      *widget,
                                            GdkDragContext *drag_context, 
                                            gint            x, 
                                            gint            y, 
                                            guint           time)
{
    GtkTreeViewDropPosition position;
    gboolean dropped_onto_row = FALSE; // Default

    gint dest_path_depth;
    GtkTreePath *dest_path_drag_motion, *source_parent_path, *dest_parent_path = NULL; // Default
    GtkTreeIter dest_parent_iter;
    gchar *menu_element_dest_parent_txt = NULL, *type_dest_parent_txt = NULL; // Defaults

    gchar *statusbar_txt = NULL; // Default

    GSList *source_paths_loop;
    GtkTreePath *source_path_loop;
    GtkTreeIter iter_loop;
    GtkTreeIter option_iter_loop; // Loop inside iter_loop.
    gint ch_cnt;

    gchar *menu_element_txt_loop, *type_txt_loop, *value_txt_loop, *element_visibility_txt_loop;
    gchar *menu_element_option_txt_loop; // Loop text for option_iter_loop.

    // Reset
    if (statusbar_msg_shown) {
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), 1); // Only one context (indicated by 1) with one message.
        statusbar_msg_shown = FALSE;
    }

    /*
        The return value of gtk_tree_view_get_dest_row_at_pos is not needed here, since dest_path_drag_motion == NULL 
        already indicates that the row has been dragged after the last row of the menu.
    */ 
    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (treeview), x, y, &dest_path_drag_motion, &position);

    if (dest_path_drag_motion) { // == Not dragged after the last row of the menu.
        dest_path_depth = gtk_tree_path_get_depth (dest_path_drag_motion);
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
            gtk_tree_model_get_iter (model, &dest_parent_iter, dest_path_drag_motion);
            dropped_onto_row = TRUE;
        } 
        else if (dest_path_depth > 1) {
            GtkTreeIter dest_iter;

            gtk_tree_model_get_iter (model, &dest_iter, dest_path_drag_motion);
            gtk_tree_model_iter_parent (model, &dest_parent_iter, &dest_iter);
        }

        if (dest_path_depth > 1 || dropped_onto_row) {
            gtk_tree_model_get (model, &dest_parent_iter, 
                                TS_MENU_ELEMENT, &menu_element_dest_parent_txt, 
                                TS_TYPE, &type_dest_parent_txt, 
                                -1);
            if (dest_path_depth > 1) {
                dest_parent_path = gtk_tree_model_get_path (model, &dest_parent_iter);
            }
        }
    }
    else { // Dragged after the last row of the menu.
        dest_path_depth = 1;
    }

    for (source_paths_loop = source_paths; source_paths_loop; source_paths_loop = source_paths_loop->next) {
        source_path_loop = gtk_tree_row_reference_get_path (source_paths_loop->data);
        gtk_tree_model_get_iter (model, &iter_loop, source_path_loop);
        gtk_tree_model_get (model, &iter_loop, 
                            TS_MENU_ELEMENT, &menu_element_txt_loop, 
                            TS_TYPE, &type_txt_loop, 
                            TS_VALUE, &value_txt_loop,
                            TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, 
                            -1);

        source_parent_path = gtk_tree_path_copy (source_path_loop);
        gtk_tree_path_up (source_parent_path);

        // Prevent that menus are dragged into themselves.
        if (streq (type_txt_loop, "menu") && dest_path_drag_motion && 
            gtk_tree_path_is_descendant (dest_path_drag_motion, source_path_loop)) {
            statusbar_txt = "!!! Menus can't be dragged into themselves !!!";
            goto cleanup;
        }

        // Prevent that menu elements are dragged to a place where they don't belong.
        if ((type_dest_parent_txt && // Not dragged to toplevel
            element_visibility_txt_loop && !streq (type_dest_parent_txt, "menu")) || 

            (streq (type_txt_loop, "action") && !streq (type_dest_parent_txt, "item")) || 

            (streq (type_txt_loop, "option") && streq (menu_element_txt_loop, "prompt") &&
            !(streq (type_dest_parent_txt, "action") &&  
            streq_any (menu_element_dest_parent_txt, "Execute", "Exit", "SessionLogout", NULL))) || 

            (streq (type_txt_loop, "option") && streq (menu_element_txt_loop, "command") && 
            !(streq (type_dest_parent_txt, "action") && 
            streq_any (menu_element_dest_parent_txt, "Execute", "Restart", NULL))) ||

            (streq (type_txt_loop, "option block") && 
            !(streq (type_dest_parent_txt, "action") && streq (menu_element_dest_parent_txt, "Execute"))) ||

            (streq (type_txt_loop, "option") && 
            streq_any (menu_element_txt_loop, "enabled", "name", "wmclass", "icon", NULL) && 
            !streq (type_dest_parent_txt, "option block"))) {
            statusbar_txt = "!!! Inappropriate new position !!!";
            goto cleanup;
        }

        // Prevent that Execute options are dragged inside the same Execute action if autosorting is activated.
        if (autosort_options && streq_any (type_txt_loop, "option", "option block", NULL) && 
            streq_any (menu_element_dest_parent_txt, "Execute", "startupnotify", NULL) &&
            gtk_tree_path_compare (source_parent_path, dest_parent_path) == 0) {
            statusbar_txt = "!!! Autosorting active - no movement of options inside the same action possible !!!";
            goto cleanup;
        }

        // Prevent multiple identical options in one action.
        if (streq_any (type_dest_parent_txt, "action", "option block", NULL) &&
            streq_any (type_txt_loop, "option", "option block", NULL) && 
            gtk_tree_path_compare (source_parent_path, dest_parent_path) != 0) {
            for (ch_cnt = 0; ch_cnt < gtk_tree_model_iter_n_children (model, &dest_parent_iter); ch_cnt++) {
                gtk_tree_model_iter_nth_child (model, &option_iter_loop, &dest_parent_iter, ch_cnt);
                gtk_tree_model_get (model, &option_iter_loop, TS_MENU_ELEMENT, &menu_element_option_txt_loop, -1);
                if (streq (menu_element_txt_loop, menu_element_option_txt_loop)) {
                    statusbar_txt = "!!! Only one option of a kind allowed !!!";
                    // Cleanup
                    g_free (menu_element_option_txt_loop);
                    goto cleanup;
                }
                // Cleanup
                g_free (menu_element_option_txt_loop);
            }
        }

        /*
            Prevent that a prompt option with a value other than "yes" or "no" 
            is dragged into the actions "Exit" and "SessionLogout".
        */
        if (streq (type_txt_loop, "option") && streq (menu_element_txt_loop, "prompt") && 
            streq_any (menu_element_dest_parent_txt, "Exit", "SessionLogout", NULL) && 
            !streq_any (value_txt_loop, "yes", "no", NULL)) {
            statusbar_txt = "!!! Prompt option must have value \"yes\" or \"no\" !!!";
        }

        cleanup:
        gtk_tree_path_free (source_path_loop);
        gtk_tree_path_free (source_parent_path);
        g_free (menu_element_txt_loop); 
        g_free (type_txt_loop);
        g_free (value_txt_loop);
        g_free (element_visibility_txt_loop);

        if (statusbar_txt) {
            break;
        }
    }

    // Cleanup
    gtk_tree_path_free (dest_path_drag_motion);
    gtk_tree_path_free (dest_parent_path);
    g_free (menu_element_dest_parent_txt);
    g_free (type_dest_parent_txt);

    if (statusbar_txt) {
        show_msg_in_statusbar (statusbar_txt);
        gdk_drag_status (drag_context, 0, time); // Indicates that a drop will not be accepted.
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/* 

    Helps with the duplication of the original subrows and the adjustment of their visibility status to 
    their new parent by two steps:
    1. Creation of arrays of the source row's subrows that include their path depth and expansion status.
    2. Adjustment of the expansion status of each subrow according to the ones of the source row.
    The element visibility of all menus, pipe menus, items and separators
    of subrows is also adjusted to the one of the destination parent.

*/

static gboolean subrows_creation_auxiliary (GtkTreeModel  *filter_model, 
                                            GtkTreePath   *filter_path, 
                                            GtkTreeIter   *filter_iter, 
                                            GPtrArray    **subrows)
{
    GtkTreePath *model_path;

    if (streq (g_ptr_array_index (subrows[SUBROWS_ROOT_VISIBILITY], 0), "n/a")) {
        // --- Step 1: Creation of subrow array(s) ---

        gint current_path_depth = gtk_tree_path_get_depth (filter_path);
        gint *max_path_depth = (gint *) subrows[SUBROWS_MAX_PATH_DEPTH][0].pdata;

        gpointer current_ts_field;

        guint8 subrows_elm_cnt;

        if (current_path_depth > *max_path_depth) {
            *max_path_depth = current_path_depth;
        }

        for (subrows_elm_cnt = 0; subrows_elm_cnt <= SUBROWS_ELEMENT_VISIBILITY; subrows_elm_cnt++) {
            gtk_tree_model_get (filter_model, filter_iter, subrows_elm_cnt, &current_ts_field, -1);
            g_ptr_array_add (subrows[subrows_elm_cnt], current_ts_field);
        }

        // SUBROWS_EXPANSION_STATUS
        // The path of the model, not filter model, is needed to check whether the row is expanded.
        model_path = gtk_tree_model_filter_convert_path_to_child_path ((GtkTreeModelFilter *) filter_model, filter_path);
        g_ptr_array_add (subrows[SUBROWS_EXPANSION_STATUS], 
                         GINT_TO_POINTER (gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), model_path)));

        // SUBROWS_CURRENT_PATH_DEPTH
        g_ptr_array_add (subrows[SUBROWS_CURRENT_PATH_DEPTH], GINT_TO_POINTER (current_path_depth));
    }
    else {
        // --- Step 2: Adjustment of subrows ---

        /* --- Step 2a: Expansion status ---

            The path of the model, not filter model, 
            is needed to set the expansion status and to check if an invisible ancestor exists.

        */
        model_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (filter_model), filter_path);

        if (GPOINTER_TO_INT (g_ptr_array_index (subrows[SUBROWS_EXPANSION_STATUS], 0))) {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), model_path, FALSE);
        }

        g_ptr_array_remove_index (subrows[SUBROWS_EXPANSION_STATUS], 0);

        // --- Step 2b: Visibilty of menus, pipe menus, items and separators ---
        gchar *element_visibility_txt_filter;

        gtk_tree_model_get (filter_model, filter_iter, TS_ELEMENT_VISIBILITY, &element_visibility_txt_filter, -1);

        if (!element_visibility_txt_filter) {
            // Cleanup
            gtk_tree_path_free (model_path);

            return FALSE;
        }

        GtkTreeIter model_iter;
        guint8 invisible_ancestor = check_if_invisible_ancestor_exists (model, model_path);
        gchar *element_visibility_txt_root = g_ptr_array_index (subrows[SUBROWS_ROOT_VISIBILITY], 0);
        gchar *new_element_visibility_txt = "visible"; // Default
        gchar *menu_element_txt_filter, *type_txt_filter;

        gtk_tree_model_get (filter_model, filter_iter, 
                            TS_MENU_ELEMENT, &menu_element_txt_filter, 
                            TS_TYPE, &type_txt_filter, 
                            -1);

        if (G_UNLIKELY (g_str_has_suffix (element_visibility_txt_root, "orphaned menu"))) {
            new_element_visibility_txt = "invisible dsct. of invisible orphaned menu";
        }
        else if (G_UNLIKELY (invisible_ancestor)) {
            new_element_visibility_txt = "invisible dsct. of invisible menu";
        }
        else if (G_UNLIKELY (!menu_element_txt_filter && !streq (type_txt_filter, "separator"))) {
            new_element_visibility_txt = (streq (type_txt_filter, "item")) ? "invisible item" : "invisible menu";
        }
 
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filter_model), &model_iter, filter_iter);
        gtk_tree_store_set (treestore, &model_iter, TS_ELEMENT_VISIBILITY, new_element_visibility_txt, -1);

        // Cleanup
        g_free (menu_element_txt_filter);
        g_free (type_txt_filter);
        g_free (element_visibility_txt_filter);
    }

    // Cleanup
    gtk_tree_path_free (model_path);

    return FALSE;
}

/* 

    Creates a new row at destination and copies all subrows of the source row to it.

*/

void drag_data_received_handler (G_GNUC_UNUSED GtkWidget      *widget, 
                                 G_GNUC_UNUSED GdkDragContext *drag_context, 
                                               gint            x, 
                                               gint            y)
{
    GtkTreeViewDropPosition position;
    gboolean dropped_onto_row = FALSE; // Default

    gboolean to_be_appended_at_toplevel = FALSE, to_be_appended_as_last_row = FALSE; // Defaults
    gint insertion_position = 0; // Initialised to avoid compiler warning since this variable might not be used.

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GSList *new_rows = NULL;
    GtkTreeModel *filter_model = NULL; // Default
    GtkTreePath *dest_path, *new_path;
    GtkTreeIter source_iter, *subrow_iters;
    GtkTreeIter dest_iter, dest_parent_iter;
    GtkTreeIter new_iter;
    gint dest_path_depth = 0; // Default
    gint current_path_depth;
    guint allocated_size;
    gchar *menu_element_dest_parent_txt = NULL, *type_dest_parent_txt = NULL, 
          *element_visibility_parent_txt = NULL; // Defaults
    gchar *menu_element_new_row_txt, *type_new_row_txt;

    gboolean source_row_has_child;
    gboolean expand_new_row = FALSE; // Initialised to avoid compiler warning.
    GPtrArray *subrows[NUMBER_OF_SUBROW_ELEMENTS];

    gpointer copied_ts_row_fields[NUMBER_OF_TS_ELEMENTS];
    gchar *new_element_visibility_txt;

    gpointer current_array;

    GSList *source_paths_loop, *new_rows_loop;
    GtkTreePath *source_path_loop;
    guint subrows_cnt;
    guint8 ts_cnt, elm_cnt, subrows_elm_cnt;

    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (treeview), x, y, &dest_path, &position);
    /*
        If the result is NULL, the function is not aborted, since the result indicates 
        that the row has been dragged after the last row of the menu.
    */ 

     // Don't continue if the source rows are dropped onto themselves.
    if (dest_path) {
        for (source_paths_loop = source_paths; source_paths_loop; source_paths_loop = source_paths_loop->next) {
            source_path_loop = gtk_tree_row_reference_get_path (source_paths_loop->data);
            if (gtk_tree_path_compare (source_path_loop, dest_path) == 0) {

                // Cleanup
                gtk_tree_path_free (source_path_loop);
                gtk_tree_path_free (dest_path);

                return;
            }
            // Cleanup
            gtk_tree_path_free (source_path_loop);
        }
    }

    if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
        gtk_tree_model_get_iter (model, &dest_parent_iter, dest_path);
        dropped_onto_row = TRUE;
    }

    if (dest_path && position == GTK_TREE_VIEW_DROP_AFTER) {
        gtk_tree_path_next (dest_path);
    }

    if (!dest_path) {
        dest_path_depth = 1;
        to_be_appended_at_toplevel = TRUE;
    }
    else if (!dropped_onto_row) {
        dest_path_depth = gtk_tree_path_get_depth (dest_path);
        insertion_position = gtk_tree_path_get_indices (dest_path)[dest_path_depth - 1];
    }

    if (dest_path_depth > 1) {
        if (gtk_tree_model_get_iter (model, &dest_iter, dest_path)) {
            gtk_tree_model_iter_parent (model, &dest_parent_iter, &dest_iter);
        }
        else { // Append as last row(s)
            GtkTreePath *dest_parent_path = gtk_tree_path_copy (dest_path);

            to_be_appended_as_last_row = TRUE;
            gtk_tree_path_up (dest_parent_path);
            gtk_tree_model_get_iter (model, &dest_parent_iter, dest_parent_path);

            // Cleanup
            gtk_tree_path_free (dest_parent_path);
        }
    }

    if (dest_path_depth > 1 || dropped_onto_row) {
        gtk_tree_model_get (model, &dest_parent_iter, 
                            TS_MENU_ELEMENT, &menu_element_dest_parent_txt,
                            TS_TYPE, &type_dest_parent_txt, 
                            TS_ELEMENT_VISIBILITY, &element_visibility_parent_txt, 
                            -1);
    }

    for (source_paths_loop = source_paths; source_paths_loop; source_paths_loop = source_paths_loop->next) {
        source_path_loop = gtk_tree_row_reference_get_path (source_paths_loop->data);
        gtk_tree_model_get_iter (model, &source_iter, source_path_loop);


        // --- Create root row at destination. ---


        // Retrieve data of the source row.
        for (ts_cnt = 0; ts_cnt < NUMBER_OF_TS_ELEMENTS; ts_cnt++) {
            gtk_tree_model_get (model, &source_iter, ts_cnt, &copied_ts_row_fields[ts_cnt], -1);
        }

        // Retrieve data of the subrows of the source row, if they exist.
        if ((source_row_has_child = gtk_tree_model_iter_has_child (model, &source_iter))) { // Parentheses avoid gcc warning.
            for (subrows_elm_cnt = 0; subrows_elm_cnt < NUMBER_OF_SUBROW_ELEMENTS; subrows_elm_cnt++) {
                subrows[subrows_elm_cnt] = g_ptr_array_new ();
            }
            g_ptr_array_add (subrows[SUBROWS_MAX_PATH_DEPTH], GUINT_TO_POINTER (0));
            g_ptr_array_add (subrows[SUBROWS_ROOT_VISIBILITY], "n/a");

            filter_model = gtk_tree_model_filter_new (model, source_path_loop);
            gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) subrows_creation_auxiliary, &subrows);
            // Cleanup
            g_object_unref (filter_model);

            expand_new_row = gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), source_path_loop);
        }

        /*
            If a menu, pipe menu, item or separator is dragged into a menu, 
            its element visibility is set according to the one of the latter. 
            If dragged to toplevel, the element visiblity is adjusted as well.
        */
        if (!type_dest_parent_txt || streq (type_dest_parent_txt, "menu")) {
            new_element_visibility_txt = "visible"; // Default

            if (G_LIKELY (!element_visibility_parent_txt || streq (element_visibility_parent_txt, "visible"))) {
                if (G_UNLIKELY (!element_visibility_parent_txt && 
                                streq (copied_ts_row_fields[SUBROWS_ELEMENT_VISIBILITY], "invisible orphaned menu"))) {
                    new_element_visibility_txt = "invisible orphaned menu";
                }
                else if (G_UNLIKELY (!copied_ts_row_fields[SUBROWS_MENU_ELEMENT] && 
                                     !streq (copied_ts_row_fields[SUBROWS_TYPE], "separator"))) {
                    new_element_visibility_txt = ((streq (copied_ts_row_fields[SUBROWS_TYPE], "item") ? 
                                                  "invisible item" : "invisible menu"));
                }
            }
            else {
                new_element_visibility_txt = (g_str_has_suffix (element_visibility_parent_txt, "invisible menu")) ? 
                                                                "invisible dsct. of invisible menu" : 
                                                                "invisible dsct. of invisible orphaned menu";
            }

            free_and_reassign (copied_ts_row_fields[SUBROWS_ELEMENT_VISIBILITY], g_strdup (new_element_visibility_txt));
        }

        // Add dragged source row at new position.
        gtk_tree_store_insert (treestore, &new_iter, (dest_path_depth == 1) ? NULL : &dest_parent_iter, 
                               (to_be_appended_at_toplevel || to_be_appended_as_last_row || dropped_onto_row) ? 
                               -1 : insertion_position++);

        for (ts_cnt = 0; ts_cnt < NUMBER_OF_TS_ELEMENTS; ts_cnt++) {
            gtk_tree_store_set (treestore, &new_iter, ts_cnt, copied_ts_row_fields[ts_cnt], -1);
        }

        // If at least one row was dropped onto another one, expand the latter if it had not already been expanded.
        if (dropped_onto_row && !gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), dest_path)) {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), dest_path, FALSE);
        }

        // Add the new row to the list of new rows.
        new_path = gtk_tree_model_get_path (model, &new_iter);
        /*
            The new rows list has to consist of row references, since a dragged row might be an option which is sorted 
            after the insertion if autosorting is enabled and thus might change the path of other inserted options which 
            have to be sorted next.
        */
        new_rows = g_slist_prepend (new_rows, gtk_tree_row_reference_new (model, new_path));


        // --- Add subrows, if they exist. ---

        if (source_row_has_child) {
            // Copy all children of the source row to the new row.
            allocated_size = sizeof (GtkTreeIter) * GPOINTER_TO_UINT (g_ptr_array_index (subrows[SUBROWS_MAX_PATH_DEPTH], 0));
            subrow_iters = g_slice_alloc (allocated_size);

            for (subrows_cnt = 0; subrows_cnt < subrows[0]->len; subrows_cnt++) {
                current_path_depth = GPOINTER_TO_INT (g_ptr_array_index (subrows[SUBROWS_CURRENT_PATH_DEPTH], subrows_cnt));
                gtk_tree_store_append (treestore, &subrow_iters[current_path_depth - 1], 
                                       (current_path_depth == 1) ? &new_iter : &subrow_iters[current_path_depth - 2]);

                for (ts_cnt = 0; ts_cnt < NUMBER_OF_TS_ELEMENTS; ts_cnt++) {
                    current_array = g_ptr_array_index (subrows[ts_cnt], subrows_cnt);
                    gtk_tree_store_set (treestore, &subrow_iters[current_path_depth - 1], ts_cnt, current_array, -1);

                    // Cleanup
                    if (ts_cnt == TS_ICON_IMG && current_array) {
                        g_object_unref (current_array);
                    }
                    else if (ts_cnt >= TS_ICON_MODIFICATION_TIME) { // No dyn. alloc. mem. for TS_ICON_IMG_STATUS.
                        g_free (current_array);
                    }
                }
            }

            // If the source row was a node that was expanded, expand the new row as well.
            if (expand_new_row) {
                gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), new_path, FALSE);
            }

            // Set expansions status of all subrows, also element visibility for all menus, pipe menus, items and separators.
            g_ptr_array_index (subrows[SUBROWS_ROOT_VISIBILITY], 0) = copied_ts_row_fields[TS_ELEMENT_VISIBILITY];
            filter_model = gtk_tree_model_filter_new (model, new_path);
            gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) subrows_creation_auxiliary, &subrows);

            // Cleanup
            g_object_unref (filter_model);
            for (subrows_elm_cnt = 0; subrows_elm_cnt < NUMBER_OF_SUBROW_ELEMENTS; subrows_elm_cnt++) {
                g_ptr_array_free (subrows[subrows_elm_cnt], TRUE);
            }
            g_slice_free1 (allocated_size, subrow_iters);
        }

        // Cleanup
        if (copied_ts_row_fields[TS_ICON_IMG]) {
            g_object_unref (copied_ts_row_fields[TS_ICON_IMG]);
        }
        for (elm_cnt = TS_ICON_MODIFICATION_TIME; elm_cnt < NUMBER_OF_TS_ELEMENTS; elm_cnt++) {
            g_free (copied_ts_row_fields[elm_cnt]);
        }
        gtk_tree_path_free (source_path_loop);
    }

    // The source rows are still selected, so they may be deleted that simple.
    remove_rows ("dnd");

    g_signal_handler_block (selection, handler_id_row_selected); // Deactivates unnecessary selection check.

    /* 
        This prevents a possible block of the change of the selection of nodes if 
        the to be dragged nodes had been clicked on again before dragging.
    */
    gtk_tree_selection_set_select_function (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), 
                                            (GtkTreeSelectionFunc) selection_block_unblock, GINT_TO_POINTER (TRUE), NULL);

    /*
        Select the new root row(s). If these rows are options or option blocks that have been moved elsewhere and 
        autosorting is activated, sort the options resp. option block.
    */
    gtk_tree_selection_unselect_all (selection);

    for (new_rows_loop = new_rows; new_rows_loop; new_rows_loop = new_rows_loop->next) {
        new_path = gtk_tree_row_reference_get_path (new_rows_loop->data);
        gtk_tree_model_get_iter (model, &iter, new_path);
        gtk_tree_model_get (model, &iter, 
                            TS_MENU_ELEMENT, &menu_element_new_row_txt, 
                            TS_TYPE, &type_new_row_txt, 
                            -1);
        /*
            If the inserted row is an option or option block and the number of children of the parent action/option block 
            (=all options that currently exist inside the current parent) > 1, then the new row is always an option 
            of "Execute" or an option of "startupnotify", since only an "Execute" action or a "startupnotify" option block 
            can have more than one child.
        */
        if (autosort_options && streq_any (type_new_row_txt, "option", "option block", NULL) && 
            gtk_tree_model_iter_n_children (model, &dest_parent_iter) > 1) {
            sort_execute_or_startupnotify_options_after_insertion (selection, &dest_parent_iter, 
                                                                   menu_element_dest_parent_txt, menu_element_new_row_txt);
        }
        else {
            gtk_tree_selection_select_path (selection, new_path);
        }

        // Cleanup
        gtk_tree_path_free (new_path);
        g_free (menu_element_new_row_txt);
        g_free (type_new_row_txt);
    }

    g_signal_handler_unblock (selection, handler_id_row_selected);

    // Cleanup
    g_free (menu_element_dest_parent_txt);
    g_free (type_dest_parent_txt);
    g_free (element_visibility_parent_txt);
    gtk_tree_path_free (dest_path);
    g_slist_free_full (new_rows, (GDestroyNotify) gtk_tree_row_reference_free);

    activate_change_done ();
    row_selected ();
}
