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
#include <string.h>

#include "definitions_and_enumerations.h"
#include "adding_and_deleting.h"

static void hide_or_deactivate_widgets (void);
void one_of_the_change_values_buttons_pressed (gchar *origin);
static GtkTreeIter add_new_execution (gchar *new_menu_element, gboolean same_level);
void add_new (gchar *new_menu_element_plus_opt_suppl);
void option_list_with_headlines (G_GNUC_UNUSED GtkCellLayout   *cell_layout, 
                                               GtkCellRenderer *action_option_combo_box_renderer, 
                                               GtkTreeModel    *action_option_combo_box_model, 
                                               GtkTreeIter     *action_option_combo_box_iter, 
                                 G_GNUC_UNUSED gpointer         data);
static guint8 generate_action_option_combo_box_items_for_Exe_and_snotify_opts (gchar *options_type);
void generate_items_for_action_option_combo_box (gchar *preset_choice);
void show_action_options (void);
void show_or_hide_startupnotify_options (void);
void single_field_entry (void);
void hide_action_option_grid (gchar *origin);
static void clear_entries (void);
void action_option_insert (gchar *origin);
static gboolean check_for_menus (              GtkTreeModel *filter_model, 
                                 G_GNUC_UNUSED GtkTreePath  *filter_path, 
                                               GtkTreeIter  *filter_iter);
void remove_menu_id (gchar *menu_id);
void remove_all_children (void);
void remove_rows (gchar *origin);

/*

    Deactivate or hide those widgets whose actions could interfere during the entry of new values.

*/

static void hide_or_deactivate_widgets (void)
{
    guint8 tb_cnt;

    gtk_widget_set_sensitive (ks.mb_edit, FALSE);
    gtk_widget_set_sensitive (ks.mb_search, FALSE);
    gtk_widget_set_sensitive (ks.mb_options, FALSE);
    for (tb_cnt = TB_MOVE_UP; tb_cnt <= TB_FIND; tb_cnt++) {
        gtk_widget_set_sensitive ((GtkWidget *) ks.tb[tb_cnt], FALSE);
    }
    gtk_widget_hide (ks.button_grid);
    gtk_widget_hide (ks.find_grid);
}

/*

    Execute the actions that can be initiated from the "change values" input screen.

*/

void one_of_the_change_values_buttons_pressed (gchar *origin)
{
    GSList *change_values_user_settings_loop;
    // AD = adding and deleting, indicating that the enums are only used here
    enum { AD_FIELD, AD_VALUE };
    gchar **parameter;

    gboolean including_action_check_button_active = 
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.including_action_check_button));
    gchar *new_menu_element = NULL; // Predefinition avoids compiler warning.
    gboolean same_level = TRUE; // default

    for (change_values_user_settings_loop = ks.change_values_user_settings; 
        change_values_user_settings_loop; 
        change_values_user_settings_loop = change_values_user_settings_loop->next) {
        parameter = g_strsplit (change_values_user_settings_loop->data, ":", -1);
        if (STREQ (origin, "done") && STREQ (parameter[AD_FIELD], "new menu element")) {
            // "value" exists only temporary
            new_menu_element = g_strdup (parameter[AD_VALUE]);
        }
        else if (STREQ (origin, "reset")) {
            if (STREQ (parameter[AD_FIELD], "new menu element")) {
                gchar *default_label_txt = g_strdup_printf ("New %s", parameter[AD_VALUE]);
                gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), default_label_txt);
                // Cleanup
                g_free (default_label_txt);

                // To keep the code simple, the rest of this if-clause resets fields even if they are currently not visible.
                gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), 
                                                "mandatory_missing");
                gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[EXECUTE_ENTRY]), 
                                                "mandatory_missing");

                gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[ICON_PATH_ENTRY]), "");
                gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[EXECUTE_ENTRY]), "");
            }
            else if (STREQ (parameter[AD_FIELD], "menu ID field")) {
                gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[MENU_ID_ENTRY]), parameter[AD_VALUE]);
            }
            else if (STREQ (parameter[AD_FIELD], "inside menu")) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.inside_menu_check_button), STREQ (parameter[AD_VALUE], "true"));
            }
            else if (STREQ (parameter[AD_FIELD], "incl. action")) {
                including_action_check_button_active = (STREQ (parameter[AD_VALUE], "true"));
                // Prevent recursion
                g_signal_handler_block (ks.including_action_check_button, ks.handler_id_including_action_check_button);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.including_action_check_button), 
                                              including_action_check_button_active);
                g_signal_handler_unblock (ks.including_action_check_button, ks.handler_id_including_action_check_button);
                gtk_widget_set_visible (ks.action_option, including_action_check_button_active);
                gtk_widget_set_visible (ks.options_grid, including_action_check_button_active);
            }
            else { // action
                gtk_combo_box_set_active_id (GTK_COMBO_BOX (ks.action_option), parameter[AD_VALUE]);
                if (streq_any (parameter[AD_VALUE], "Execute", "Restart", NULL)) {
                    clear_entries ();
                }
                else if (streq_any (parameter[AD_VALUE], "Exit", "SessionLogout", NULL)) {
                    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]), TRUE);
                }
            }
        }
        else if (STREQ (origin, "incl. action") && STREQ (parameter[AD_FIELD], "action")) {
            gtk_combo_box_set_active_id (GTK_COMBO_BOX (ks.action_option), parameter[AD_VALUE]);
            if (!including_action_check_button_active) {
                clear_entries ();
            }
            else if (streq_any (parameter[AD_VALUE], "Exit", "SessionLogout", NULL)) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]), TRUE);
            }
            gtk_widget_set_visible (ks.action_option, including_action_check_button_active);
            gtk_widget_set_visible (ks.options_grid, including_action_check_button_active);
        }

        // Cleanup
        g_strfreev (parameter);
    }

    if (STREQ (origin, "done")) {
        gboolean add_action = gtk_widget_get_visible (ks.action_option);
        gchar *menu_id = NULL; // Predefinition avoids compiler warning.
        gboolean missing_entry_or_error = FALSE; // default

        // Missing label for new menu element.
        if (!(*((gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]))))) {
            wrong_or_missing (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], ks.menu_element_or_value_entry_css_provider);
            missing_entry_or_error = TRUE;
        }
        else {
            gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), 
                                            "mandatory_missing");
        }

        // "Execute" missing of new pipe menu.
        if (STREQ (new_menu_element, "pipe menu") && 
            !(*((gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[EXECUTE_ENTRY]))))) {
            wrong_or_missing (ks.entry_fields[EXECUTE_ENTRY], ks.execute_entry_css_provider);
            missing_entry_or_error = TRUE;
        }
        else {
            gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[EXECUTE_ENTRY]), 
                                            "mandatory_missing");
        }

        // Prevent empty command field for "Execute".
        if (add_action &&
            STREQ ((gchar *) gtk_combo_box_get_active_id (GTK_COMBO_BOX (ks.action_option)), "Execute") && 
            !(*((gchar *) gtk_entry_get_text (GTK_ENTRY (ks.options_fields[COMMAND]))))) {
            wrong_or_missing (ks.options_fields[COMMAND], ks.execute_options_css_providers[COMMAND]);
            missing_entry_or_error = TRUE;
        }
        else {
            gtk_style_context_remove_class (gtk_widget_get_style_context (ks.options_fields[COMMAND]), "mandatory_missing");
        }

        // Invalid icon file or path
        const gchar *icon_path = gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[ICON_PATH_ENTRY]));
        if (*icon_path && !set_icon (&ks.iter, (gchar *) icon_path, TRUE)) { // TRUE = display error message if error occurs.
            missing_entry_or_error = TRUE;
        }

        // Non-unique menu ID
        if (!STREQ (new_menu_element, "item")) { // menu or pipe menu
            menu_id = (gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[MENU_ID_ENTRY]));
            if (G_UNLIKELY (g_slist_find_custom (ks.menu_ids, menu_id, (GCompareFunc) strcmp))) {
                show_errmsg ("The chosen menu ID already exists. Please choose another one.");
                missing_entry_or_error = TRUE;
            }
        }

        if (missing_entry_or_error) {
            // Cleanup
            g_free (new_menu_element);

            return;
        }

        if (!STREQ (new_menu_element, "item")) {
            ks.menu_ids = g_slist_prepend (ks.menu_ids, g_strdup (menu_id));
        }

        if (gtk_widget_get_visible (ks.inside_menu_check_button)) {
            same_level = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.inside_menu_check_button));
        }

        GtkTreeIter new_menu_element_iter;
        ks.iter = new_menu_element_iter = add_new_execution (new_menu_element, same_level);

        if (add_action) {
            repopulate_txt_fields_array (); // This is usually called by row_selected () and needed by the following function.
            action_option_insert ("by combo box");
        }

        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
        // This triggers row_selected, resulting in a complete reset.
        gtk_tree_selection_select_iter (selection, &new_menu_element_iter);

        // Cleanup
        g_free (new_menu_element);
    }
}

/*

    Adds a new menu element.

*/

static GtkTreeIter add_new_execution (gchar *new_menu_element, gboolean same_level)
{
    gpointer new_ts_fields[NUMBER_OF_TS_ELEMENTS] = { NULL }; // Defaults

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));

    GtkTreePath *path = NULL; // Default
    GtkTreeIter new_iter;

    gboolean path_is_on_toplevel = TRUE; // Default

    guint8 ts_fields_cnt;

    if (streq_any (new_menu_element, "menu", "pipe menu", "item", NULL)) {
        new_ts_fields[TS_TYPE] = new_menu_element;
        new_ts_fields[TS_MENU_ELEMENT] = (gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]));

        if (*(gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[ICON_PATH_ENTRY])))) {
            GdkPixbuf *icon_in_original_size;

            new_ts_fields[TS_ICON_PATH] = (gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[ICON_PATH_ENTRY]));
            icon_in_original_size = gdk_pixbuf_new_from_file (new_ts_fields[TS_ICON_PATH], NULL);
            new_ts_fields[TS_ICON_IMG] = gdk_pixbuf_scale_simple (icon_in_original_size, 
                                                                  ks.font_size + 10, ks.font_size + 10, 
                                                                  GDK_INTERP_BILINEAR);
            new_ts_fields[TS_ICON_MODIFICATION_TIME] = get_modification_time_for_icon (new_ts_fields[TS_ICON_PATH]);

            // Cleanup
            g_object_unref (icon_in_original_size);
        }
        if (streq_any (new_menu_element, "menu", "pipe menu", NULL)) {
            new_ts_fields[TS_MENU_ID] = (gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[MENU_ID_ENTRY]));
            if (STREQ (new_menu_element, "pipe menu") && *(gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[EXECUTE_ENTRY])))) {
                new_ts_fields[TS_EXECUTE] = (gchar *) gtk_entry_get_text (GTK_ENTRY (ks.entry_fields[EXECUTE_ENTRY]));
            }
        }
    }
    else if (STREQ (new_menu_element, "separator")) {
        new_ts_fields[TS_TYPE] = "separator";
    }
    else {
        new_ts_fields[TS_TYPE] = "option";
        new_ts_fields[TS_MENU_ELEMENT] = new_menu_element;

        // --- Assigning predefinied values for options. ---

        if (STREQ (new_menu_element, "enabled") || 
            streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL)) { // new_menu_element == "prompt"
            new_ts_fields[TS_VALUE] = "yes";
        }
    }


    if (gtk_tree_selection_count_selected_rows (selection) && 
        gtk_tree_path_get_depth (path = gtk_tree_model_get_path (ks.model, &ks.iter)) > 1) {
        path_is_on_toplevel = FALSE;
    }

    // Adding options inside an action or an option block.
    if (STREQ (ks.txt_fields[TYPE_TXT], "action") || 
        (STREQ (ks.txt_fields[TYPE_TXT], "option block") && !streq_any (new_menu_element, "prompt", "command", NULL))) {
        same_level = FALSE;
    }


    // --- Add the new row. ---


    // Prevents that the default check for change of selection(s) gets in the way.
    g_signal_handler_block (selection, ks.handler_id_row_selected);

    gtk_tree_selection_unselect_all (selection); // The old selection will be replaced by the one of the new row.

    if (!path || !same_level) {
        gtk_tree_store_append (ks.treestore, &new_iter, (!path) ? NULL : &ks.iter);

        if (path) {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (ks.treeview), path, FALSE); // FALSE == just expand immediate children.
        }
    }
    else {
        GtkTreeIter parent;

        if (!path_is_on_toplevel) {
            gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
        }
        gtk_tree_store_insert_after (ks.treestore, &new_iter, (path_is_on_toplevel) ? NULL : &parent, &ks.iter);
    }

    // Set element visibility of menus, pipe menus, items and separators.
    if (g_regex_match_simple ("menu|pipe menu|item.*|separator", new_menu_element, 0, 0)) {
        new_ts_fields[TS_ELEMENT_VISIBILITY] = "visible"; // Default
        /*
            path == NULL means that the new menu element has been added at toplevel;
            such a menu element is always visible after its addition, since it can't have an invisible ancestor.
        */
        if (path) {
            GtkTreePath *new_path = gtk_tree_model_get_path (ks.model, &new_iter);
            guint8 invisible_ancestor;

            if ((invisible_ancestor = check_if_invisible_ancestor_exists (ks.model, new_path))) { // Parentheses avoid gcc warning.
                new_ts_fields[TS_ELEMENT_VISIBILITY] = (invisible_ancestor == INVISIBLE_ANCESTOR) ?
                "invisible dsct. of invisible menu" : "invisible dsct. of invisible orphaned menu";       
            }
            // Cleanup
            gtk_tree_path_free (new_path);
        }
    }

    for (ts_fields_cnt = 0; ts_fields_cnt < NUMBER_OF_TS_ELEMENTS; ts_fields_cnt++) {
        gtk_tree_store_set (ks.treestore, &new_iter, ts_fields_cnt, new_ts_fields[ts_fields_cnt], -1);
    }
 
    // Cleanup
    gtk_tree_path_free (path);
    if (new_ts_fields[TS_ICON_IMG]) {
        g_object_unref (new_ts_fields[TS_ICON_IMG]);
    }
    g_free (new_ts_fields[TS_ICON_MODIFICATION_TIME]);

    if (!gtk_widget_get_visible (ks.change_values_label)) {
        gtk_tree_selection_select_iter (selection, &new_iter);
    }

    /*
        In case that autosorting is activated:
        If the type of the new row is...
        ...a prompt or command option of Execute, ... OR:
        ...an option of startupnotify, ...
        ...sort all options of the parent row and move the selection to the new option.
    */
     if (ks.autosort_options && STREQ (new_ts_fields[TS_TYPE], "option")) {
        GtkTreeIter parent;

        gtk_tree_model_iter_parent (ks.model, &parent, &new_iter);
        if (gtk_tree_model_iter_n_children (ks.model, &parent) > 1) {
            sort_execute_or_startupnotify_options_after_insertion (selection, &parent, 
                                                                   (streq_any (new_menu_element, "prompt", "command", NULL)) ? 
                                                                   "Execute" : "startupnotify", 
                                                                   new_menu_element);
        }
    }

    g_signal_handler_unblock (selection, ks.handler_id_row_selected);

    activate_change_done ();

    return new_iter;
}

/* 

    Preperations for the addition of a new menu element and display of the "change values" screen.

*/

void add_new (gchar *new_menu_element_plus_opt_suppl)
{
    gboolean same_level = TRUE; // Default
    gchar *new_menu_element = (!g_regex_match_simple ("item.*", new_menu_element_plus_opt_suppl, 0, 0)) ? 
                              new_menu_element_plus_opt_suppl : "item";

    /*
        If the selection currently points to a menu a decision has to be made to 
        either put the new element inside that menu or next to it on the same level.
    */
    if (STREQ (ks.txt_fields[TYPE_TXT], "menu")) {
        GtkWidget *dialog;
        gchar *label_txt;

        gint result;

        enum { INSIDE_MENU = 2, CANCEL };

        label_txt = g_strdup_printf ("Insert new %s on the same level or into the currently selected menu?", 
                                     new_menu_element);

        create_dialog (&dialog, "Same level or inside menu?", "dialog-question", label_txt, 
                       "_Same level", "_Inside menu", "_Cancel", TRUE); // TRUE == dialog is shown immediately.

        // Cleanup
        g_free (label_txt);

        result = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        switch (result) {
            case INSIDE_MENU:
                same_level = FALSE;
                break;
            case CANCEL:
            case GTK_RESPONSE_DELETE_EVENT:
                return;
        }
    }

    if (!streq_any (new_menu_element, "menu", "pipe menu", "item", NULL)) {
        add_new_execution (new_menu_element, same_level);
        row_selected ();
    }
    else {
        gchar *field_value_pair;

        gchar *change_values_markup, *default_label_txt;

        gboolean menu_or_pipe_menu = (!STREQ (new_menu_element, "item"));

        guint8 entry_cnt;

        gtk_box_reorder_child (GTK_BOX (ks.main_box), ks.entry_grid, 0);

        if (STREQ (ks.txt_fields[TYPE_TXT], "menu")) {
            gtk_widget_show (ks.action_option_grid);
            gtk_widget_hide (ks.options_grid);
            gtk_widget_show (ks.inside_menu_label);
            gtk_widget_show (ks.inside_menu_check_button);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.inside_menu_check_button), !same_level);
            field_value_pair = g_strdup_printf ("inside menu:%s", (same_level) ? "false" : "true");
            ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, field_value_pair);
        }
        if (STREQ (new_menu_element, "item")) {
            GtkTreeIter action_option_combo_box_iter;

            gboolean incl_action;

            guint8 action_cnt;


            gtk_widget_show (ks.including_action_label);
            gtk_widget_show (ks.including_action_check_button);
            gtk_widget_show (ks.action_option_grid);
            incl_action = !STREQ (new_menu_element_plus_opt_suppl, "item w/o action");
            // Prevent callback
            g_signal_handler_block (ks.including_action_check_button, ks.handler_id_including_action_check_button);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.including_action_check_button), incl_action);
            g_signal_handler_unblock (ks.including_action_check_button, ks.handler_id_including_action_check_button);
            field_value_pair = g_strdup_printf ("incl. action:%s", (incl_action) ? "true" : "false");
            ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, field_value_pair);

            gtk_list_store_clear (ks.action_option_combo_box_liststore);
            for (action_cnt = 0; action_cnt < NUMBER_OF_ACTIONS; action_cnt++) {
                gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
                                                   ACTION_OPTION_COMBO_ITEM, ks.actions[action_cnt], -1);
            }

            if (g_regex_match_simple ("item\\+.*", new_menu_element_plus_opt_suppl, 0, 0)) {
                gchar *action = extract_substring_via_regex (new_menu_element_plus_opt_suppl, "(?<=\\+).*");
                gtk_combo_box_set_active_id (GTK_COMBO_BOX (ks.action_option), action);
                field_value_pair = g_strdup_printf ("action:%s", action);
                // Cleanup
                g_free (action);
                ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, field_value_pair);
            }
            else { // "item" or "item w/o action". The action_option combo box is set just in case the user changes his/her mind.
                ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, g_strdup ("action:Execute"));
                gtk_combo_box_set_active_id (GTK_COMBO_BOX (ks.action_option), "Execute");
                if (!STREQ (new_menu_element_plus_opt_suppl, "item")) {
                    gtk_widget_hide (ks.action_option);
                }
            }
            // Has to be after a possible setting of the action_option combo box, because this triggers showing the grid.
            gtk_widget_set_visible (ks.options_grid, incl_action);
        }
        else { // menu or pipe menu
            gtk_widget_hide (ks.action_option);
            gtk_widget_hide (ks.options_grid);
        }

        hide_or_deactivate_widgets ();

        change_values_markup = g_strdup_printf ("<span font='%i'>Change values for new %s?</span>", 
                                                ks.font_size + 2, new_menu_element);
        gtk_label_set_markup (GTK_LABEL (ks.change_values_label), change_values_markup);
        // Cleanup
        g_free (change_values_markup);
        // Check for change of font size, so that the label's font size can be adjusted in that case.
        if (!ks.rows_with_icons) {
            g_timeout_add (1000, (GSourceFunc) check_for_external_file_and_settings_changes, "timeout");
        }

        gtk_widget_show (ks.change_values_label);
        gtk_widget_hide (ks.action_option_cancel);
        gtk_widget_hide (ks.action_option_done);
        gtk_widget_show (ks.mandatory);
        if (!STREQ (new_menu_element, "item") && !STREQ (ks.txt_fields[TYPE_TXT], "menu")) {
            gtk_widget_set_margin_top (ks.mandatory, 5);
        }
        gtk_widget_show (ks.separator);
        gtk_widget_show (ks.change_values_buttons_grid);
        for (entry_cnt = 0; entry_cnt < NUMBER_OF_ENTRY_FIELDS; entry_cnt++) {
            g_signal_handler_disconnect (ks.entry_fields[entry_cnt], ks.handler_id_entry_fields[entry_cnt]);
        }
        gtk_label_set_markup (GTK_LABEL (ks.entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY]), 
                                         " Label (<span foreground='darkred'>*</span>): ");
        gtk_label_set_markup (GTK_LABEL (ks.entry_labels[EXECUTE_ENTRY]), 
                                         " Execute (<span foreground='darkred'>*</span>): ");
        gtk_widget_show (ks.icon_chooser);
        gtk_widget_hide (ks.remove_icon);
        gtk_widget_set_visible (ks.entry_labels[ICON_PATH_ENTRY], TRUE);
        gtk_widget_set_visible (ks.entry_fields[ICON_PATH_ENTRY], TRUE);
        // In case that currently a (pipe) menu or item with an incorrect iconpath is selected.
        gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[ICON_PATH_ENTRY]), "mandatory_missing");
        gtk_widget_set_visible (ks.entry_labels[MENU_ID_ENTRY], menu_or_pipe_menu);
        gtk_widget_set_visible (ks.entry_fields[MENU_ID_ENTRY], menu_or_pipe_menu);
        gtk_widget_set_visible (ks.entry_labels[EXECUTE_ENTRY], STREQ (new_menu_element, "pipe menu"));
        gtk_widget_set_visible (ks.entry_fields[EXECUTE_ENTRY], STREQ (new_menu_element, "pipe menu"));

        field_value_pair = g_strdup_printf ("new menu element:%s", new_menu_element);
        ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, field_value_pair);
        default_label_txt = g_strconcat ("New ", new_menu_element, NULL);
        gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), default_label_txt);
        // In case that currently a (pipe) menu or item without label is selected.
        gtk_widget_set_sensitive (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], TRUE);
        // Cleanup
        g_free (default_label_txt);

        gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[ICON_PATH_ENTRY]), "");

        if (menu_or_pipe_menu) {
            gchar *menu_ID;
            guint menu_id_index = 1;

            // Menu IDs have to be unique, so the list of menu IDs has to be checked for existing values.
            while (g_slist_find_custom (ks.menu_ids, menu_ID = g_strdup_printf ("New menu %i", menu_id_index++), 
                                                                                (GCompareFunc) strcmp)) {
                g_free (menu_ID);
            }

            field_value_pair = g_strdup_printf ("menu ID field:%s", menu_ID);
            ks.change_values_user_settings = g_slist_prepend (ks.change_values_user_settings, field_value_pair);

            gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[MENU_ID_ENTRY]), menu_ID);
            gtk_entry_set_text (GTK_ENTRY (ks.entry_fields[EXECUTE_ENTRY]), "");

            // Cleanup
            g_free (menu_ID);
        }

        gtk_widget_grab_focus (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]);

        gtk_widget_show (ks.entry_grid);
    }
}

/* 

    Sets up a cell layout that includes headlines that are highlighted in colour.

*/

void option_list_with_headlines (G_GNUC_UNUSED GtkCellLayout   *cell_layout, 
                                               GtkCellRenderer *action_option_combo_box_renderer, 
                                               GtkTreeModel    *action_option_combo_box_model, 
                                               GtkTreeIter     *action_option_combo_box_iter, 
                                 G_GNUC_UNUSED gpointer         data)
{
    gchar *action_option_combo_item;

    gboolean headline;

    gtk_tree_model_get (action_option_combo_box_model, action_option_combo_box_iter, 
                        ACTION_OPTION_COMBO_ITEM, &action_option_combo_item, 
                        -1);

    headline = g_regex_match_simple ("Add|Choose", action_option_combo_item, G_REGEX_ANCHORED, 0);

    g_object_set (action_option_combo_box_renderer, 
                  "foreground-rgba", (headline) ? &((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }) : NULL, 
                  "background-rgba", (g_str_has_prefix (action_option_combo_item, "Choose")) ? 
                                     &((GdkRGBA) { 0.31, 0.31, 0.79, 1.0 }) : 
                                     ((g_str_has_prefix (action_option_combo_item, "Add")) ? 
                                     &((GdkRGBA) { 0.0, 0.0, 0.0, 1.0 }) : NULL),
                  "sensitive", !headline, 
                  NULL);

    // Cleanup
    g_free (action_option_combo_item);
}

/* 

    Appends possible options for Execute or startupnotify to combobox.

*/

static guint8 generate_action_option_combo_box_items_for_Exe_and_snotify_opts (gchar *options_type)
{
    gboolean execute = STREQ (options_type, "Execute");
    GtkTreeIter parent, action_option_combo_box_new_iter;
    const guint8 number_of_opts = (execute) ? NUMBER_OF_EXECUTE_OPTS : NUMBER_OF_STARTUPNOTIFY_OPTS;
    const guint allocated_size = sizeof (gboolean) * number_of_opts;
    gboolean *opts_exist = g_slice_alloc0 (allocated_size); // Initialise all elements to FALSE.

    guint8 number_of_added_opts = 0; // Start value
    guint8 opts_cnt;

    if (STREQ (ks.txt_fields[MENU_ELEMENT_TXT], options_type)) { // "Execute" or "startupnotify"
        parent = ks.iter;
    }
    else {
        gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
    }

    check_for_existing_options (&parent, number_of_opts, (execute) ? ks.execute_options : ks.startupnotify_options, opts_exist);

    for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++) {
        if (!opts_exist[opts_cnt]) {
            gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_new_iter, -1, 
                                               ACTION_OPTION_COMBO_ITEM, (execute) ? 
                                               ks.execute_displayed_txts[opts_cnt] : ks.startupnotify_displayed_txts[opts_cnt],
                                               -1);
            number_of_added_opts++;
        }
    }

    // Cleanup
    g_slice_free1 (allocated_size, opts_exist);

    return number_of_added_opts;
}

/* 

    Shows the action combo box, to which a dynamically generated list of options has been added.

*/

void generate_items_for_action_option_combo_box (gchar *preset_choice)
{
    GtkTreeIter action_option_combo_box_iter;

    const gchar *bt_add_action_option_label_txt;

    gchar *add_inside_txt = NULL; // Predefinition avoids compiler warning.

    guint8 number_of_added_actions = 0, number_of_added_action_opts = 0, number_of_added_snotify_opts = 0; // Defaults

    gtk_widget_set_sensitive (ks.action_option, TRUE);


    // --- Build combo list. ---


    // Prevents that the signal connected to the combo box gets in the way.
    g_signal_handler_block (ks.action_option, ks.handler_id_action_option_combo_box);

    gtk_list_store_clear (ks.action_option_combo_box_liststore);

    // Generate "Choose..." headline according to the text of bt_add_action_option_label.
    bt_add_action_option_label_txt = gtk_label_get_text (GTK_LABEL (ks.bt_add_action_option_label));
    gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
                                       ACTION_OPTION_COMBO_ITEM,
                                       (STREQ (bt_add_action_option_label_txt, "Action")) ? "Choose an action" : 
                                       (STREQ (bt_add_action_option_label_txt, "Option")) ? 
                                       "Choose an option" : "Choose an action/option", 
                                       -1);

    // Startupnotify options: enabled, name, wmclass, icon
    if (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL) && 
        streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "startupnotify", "enabled", "name", "wmclass", "icon", NULL)) {
        number_of_added_snotify_opts = generate_action_option_combo_box_items_for_Exe_and_snotify_opts ("startupnotify");
    }

    // Execute options: prompt, command and startupnotify
    if ((STREQ (ks.txt_fields[TYPE_TXT], "action") && STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Execute")) ||
        (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL) && 
        streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "prompt", "command", "startupnotify", NULL))) {
        number_of_added_action_opts = generate_action_option_combo_box_items_for_Exe_and_snotify_opts ("Execute");
    }

    // Exit and SessionLogout option: prompt, Restart option: command
    if (STREQ (ks.txt_fields[TYPE_TXT], "action") && 
        streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "Exit", "Restart", "SessionLogout", NULL) && 
        !gtk_tree_model_iter_has_child (ks.model, &ks.iter)) {
        gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
                                           ACTION_OPTION_COMBO_ITEM, 
                                           (STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Restart")) ? "Command" : "Prompt", 
                                           -1);
        number_of_added_action_opts = 1;
    }

    // Actions
    if (streq_any (ks.txt_fields[TYPE_TXT], "item", "action", NULL)) { 
        for (; number_of_added_actions < NUMBER_OF_ACTIONS; number_of_added_actions++) {
            gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
                                               ACTION_OPTION_COMBO_ITEM, ks.actions[number_of_added_actions], 
                                              -1);
        }
    }

    /*
        Check if there are two kinds of addable actions/options: the first one inside the current selection, 
        the second one next to the current selection. If so, add headlines to the list to separate them from each other.
    */
    if (number_of_added_snotify_opts && number_of_added_action_opts) {
        add_inside_txt = "Add inside startupnotify";
    }
    else if (number_of_added_action_opts && number_of_added_actions) {
        add_inside_txt = g_strdup_printf ("Add inside currently selected %s action", ks.txt_fields[MENU_ELEMENT_TXT]);
    }

    if (add_inside_txt) {
        gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, 1, 
                                           ACTION_OPTION_COMBO_ITEM, add_inside_txt, 
                                           -1);

         /*
            Position of "Add additional action" headline:
            "Add inside currently selected xxx action" + number of added action options + "Add additional action" == 
            1 + number_of_added_action_opts + 1 -> number_of_added_action_opts + 2 
            Position of "Add next to startupnotify" headline:
            "Add inside startupnotify" + number of added startupnotify options + "Add next to startupnotify" == 
            1 + number_of_added_snotify_opts + 1 -> number_of_added_snotify_opts + 2
        */
        gtk_list_store_insert_with_values (ks.action_option_combo_box_liststore, &action_option_combo_box_iter, 
                                           ((number_of_added_actions) ? 
                                           number_of_added_action_opts : number_of_added_snotify_opts) + 2, 
                                           ACTION_OPTION_COMBO_ITEM, (number_of_added_actions) ? 
                                           "Add additional action" : "Add next to startupnotify", 
                                           -1);

        // Cleanup
        if (number_of_added_actions) {
            g_free (add_inside_txt);
        }
    }

    // Show the action combo box and deactivate or hide all events and widgets that could interfere.
    hide_or_deactivate_widgets ();

    gtk_widget_show (ks.action_option_grid);

    gtk_combo_box_set_active (GTK_COMBO_BOX (ks.action_option), 0);
    gtk_widget_hide (ks.action_option_done);
    gtk_widget_hide (ks.options_grid);

    g_signal_handler_unblock (ks.action_option, ks.handler_id_action_option_combo_box);

    gtk_widget_queue_draw (GTK_WIDGET (ks.treeview)); // Force redrawing of treeview (if a search was active).

    /*
        If this function was called by the context menu or a "Startupnotify" button, 
        preselect the appropriate combo box item.
    */
    if (preset_choice) {
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (ks.action_option), preset_choice);
        // If there is only one choice, there is nothing to choose from the combo box.
        if (gtk_tree_model_iter_n_children (ks.action_option_combo_box_model, NULL) == 2) {
            gtk_widget_set_sensitive (ks.action_option, FALSE);
        }
    }

    if (!gtk_widget_get_visible (ks.change_values_label)) {
        gtk_widget_set_margin_top (ks.new_action_option_grid, 0);
    }
}

/* 

    Shows the fields that may be changed according to the chosen action/option.

*/

void show_action_options (void)
{
    const gchar *combo_choice = gtk_combo_box_get_active_id (GTK_COMBO_BOX (ks.action_option));
    gboolean with_new_item = gtk_widget_get_visible (ks.including_action_check_button);

    guint8 options_cnt, snotify_opts_cnt;

    clear_entries ();

    // Defaults
    gtk_widget_set_margin_bottom (ks.action_option_grid, 5);
    gtk_widget_set_visible (ks.options_grid, !STREQ (combo_choice, "Reconfigure"));
    for (options_cnt = 0; options_cnt < NUMBER_OF_EXECUTE_OPTS; options_cnt++) {
        gtk_widget_hide (ks.options_labels[options_cnt]);
        gtk_widget_hide (ks.options_fields[options_cnt]);
    }
    gtk_widget_hide (ks.suboptions_grid);
    for (snotify_opts_cnt = 0; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
        gtk_widget_show (ks.suboptions_labels[snotify_opts_cnt]);
        gtk_widget_show (ks.suboptions_fields[snotify_opts_cnt]);
    }
    gtk_label_set_text (GTK_LABEL (ks.options_labels[PROMPT]), " Prompt: ");
    if (!gtk_widget_get_visible (ks.including_action_label)) {
        gtk_widget_hide (ks.mandatory);
    }
    gtk_widget_set_margin_bottom (ks.mandatory, 0);
    gtk_widget_grab_focus (ks.options_fields[PROMPT]);

    // "Execute" action
    if (STREQ (combo_choice, "Execute")) {
        for (options_cnt = 0; options_cnt < NUMBER_OF_EXECUTE_OPTS; options_cnt++) {
            gtk_label_set_markup (GTK_LABEL (ks.options_labels[options_cnt]), (options_cnt != COMMAND) ? 
                                                                              ks.options_label_txts[options_cnt] : 
                                                                              " Command (<span foreground='darkred'>*</span>): ");
            gtk_widget_show (ks.options_labels[options_cnt]);
            gtk_widget_show (ks.options_fields[options_cnt]);
        }
        gtk_widget_show (ks.mandatory);
        if (!gtk_widget_get_visible (ks.including_action_label)) {
            gtk_widget_set_margin_bottom (ks.mandatory, 5);
        }
        if (!g_signal_handler_is_connected (ks.options_fields[SN_OR_PROMPT], ks.handler_id_show_or_hide_startupnotify_options)) {
            ks.handler_id_show_or_hide_startupnotify_options = g_signal_connect (ks.options_fields[SN_OR_PROMPT], "toggled", 
                                                                                 G_CALLBACK (show_or_hide_startupnotify_options), NULL);
        }
    }

    // "Restart" action or "command" option (the latter for "Restart" and "Execute" actions)
    if (streq_any (combo_choice, "Restart", "Command", NULL)) {
        gtk_label_set_markup (GTK_LABEL (ks.options_labels[COMMAND]), (STREQ (combo_choice, "Restart")) ? 
                                                                      " Command: " : 
                                                                      " Command (<span foreground='darkred'>*</span>): ");
        gtk_widget_show (ks.options_labels[COMMAND]);
        gtk_widget_show (ks.options_fields[COMMAND]);
        if (STREQ (combo_choice, "Command")) { 
            gtk_widget_show (ks.mandatory);
            gtk_widget_set_margin_bottom (ks.mandatory, 5);
        }
        gtk_widget_grab_focus (ks.options_fields[COMMAND]);
    }

    // "Exit" or "SessionLogout" action or "prompt" option (the latter for "Execute", "Exit" and "SessionLogout" actions)
    if (streq_any (combo_choice, "Exit", "Prompt", "SessionLogout", NULL)) {
         if (!STREQ (combo_choice, "Prompt") || 
            (STREQ (combo_choice, "Prompt") && streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL))) {
            gtk_label_set_text (GTK_LABEL (ks.options_labels[SN_OR_PROMPT]), " Prompt: ");
            gtk_widget_show (ks.options_labels[SN_OR_PROMPT]);
            if (g_signal_handler_is_connected (ks.options_fields[SN_OR_PROMPT], ks.handler_id_show_or_hide_startupnotify_options)) {
                g_signal_handler_disconnect (ks.options_fields[SN_OR_PROMPT], ks.handler_id_show_or_hide_startupnotify_options);
            }
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]), TRUE);
            gtk_widget_show (ks.options_fields[SN_OR_PROMPT]);
            gtk_widget_grab_focus (ks.options_fields[SN_OR_PROMPT]);
        }
        else { // Option of "Execute" action.
            gtk_widget_show (ks.options_labels[PROMPT]);
            gtk_label_set_markup (GTK_LABEL (ks.options_labels[PROMPT]), " Prompt (<span foreground='darkred'>*</span>): ");
            gtk_widget_show (ks.options_fields[PROMPT]);
            gtk_widget_show (ks.mandatory);
            gtk_widget_set_margin_bottom (ks.mandatory, 5);
        }
    }

    // "startupnotify" option and its suboptions
    if (streq_any (combo_choice, "Startupnotify", "Enabled", "Name", "WM_CLASS", "Icon", NULL)) {
        gtk_widget_show (ks.suboptions_grid);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start (ks.suboptions_grid, 5);
#else
        gtk_widget_set_margin_left (ks.suboptions_grid, 5);
#endif
        if (STREQ (combo_choice, "Startupnotify")) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.suboptions_fields[ENABLED]), TRUE);
            gtk_widget_grab_focus (ks.suboptions_fields[ENABLED]);
        }
        else { // Enabled, Name, WM_CLASS & Icon
            for (snotify_opts_cnt = 0; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
                if (!STREQ (combo_choice, ks.startupnotify_displayed_txts[snotify_opts_cnt])) {
                    gtk_widget_hide (ks.suboptions_labels[snotify_opts_cnt]);
                    gtk_widget_hide (ks.suboptions_fields[snotify_opts_cnt]);
                }
                else {
                    gtk_widget_grab_focus (ks.suboptions_fields[snotify_opts_cnt]);
                }
            }
        }
    }

    if (!with_new_item) {
        gtk_widget_show (ks.action_option_done);
    }
}

/* 

    Shows the options for startupnotify after the corresponding check box has been clicked.

*/

void show_or_hide_startupnotify_options (void)
{
    gboolean show_startupnotify_options = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]));

    gtk_widget_set_visible (ks.suboptions_grid, show_startupnotify_options);
    if (!show_startupnotify_options) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.suboptions_fields[ENABLED]), TRUE);
        gtk_entry_set_text (GTK_ENTRY (ks.suboptions_fields[NAME]), "");
        gtk_entry_set_text (GTK_ENTRY (ks.suboptions_fields[WM_CLASS]), "");
        gtk_entry_set_text (GTK_ENTRY (ks.suboptions_fields[ICON]), "");
    }
}

/* 

    If only a single entry field is shown, "Enter" is enough, clicking on "Done" is not necessary.

*/

void single_field_entry (void)
{
    /*
        Applies to 
        - the _single_ "Execute" options "command" and "prompt"
        - the _single_ "startupnotify" options "name", "wmclass" and "icon"
        - the "Restart" action and its option "command".

        If action fields are shown in conjunctin if item fields, single field entry is not executed.
    */
    if (!gtk_widget_get_visible (ks.including_action_check_button) && 
        !streq_any (gtk_combo_box_get_active_id (GTK_COMBO_BOX (ks.action_option)), "Execute", "Startupnotify", NULL)) {
        action_option_insert ("by combo box");
    }
}

/* 

   Resets everything after the cancel button has been clicked or an insertion has been done.

*/

void hide_action_option_grid (gchar *origin)
{
    clear_entries ();
    gtk_widget_set_sensitive (ks.mb_options, TRUE);
    gtk_widget_hide (ks.action_option_grid);
    gtk_widget_set_margin_bottom (ks.action_option_grid, 0);
    gtk_widget_set_margin_top (ks.new_action_option_grid, 5);
    gtk_widget_show (ks.button_grid);
    if (*ks.search_term->str) {
        gtk_widget_show (ks.find_grid);
    }
    gtk_widget_hide (ks.mandatory);
    gtk_widget_set_margin_bottom (ks.mandatory, 0);

    if (STREQ (origin, "cancel button")) {
        row_selected (); // Reset visibility of menu bar and toolbar items.
    }
}

/*

    Clears all entries for actions/options.

*/

static void clear_entries (void)
{
    guint8 options_cnt, snotify_opts_cnt;

    for (options_cnt = PROMPT; options_cnt <= COMMAND; options_cnt++) {
        gtk_entry_set_text (GTK_ENTRY (ks.options_fields[options_cnt]), "");
        gtk_style_context_remove_class (gtk_widget_get_style_context (ks.options_fields[options_cnt]), "mandatory_missing");
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.suboptions_fields[ENABLED]), TRUE);
    for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
        gtk_entry_set_text (GTK_ENTRY (ks.suboptions_fields[snotify_opts_cnt]), "");
        gtk_style_context_remove_class (gtk_widget_get_style_context (ks.suboptions_fields[snotify_opts_cnt]), 
                                        "mandatory_missing");
    }
}

/* 

    Inserts an action and/or its option(s) with the entered values.

*/

void action_option_insert (gchar *origin)
{
    const gchar *choice = (STREQ (origin, "by combo box")) ? 
                           gtk_combo_box_get_active_id (GTK_COMBO_BOX (ks.action_option)) : "Reconfigure";
    gboolean options_check_button_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.options_fields[SN_OR_PROMPT]));
    gboolean enabled_check_button_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.suboptions_fields[ENABLED]));
    const gchar *options_entries[] = { gtk_entry_get_text (GTK_ENTRY (ks.options_fields[PROMPT])), 
                                       gtk_entry_get_text (GTK_ENTRY (ks.options_fields[COMMAND])) };
    const gchar *suboption_entry;

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    GtkTreeIter new_iter, new_iter2, parent, execute_parent, execute_iter;

    gint insertion_position = -1; // Default

    // Defaults
    gboolean action = FALSE;
    gboolean execute_done = FALSE;
    gboolean startupnotify_done = FALSE;
    gchar *option_of_execute = NULL, *option_of_startupnotify = NULL;

    // Defaults
    gboolean used_Execute_opts[NUMBER_OF_EXECUTE_OPTS] = { FALSE };
    guint8 snotify_start = NAME;

    guint8 execute_opts_cnt, snotify_opts_cnt;


    // If only one entry or a mandatory field was displayed, check if it has been filled out.
    if (gtk_widget_get_visible (ks.action_option_grid)) {
        if (streq_any (choice, "Execute", "Command", NULL) && !(*options_entries[COMMAND])) {
            wrong_or_missing (ks.options_fields[COMMAND], ks.execute_options_css_providers[COMMAND]);
            return;
        }
        else if (STREQ (choice, "Prompt") && // "Execute" action or option of it currently selected.
                 gtk_widget_get_visible (ks.options_fields[PROMPT]) && !(*options_entries[PROMPT])) {
            wrong_or_missing (ks.options_fields[PROMPT], ks.execute_options_css_providers[PROMPT]);
            return;
        }
        else {
             for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
                if (STREQ (choice, ks.startupnotify_displayed_txts[snotify_opts_cnt]) && 
                    !(*gtk_entry_get_text (GTK_ENTRY (ks.suboptions_fields[snotify_opts_cnt])))) {
                    // "Enabled" is not part of the css_providers, thus - 1.
                    wrong_or_missing (ks.suboptions_fields[snotify_opts_cnt], ks.suboptions_fields_css_providers[snotify_opts_cnt - 1]);
                    return;
                }
            }
        }
    }

    gtk_widget_hide (ks.mandatory);
    gtk_widget_set_margin_bottom (ks.mandatory, 0);

    /*
        Check if the insertion will be done at the current level and before the last child of the current parent. 
        If so, set the insertion position.
    */
    if (!(STREQ (ks.txt_fields[TYPE_TXT], "item") || 
          (STREQ (ks.txt_fields[TYPE_TXT], "action") && 
           streq_any (choice, "Prompt", "Command", "Startupnotify", NULL)) || 
          (STREQ (ks.txt_fields[TYPE_TXT], "option block") && 
           !streq_any (choice, "Prompt", "Command", NULL)))) {
        GtkTreePath *path = gtk_tree_model_get_path (ks.model, &ks.iter);
        gint last_path_index = gtk_tree_path_get_indices (path)[gtk_tree_path_get_depth (path) - 1];

        gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
        if (gtk_tree_model_iter_n_children (ks.model, &parent) > 1 && 
            last_path_index < gtk_tree_model_iter_n_children (ks.model, &parent) - 1) {
            insertion_position = last_path_index + 1;
        }

        // Cleanup
        gtk_tree_path_free (path);
    }


    // --- Create the new row(s). ---


    // Prevents that the default check for change of selection(s) gets in the way.
    g_signal_handler_block (selection, ks.handler_id_row_selected);
    gtk_tree_selection_unselect_all (selection); // The old selection will be replaced by the one of the new row.

    // Execute
    if (STREQ (choice, "Execute")) {
        if (STREQ (ks.txt_fields[TYPE_TXT], "action")) {
            gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
            ks.iter = parent; // Move up one level and start from there.
        }

        gtk_tree_store_insert_with_values (ks.treestore, &new_iter, &ks.iter, insertion_position,
                                           TS_MENU_ELEMENT, "Execute",
                                           TS_TYPE, "action",
                                           -1);

        execute_parent = ks.iter;
        execute_iter = new_iter;
        ks.iter = new_iter; // New base level

        action = TRUE;
        execute_done = TRUE;
        insertion_position = -1; // Reset for the options, since they should be always appended.
    }

    // Prompt - only if it is an Execute option and not empty.
    if ((execute_done || STREQ (choice, "Prompt")) && *options_entries[PROMPT]) {
        used_Execute_opts[PROMPT] = TRUE;
    }

    // Command - only if it is an Execute option.
    if ((execute_done || 
        (STREQ (choice, "Command") && !STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "Restart")))) {
        used_Execute_opts[COMMAND] = TRUE;
    }

    // Startupnotify
    if ((execute_done && options_check_button_state) || STREQ (choice, "Startupnotify")) {
        used_Execute_opts[STARTUPNOTIFY] = TRUE;
    }

    // Insert Execute option, if used.
    for (execute_opts_cnt = 0; execute_opts_cnt < NUMBER_OF_EXECUTE_OPTS; execute_opts_cnt++) {
        if (used_Execute_opts[execute_opts_cnt]) {
            if (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL)) {
                gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
                ks.iter = parent; // Move up one level and start from there.
            }
            gtk_tree_store_insert_with_values (ks.treestore, &new_iter, &ks.iter, insertion_position,
                                               TS_MENU_ELEMENT, ks.execute_options[execute_opts_cnt], 
                                               TS_TYPE, (execute_opts_cnt != STARTUPNOTIFY) ? "option" : "option block",
                                               TS_VALUE, (execute_opts_cnt != STARTUPNOTIFY) ? 
                                                          options_entries[execute_opts_cnt] : NULL, 
                                               -1);

            if (!execute_done) { // Single option, not via an insertion of an "Execute" action.
                option_of_execute = ks.execute_options[execute_opts_cnt];
                expand_row_from_iter (&ks.iter);
                gtk_tree_selection_select_iter (selection, &new_iter);
                if (execute_opts_cnt == STARTUPNOTIFY) {
                    startupnotify_done = TRUE;
                }
            }
        }
    }

    // Setting parent iterator for startupnotify options, if they are added as single elements.
    if (streq_any (ks.txt_fields[TYPE_TXT], "option", "option block", NULL) && 
        !streq_any (ks.txt_fields[MENU_ELEMENT_TXT], "prompt", "command", NULL)) {
        if (STREQ (ks.txt_fields[MENU_ELEMENT_TXT], "startupnotify")) {
            new_iter = ks.iter;
        }
        else {
            gtk_tree_model_iter_parent (ks.model, &new_iter, &ks.iter);
        }
    }

    // Enabled
    if ((execute_done && options_check_button_state) || streq_any (choice, "Startupnotify", "Enabled", NULL)) {
        snotify_start = ENABLED;
    }

    // Insert startupnotify option, if used.
    for (snotify_opts_cnt = snotify_start; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
        if (snotify_opts_cnt == ENABLED || 
            *(suboption_entry = gtk_entry_get_text (GTK_ENTRY (ks.suboptions_fields[snotify_opts_cnt])))) {
            gtk_tree_store_insert_with_values (ks.treestore, &new_iter2, &new_iter, insertion_position,
                                               TS_MENU_ELEMENT, ks.startupnotify_options[snotify_opts_cnt],
                                               TS_TYPE, "option", 
                                               TS_VALUE, (snotify_opts_cnt == ENABLED) ? 
                                                         ((enabled_check_button_state) ? "yes" : "no") : 
                                                          suboption_entry,
                                              -1);

            expand_row_from_iter (&new_iter);
            if (!execute_done && !startupnotify_done) { // Single option.
                option_of_startupnotify = ks.startupnotify_options[snotify_opts_cnt];
                gtk_tree_selection_select_iter (selection, &new_iter2);    
            }
        }
    }

    // Action other than Execute
    if (streq_any (choice, "Exit", "Reconfigure", "Restart", "SessionLogout", NULL)) {
        if (STREQ (ks.txt_fields[TYPE_TXT], "action")) {
            gtk_tree_model_iter_parent (ks.model, &parent, &ks.iter);
            ks.iter = parent; // Move up one level and start from there.
        }

        gtk_tree_store_insert_with_values (ks.treestore, &new_iter, &ks.iter, insertion_position,
                                           TS_MENU_ELEMENT, choice,
                                           TS_TYPE, "action",
                                           -1);

        if (!STREQ (choice, "Reconfigure") && !(STREQ (choice, "Restart") && !(*options_entries[COMMAND]))) {
            gtk_tree_store_insert_with_values (ks.treestore, &new_iter2, &new_iter, -1, 
                                               TS_MENU_ELEMENT, (STREQ (choice, "Restart")) ? "command" : "prompt", 
                                               TS_TYPE, "option",
                                               TS_VALUE, (STREQ (choice, "Restart")) ? options_entries[COMMAND] : 
                                                          ((options_check_button_state) ? "yes" : "no"),
                                               -1);
        }

        action = TRUE;
    }

    // Exit & SessionLogout option (prompt) or Restart option (command)
    if ((STREQ (choice, "Prompt") && gtk_widget_get_visible (ks.options_fields[SN_OR_PROMPT])) || 
        (STREQ (choice, "Command") && STREQ (ks.txt_fields [MENU_ELEMENT_TXT], "Restart"))) {
        gtk_tree_store_insert_with_values (ks.treestore, &new_iter, &ks.iter, -1, 
                                           TS_MENU_ELEMENT, (STREQ (choice, "Prompt")) ? "prompt" : "command", 
                                           TS_TYPE, "option",
                                           TS_VALUE, (STREQ (choice, "Command")) ? options_entries[COMMAND] : 
                                                      ((options_check_button_state) ? "yes" : "no"),
                                           -1);

        expand_row_from_iter (&ks.iter);
        gtk_tree_selection_select_iter (selection, &new_iter);
    }

    /*
        Show all children of the new action. If the parent row had not been expanded, expand it is well, 
        but leave all preceding nodes (if any) of the new action collapsed.
    */
    if (action) {
        if (execute_done) {
            ks.iter = execute_parent;
        }
        GtkTreePath *parent_path = gtk_tree_model_get_path (ks.model, &ks.iter);
        GtkTreePath *path = gtk_tree_model_get_path (ks.model, (execute_done) ? &execute_iter : &new_iter);

        if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (ks.treeview), parent_path)) {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (ks.treeview), parent_path, FALSE);
        }
        // TRUE == expand recursively, this is for a new Execute action with startupnotify options, so the latter are shown.
        gtk_tree_view_expand_row (GTK_TREE_VIEW (ks.treeview), path, TRUE);
        if (!gtk_widget_get_visible (ks.change_values_label)) {
            gtk_tree_selection_select_path (selection, path);
        }

        // Cleanup
        gtk_tree_path_free (parent_path);
        gtk_tree_path_free (path);
    }

    if (ks.autosort_options) {
        // If the new row is an option of Execute, sort all options of the latter and move the selection to the new option.
        if (option_of_execute && gtk_tree_model_iter_n_children (ks.model, &ks.iter) > 1) {
             sort_execute_or_startupnotify_options_after_insertion (selection, &ks.iter, "Execute", option_of_execute);
        }

        // The same for startupnotify. new_iter always points to an "option block" == startupnotify.
        if (option_of_startupnotify && gtk_tree_model_iter_n_children (ks.model, &new_iter) > 1) {
            sort_execute_or_startupnotify_options_after_insertion (selection, &new_iter, 
                                                                   "startupnotify", option_of_startupnotify);
        }
    }

    hide_action_option_grid ("action option insert");

    g_signal_handler_unblock (selection, ks.handler_id_row_selected);
    activate_change_done ();
    if (!gtk_widget_get_visible (ks.change_values_label)) {
        row_selected ();
    }
}

/* 

    Checks for menus or pipe menus that are descendants of the currently processed row.
    If found, their menu ID is passed to a function that removes this ID from the list of menu IDs.

*/

static gboolean check_for_menus (              GtkTreeModel *filter_model, 
                                 G_GNUC_UNUSED GtkTreePath  *filter_path, 
                                               GtkTreeIter  *filter_iter)
{
    gchar *type_txt_filter, *menu_id_txt_filter;

    gtk_tree_model_get (filter_model, filter_iter, TS_TYPE, &type_txt_filter, -1);

    if (streq_any (type_txt_filter, "menu", "pipe menu", NULL)) {
        gtk_tree_model_get (filter_model, filter_iter, TS_MENU_ID, &menu_id_txt_filter, -1);

        remove_menu_id (menu_id_txt_filter);

        // Cleanup
        g_free (menu_id_txt_filter);
    }

    // Cleanup
    g_free (type_txt_filter);

    return FALSE;
}

/* 

    Removes a menu ID from the list of menu IDs.

*/

void remove_menu_id (gchar *menu_id)
{
    GSList *to_be_removed_element = g_slist_find_custom (ks.menu_ids, menu_id, (GCompareFunc) strcmp);

    g_free (to_be_removed_element->data);
    ks.menu_ids = g_slist_remove (ks.menu_ids, to_be_removed_element->data);
}

/* 

    Removes all children of a node.

    Children are removed from bottom to top so the paths of the other children and nodes are kept.
    After the removal of the children the function checks for the existence of each parent
    before it reselects them, because if subnodes of the selected nodes had also been selected,
    they can't be reselected again since they got removed.

*/

void remove_all_children (void)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    /*
        The list of rows is reverted, since if a top to bottom direction was kept and 
        children of selected nodes was also selected, these children would be selected for removal first, 
        but since their own children are also intended for removal, they are unselected again.

        Example:

        Selection done by user:

        -menu ::: selected for removal of children
            -menu
            -menu ::: selected for removal of child
                -menu
            -menu

        First selection loop run:

        -menu 1. element whose children are selected
            -menu ::: selected
            -menu ::: selected
                -menu
            -menu ::: selected

        Second selection loop run:

        -menu
            -menu ::: selected
            -menu -> 2. element whose children are selected, unintended unselection
                -menu ::: selected
            -menu ::: selected
    */
    GList *selected_rows = g_list_reverse (gtk_tree_selection_get_selected_rows (selection, &ks.model));

    GList *selected_rows_loop;
    GtkTreePath *path_loop;
    GtkTreeIter iter_loop;

    // Prevents that the default check for change of selection(s) gets in the way.
    g_signal_handler_block (selection, ks.handler_id_row_selected); 

    for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
        path_loop = selected_rows_loop->data;
        // (Note: Rows are not selected if they are not visible.)
        gtk_tree_view_expand_row (GTK_TREE_VIEW (ks.treeview), path_loop, FALSE);
        gtk_tree_selection_unselect_path (selection, path_loop);
        gtk_tree_path_down (path_loop);
        gtk_tree_model_get_iter (ks.model, &iter_loop, path_loop);
        do {
            gtk_tree_selection_select_iter (selection, &iter_loop);
        } while (gtk_tree_model_iter_next (ks.model, &iter_loop));
        gtk_tree_path_up (path_loop);
    }

    remove_rows ("rm. all chldn.");

    g_signal_handler_unblock (selection, ks.handler_id_row_selected);

    for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
        // Might be a former subnode that was selected and got removed.
        if (gtk_tree_model_get_iter (ks.model, &iter_loop, selected_rows_loop->data)) {
            gtk_tree_selection_select_iter (selection, &iter_loop);
        }
    }

    // Cleanup
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

/* 

    Removes all currently selected rows.

*/

void remove_rows (gchar *origin)
{
    GtkTreeModel *filter_model;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    // The list of rows is reverted, since only with a bottom to top direction the paths will stay the same.
    GList *selected_rows = g_list_reverse (gtk_tree_selection_get_selected_rows (selection, &ks.model));

    GList *selected_rows_loop;
    GtkTreePath *path_loop;
    GtkTreeIter iter_loop;

    gchar *type_txt_loop, *menu_id_txt_loop;

    // Prevents that the default check for change of selection(s) gets in the way.
    g_signal_handler_block (selection, ks.handler_id_row_selected);

    for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
        path_loop = selected_rows_loop->data;
        gtk_tree_model_get_iter (ks.model, &iter_loop, path_loop);
        
        if (!STREQ (origin, "dnd")) { // A drag & drop just moves rows; there are no new, deleted or changed menu IDs in this case.
            gtk_tree_model_get (ks.model, &iter_loop, TS_TYPE, &type_txt_loop, -1);

            if (streq_any (type_txt_loop, "menu", "pipe menu", NULL)) { 
                // Keep menu IDs in the GSList equal to the menu IDs of the treestore.
                gtk_tree_model_get (ks.model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
                remove_menu_id (menu_id_txt_loop);

                /*
                    If the to be deleted row is a menu that contains other menus as children, 
                    their menu IDs have to be deleted, too.
                */
                if (gtk_tree_model_iter_has_child (ks.model, &iter_loop)) { // Has to be a menu, because pipe menus don't have children.
                    filter_model = gtk_tree_model_filter_new (ks.model, path_loop);
                    gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_for_menus, NULL);

                    // Cleanup
                    g_object_unref (filter_model);
                }
                // Cleanup
                g_free (menu_id_txt_loop);
            }
            // Cleanup
            g_free (type_txt_loop);
        }

        gtk_tree_store_remove (GTK_TREE_STORE (ks.model), &iter_loop);
    }

    // If all rows have been deleted and the search functionality had been activated before, deactivate the latter.
    if (!gtk_tree_model_get_iter_first (ks.model, &iter_loop) && gtk_widget_get_visible (ks.find_grid)) {
        show_or_hide_find_grid ();
    }

    g_signal_handler_unblock (selection, ks.handler_id_row_selected);

    // Cleanup
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

    if (!streq_any (origin, "dnd", "load menu", NULL)) { // There might be further changes during Drag & Drop / loading a menu.
        activate_change_done ();
        row_selected ();
    }
}
