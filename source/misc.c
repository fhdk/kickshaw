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

#include <gtk/gtk.h>

#include "definitions_and_enumerations.h"
#include "misc.h"

gboolean key_pressed (GtkWidget *treeview, GdkEventKey *event);
gboolean selection_block_unblock (G_GNUC_UNUSED GtkTreeSelection *selection, 
                                  G_GNUC_UNUSED GtkTreeModel     *model,
                                  G_GNUC_UNUSED GtkTreePath      *path, 
                                  G_GNUC_UNUSED gboolean          path_currently_selected, 
                                  gpointer                        block_state);
gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event);
gboolean mouse_released (GtkTreeView *treeview);
void expand_or_collapse_all (gpointer expand_pointer);

/* 

    Function that deals with key press events; currently only used for the "delete" key.

    Because the callback function for "key-press-event" receives three arguments 
    (widget, event and user data) and event is needed, the function can't be called via 
    g_signal_connect_swapped (), so the widget argument has to remain. 
    The "user data" argument is omitted.

*/

gboolean key_pressed (GtkWidget *treeview, GdkEventKey *event)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

    if (gtk_tree_selection_count_selected_rows (selection) > 0 && event->keyval == GDK_KEY_Delete) {
        remove_rows ("delete key");
    }

    // FALSE = Propagate other events.
    return FALSE;
}

/* 

    Sets if the selection state of a node may be toggled.

*/

gboolean selection_block_unblock (G_GNUC_UNUSED GtkTreeSelection *selection, 
                                  G_GNUC_UNUSED GtkTreeModel     *model,
                                  G_GNUC_UNUSED GtkTreePath      *path, 
                                  G_GNUC_UNUSED gboolean          path_currently_selected, 
                                                gpointer          block_state)
{
    return GPOINTER_TO_INT (block_state);
}

/* 

    A right click inside the treeview opens the context menu, a left click activates blocking of
    selection changes if more than one row has been selected and one of these rows has been clicked again.

    Because the callback function for "button-press-event" receives three arguments 
    (widget, event and user data) and event is needed, the function can't be called via 
    g_signal_connect_swapped (), so the widget argument has to remain. 
    The "user data" argument is omitted.

*/

gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 3) { // Right click
            create_context_menu (event);
            return TRUE; // Stop other handlers from being invoked for the event.
        }
        else if (event->button == 1 && gtk_tree_selection_count_selected_rows (selection) > 1) {
            if (event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK)) {
                return FALSE;
            }


            GtkTreePath *path;

            // The last three arguments (column, cell_x and cell_y) are not used.
            if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview), event->x, event->y, &path, NULL, NULL, NULL)) {
                return FALSE; // No row clicked, return without propagating the event further.
            }

            if (gtk_tree_selection_path_is_selected (selection, path)) {
                // NULL == No destroy function for user data.
                gtk_tree_selection_set_select_function (selection, (GtkTreeSelectionFunc) selection_block_unblock, 
                                                        GINT_TO_POINTER (FALSE), NULL);
            }

            // Cleanup
            gtk_tree_path_free (path);
        }
    }
    return FALSE; // Propagate the event further.
}

/* 

    Unblocks selection changes.

    Of the three arguments of the callback function for "button-release-event", only the first for the widget is used. 
    Even that one could actually be omitted if ks.treeview was chosen instead.

*/

gboolean mouse_released (GtkTreeView *treeview)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

    if (gtk_tree_selection_count_selected_rows (selection) < 2) {
        return FALSE;
    }

    // NULL == No destroy function for user data.
    gtk_tree_selection_set_select_function (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), 
                                            (GtkTreeSelectionFunc) selection_block_unblock, GINT_TO_POINTER (TRUE), NULL);

    return FALSE;
}

/* 

    Expands the tree view so the whole structure is visible or
    collapses the tree view so just the toplevel elements are visible.

*/

void expand_or_collapse_all (gpointer expand_pointer)
{
    gboolean expand = GPOINTER_TO_INT (expand_pointer);

    if (expand) {
        gtk_tree_view_expand_all (GTK_TREE_VIEW (ks.treeview));
    }
    else {
        gtk_tree_view_collapse_all (GTK_TREE_VIEW (ks.treeview));
        gtk_tree_view_columns_autosize (GTK_TREE_VIEW (ks.treeview));
    }

    gtk_widget_set_sensitive (ks.mb_expand_all_nodes, !expand);
    gtk_widget_set_sensitive (ks.mb_collapse_all_nodes, expand);
    gtk_widget_set_sensitive ((GtkWidget *) ks.tb[TB_EXPAND_ALL], !expand);
    gtk_widget_set_sensitive ((GtkWidget *) ks.tb[TB_COLLAPSE_ALL], expand);
}
