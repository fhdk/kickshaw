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

#include <gtk/gtk.h>

#include "definitions_and_enumerations.h"
#include "context_menu.h"

// CM = context menu, indicating that the enums are only used here
enum { CM_RECURSIVELY, CM_IMMEDIATE, CM_COLLAPSE, CM_NUMBER_OF_EXPANSION_STATUS_CHANGES };

static void create_cm_headline (GtkWidget *context_menu, gchar *headline_txt);
static void expand_or_collapse_selected_rows (gpointer action_pointer);
static void add_startupnotify_or_execute_options_to_context_menu (GtkWidget *context_menu, gboolean startupnotify_opts,
                                                                  GtkTreeIter *parent, guint8 number_of_opts,
                                                                  gchar **options_array);
void create_context_menu (GdkEventButton *event);

/*

    Creates a headline inside the context menu.

*/

static void create_cm_headline (GtkWidget *context_menu,
                                gchar     *headline_txt)
{
    GtkWidget *headline_label = gtk_label_new (headline_txt);
    GtkWidget *menu_item = gtk_menu_item_new ();

/*
    This will result in a warning when using GTK 3.14, because gtk_misc_set_alignment is deprecated since 3.14,
    but gtk_label_set_xalign is available only since 3.16. With gtk_widget_set_halign it is not possible to
    style the label in the desired way as with gtk_label_set_xalign. The warning has no repercussions though.
*/
#if GTK_CHECK_VERSION(3,16,0)
    gtk_label_set_xalign (GTK_LABEL (headline_label), 0.0);
#else
    gtk_misc_set_alignment (GTK_MISC (headline_label), 0.0, 0.5);
#endif
    gtk_style_context_add_class (gtk_widget_get_style_context (headline_label), "cm_class");
    gtk_css_provider_load_from_data (ks.cm_css_provider, ".cm_class { color: rgba(255,255,255,100); background: rgba(102,102,155,100) }", -1, NULL);
    gtk_style_context_add_provider (gtk_widget_get_style_context (headline_label),
                                    GTK_STYLE_PROVIDER (ks.cm_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_container_add (GTK_CONTAINER (menu_item), headline_label);
    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
}

/*

    Expands or collapses all selected rows according to the choice done.

*/

static void expand_or_collapse_selected_rows (gpointer action_pointer)
{
    guint8 action = GPOINTER_TO_UINT (action_pointer);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &ks.model);

    GList *selected_rows_loop;
    GtkTreePath *path_loop;

    for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
        path_loop = selected_rows_loop->data;
        /*
            If the nodes are already expanded recursively and only the immediate children shall be expanded now,
            it is the fastest way to collapse all nodes first.
        */
        gtk_tree_view_collapse_row (GTK_TREE_VIEW (ks.treeview), path_loop);
        if (action == CM_COLLAPSE) {
            gtk_tree_view_columns_autosize (GTK_TREE_VIEW (ks.treeview));
        }
        else {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (ks.treeview), path_loop, action == CM_RECURSIVELY);
        }
    }

    // Cleanup
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

/*

    Adds all currently unused startupnotify or Execute options to the context menu.

*/

static void add_startupnotify_or_execute_options_to_context_menu (GtkWidget    *context_menu,
                                                                  gboolean      startupnotify_opts,
                                                                  GtkTreeIter  *parent,
                                                                  guint8        number_of_opts,
                                                                  gchar       **options_array)
{
    guint allocated_size = sizeof (gboolean) * number_of_opts;
    // Used because VLAs cannot be initialized by any form of initialization syntax.
    gboolean *opts_exist = g_slice_alloc0 (allocated_size); // Initialise all elements to FALSE.
    gchar *menu_item_txt;
    GtkWidget *menu_item;
    gboolean add_new_callback;

    guint8 opts_cnt;

    check_for_existing_options (parent, number_of_opts, options_array, opts_exist);

    for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++) {
        if (!opts_exist[opts_cnt]) {
            menu_item_txt = g_strconcat ("Add ", options_array[opts_cnt], NULL);
            menu_item = gtk_menu_item_new_with_label (menu_item_txt);

            // Cleanup
            g_free (menu_item_txt);

            add_new_callback = startupnotify_opts || opts_cnt != STARTUPNOTIFY;
            g_signal_connect_swapped (menu_item, "activate",
                                      G_CALLBACK ((add_new_callback) ? add_new : generate_items_for_action_option_combo_box),
                                                  (add_new_callback) ? options_array[opts_cnt] : "Startupnotify");
            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
        }
    }

    // Cleanup
    g_slice_free1 (allocated_size, opts_exist);
}

/*

    Right click on the treeview presents a context menu for adding, changing and deleting menu elements.

*/

void create_context_menu (GdkEventButton *event)
{
    GtkWidget *context_menu;
    GtkWidget *menu_item;
    gchar *menu_item_txt;

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);
    GList *selected_rows = NULL;
    GtkTreePath *path;

    gboolean selected_row_is_one_of_the_previously_selected_ones = FALSE; // Default

    GList *selected_rows_loop;
    GtkTreeIter iter_loop;

    // The last three arguments (column, cell_x and cell_y) aren't used.
    gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (ks.treeview), event->x, event->y, &path, NULL, NULL, NULL);

    context_menu = gtk_menu_new ();

    if (path) {
        if (number_of_selected_rows > 1) {
            /*
                If several rows are selected and a row that is not one of these is rightclicked,
                select the latter and unselect the former ones.
            */
            selected_rows = gtk_tree_selection_get_selected_rows (selection, &ks.model);
            for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
                if (gtk_tree_path_compare (path, selected_rows_loop->data) == 0) {
                    selected_row_is_one_of_the_previously_selected_ones = TRUE;
                    break;
                }
            }
            if (!selected_row_is_one_of_the_previously_selected_ones) {
                g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
            }
            else {
                gboolean at_least_one_row_has_no_icon = FALSE; // Default

                GdkPixbuf *icon_img_pixbuf_loop;

                // Add context menu item for deletion of icons if all selected rows contain icons.
                for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
                    gtk_tree_model_get_iter (ks.model, &iter_loop, selected_rows_loop->data);
                    gtk_tree_model_get (ks.model, &iter_loop, TS_ICON_IMG, &icon_img_pixbuf_loop, -1);
                    if (!icon_img_pixbuf_loop) {
                        at_least_one_row_has_no_icon = TRUE;
                        break;
                    }
                    // Cleanup
                    g_object_unref (icon_img_pixbuf_loop);
                }
                if (!at_least_one_row_has_no_icon) {
                    menu_item = gtk_menu_item_new_with_label ("Remove icons");
                    g_signal_connect (menu_item, "activate", G_CALLBACK (remove_icons_from_menus_or_items), NULL);
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
                }
            }
        }
        /*
            Either no or one row had been selected before or
            two or more rows had been selected before and another row not one of these has now been rightclicked.
        */
        if (number_of_selected_rows < 2 || !selected_row_is_one_of_the_previously_selected_ones) {
            gboolean not_all_options_of_selected_action_set = FALSE; // Default

            gtk_tree_selection_unselect_all (selection);
            gtk_tree_selection_select_path (selection, path);
            selected_rows = gtk_tree_selection_get_selected_rows (selection, &ks.model);
            number_of_selected_rows = 1; // If there had not been a selection before.

            // Icon
            if (streq_any (ks.txt_fields[TYPE_TXT], "menu", "pipe menu", "item", NULL)) {
                if (STREQ (ks.txt_fields[TYPE_TXT], "item")) {
                    create_cm_headline (context_menu, " Icon"); // Items have "Icon" and "Action" as context menu headlines.
                }
                menu_item = gtk_menu_item_new_with_label ((ks.txt_fields[ICON_PATH_TXT]) ? "Change icon" : "Add icon");
                g_signal_connect (menu_item, "activate", G_CALLBACK (icon_choosing_by_button_or_context_menu), NULL);
                gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                if (ks.txt_fields[ICON_PATH_TXT]) {
                    menu_item = gtk_menu_item_new_with_label ("Remove icon");
                    g_signal_connect (menu_item, "activate", G_CALLBACK (remove_icons_from_menus_or_items), NULL);
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                }
                if (!STREQ (ks.txt_fields[TYPE_TXT], "item")) { // Items have "Icon" and "Actions" as context menu headlines.
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
                }
            }

            if (gtk_tree_path_get_depth (path) > 1) {
                GtkTreeIter parent;

                // Startupnotify options
                if (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL) &&
                    streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "startupnotify", "enabled", "name", "wmclass", "icon", NULL)) {
                    if (!STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "startupnotify")) {
                        gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
                    }
                    else {
                        parent = ks.iter;
                    }
                    if (gtk_tree_model_iter_n_children (ks.model, &parent) < NUMBER_OF_STARTUPNOTIFY_OPTS) {
                        GtkTreeIter grandparent;
                        gtk_tree_model_iter_parent (ks.model, &grandparent, &parent);
                        gint number_of_chldn_of_gp = gtk_tree_model_iter_n_children (ks.model, &grandparent);

                        if (STREQ (ks.txt_fields[TYPE_TXT], "option block") && number_of_chldn_of_gp < NUMBER_OF_EXECUTE_OPTS) {
                            create_cm_headline (context_menu,
                                                (gtk_tree_model_iter_n_children (ks.model, &parent) ==
                                                NUMBER_OF_STARTUPNOTIFY_OPTS - 1) ?
                                                " Startupnotify option" : " Startupnotify options");
                        }

                        add_startupnotify_or_execute_options_to_context_menu (context_menu, TRUE, &parent,
                                                                              NUMBER_OF_STARTUPNOTIFY_OPTS,
                                                                              ks.startupnotify_options);

                        // No "Execute option(s)" headline following.
                        if (number_of_chldn_of_gp == NUMBER_OF_EXECUTE_OPTS || STREQ (ks.txt_fields[TYPE_TXT], "option")) {
                            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
                        }
                    }
                }

                gchar *menu_element_txt_parent;
                gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
                gtk_tree_model_get (ks.model, &parent, TS_MENU_ELEMENT, &menu_element_txt_parent, -1);
                // Execute options
                if ((STREQ (ks.txt_fields[TYPE_TXT], "action") && STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Execute")) ||
                    (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL) &&
                    STREQ (menu_element_txt_parent, "Execute"))) {
                    if (!STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Execute")) {
                        gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
                    }
                    else {
                        parent = ks.iter;
                    }
                    if (gtk_tree_model_iter_n_children (ks.model, &parent) < NUMBER_OF_EXECUTE_OPTS) {
                        if (STREQ (ks.txt_fields[TYPE_TXT], "action") ||
                            (STREQ (ks.txt_fields[TYPE_TXT], "option block") &&
                            gtk_tree_model_iter_n_children (ks.model, &ks.iter) < NUMBER_OF_STARTUPNOTIFY_OPTS)) {
                            create_cm_headline (context_menu,
                                (gtk_tree_model_iter_n_children (ks.model, &parent) == NUMBER_OF_EXECUTE_OPTS - 1) ?
                                " Execute option" : " Execute options");
                        }

                        add_startupnotify_or_execute_options_to_context_menu (context_menu, FALSE, &parent,
                                                                              NUMBER_OF_EXECUTE_OPTS, ks.execute_options);

                        if (STREQ (ks.txt_fields[TYPE_TXT], "action")) {
                            not_all_options_of_selected_action_set = TRUE;
                        }
                        else { // If "Execute" is selected, the headline for the actions follows.
                            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
                        }
                    }
                }
                // Cleanup
                g_free (menu_element_txt_parent);

                // Option of "Exit"/"SessionLogout" (prompt) or "Restart" action (command)
                if (STREQ (ks.txt_fields[TYPE_TXT], "action") &&
                    streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", "Restart", NULL) &&
                    !gtk_tree_model_iter_has_child (ks.model, &ks.iter)) {
                    gboolean restart = STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Restart");
                    create_cm_headline (context_menu, (restart) ? " Restart option" :
                                        (STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Exit") ?
                                        " Exit option" : " SessionLogout option"));
                    menu_item = gtk_menu_item_new_with_label ((restart) ? "Add command" : "Add prompt");
                    g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (add_new), (restart) ? "command" : "prompt");
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);

                    not_all_options_of_selected_action_set = TRUE;
                }
            }

            // Actions
            if (streq_any (ks.txt_fields[TYPE_TXT], "item", "action", NULL)) {
                guint8 actions_cnt;

                if (STREQ (ks.txt_fields[TYPE_TXT], "item") || not_all_options_of_selected_action_set) {
                    create_cm_headline (context_menu, " Actions");
                }
                for (actions_cnt = 0; actions_cnt < NUMBER_OF_ACTIONS; actions_cnt++) {
                    menu_item_txt = g_strconcat ("Add ", ks.actions[actions_cnt], NULL);
                    menu_item = gtk_menu_item_new_with_label (menu_item_txt);
                    // Cleanup
                    g_free (menu_item_txt);

                    g_signal_connect_swapped (menu_item, "activate",
                                              G_CALLBACK ((actions_cnt == RECONFIGURE) ?
                                              action_option_insert : generate_items_for_action_option_combo_box),
                                              (actions_cnt == RECONFIGURE) ? "context menu" : ks.actions[actions_cnt]);
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                }
                gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
            }
        }
    }

    // Basic menu elements

    /*
        menu, pipe menu, item & separator -> if (txt_fields[ELEMENT_VISIBILITY_TXT]) == TRUE
        If number of rows is > 1, then txt_fields[ELEMENT_VISIBILITY_TXT] == NULL.
    */
    if (!path || ks.txt_fields[ELEMENT_VISIBILITY_TXT]) {
        gchar *basic_menu_elements[] = { "menu", "pipe menu", "item", "separator" };
        gchar *items[] = { "item+Execute", "item+Exit", "item+Reconfigure", "item+Restart", "item+SessionLogout" };
        enum { MENU, PIPE_MENU, ITEM, SEPARATOR };
        const guint8 NUMBER_OF_BASIC_MENU_ELEMENTS = G_N_ELEMENTS (basic_menu_elements);

        GtkWidget *item_submenu = gtk_menu_new ();
        GtkWidget *submenu_item;

        guint8 basic_menu_elements_cnt, submenu_elements_cnt;

        if (!path) {
            gtk_tree_selection_unselect_all (selection);
            create_cm_headline (context_menu, " Add at toplevel");
        }
        for (basic_menu_elements_cnt = 0;
             basic_menu_elements_cnt < NUMBER_OF_BASIC_MENU_ELEMENTS;
             basic_menu_elements_cnt++) {
            menu_item_txt = g_strconcat ((path) ? "Add " : "", basic_menu_elements[basic_menu_elements_cnt], NULL);
            menu_item = gtk_menu_item_new_with_label (menu_item_txt);
            // Cleanup
            g_free (menu_item_txt);

            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);

            if (basic_menu_elements_cnt != ITEM) {
                g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (add_new),
                                            basic_menu_elements[basic_menu_elements_cnt]);
            }
            else {
                for (submenu_elements_cnt = 0; submenu_elements_cnt < NUMBER_OF_ACTIONS; submenu_elements_cnt++) {
                    menu_item_txt = g_strconcat ("+ ", ks.actions[submenu_elements_cnt], NULL);
                    submenu_item = gtk_menu_item_new_with_label (menu_item_txt);
                    // Cleanup
                    g_free (menu_item_txt);

                    gtk_menu_shell_append (GTK_MENU_SHELL (item_submenu), submenu_item);

                    g_signal_connect_swapped (submenu_item, "activate", G_CALLBACK (add_new), items[submenu_elements_cnt]);
                }

                submenu_item = gtk_menu_item_new_with_label ("no action");
                gtk_menu_shell_append (GTK_MENU_SHELL (item_submenu), submenu_item);
                g_signal_connect_swapped (submenu_item, "activate", G_CALLBACK (add_new), "item w/o action");

                gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), item_submenu);
            }
        }
        if (path) {
            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
        }
    }

    if (path) {
        // Defaults
        gboolean invalid_row_for_change_of_element_visibility_exists = FALSE;
        gboolean at_least_one_descendant_is_invisible = FALSE;
        gboolean at_least_one_selected_row_has_no_children = FALSE;
        gboolean at_least_one_selected_row_is_expanded = FALSE;

        gboolean expansion_statuses_of_subnodes[NUMBER_OF_EXPANSION_STATUSES] = { FALSE };

        GtkTreeModel *filter_model;

        GtkTreePath *path_loop;

        gchar *element_visibility_txt_loop;

        // Visualisation of invisible menus, pipe menus, items and separators.
        for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
            path_loop = selected_rows_loop->data;
            gtk_tree_model_get_iter (ks.model, &iter_loop, path_loop);
            gtk_tree_model_get (ks.model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);
            if (G_LIKELY (!element_visibility_txt_loop || STREQ (element_visibility_txt_loop, "visible"))) {
                invalid_row_for_change_of_element_visibility_exists = TRUE;

                // Cleanup
                g_free (element_visibility_txt_loop);

                break;
            }
            else if (gtk_tree_model_iter_has_child (ks.model, &iter_loop)) {
                filter_model = gtk_tree_model_filter_new (ks.model, path_loop);
                gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_if_invisible_descendant_exists,
                                        &at_least_one_descendant_is_invisible);

                // Cleanup
                g_object_unref (filter_model);
            }

            // Cleanup
            g_free (element_visibility_txt_loop);
        }

        if (G_UNLIKELY (!invalid_row_for_change_of_element_visibility_exists)) {
            enum { VISUALISE, VISUALISE_CM_RECURSIVELY };

            guint8 visualisation_cnt;

            for (visualisation_cnt = VISUALISE; visualisation_cnt <= VISUALISE_CM_RECURSIVELY; visualisation_cnt++) {
                if (!(visualisation_cnt == VISUALISE_CM_RECURSIVELY && !at_least_one_descendant_is_invisible)) {
                    menu_item = gtk_menu_item_new_with_label ((visualisation_cnt == VISUALISE) ?
                                                              "Visualise" : "Visualise recursively");
                    g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (visualise_menus_items_and_separators),
                                              GUINT_TO_POINTER (visualisation_cnt)); // recursively == 1 == TRUE.
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                }
             }

            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
        }

        // Remove
        menu_item = gtk_menu_item_new_with_label ("Remove");
        gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
        g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (remove_rows), "context menu");

        for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
            path_loop = selected_rows_loop->data;
            gtk_tree_model_get_iter (ks.model, &iter_loop, path_loop);
            if (!gtk_tree_model_iter_has_child (ks.model, &iter_loop)) {
                at_least_one_selected_row_has_no_children = TRUE;
                break;
            }
            else {
                filter_model = gtk_tree_model_filter_new (ks.model, path_loop);

                gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_expansion_statuses_of_nodes,
                                        expansion_statuses_of_subnodes);

                // Cleanup
                g_object_unref (filter_model);

                if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (ks.treeview), path_loop)) {
                    at_least_one_selected_row_is_expanded = TRUE;
                }
            }
        }

        if (!at_least_one_selected_row_has_no_children) {
            guint8 exp_status_changes_cnt;

            // Remove all children
            menu_item = gtk_menu_item_new_with_label ("Remove all children");
            g_signal_connect (menu_item, "activate", G_CALLBACK (remove_all_children), NULL);
            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);

            gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());

            // Expand or collapse node(s)
            for (exp_status_changes_cnt = 0;
                 exp_status_changes_cnt < CM_NUMBER_OF_EXPANSION_STATUS_CHANGES;
                 exp_status_changes_cnt++) {
                if ((exp_status_changes_cnt == CM_RECURSIVELY && expansion_statuses_of_subnodes[AT_LEAST_ONE_IS_COLLAPSED]) ||
                    (exp_status_changes_cnt == CM_IMMEDIATE &&
                     (!at_least_one_selected_row_is_expanded ||
                     expansion_statuses_of_subnodes[AT_LEAST_ONE_IMD_CH_IS_EXP])) ||
                    (exp_status_changes_cnt == CM_COLLAPSE && at_least_one_selected_row_is_expanded)) {
                    menu_item_txt = g_strdup_printf ((exp_status_changes_cnt == CM_RECURSIVELY) ? "Expand row%s recursively" :
                                                     ((exp_status_changes_cnt == CM_IMMEDIATE) ? "Expand row%s (imd. ch. only)" :
                                                     "Collapse row%s"), (number_of_selected_rows == 1) ? "" : "s");
                    menu_item = gtk_menu_item_new_with_label (menu_item_txt);
                    // Cleanup
                    g_free (menu_item_txt);

                    g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (expand_or_collapse_selected_rows),
                    GUINT_TO_POINTER (exp_status_changes_cnt));
                    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
                }
            }
        }
    }

    // Cleanup
    gtk_tree_path_free (path);
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

    gtk_widget_show_all (context_menu);

#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer (GTK_MENU (context_menu), NULL);
#else
    /*
        Arguments two to five
        (parent_menu_shell, parent_menu_item, user defined positioning function and user data for that function)
        are not used.
    */
    gtk_menu_popup (GTK_MENU (context_menu), NULL, NULL, NULL, NULL,
                    (event) ? event->button : 0, gdk_event_get_time ((GdkEvent*) event));
#endif
}
