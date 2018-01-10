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

#include <stdlib.h>
#include <string.h>

#include "definitions_and_enumerations.h"
#include "kickshaw.h"

ks_data ks = {
    .options_label_txts = { " Prompt (optional): ", " Command: ", " Startupnotify " }, 
    .actions = { "Execute", "Exit", "Reconfigure", "Restart", "SessionLogout" }, 
    .execute_options = { "prompt", "command", "startupnotify" }, 
    .startupnotify_options = { "enabled", "name", "wmclass", "icon" }, 
    
    .execute_displayed_txts = { "Prompt", "Command", "Startupnotify" }, 
    .startupnotify_displayed_txts = { "Enabled", "Name", "WM_CLASS", "Icon" },

    .column_header_txts = { "Menu Element", "Type", "Value", "Menu ID", "Execute", "Element Visibility" }, 

    .enable_list = {{ "STRING", GTK_TARGET_SAME_WIDGET, 0 }}
};

static void general_initialisiation (void);
static void add_button_content (GtkWidget *button, gchar *label_text);
gboolean selection_block_unblock (G_GNUC_UNUSED GtkTreeSelection *selection, 
                                  G_GNUC_UNUSED GtkTreeModel     *model,
                                  G_GNUC_UNUSED GtkTreePath      *path, 
                                  G_GNUC_UNUSED gboolean          path_currently_selected, 
                                  gpointer                        block_state);
static gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event);
static gboolean mouse_released (void);
static void set_column_attributes (G_GNUC_UNUSED GtkTreeViewColumn *cell_column, 
                                                 GtkCellRenderer   *txt_renderer,
                                                 GtkTreeModel      *cell_model, 
                                                 GtkTreeIter       *cell_iter, 
                                                 gpointer           column_number_pointer);
static void change_view_and_options (gpointer activated_menu_item_pointer);
static void expand_or_collapse_all (gpointer expand_pointer);
void set_status_of_expand_and_collapse_buttons_and_menu_items (void);
static void about (void);
gboolean continue_despite_unsaved_changes (void);
void clear_global_data (void);
static void new_menu (void);
static void quit_program (void);
void repopulate_txt_fields_array (void);
void activate_change_done (void);
static void write_settings (void);
void set_filename_and_window_title (gchar *new_filename);
void show_errmsg (gchar *errmsg_raw_txt);
void show_msg_in_statusbar (gchar *message);

int main (G_GNUC_UNUSED int argc, char *argv[])
{
    // --- Startup checks --- 


    // ### Display program version. ###

    if (G_UNLIKELY (STREQ (argv[1], "--version"))) {
        g_print ("Kickshaw " KICKSHAW_VERSION
"\nCopyright (C) 2010-2018    Marcus Schaetzle\
\n\nKickshaw comes with ABSOLUTELY NO WARRANTY.\
\nYou may redistribute copies of Kickshaw\
\nunder the terms of the GNU General Public License.\
\nFor more information about these matters, see the file named COPYING.\n");
        exit (EXIT_SUCCESS);
    }

    // ### Check if a windowing system is running. ###

    if (G_UNLIKELY (!g_getenv ("DISPLAY"))) {
        g_printerr ("No windowing system currently running, program aborted.\n");
        exit (EXIT_FAILURE);
    }


    // --- Initialise GTK, create the GUI and set up everything else. ---

    ks.app = gtk_application_new ("savannah.nongnu.org.kickshaw", G_APPLICATION_FLAGS_NONE);
    gint status;

    g_signal_connect_swapped (ks.app, "activate", G_CALLBACK (general_initialisiation), NULL);

    status = g_application_run (G_APPLICATION (ks.app), 0, NULL);

    g_object_unref (ks.app);

    return status;
}

/* 

    Creates GUI and signals, also loads settings and standard menu file, if they exist.

*/

static void general_initialisiation (void)
{
    GList *windows = gtk_application_get_windows (ks.app);

    if (G_UNLIKELY (windows)) {
        gtk_window_present (GTK_WINDOW (windows->data));

        return;
    }

    GtkWidget *main_grid;

    GtkWidget *menubar;
    GSList *element_visibility_menu_item_group = NULL, *grid_menu_item_group = NULL;
    GtkWidget *mb_file, *mb_find, *mb_view, *mb_help,
              *mb_show_element_visibility_column, *mb_show_grid, *mb_about;
    enum { FILE_MENU, EDIT_MENU, SEARCH_MENU, VIEW_MENU, OPTIONS_MENU, HELP_MENU, 
           SHOW_ELEMENT_VISIBILITY_COLUMN_MENU, SHOW_GRID_MENU, NUMBER_OF_SUBMENUS };
    GtkWidget *submenus[NUMBER_OF_SUBMENUS];
    GtkMenuShell *view_submenu;

    GtkAccelGroup *accel_group = NULL;
    guint accel_key;
    GdkModifierType accel_mod;

    gchar *file_menu_item_txts[] = { "_New", "_Open", "_Save", "Save As", "", "_Quit"};
    gchar *file_menu_item_accelerators[] = { "<Ctl>N", "<Ctl>O", "<Ctl>S", "<Shift><Ctl>S", "", "<Ctl>Q"};

    gchar *visibility_txts[] = { "Show Element Visibility column", "Keep highlighting", "Don't keep highlighting"};
    gchar *grid_txts[] = { "No grid lines", "Horizontal", "Vertical", "Both" };
    guint default_checked[] = { SHOW_MENU_ID_COL, SHOW_EXECUTE_COL, SHOW_ICONS, SET_OFF_SEPARATORS, SHOW_TREE_LINES };
    guint8 txts_cnt;

    GtkWidget *toolbar;
    gchar *button_IDs[] = { "document-new", "document-open", "document-save", "document-save-as", "go-up", 
                            "go-down", "list-remove", "edit-find", "zoom-in", "zoom-out", "application-exit"};
    gchar *tb_tooltips[] = { "New menu", "Open menu", "Save menu", "Save menu as...", "Move up", 
                             "Move down", "Remove", "Find", "Expand all", "Collapse all", "Quit" };
    GtkToolItem *tb_separator;

    gchar *suboptions_label_txt;

    // Label text of the last button is set dynamically dependent on the type of the selected row.
    gchar *bt_add_txts[] = { "Menu", "Pipe Menu", "Item", "Separator", "" };
    gchar *add_txts[] = { "menu", "pipe menu", "item", "separator" };

    GtkCssProvider *change_values_label_css_provider;
    GtkWidget *change_values_cancel, *change_values_reset, *change_values_done;

#if GTK_CHECK_VERSION(3,10,0)
    gchar *find_entry_buttons_imgs[] = { "window-close", "go-previous", "go-next" };
#else
    gchar *find_entry_buttons_imgs[] = { GTK_STOCK_CLOSE, GTK_STOCK_GO_BACK, GTK_STOCK_GO_FORWARD };
#endif
    gchar *find_entry_buttons_tooltips[] = { "Close", "Previous", "Next" };
    enum { ENTRY, COLUMNS_SELECTION, SPECIAL_OPTIONS, NUMBER_OF_FIND_SUBGRIDS };
    GtkWidget *find_subgrids[NUMBER_OF_FIND_SUBGRIDS];

    GtkWidget *scrolled_window;
    GtkTreeSelection *selection;

    GtkCellRenderer *action_option_combo_box_renderer;

    gchar *settings_file_path;

    gchar *new_filename;

    guint8 buttons_cnt, columns_cnt, entry_fields_cnt, grids_cnt, mb_menu_items_cnt, 
           options_cnt, snotify_opts_cnt, submenus_cnt, view_and_opts_cnt;


    // --- Creating the GUI. ---


    // ### Create a new window. ###
           
#if GTK_CHECK_VERSION(3,4,0)
    ks.window = gtk_application_window_new (GTK_APPLICATION (ks.app));
#else
    ks.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif
    
    gtk_window_set_default_size (GTK_WINDOW (ks.window), 650, 550);
    gtk_window_set_title (GTK_WINDOW (ks.window), "Kickshaw");
    gtk_window_set_position (GTK_WINDOW (ks.window), GTK_WIN_POS_CENTER);
#if GLIB_CHECK_VERSION(2,32,0)
    g_signal_connect_swapped (ks.window, "destroy", G_CALLBACK (g_application_quit), G_APPLICATION (ks.app));
#else
    g_signal_connect (ks.window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
#endif

    // ### Create a new grid that will contain all elements. ###

    main_grid = gtk_grid_new ();
    gtk_grid_set_column_homogeneous (GTK_GRID (main_grid), TRUE);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (main_grid), GTK_ORIENTATION_VERTICAL);
    gtk_container_add (GTK_CONTAINER (ks.window), main_grid);

    // ### Create a menu bar. ###

    menubar = gtk_menu_bar_new ();
    gtk_container_add (GTK_CONTAINER (main_grid), menubar);

    // Submenus
    for (submenus_cnt = 0; submenus_cnt < NUMBER_OF_SUBMENUS; submenus_cnt++) {
        submenus[submenus_cnt] = gtk_menu_new ();
    }

    // Accelerator Group
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (ks.window), accel_group);

    // File
    mb_file = gtk_menu_item_new_with_mnemonic ("_File");

    for (mb_menu_items_cnt = MB_NEW; mb_menu_items_cnt <= MB_QUIT; mb_menu_items_cnt++) {
        if (mb_menu_items_cnt != MB_SEPARATOR_FILE) {
            ks.mb_file_menu_items[mb_menu_items_cnt] = gtk_menu_item_new_with_mnemonic (file_menu_item_txts[mb_menu_items_cnt]);
            gtk_accelerator_parse (file_menu_item_accelerators[mb_menu_items_cnt], &accel_key, &accel_mod);
            gtk_widget_add_accelerator (ks.mb_file_menu_items[mb_menu_items_cnt], "activate", accel_group, 
                                        accel_key, accel_mod, GTK_ACCEL_VISIBLE);
        }
        else {
            ks.mb_file_menu_items[MB_SEPARATOR_FILE] = gtk_separator_menu_item_new ();
        }
    }

    for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_FILE_MENU_ITEMS; mb_menu_items_cnt++) {
        gtk_menu_shell_append (GTK_MENU_SHELL (submenus[FILE_MENU]), ks.mb_file_menu_items[mb_menu_items_cnt]);
    }
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_file), submenus[FILE_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_file);

    // Edit
    ks.mb_edit = gtk_menu_item_new_with_mnemonic ("_Edit");

    ks.mb_edit_menu_items[MB_MOVE_TOP] = gtk_menu_item_new_with_label ("Move to top");
    gtk_accelerator_parse ("<Ctl>T", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_edit_menu_items[MB_MOVE_TOP], "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_edit_menu_items[MB_MOVE_UP] = gtk_menu_item_new_with_label ("Move up");
    gtk_accelerator_parse ("<Ctl>minus", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_edit_menu_items[MB_MOVE_UP], "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_edit_menu_items[MB_MOVE_DOWN] = gtk_menu_item_new_with_label ("Move down");
    gtk_accelerator_parse ("<Ctl>plus", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_edit_menu_items[MB_MOVE_DOWN], "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_edit_menu_items[MB_MOVE_BOTTOM] = gtk_menu_item_new_with_label ("Move to bottom");
    gtk_accelerator_parse ("<Ctl>B", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_edit_menu_items[MB_MOVE_BOTTOM], "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_edit_menu_items[MB_REMOVE] = gtk_menu_item_new_with_label ("Remove");
    gtk_accelerator_parse ("<Ctl>R", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_edit_menu_items[MB_REMOVE], "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_edit_menu_items[MB_SEPARATOR_EDIT1] = gtk_separator_menu_item_new ();
    ks.mb_edit_menu_items[MB_REMOVE_ALL_CHILDREN] = gtk_menu_item_new_with_label ("Remove all children");
    ks.mb_edit_menu_items[MB_SEPARATOR_EDIT2] = gtk_separator_menu_item_new ();
    ks.mb_edit_menu_items[MB_VISUALISE] = gtk_menu_item_new_with_label ("Visualise");
    ks.mb_edit_menu_items[MB_VISUALISE_RECURSIVELY] = gtk_menu_item_new_with_label ("Visualise recursively");

    for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_EDIT_MENU_ITEMS; mb_menu_items_cnt++) {
        gtk_menu_shell_append (GTK_MENU_SHELL (submenus[EDIT_MENU]), ks.mb_edit_menu_items[mb_menu_items_cnt]);
    }
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (ks.mb_edit), submenus[EDIT_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), ks.mb_edit);

    // Search
    ks.mb_search = gtk_menu_item_new_with_mnemonic ("_Search");

    mb_find = gtk_menu_item_new_with_mnemonic ("_Find");
    gtk_accelerator_parse ("<Ctl>F", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (mb_find, "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);

    gtk_menu_shell_append (GTK_MENU_SHELL (submenus[SEARCH_MENU]), mb_find);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (ks.mb_search), submenus[SEARCH_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), ks.mb_search);

    // View
    mb_view = gtk_menu_item_new_with_mnemonic ("_View");

    ks.mb_expand_all_nodes = gtk_menu_item_new_with_label ("Expand all nodes");
    gtk_accelerator_parse ("<Ctl><Shift>X", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_expand_all_nodes, "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_collapse_all_nodes = gtk_menu_item_new_with_label ("Collapse all nodes");
    gtk_accelerator_parse ("<Ctl><Shift>C", &accel_key, &accel_mod);
    gtk_widget_add_accelerator (ks.mb_collapse_all_nodes, "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
    ks.mb_view_and_options[SHOW_MENU_ID_COL] = gtk_check_menu_item_new_with_label ("Show Menu ID column");
    ks.mb_view_and_options[SHOW_EXECUTE_COL] = gtk_check_menu_item_new_with_label ("Show Execute column");
    mb_show_element_visibility_column = gtk_menu_item_new_with_label ("Show Element Visibility column");
    ks.mb_view_and_options[SHOW_ICONS] = gtk_check_menu_item_new_with_label ("Show icons");
    ks.mb_view_and_options[SET_OFF_SEPARATORS] = gtk_check_menu_item_new_with_label ("Set off separators");
    ks.mb_view_and_options[DRAW_ROWS_IN_ALT_COLOURS] = 
        gtk_check_menu_item_new_with_label ("Draw rows in altern. colours (theme dep.)");
    ks.mb_view_and_options[SHOW_TREE_LINES] = gtk_check_menu_item_new_with_label ("Show tree lines"); 
    mb_show_grid = gtk_menu_item_new_with_label ("Show grid");

    for (mb_menu_items_cnt = SHOW_ELEMENT_VISIBILITY_COL_ACTVTD, txts_cnt = 0;
         mb_menu_items_cnt <= SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL; 
         mb_menu_items_cnt++, txts_cnt++) {
        if (mb_menu_items_cnt == SHOW_ELEMENT_VISIBILITY_COL_ACTVTD) {
            ks.mb_view_and_options[mb_menu_items_cnt] = gtk_check_menu_item_new_with_label (visibility_txts[txts_cnt]);
        }
        else {
            ks.mb_view_and_options[mb_menu_items_cnt] = gtk_radio_menu_item_new_with_label (element_visibility_menu_item_group, 
                                                                                            visibility_txts[txts_cnt]);
            element_visibility_menu_item_group =
                gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (ks.mb_view_and_options[mb_menu_items_cnt]));
        }
        gtk_menu_shell_append (GTK_MENU_SHELL (submenus[SHOW_ELEMENT_VISIBILITY_COLUMN_MENU]), 
                               ks.mb_view_and_options[mb_menu_items_cnt]);
    }
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_show_element_visibility_column), 
        submenus[SHOW_ELEMENT_VISIBILITY_COLUMN_MENU]);

    for (mb_menu_items_cnt = NO_GRID_LINES, txts_cnt = 0;
         mb_menu_items_cnt <= BOTH;
         mb_menu_items_cnt++, txts_cnt++) {
        ks.mb_view_and_options[mb_menu_items_cnt] = gtk_radio_menu_item_new_with_label (grid_menu_item_group, 
                                                    grid_txts[txts_cnt]);
        grid_menu_item_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (ks.mb_view_and_options[mb_menu_items_cnt]));
        gtk_menu_shell_append (GTK_MENU_SHELL (submenus[SHOW_GRID_MENU]), ks.mb_view_and_options[mb_menu_items_cnt]);
    }
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_show_grid), submenus[SHOW_GRID_MENU]);

    view_submenu = GTK_MENU_SHELL (submenus[VIEW_MENU]);
    gtk_menu_shell_append (view_submenu, ks.mb_expand_all_nodes);
    gtk_menu_shell_append (view_submenu, ks.mb_collapse_all_nodes);
    gtk_menu_shell_append (view_submenu, gtk_separator_menu_item_new ());
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[SHOW_MENU_ID_COL]);
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[SHOW_EXECUTE_COL]);
    gtk_menu_shell_append (view_submenu, mb_show_element_visibility_column);
    gtk_menu_shell_append (view_submenu, gtk_separator_menu_item_new ());
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[SHOW_ICONS]);
    gtk_menu_shell_append (view_submenu, gtk_separator_menu_item_new ());
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[SET_OFF_SEPARATORS]);
    gtk_menu_shell_append (view_submenu, gtk_separator_menu_item_new ());
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[DRAW_ROWS_IN_ALT_COLOURS]);
    gtk_menu_shell_append (view_submenu, ks.mb_view_and_options[SHOW_TREE_LINES]);
    gtk_menu_shell_append (view_submenu, mb_show_grid);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_view), submenus[VIEW_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_view);

    // Default settings
    for (mb_menu_items_cnt = 0; mb_menu_items_cnt < G_N_ELEMENTS (default_checked); mb_menu_items_cnt++) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[default_checked[mb_menu_items_cnt]]), TRUE);
    }
    gtk_widget_set_sensitive (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL], FALSE);
    gtk_widget_set_sensitive (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL], FALSE);

    // Options
    ks.mb_options = gtk_menu_item_new_with_mnemonic ("_Options");
    ks.mb_view_and_options[CREATE_BACKUP_BEFORE_OVERWRITING_MENU] = 
        gtk_check_menu_item_new_with_label ("Create backup before overwriting menu");
    ks.mb_view_and_options[SORT_EXECUTE_AND_STARTUPN_OPTIONS] = 
        gtk_check_menu_item_new_with_label ("Sort execute/startupnotify options");
    ks.mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS] = 
        gtk_check_menu_item_new_with_label ("Always notify about execute opt. conversions");

    gtk_menu_shell_append (GTK_MENU_SHELL (submenus[OPTIONS_MENU]), ks.mb_view_and_options[CREATE_BACKUP_BEFORE_OVERWRITING_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenus[OPTIONS_MENU]), ks.mb_view_and_options[SORT_EXECUTE_AND_STARTUPN_OPTIONS]);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenus[OPTIONS_MENU]), ks.mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (ks.mb_options), submenus[OPTIONS_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), ks.mb_options);

    // Default settings
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[CREATE_BACKUP_BEFORE_OVERWRITING_MENU]), TRUE);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]), TRUE);

    // Help
    mb_help = gtk_menu_item_new_with_mnemonic ("_Help");

    mb_about = gtk_menu_item_new_with_label ("About");

    gtk_menu_shell_append (GTK_MENU_SHELL (submenus[HELP_MENU]), mb_about);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_help), submenus[HELP_MENU]);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_help);

    // ### Create toolbar. ###

    toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_container_add (GTK_CONTAINER (main_grid), toolbar);

    gtk_container_set_border_width (GTK_CONTAINER (toolbar), 2);
    for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_TB_BUTTONS; buttons_cnt++) {
        if (buttons_cnt == TB_MOVE_UP || buttons_cnt == TB_FIND || buttons_cnt == TB_QUIT) {
            tb_separator = gtk_separator_tool_item_new ();
            gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tb_separator, -1);
        }

        ks.tb[buttons_cnt] = gtk_tool_button_new (NULL, NULL);
        gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (ks.tb[buttons_cnt]), button_IDs[buttons_cnt]);

        gtk_widget_set_tooltip_text (GTK_WIDGET (ks.tb[buttons_cnt]), tb_tooltips[buttons_cnt]);
        gtk_toolbar_insert (GTK_TOOLBAR (toolbar), ks.tb[buttons_cnt], -1);
    }

    // ### Create Button Bar. ###

    ks.button_grid = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (main_grid), ks.button_grid);

    ks.add_image = gtk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (ks.button_grid), ks.add_image);

    // Label text is set dynamically dependent on whether it is possible to add a row or not.
    ks.bt_bar_label = gtk_label_new ("");
    gtk_container_add (GTK_CONTAINER (ks.button_grid), ks.bt_bar_label);

     // Label text is set dynamically dependent on the type of the selected row.
    ks.bt_add_action_option_label = gtk_label_new ("");

    for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_ADD_BUTTONS; buttons_cnt++) {
        ks.bt_add[buttons_cnt] = gtk_button_new ();
        add_button_content (ks.bt_add[buttons_cnt], bt_add_txts[buttons_cnt]);
        gtk_container_add (GTK_CONTAINER (ks.button_grid), ks.bt_add[buttons_cnt]);
    }


    ks.change_values_label = gtk_label_new ("");
    gtk_container_add (GTK_CONTAINER (main_grid), ks.change_values_label);
    gtk_widget_set_margin_bottom (ks.change_values_label, 12);
    change_values_label_css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (ks.change_values_label), 
                                    GTK_STYLE_PROVIDER (change_values_label_css_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data (change_values_label_css_provider, 
                                     ".label { border-top-style:groove;"
                                     "         border-bottom-style:groove;" 
                                     "         border-width: 2px;"
                                     "         border-color:#a8a8a8; "
                                     "         padding:3px; }", 
                                     -1, NULL);


    ks.main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (main_grid), ks.main_box);


    // ### Grid for entering the values of new rows. ###

    ks.action_option_grid = gtk_grid_new ();
    gtk_orientable_set_orientation (GTK_ORIENTABLE (ks.action_option_grid), GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX (ks.main_box), ks.action_option_grid, FALSE, FALSE, 0);

    ks.new_action_option_grid = gtk_grid_new ();
    gtk_widget_set_margin_top (ks.new_action_option_grid, 5);
    gtk_widget_set_margin_bottom (ks.new_action_option_grid, 5);
    gtk_container_add (GTK_CONTAINER (ks.action_option_grid), ks.new_action_option_grid);

    ks.inside_menu_label = gtk_label_new (" Inside menu ");
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.inside_menu_label);
    ks.inside_menu_check_button = gtk_check_button_new ();
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.inside_menu_check_button);
    ks.including_action_label = gtk_label_new (" Incl. action ");
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.including_action_label);
    ks.including_action_check_button = gtk_check_button_new ();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.including_action_check_button), TRUE);
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.including_action_check_button);

    // Action/Option Combo Box
    ks.action_option_combo_box_liststore = gtk_list_store_new (NUMBER_OF_ACTION_OPTION_COMBO_ELEMENTS, G_TYPE_STRING);
    ks.action_option_combo_box_model = GTK_TREE_MODEL (ks.action_option_combo_box_liststore);

    ks.action_option = gtk_combo_box_new_with_model (ks.action_option_combo_box_model);
    action_option_combo_box_renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ks.action_option), action_option_combo_box_renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ks.action_option), action_option_combo_box_renderer, "text", 0, NULL);
    gtk_combo_box_set_id_column (GTK_COMBO_BOX (ks.action_option), 0);

    g_object_unref (ks.action_option_combo_box_liststore);

    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (ks.action_option), action_option_combo_box_renderer, 
                                        option_list_with_headlines, NULL, NULL); // No user data, no destroy notify for user data.*/
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.action_option);


    ks.action_option_done = gtk_button_new ();
    add_button_content (ks.action_option_done, "Done");
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.action_option_done);

    ks.action_option_cancel = gtk_button_new ();
    add_button_content (ks.action_option_cancel, "Cancel");
    gtk_container_add (GTK_CONTAINER (ks.new_action_option_grid), ks.action_option_cancel);

    // Execute options
    ks.options_grid = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (ks.action_option_grid), ks.options_grid);

    ks.options_fields[SN_OR_PROMPT] = gtk_check_button_new ();

    for (options_cnt = 0; options_cnt < NUMBER_OF_EXECUTE_OPTS; options_cnt++) {
        ks.options_labels[options_cnt] = gtk_label_new (ks.options_label_txts[options_cnt]);

        gtk_widget_set_halign (ks.options_labels[options_cnt], GTK_ALIGN_START);

        if (options_cnt < SN_OR_PROMPT) {
            ks.options_fields[options_cnt] = gtk_entry_new ();
            ks.execute_options_css_providers[options_cnt] = gtk_css_provider_new ();
            gtk_style_context_add_provider (gtk_widget_get_style_context (ks.options_fields[options_cnt]), 
                                            GTK_STYLE_PROVIDER (ks.execute_options_css_providers[options_cnt]), 
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            gtk_widget_set_hexpand (ks.options_fields[options_cnt], TRUE);
        }
        gtk_grid_attach (GTK_GRID (ks.options_grid), ks.options_labels[options_cnt], 0, options_cnt, 
                         (options_cnt < SN_OR_PROMPT) ? 2 : 1, 1);
        gtk_grid_attach (GTK_GRID (ks.options_grid), ks.options_fields[options_cnt], 
                         (options_cnt < SN_OR_PROMPT) ? 2 : 1, options_cnt, 1, 1);
    }

    ks.suboptions_grid = gtk_grid_new ();
    gtk_grid_attach (GTK_GRID (ks.options_grid), ks.suboptions_grid, 2, 2, 1, 1);

    for (snotify_opts_cnt = ENABLED; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
        suboptions_label_txt = g_strconcat (ks.startupnotify_displayed_txts[snotify_opts_cnt], ": ", NULL);
        ks.suboptions_labels[snotify_opts_cnt] = gtk_label_new (suboptions_label_txt);

        // Cleanup
        g_free (suboptions_label_txt);

        gtk_widget_set_halign (ks.suboptions_labels[snotify_opts_cnt], GTK_ALIGN_START);

        gtk_grid_attach (GTK_GRID (ks.suboptions_grid), ks.suboptions_labels[snotify_opts_cnt], 0, snotify_opts_cnt, 1, 1);

        ks.suboptions_fields[snotify_opts_cnt] = (snotify_opts_cnt == ENABLED) ? gtk_check_button_new () : gtk_entry_new ();
        if (snotify_opts_cnt > ENABLED) {
            ks.suboptions_fields_css_providers[snotify_opts_cnt - 1] = gtk_css_provider_new ();
            gtk_style_context_add_provider (gtk_widget_get_style_context (ks.suboptions_fields[snotify_opts_cnt]), 
                                            GTK_STYLE_PROVIDER (ks.suboptions_fields_css_providers[snotify_opts_cnt - 1]), 
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
        gtk_grid_attach (GTK_GRID (ks.suboptions_grid), ks.suboptions_fields[snotify_opts_cnt], 1, snotify_opts_cnt, 1, 1);

        gtk_widget_set_hexpand (ks.suboptions_fields[snotify_opts_cnt], TRUE);
    }

    ks.separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top (ks.separator, 10);
    gtk_widget_set_margin_bottom (ks.separator, 10);
    gtk_container_add (GTK_CONTAINER (ks.main_box), ks.separator);

    ks.change_values_buttons_grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (ks.change_values_buttons_grid), 10);
    gtk_widget_set_margin_bottom (ks.change_values_buttons_grid, 10);
    gtk_container_add (GTK_CONTAINER (ks.main_box), ks.change_values_buttons_grid);
    
    change_values_cancel = gtk_button_new_with_label ("Cancel");
    change_values_reset = gtk_button_new_with_label ("Reset");
    change_values_done = gtk_button_new_with_label ("Done");

    gtk_widget_set_hexpand (change_values_cancel, TRUE);
    gtk_widget_set_hexpand (change_values_reset, FALSE);
    gtk_widget_set_hexpand (change_values_done, TRUE);

    gtk_grid_attach (GTK_GRID (ks.change_values_buttons_grid), change_values_cancel, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (ks.change_values_buttons_grid), change_values_reset, 1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (ks.change_values_buttons_grid), change_values_done, 2, 0, 1, 1);

    gtk_widget_set_halign (change_values_cancel, GTK_ALIGN_END);
    gtk_widget_set_halign (change_values_reset, GTK_ALIGN_END);
    gtk_widget_set_halign (change_values_done, GTK_ALIGN_START);

    // ### Create find grid. ###

    ks.find_grid = gtk_grid_new ();
    gtk_orientable_set_orientation (GTK_ORIENTABLE (ks.find_grid), GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX (ks.main_box), ks.find_grid, FALSE, FALSE, 0);

    for (grids_cnt = 0; grids_cnt < NUMBER_OF_FIND_SUBGRIDS; grids_cnt++) {
        find_subgrids[grids_cnt] = gtk_grid_new ();
        gtk_container_add (GTK_CONTAINER (ks.find_grid), find_subgrids[grids_cnt]);
    }

    for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_FIND_ENTRY_BUTTONS; buttons_cnt++) {
#if GTK_CHECK_VERSION(3,10,0)
        ks.find_entry_buttons[buttons_cnt] = gtk_button_new_from_icon_name (find_entry_buttons_imgs[buttons_cnt], 
                                                                            GTK_ICON_SIZE_BUTTON);
#else
        ks.find_entry_buttons[buttons_cnt] = gtk_button_new ();
        gtk_button_set_image (GTK_BUTTON (ks.find_entry_buttons[buttons_cnt]), 
        gtk_image_new_from_stock (ks.find_entry_buttons_imgs[buttons_cnt], GTK_ICON_SIZE_BUTTON));
#endif
        gtk_widget_set_tooltip_text (GTK_WIDGET (ks.find_entry_buttons[buttons_cnt]), find_entry_buttons_tooltips[buttons_cnt]);
        gtk_container_add (GTK_CONTAINER (find_subgrids[ENTRY]), ks.find_entry_buttons[buttons_cnt]);
    }

    ks.find_entry = gtk_entry_new ();
    ks.find_entry_css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (ks.find_entry), 
                                    GTK_STYLE_PROVIDER (ks.find_entry_css_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_widget_set_hexpand (ks.find_entry, TRUE);
    gtk_container_add (GTK_CONTAINER (find_subgrids[ENTRY]), ks.find_entry);

    for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS - 1; columns_cnt++) {
        ks.find_in_columns[columns_cnt] = gtk_check_button_new_with_label (ks.column_header_txts[columns_cnt]);
        gtk_container_add (GTK_CONTAINER (find_subgrids[COLUMNS_SELECTION]), ks.find_in_columns[columns_cnt]);

        ks.find_in_columns_css_providers[columns_cnt] = gtk_css_provider_new ();
        gtk_style_context_add_provider (gtk_widget_get_style_context (ks.find_in_columns[columns_cnt]), 
                                        GTK_STYLE_PROVIDER (ks.find_in_columns_css_providers[columns_cnt]), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    ks.find_in_all_columns = gtk_check_button_new_with_label ("All columns");
    gtk_container_add (GTK_CONTAINER (find_subgrids[COLUMNS_SELECTION]), ks.find_in_all_columns);
    ks.find_in_all_columns_css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (ks.find_in_all_columns), 
                                    GTK_STYLE_PROVIDER (ks.find_in_all_columns_css_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    ks.find_match_case = gtk_check_button_new_with_label ("Match case");
    gtk_container_add (GTK_CONTAINER (find_subgrids[SPECIAL_OPTIONS]), ks.find_match_case);

    ks.find_regular_expression = gtk_check_button_new_with_label ("Regular expression");
    gtk_container_add (GTK_CONTAINER (find_subgrids[SPECIAL_OPTIONS]), ks.find_regular_expression);

    // Default setting
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.find_regular_expression), TRUE);

    // ### Initialise Tree View. ###

    ks.treeview = gtk_tree_view_new ();
    gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (ks.treeview), TRUE); // Default

    // Create columns with cell renderers and add them to the treeview.

    for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS; columns_cnt++) {
        ks.columns[columns_cnt] = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (ks.columns[columns_cnt], ks.column_header_txts[columns_cnt]);

        if (columns_cnt == COL_MENU_ELEMENT) {
            ks.renderers[PIXBUF_RENDERER] = gtk_cell_renderer_pixbuf_new ();
            ks.renderers[EXCL_TXT_RENDERER] = gtk_cell_renderer_text_new ();
            gtk_tree_view_column_pack_start (ks.columns[COL_MENU_ELEMENT], ks.renderers[PIXBUF_RENDERER], FALSE);
            gtk_tree_view_column_pack_start (ks.columns[COL_MENU_ELEMENT], ks.renderers[EXCL_TXT_RENDERER], FALSE);
            gtk_tree_view_column_set_attributes (ks.columns[COL_MENU_ELEMENT], ks.renderers[PIXBUF_RENDERER], 
                                                 "pixbuf", TS_ICON_IMG, NULL);
        }
        else if (columns_cnt == COL_VALUE) {
            ks.renderers[BOOL_RENDERER] = gtk_cell_renderer_toggle_new ();
            g_signal_connect (ks.renderers[BOOL_RENDERER], "toggled", G_CALLBACK (boolean_toggled), NULL);
            gtk_tree_view_column_pack_start (ks.columns[COL_VALUE], ks.renderers[BOOL_RENDERER], FALSE);
        }

        ks.renderers[TXT_RENDERER] = gtk_cell_renderer_text_new ();
        g_signal_connect (ks.renderers[TXT_RENDERER], "edited", G_CALLBACK (cell_edited), GUINT_TO_POINTER (columns_cnt));
        gtk_tree_view_column_pack_start (ks.columns[columns_cnt], ks.renderers[TXT_RENDERER], FALSE);
        gtk_tree_view_column_set_attributes (ks.columns[columns_cnt], ks.renderers[TXT_RENDERER], 
                                             "text", columns_cnt + TREEVIEW_COLUMN_OFFSET, NULL);

        gtk_tree_view_column_set_cell_data_func (ks.columns[columns_cnt], ks.renderers[TXT_RENDERER], 
                                                 (GtkTreeCellDataFunc) set_column_attributes, 
                                                 GUINT_TO_POINTER (columns_cnt), 
                                                 NULL); // NULL -> no destroy notify for user data.
        gtk_tree_view_column_set_resizable (ks.columns[columns_cnt], TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ks.treeview), ks.columns[columns_cnt]);
    }

    // Default setting
    gtk_tree_view_column_set_visible (ks.columns[COL_ELEMENT_VISIBILITY], FALSE);

    // Set treestore and model.
    ks.treestore = gtk_tree_store_new (NUMBER_OF_TS_ELEMENTS, GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, 
                                       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
                                       G_TYPE_STRING);

    gtk_tree_view_set_model (GTK_TREE_VIEW (ks.treeview), GTK_TREE_MODEL (ks.treestore));
    ks.model = gtk_tree_view_get_model (GTK_TREE_VIEW (ks.treeview));
    g_object_unref (ks.treestore);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

    // Set scrolled window that contains the treeview.
    scrolled_window = gtk_scrolled_window_new (NULL, NULL); // No manual horizontal or vertical adjustment == NULL.
    gtk_box_pack_start (GTK_BOX (ks.main_box), scrolled_window, TRUE, TRUE, 0);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled_window), ks.treeview);
    gtk_widget_set_vexpand (scrolled_window, TRUE);

    // Set drag and drop destination parameters.
    gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (ks.treeview), ks.enable_list, 1, GDK_ACTION_MOVE);

    // ### Create entry grid. ###

    ks.entry_grid = gtk_grid_new ();
    gtk_box_pack_start (GTK_BOX (ks.main_box), ks.entry_grid, FALSE, FALSE, 0);

    // Label text is set dynamically dependent on the type of the selected row.
    ks.entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY] = gtk_label_new ("");
    gtk_widget_set_halign (ks.entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY], GTK_ALIGN_START);

    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY], 0, 0, 1, 1);

    ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY] = gtk_entry_new ();
    ks.menu_element_or_value_entry_css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY]), 
                                    GTK_STYLE_PROVIDER (ks.menu_element_or_value_entry_css_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_widget_set_hexpand (ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], TRUE);
    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], 1, 0, 1, 1);

    ks.entry_labels[ICON_PATH_ENTRY] = gtk_label_new (" Icon: ");
    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_labels[ICON_PATH_ENTRY], 2, 0, 1, 1);

#if GTK_CHECK_VERSION(3,10,0)
    ks.icon_chooser = gtk_button_new_from_icon_name ("document-open", GTK_ICON_SIZE_BUTTON);
#else
    ks.icon_chooser = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (ks.icon_chooser), gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON));
#endif
    gtk_widget_set_tooltip_text (GTK_WIDGET (ks.icon_chooser), "Add/Change icon");
    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.icon_chooser, 3, 0, 1, 1);

#if GTK_CHECK_VERSION(3,10,0)
    ks.remove_icon = gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON);
#else
    ks.remove_icon = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (ks.remove_icon), gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
#endif
    gtk_widget_set_tooltip_text (GTK_WIDGET (ks.remove_icon), "Remove icon");
    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.remove_icon, 4, 0, 1, 1);

    ks.entry_fields[ICON_PATH_ENTRY] = gtk_entry_new ();
    ks.icon_path_entry_css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (ks.entry_fields[ICON_PATH_ENTRY]), 
                                    GTK_STYLE_PROVIDER (ks.icon_path_entry_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_widget_set_hexpand (ks.entry_fields[ICON_PATH_ENTRY], TRUE);
    gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_fields[ICON_PATH_ENTRY], 5, 0, 1, 1);

    for (entry_fields_cnt = 2; entry_fields_cnt < NUMBER_OF_ENTRY_FIELDS; entry_fields_cnt++) {
        ks.entry_labels[entry_fields_cnt] = gtk_label_new ((entry_fields_cnt == 2) ? " Menu ID: " : " Execute: ");
        gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_labels[entry_fields_cnt], 0, entry_fields_cnt - 1, 1, 1);

        gtk_widget_set_halign (ks.entry_labels[entry_fields_cnt], GTK_ALIGN_START);

        ks.entry_fields[entry_fields_cnt] = gtk_entry_new ();
        gtk_grid_attach (GTK_GRID (ks.entry_grid), ks.entry_fields[entry_fields_cnt], 1, entry_fields_cnt - 1, 5, 1);
    }

    // ### Statusbar ###

    ks.statusbar = gtk_statusbar_new ();
    gtk_container_add (GTK_CONTAINER (main_grid), ks.statusbar);

    // ### CSS provider for context menu

    ks.cm_css_provider = gtk_css_provider_new ();


    // --- Create signals for all buttons and relevant events. ---


    ks.handler_id_row_selected = g_signal_connect (selection, "changed", G_CALLBACK (row_selected), NULL);
    g_signal_connect (ks.treeview, "row-expanded", G_CALLBACK (set_status_of_expand_and_collapse_buttons_and_menu_items),
                      NULL);
    g_signal_connect (ks.treeview, "row-collapsed", G_CALLBACK (set_status_of_expand_and_collapse_buttons_and_menu_items), 
                      NULL);
    g_signal_connect (ks.treeview, "button-press-event", G_CALLBACK (mouse_pressed), NULL);
    g_signal_connect (ks.treeview, "button-release-event", G_CALLBACK (mouse_released), NULL);

    g_signal_connect (ks.treeview, "key-press-event", G_CALLBACK (key_pressed), NULL);

    g_signal_connect (ks.treeview, "drag-motion", G_CALLBACK (drag_motion_handler), NULL);
    g_signal_connect (ks.treeview, "drag_data_received", G_CALLBACK (drag_data_received_handler), NULL);

    g_signal_connect (ks.mb_file_menu_items[MB_NEW], "activate", G_CALLBACK (new_menu), NULL);
    g_signal_connect (ks.mb_file_menu_items[MB_OPEN], "activate", G_CALLBACK (open_menu), NULL);
    g_signal_connect_swapped (ks.mb_file_menu_items[MB_SAVE], "activate", G_CALLBACK (save_menu), NULL);
    g_signal_connect (ks.mb_file_menu_items[MB_SAVE_AS], "activate", G_CALLBACK (save_menu_as), NULL);

    g_signal_connect (ks.mb_file_menu_items[MB_QUIT], "activate", G_CALLBACK (quit_program), NULL);

    for (mb_menu_items_cnt = TOP; mb_menu_items_cnt <= BOTTOM; mb_menu_items_cnt++) {
        g_signal_connect_swapped (ks.mb_edit_menu_items[mb_menu_items_cnt], "activate", 
                                  G_CALLBACK (move_selection), GUINT_TO_POINTER (mb_menu_items_cnt));
    }
    g_signal_connect_swapped (ks.mb_edit_menu_items[MB_REMOVE], "activate", G_CALLBACK (remove_rows), "menu bar");
    g_signal_connect (ks.mb_edit_menu_items[MB_REMOVE_ALL_CHILDREN], "activate", G_CALLBACK (remove_all_children), NULL);
    g_signal_connect_swapped (ks.mb_edit_menu_items[MB_VISUALISE], "activate", 
                              G_CALLBACK (visualise_menus_items_and_separators), GUINT_TO_POINTER (FALSE));
    g_signal_connect_swapped (ks.mb_edit_menu_items[MB_VISUALISE_RECURSIVELY], "activate", 
                              G_CALLBACK (visualise_menus_items_and_separators), GUINT_TO_POINTER (TRUE));

    g_signal_connect (mb_find, "activate", G_CALLBACK (show_or_hide_find_grid), NULL);

    g_signal_connect_swapped (ks.mb_expand_all_nodes, "activate", G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (TRUE));
    g_signal_connect_swapped (ks.mb_collapse_all_nodes, "activate", G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (FALSE));
    for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_VIEW_AND_OPTIONS; mb_menu_items_cnt++) {
        g_signal_connect_swapped (ks.mb_view_and_options[mb_menu_items_cnt], "activate", 
                                  G_CALLBACK (change_view_and_options), GUINT_TO_POINTER (mb_menu_items_cnt));
    }

    g_signal_connect (mb_about, "activate", G_CALLBACK (about), NULL);

    g_signal_connect (ks.tb[TB_NEW], "clicked", G_CALLBACK (new_menu), NULL);
    g_signal_connect (ks.tb[TB_OPEN], "clicked", G_CALLBACK (open_menu), NULL);

    g_signal_connect_swapped (ks.tb[TB_SAVE], "clicked", G_CALLBACK (save_menu), NULL);
    g_signal_connect (ks.tb[TB_SAVE_AS], "clicked", G_CALLBACK (save_menu_as), NULL);
    g_signal_connect_swapped (ks.tb[TB_MOVE_UP], "clicked", G_CALLBACK (move_selection), GUINT_TO_POINTER (UP));
    g_signal_connect_swapped (ks.tb[TB_MOVE_DOWN], "clicked", G_CALLBACK (move_selection), GUINT_TO_POINTER (DOWN));
    g_signal_connect_swapped (ks.tb[TB_REMOVE], "clicked", G_CALLBACK (remove_rows), "toolbar");
    g_signal_connect (ks.tb[TB_FIND], "clicked", G_CALLBACK (show_or_hide_find_grid), NULL);
    g_signal_connect_swapped (ks.tb[TB_EXPAND_ALL], "clicked", G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (TRUE));
    g_signal_connect_swapped (ks.tb[TB_COLLAPSE_ALL], "clicked", G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (FALSE));
    g_signal_connect (ks.tb[TB_QUIT], "clicked", G_CALLBACK (quit_program), NULL);

    for (buttons_cnt = 0; buttons_cnt < ACTION_OR_OPTION; buttons_cnt++) {
        g_signal_connect_swapped (ks.bt_add[buttons_cnt], "clicked", G_CALLBACK (add_new), add_txts[buttons_cnt]);
    }
    /*
        The signal for the "Action/Option" button is always disconnected first, before it is reconnected with the 
        currently appropriate function. This means that the function and parameter are meaningless here since the signal 
        is an unused "dummy", but nevertheless necessary because there has to be a signal that can be disconnected before 
        adding a new one.
    */
    ks.handler_id_action_option_button_clicked = g_signal_connect_swapped (ks.bt_add[ACTION_OR_OPTION], "clicked", 
                                                                           G_CALLBACK (add_new), NULL);

    ks.handler_id_including_action_check_button = g_signal_connect_swapped (ks.including_action_check_button, "clicked", 
                                                                            G_CALLBACK (one_of_the_change_values_buttons_pressed), 
                                                                            "incl. action");
    g_signal_connect_swapped (change_values_reset, "clicked", 
                              G_CALLBACK (one_of_the_change_values_buttons_pressed), "reset");
    g_signal_connect_swapped (change_values_done, "clicked", 
                              G_CALLBACK (one_of_the_change_values_buttons_pressed), "done");

    // This "simulates" a click on another row.
    g_signal_connect (change_values_cancel, "clicked", G_CALLBACK (row_selected), NULL);
    ks.handler_id_action_option_combo_box = g_signal_connect (ks.action_option, "changed", G_CALLBACK (show_action_options), NULL);
    g_signal_connect_swapped (ks.action_option_done, "clicked", G_CALLBACK (action_option_insert), "by combo box");
    g_signal_connect_swapped (ks.action_option_cancel, "clicked", G_CALLBACK (hide_action_option_grid), "cancel button");
    ks.handler_id_show_startupnotify_options = g_signal_connect (ks.options_fields[SN_OR_PROMPT], "toggled", 
                                                                 G_CALLBACK (show_startupnotify_options), NULL);

    g_signal_connect (ks.find_entry_buttons[CLOSE], "clicked", G_CALLBACK (show_or_hide_find_grid), NULL);
    for (buttons_cnt = BACK; buttons_cnt <= FORWARD; buttons_cnt++) {
        g_signal_connect_swapped (ks.find_entry_buttons[buttons_cnt], "clicked", 
                                  G_CALLBACK (jump_to_previous_or_next_occurrence),
                                  GUINT_TO_POINTER ((buttons_cnt == FORWARD)));
    }
    g_signal_connect (ks.find_entry, "activate", G_CALLBACK (run_search), NULL);
    for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS - 1; columns_cnt++) {
        ks.handler_id_find_in_columns[columns_cnt] = g_signal_connect_swapped (ks.find_in_columns[columns_cnt], "clicked", 
                                                                               G_CALLBACK (find_buttons_management), "specif");
    }
    g_signal_connect_swapped (ks.find_in_all_columns, "clicked", G_CALLBACK (find_buttons_management), "all");
    g_signal_connect_swapped (ks.find_match_case, "clicked", G_CALLBACK (find_buttons_management), NULL);
    g_signal_connect_swapped (ks.find_regular_expression, "clicked", G_CALLBACK (find_buttons_management), NULL);

    for (entry_fields_cnt = 0; entry_fields_cnt < NUMBER_OF_ENTRY_FIELDS; entry_fields_cnt++) {
        ks.handler_id_entry_fields[entry_fields_cnt] = g_signal_connect (ks.entry_fields[entry_fields_cnt], "activate", 
                                                                         G_CALLBACK (change_row), NULL);
    }
    g_signal_connect (ks.icon_chooser, "clicked", G_CALLBACK (icon_choosing_by_button_or_context_menu), NULL);
    g_signal_connect (ks.remove_icon, "clicked", G_CALLBACK (remove_icons_from_menus_or_items), NULL);

    g_signal_connect (ks.options_fields[PROMPT], "activate", G_CALLBACK (single_field_entry), NULL);
    g_signal_connect (ks.options_fields[COMMAND], "activate", G_CALLBACK (single_field_entry), NULL);
    for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
        g_signal_connect (ks.suboptions_fields[snotify_opts_cnt], "activate", G_CALLBACK (single_field_entry), NULL);
    }

    // --- Default settings ---

    ks.font_size = get_font_size (); // Get the default font size ...
    g_object_get (gtk_settings_get_default (), "gtk-icon-theme-name", &ks.icon_theme_name, NULL); // ... and the icon theme name.
    create_invalid_icon_imgs (); // Create broken/invalid path icon images suitable for that font size.
    ks.search_term = g_string_new ("");
    // Has to be placed here because the signal to deactivate all other column check boxes has to be triggered.
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ks.find_in_all_columns), TRUE); // Deactivate user defined column search.

    /*
        The height of the message label is set to be identical to the one of the buttons, so the button grid doesn't 
        shrink if the buttons are missing. This can only be done after all widgets have already been added to the grid, 
        because only then the height of the button grid (with buttons shown) can be determined correctly.
    */
    gtk_widget_set_size_request (ks.bt_bar_label, -1, gtk_widget_get_allocated_height (ks.bt_add[0]));


    gtk_widget_show_all (ks.window);
    // Default
    gtk_widget_hide (ks.action_option_grid);
    gtk_widget_hide (ks.find_grid);
    gtk_widget_hide (ks.separator);
    gtk_widget_hide (ks.change_values_buttons_grid);
    gtk_widget_hide (ks.inside_menu_label);
    gtk_widget_hide (ks.inside_menu_check_button);
    gtk_widget_hide (ks.including_action_label);
    gtk_widget_hide (ks.including_action_check_button);

    /*
        The height of the message label is set to be identical to the one of the buttons, so the button grid doesn't 
        shrink if the buttons are missing. This can only be done after all widgets have already been added to the grid, 
        because only then the height of the button grid (with buttons shown) can be determined correctly.
    */
    gtk_widget_set_size_request (ks.bt_bar_label, -1, gtk_widget_get_allocated_height (ks.bt_add[0]));


    // --- Load settings and standard menu file, if existent. ---


    settings_file_path = g_strconcat (g_getenv ("HOME"), "/.kickshawrc", NULL);
    if (G_LIKELY (g_file_test (settings_file_path, G_FILE_TEST_EXISTS))) {
        GKeyFile *settings_file = g_key_file_new ();
        GError *settings_file_error = NULL;

        if (G_LIKELY (g_key_file_load_from_file (settings_file, settings_file_path, 
                      G_KEY_FILE_KEEP_COMMENTS, &settings_file_error))) {
            gchar *comment = NULL;
            if (!g_key_file_get_comment (settings_file, NULL, NULL, NULL)) { // Versions up to 0.5.7 don't have a comment in the settings file.
                GtkWidget *dialog;

                create_dialog (&dialog, "Settings resetted due to change in settings setup", "gtk-info", 
                               "Due to a change in the setup of the settings the settings file has been rewritten "
                               "and the settings have been reset to defaults. Use \"View\" and \"Options\" " 
                               "in the menu bar to readjust the settings.",  
                               "Close", NULL, NULL, TRUE);

                write_settings ();
                
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_free (comment);                                
            }
            else {
                for (view_and_opts_cnt = 0; view_and_opts_cnt < NUMBER_OF_VIEW_AND_OPTIONS; view_and_opts_cnt++) {
                    gtk_check_menu_item_set_active 
                        (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[view_and_opts_cnt]), 
                                              g_key_file_get_boolean (settings_file, 
                                              (view_and_opts_cnt < CREATE_BACKUP_BEFORE_OVERWRITING_MENU) ? "VIEW" : "OPTIONS", 
                                              gtk_menu_item_get_label ((GtkMenuItem *) ks.mb_view_and_options[view_and_opts_cnt]), 
                                              NULL));
                }
            }
        }
        else {
            gchar *errmsg_txt = g_strdup_printf ("<b>Could not open settings file</b>\n<tt>%s</tt>\n<b>for reading!\n\n"
                                                 "<span foreground='#8a1515'>%s</span></b>", 
                                                 settings_file_path, settings_file_error->message);

            show_errmsg (errmsg_txt);

            // Cleanup
            g_error_free (settings_file_error);
            g_free (errmsg_txt);
        }

        // Cleanup
        g_key_file_free (settings_file);

    }
    else {
        write_settings ();
    }
    // Cleanup
    g_free (settings_file_path);


    // --- Load standard menu file, if existent. ---


    if (G_LIKELY (g_file_test (new_filename = g_strconcat (g_getenv ("HOME"), "/.config/openbox/menu.xml", NULL), 
                  G_FILE_TEST_EXISTS))) {
        get_menu_elements_from_file (new_filename);
    }
    else {
        g_free (new_filename);
    }


    // Sets settings for menu- and toolbar, that depend on whether there is a menu file or not.
    row_selected ();

    gtk_widget_grab_focus (ks.treeview);
}

/* 

    Creates a label for an "Add new" button, adds it to a grid with additional margin, which is added to the button.

*/

static void add_button_content (GtkWidget *button, gchar *label_text)
{
    GtkWidget *grid = gtk_grid_new ();
    GtkWidget *label = (*label_text) ? gtk_label_new (label_text) : ks.bt_add_action_option_label;

    gtk_container_set_border_width (GTK_CONTAINER (grid), 2);
    gtk_container_add (GTK_CONTAINER (grid), label);
    gtk_container_add (GTK_CONTAINER (button), grid);
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

*/

static gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event)
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

*/

static gboolean mouse_released (void)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));

    if (gtk_tree_selection_count_selected_rows (selection) < 2) {
        return FALSE;
    }

    // NULL == No destroy function for user data.
    gtk_tree_selection_set_select_function (gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview)), 
                                            (GtkTreeSelectionFunc) selection_block_unblock, GINT_TO_POINTER (TRUE), NULL);

    return FALSE;
}

/* 

    Sets attributes like foreground and background colour, visibility of cell renderers and 
    editability of cells according to certain conditions. Also highlights search results.

*/

static void set_column_attributes (G_GNUC_UNUSED GtkTreeViewColumn *cell_column, 
                                                 GtkCellRenderer   *txt_renderer,
                                                 GtkTreeModel      *cell_model, 
                                                 GtkTreeIter       *cell_iter, 
                                                 gpointer           column_number_pointer)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));

    gboolean row_is_selected = gtk_tree_selection_iter_is_selected (selection, cell_iter);

    guint column_number = GPOINTER_TO_UINT (column_number_pointer);
    GtkTreePath *cell_path = gtk_tree_model_get_path (cell_model, cell_iter);
    guint cell_data_icon_img_status;
    gchar *cell_data[NUMBER_OF_TXT_FIELDS];

    // Defaults
    gboolean visualise_txt_renderer = TRUE;
    gboolean visualise_bool_renderer = FALSE;

    // Defaults
    gchar *background = NULL;
    gboolean background_set = FALSE;
    gchar *highlighted_txt = NULL;

    gboolean show_icons = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[SHOW_ICONS]));
    gboolean set_off_separators = 
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[SET_OFF_SEPARATORS]));
    gboolean keep_highlighting = 
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM 
                                        (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL]));

    gtk_tree_model_get (cell_model, cell_iter, 
                        TS_ICON_IMG_STATUS, &cell_data_icon_img_status, 
                        TS_ICON_PATH, &cell_data[ICON_PATH_TXT], 
                        TS_MENU_ELEMENT, &cell_data[MENU_ELEMENT_TXT], 
                        TS_TYPE, &cell_data[TYPE_TXT], 
                        TS_VALUE, &cell_data[VALUE_TXT], 
                        TS_MENU_ID, &cell_data[MENU_ID_TXT], 
                        TS_EXECUTE, &cell_data[EXECUTE_TXT], 
                        TS_ELEMENT_VISIBILITY, &cell_data[ELEMENT_VISIBILITY_TXT], 
                        -1);

    gboolean icon_to_be_shown = show_icons && cell_data[ICON_PATH_TXT];

    /*
        Set the cell renderer type of the "Value" column to toggle if it is a "prompt" option of a non-Execute action or 
        an "enabled" option of a "startupnotify" option block.
    */

    if (column_number == COL_VALUE && STREQ (cell_data[TYPE_TXT], "option") && 
        streq_any (cell_data[MENU_ELEMENT_TXT], "prompt", "enabled", NULL)) {
        GtkTreeIter parent;
        gchar *cell_data_menu_element_parent_txt;

        gtk_tree_model_iter_parent (cell_model, &parent, cell_iter);
        gtk_tree_model_get (ks.model, &parent, TS_MENU_ELEMENT, &cell_data_menu_element_parent_txt, -1);
        if (!STREQ (cell_data_menu_element_parent_txt, "Execute")) {
            visualise_txt_renderer = FALSE;
            visualise_bool_renderer = TRUE;
            g_object_set (ks.renderers[BOOL_RENDERER], "active", STREQ (cell_data[VALUE_TXT], "yes"), NULL);
        }
        // Cleanup
        g_free (cell_data_menu_element_parent_txt);
    }

    g_object_set (txt_renderer, "visible", visualise_txt_renderer, NULL);
    g_object_set (ks.renderers[BOOL_RENDERER], "visible", visualise_bool_renderer, NULL);
    g_object_set (ks.renderers[PIXBUF_RENDERER], "visible", icon_to_be_shown, NULL);
    g_object_set (ks.renderers[EXCL_TXT_RENDERER], "visible", 
                  G_UNLIKELY (icon_to_be_shown && cell_data_icon_img_status), NULL);

    /*
        If the icon is one of the two built-in types that indicate an invalid path or icon image, 
        set two red exclamation marks behind it to clearly distinguish this icon from icons of valid image files.
    */ 
    if (G_UNLIKELY (column_number == COL_MENU_ELEMENT && cell_data_icon_img_status)) {
        g_object_set (ks.renderers[EXCL_TXT_RENDERER], "markup", "<span foreground='red'><b>!!</b></span>", NULL);
    }

    // Emphasis that a menu, pipe menu or item has no label (=invisible).
    if (G_UNLIKELY (column_number == COL_MENU_ELEMENT && 
                    streq_any (cell_data[TYPE_TXT], "menu", "pipe menu", "item", NULL) && !cell_data[MENU_ELEMENT_TXT])) {
        g_object_set (txt_renderer, "text", "(No label)", NULL);
    }

    if (keep_highlighting) {
        guint8 visibility_of_parent = NONE_OR_VISIBLE_ANCESTOR; // Default
        if (G_UNLIKELY ((cell_data[ELEMENT_VISIBILITY_TXT] && !STREQ (cell_data[ELEMENT_VISIBILITY_TXT], "visible")) || 
                        (visibility_of_parent = check_if_invisible_ancestor_exists (cell_model, cell_path)))) {
            background = ((cell_data[ELEMENT_VISIBILITY_TXT] && 
                          g_str_has_suffix (cell_data[ELEMENT_VISIBILITY_TXT], "orphaned menu")) || 
                          visibility_of_parent == INVISIBLE_ORPHANED_ANCESTOR) ? "#364074" : "#656772";
            background_set = TRUE;
        }
    }

    // If a search is going on, highlight all matches.
    if (*ks.search_term->str && column_number < COL_ELEMENT_VISIBILITY && !gtk_widget_get_visible (ks.action_option_grid) && 
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.find_in_columns[column_number])) && 
        check_for_match (cell_iter, column_number)) {
        // cell_data starts with ICON_PATH, but this is not part of the treeview.
        gchar *cell_txt = cell_data[column_number + 1];
        gchar *search_term_str_escaped = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.find_regular_expression))) ? 
                                         NULL : g_regex_escape_string (ks.search_term->str, -1);
        GRegex *regex = g_regex_new ((search_term_str_escaped) ? search_term_str_escaped : ks.search_term->str, 
                                     (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ks.find_match_case))) ? 
                                     0 : G_REGEX_CASELESS, 0, NULL);
        GMatchInfo *match_info = NULL;
        gchar *match;
        GSList *matches = NULL;
        GSList *replacement_txts = NULL;
        GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
        gchar *highlighted_txt_core;

        g_regex_match (regex, cell_txt, 0, &match_info);
        while (g_match_info_matches (match_info)) {
            match = g_match_info_fetch (match_info, 0);
            if (!g_slist_find_custom (matches, match = g_match_info_fetch (match_info, 0), (GCompareFunc) strcmp)) {
                matches = g_slist_prepend (matches, g_strdup (match));
                replacement_txts = g_slist_prepend (replacement_txts, 
                                                    g_strconcat ("<span background='", 
                                                                 (row_is_selected) ? "black'>" : "yellow' foreground='black'>", 
                                                                 match, "</span>", NULL));
                g_hash_table_insert (hash_table, matches->data, replacement_txts->data);
            }
            g_match_info_next (match_info, NULL);

            // Cleanup
            g_free (match);
        }

        /*
            Replace all findings with a highlighted version at once.
            -1 == nul-terminated string, 0 == start position, 0 == no match flags.
        */
        highlighted_txt_core = g_regex_replace_eval (regex, cell_txt, -1, 0, 0, 
                               multiple_str_replacement_callback_func, hash_table, NULL);
        highlighted_txt = (!background_set) ? 
                          g_strdup (highlighted_txt_core) : 
                          g_strdup_printf ("<span foreground='white'>%s</span>", highlighted_txt_core);

        g_object_set (txt_renderer, "markup", highlighted_txt, NULL);

        // Cleanup
        g_free (search_term_str_escaped);
        g_regex_unref (regex);
        g_match_info_free (match_info);
        g_slist_free_full (matches, (GDestroyNotify) g_free);
        g_slist_free_full (replacement_txts, (GDestroyNotify) g_free);
        g_hash_table_destroy (hash_table);
        g_free (highlighted_txt_core);
    }

    // Set foreground and background font and cell colours. Also set editability of cells.
    g_object_set (txt_renderer, "weight", (set_off_separators && STREQ (cell_data[TYPE_TXT], "separator")) ? 1000 : 400, 

                  "family", (STREQ (cell_data[TYPE_TXT], "separator") && set_off_separators) ? 
                            "monospace, courier new, courier" : "sans, sans-serif, arial, helvetica",
 
                  "foreground", "white", "foreground-set", (row_is_selected || (background_set && !highlighted_txt)),
                  "background", background, "background-set", background_set, 

                  "editable", 
                  (((column_number == COL_MENU_ELEMENT && 
                  (STREQ (cell_data[TYPE_TXT], "separator") || 
                  (cell_data[MENU_ELEMENT_TXT] &&
                  streq_any (cell_data[TYPE_TXT], "menu", "pipe menu", "item", NULL)))) || 
                  (column_number == COL_VALUE && STREQ (cell_data[TYPE_TXT], "option")) || 
                  (column_number == COL_MENU_ID && streq_any (cell_data[TYPE_TXT], "menu", "pipe menu", NULL)) ||
                  (column_number == COL_EXECUTE && STREQ (cell_data[TYPE_TXT], "pipe menu")))), 

                  NULL);

    for (guint8 renderer_cnt = EXCL_TXT_RENDERER; renderer_cnt < NUMBER_OF_RENDERERS; renderer_cnt++) {
        g_object_set (ks.renderers[renderer_cnt], "cell-background", background, "cell-background-set", background_set, NULL);
    }

    if (highlighted_txt && gtk_cell_renderer_get_visible (ks.renderers[BOOL_RENDERER])) {
        g_object_set (ks.renderers[BOOL_RENDERER], "cell-background", "yellow", "cell-background-set", TRUE, NULL);
    }

    // Cleanup
    g_free (highlighted_txt);
    gtk_tree_path_free (cell_path);
    free_elements_of_static_string_array (cell_data, NUMBER_OF_TXT_FIELDS, FALSE);
}

/* 

    Changes certain aspects of the tree view or misc. settings.

*/

static void change_view_and_options (gpointer activated_menu_item_pointer)
{
    guint8 activated_menu_item = GPOINTER_TO_UINT (activated_menu_item_pointer);

    gboolean menu_item_activated = 
        (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[activated_menu_item])));

    if (activated_menu_item <= SHOW_ELEMENT_VISIBILITY_COL_ACTVTD) {
        /*
            SHOW_MENU_ID_COL (0)                   + 3 = COL_MENU_ID (3)
            SHOW_EXECUTE_COL (1)                   + 3 = COL_EXECUTE (4)
            SHOW_ELEMENT_VISIBILITY_COL_ACTVTD (2) + 3 = COL_ELEMENT_VISIBILITY (5)
        */
        guint8 column_position = activated_menu_item + 3;

        gtk_tree_view_column_set_visible (ks.columns[column_position], menu_item_activated);

        if (column_position == COL_ELEMENT_VISIBILITY) {
            if (!menu_item_activated) {
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM 
                                                (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL]), TRUE);
            }
            gtk_widget_set_sensitive (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL], menu_item_activated);
            gtk_widget_set_sensitive (ks.mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL], menu_item_activated);
        }
    }
    else if (activated_menu_item == DRAW_ROWS_IN_ALT_COLOURS) {
        g_object_set (ks.treeview, "rules-hint", menu_item_activated, NULL);
    }
    else if (activated_menu_item >= NO_GRID_LINES && activated_menu_item <= BOTH) {
        guint8 grid_settings_cnt, grid_lines_type_cnt;

        for (grid_settings_cnt = NO_GRID_LINES, grid_lines_type_cnt = GTK_TREE_VIEW_GRID_LINES_NONE; 
             grid_settings_cnt <= BOTH; 
             grid_settings_cnt++, grid_lines_type_cnt++) {
            if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[grid_settings_cnt]))) {
                gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (ks.treeview), grid_lines_type_cnt);
                break;
            }
        }
    }
    else if (activated_menu_item == SHOW_TREE_LINES) {
        gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (ks.treeview), menu_item_activated);
    }
    else if (activated_menu_item == SORT_EXECUTE_AND_STARTUPN_OPTIONS) {
        if ((ks.autosort_options = menu_item_activated)) {
            gtk_tree_model_foreach (ks.model, (GtkTreeModelForeachFunc) sort_loop_after_sorting_activation, NULL);
        }
        /*
            A change of the activation status of autosorting requires 
            (de)activation of the movement arrows of the menu- and toolbar.
        */
        row_selected ();
    }
    else {
        gtk_tree_view_columns_autosize (GTK_TREE_VIEW (ks.treeview)); // If icon visibility has been switched on.
    }

    write_settings ();
}

/* 

    Expands the tree view so the whole structure is visible or
    collapses the tree view so just the toplevel elements are visible.

*/

static void expand_or_collapse_all (gpointer expand_pointer)
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

/*

    Sets the sensivity of the expand/collapse menu items and toolbar buttons
    according to the status of the nodes of the treeview.

*/

void set_status_of_expand_and_collapse_buttons_and_menu_items (void)
{
    gboolean expansion_statuses_of_nodes[NUMBER_OF_EXPANSION_STATUSES] = { FALSE };

    gtk_tree_model_foreach (ks.model, (GtkTreeModelForeachFunc) check_expansion_statuses_of_nodes, expansion_statuses_of_nodes);

    gtk_widget_set_sensitive (ks.mb_collapse_all_nodes, expansion_statuses_of_nodes[AT_LEAST_ONE_IS_EXPANDED]);
    gtk_widget_set_sensitive ((GtkWidget *) ks.tb[TB_COLLAPSE_ALL], 
                              expansion_statuses_of_nodes[AT_LEAST_ONE_IS_EXPANDED]);
    gtk_widget_set_sensitive (ks.mb_expand_all_nodes, expansion_statuses_of_nodes[AT_LEAST_ONE_IS_COLLAPSED]);
    gtk_widget_set_sensitive ((GtkWidget *) ks.tb[TB_EXPAND_ALL], 
                              expansion_statuses_of_nodes[AT_LEAST_ONE_IS_COLLAPSED]);
}

/* 

    Dialog window that shows the program name and version, a short description, the website, author and license.

*/

static void about (void) 
{
    gchar *authors[] = { "Marcus Schaetzle", NULL };

    gtk_show_about_dialog (GTK_WINDOW (ks.window), 
                           "program-name", "Kickshaw", 
                           "version", KICKSHAW_VERSION, 
                           "comments", "A menu editor for Openbox", 
                           "website", "https://savannah.nongnu.org/projects/obladi/", 
                           "website_label", "Project page at GNU Savannah", 
                           "copyright", "(c) 2010-2018 Marcus Schaetzle", 
                           "license-type", GTK_LICENSE_GPL_2_0, 

                           "authors", authors, 
                           NULL);
}

/* 

    Asks whether to continue if there are unsaved changes.

*/

gboolean continue_despite_unsaved_changes (void)
{
    GtkWidget *dialog;

    gint result;

    #define CONTINUE_DESPITE_UNSAVED_CHANGES 2

    create_dialog (&dialog, "Menu has unsaved changes", "dialog-warning", 
                   "<b>This menu has unsaved changes. Continue anyway?</b>", "_Cancel", "_Yes", NULL, TRUE);

    result = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);

    return (result == CONTINUE_DESPITE_UNSAVED_CHANGES);
}

/* 

    Clears the data that is held throughout the running time of the program.

*/

void clear_global_data (void)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));

    FREE_AND_REASSIGN (ks.filename, NULL);
    g_slist_free_full (ks.menu_ids, (GDestroyNotify) g_free);
    ks.menu_ids = NULL;
    if (ks.rows_with_icons) {
        stop_timeout ();
    }
    if (gtk_widget_get_visible (ks.find_grid)) {
        show_or_hide_find_grid ();
    }
    g_signal_handler_block (selection, ks.handler_id_row_selected);
    gtk_tree_store_clear (ks.treestore);
    g_signal_handler_unblock (selection, ks.handler_id_row_selected);
    ks.statusbar_msg_shown = FALSE;
}

/* 

    Clears treestore and global variables, also resets window title and menu-/tool-/button bar widgets accordingly.

*/

static void new_menu (void)
{
    if (ks.change_done && !continue_despite_unsaved_changes ()) {
        return;
    }

    gtk_window_set_title (GTK_WINDOW (ks.window), "Kickshaw");

    clear_global_data ();
    ks.change_done = FALSE;

    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (ks.treeview));
    row_selected (); // Switches the settings for menu- and toolbar to that of an empty menu.
}

/* 

    Quits program, if there are unsaved changes a confirmation dialog window is shown.

*/

static void quit_program (void) 
{
    if (ks.change_done && !continue_despite_unsaved_changes ()) {
        return;
    }

#if GLIB_CHECK_VERSION(2,32,0)
    g_application_quit (G_APPLICATION (ks.app));
#else
    exit (EXIT_SUCCESS);
#endif
}

/* 

    Refreshes the txt_fields array with the values of the currently selected row.

*/

void repopulate_txt_fields_array (void)
{
    // FALSE = Don't set array elements to NULL after freeing.
    free_elements_of_static_string_array (ks.txt_fields, NUMBER_OF_TXT_FIELDS, FALSE);
    gtk_tree_model_get (ks.model, &ks.iter, 
                        TS_ICON_PATH, &ks.txt_fields[ICON_PATH_TXT],
                        TS_MENU_ELEMENT, &ks.txt_fields[MENU_ELEMENT_TXT],
                        TS_TYPE, &ks.txt_fields[TYPE_TXT],
                        TS_VALUE, &ks.txt_fields[VALUE_TXT],
                        TS_MENU_ID, &ks.txt_fields[MENU_ID_TXT],
                        TS_EXECUTE, &ks.txt_fields[EXECUTE_TXT],
                        TS_ELEMENT_VISIBILITY, &ks.txt_fields[ELEMENT_VISIBILITY_TXT],
                        -1);
}

/* 

    Activates "Save" menubar item/toolbar button (provided that there is a filename) if a change has been done.
    Also sets a global veriable so a program-wide check for a change is possible.
    If a search is still active, the list of results is recreated.     
    A list of rows with icons is (re)created, too.

*/

void activate_change_done (void)
{
    if (ks.filename) {
        gtk_widget_set_sensitive (ks.mb_file_menu_items[MB_SAVE], TRUE);
        gtk_widget_set_sensitive ((GtkWidget *) ks.tb[TB_SAVE], TRUE);
    }

    if (*ks.search_term->str) {
        create_list_of_rows_with_found_occurrences ();
    }

    if (ks.rows_with_icons) {
        stop_timeout ();
    }
    create_list_of_icon_occurrences ();

    ks.change_done = TRUE;
}

/* 

    Writes all view and option settings into a file.

*/

static void write_settings (void)
{
    FILE *settings_file;
    gchar *settings_file_path = g_strconcat (g_getenv ("HOME"), "/.kickshawrc", NULL);

    if (G_UNLIKELY (!(settings_file = fopen (settings_file_path, "w")))) {
        gchar *errmsg_txt = g_strdup_printf ("<b>Could not open settings file</b>\n<tt>%s</tt>\n<b>for writing!</b>", 
                                             settings_file_path);

        show_errmsg (errmsg_txt);

        // Cleanup
        g_free (errmsg_txt);
        g_free (settings_file_path);

        return;
    }

    // Cleanup
    g_free (settings_file_path);

    fputs ("# Generated by Kickshaw " KICKSHAW_VERSION "\n\n[VIEW]\n\n", settings_file);

    for (guint8 view_and_opts_cnt = 0; view_and_opts_cnt < NUMBER_OF_VIEW_AND_OPTIONS; view_and_opts_cnt++) {
        if (view_and_opts_cnt == CREATE_BACKUP_BEFORE_OVERWRITING_MENU) {
            fputs ("\n[OPTIONS]\n\n", settings_file);
        }
        fprintf (settings_file, "%s=%s\n", gtk_menu_item_get_label ((GtkMenuItem *) ks.mb_view_and_options[view_and_opts_cnt]), 
                 (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (ks.mb_view_and_options[view_and_opts_cnt]))) ? 
                 "true" : "false");
    }

    fclose (settings_file);
}

/* 

    Replaces the filename and window title.

*/

void set_filename_and_window_title (gchar *new_filename)
{
    gchar *basename;
    gchar *window_title;

    FREE_AND_REASSIGN (ks.filename, new_filename);
    basename = g_path_get_basename (ks.filename);
    window_title = g_strconcat ("Kickshaw - ", basename, NULL);
    gtk_window_set_title (GTK_WINDOW (ks.window), window_title);

    // Cleanup
    g_free (basename);
    g_free (window_title);
}

/* 

    Shows an error message dialog.

*/

void show_errmsg (gchar *errmsg_raw_txt)
{
    GtkWidget *dialog;
    gchar *label_txt = g_strdup_printf ((!strstr (errmsg_raw_txt, "<b>")) ? "<b>%s</b>" : "%s", errmsg_raw_txt);

    create_dialog (&dialog, "Error", "dialog-error", label_txt, "Close", NULL, NULL, TRUE);

    // Cleanup
    g_free (label_txt);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/* 

    Shows a message in the statusbar at the botton for information.

*/

void show_msg_in_statusbar (gchar *message)
{
    ks.statusbar_msg_shown = TRUE;

    message = g_strdup_printf (" %s", message);
    gtk_statusbar_push (GTK_STATUSBAR (ks.statusbar), 1, message); // Only one context (indicated by 1) with one message.

    // Cleanup
    g_free (message);
}
