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

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <string.h>

#include "general_header_files/enum__ancestor_visibility.h"
#include "general_header_files/enum__invalid_icon_img_statuses.h"
#include "general_header_files/enum__menu_bar_items_view_and_options.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "load_menu.h"

enum { TS_BUILD_ICON_IMG, TS_BUILD_ICON_IMG_STATUS, TS_BUILD_ICON_MODIFICATION_TIME, TS_BUILD_ICON_PATH, 
       TS_BUILD_MENU_ELEMENT, TS_BUILD_TYPE, TS_BUILD_VALUE, TS_BUILD_MENU_ID, TS_BUILD_EXECUTE, 
       TS_BUILD_ELEMENT_VISIBILITY, TS_BUILD_PATH_DEPTH, NUMBER_OF_TS_BUILD_FIELDS };
enum { MENUS, ROOT_MENU, NUMBER_OF_MENU_LEVELS };
enum { ITEM, ACTION, OPTION, NUMBER_OF_CURRENT_ELEMENTS };
enum { MENU_IDS_LIST, MENUS_LIST = 0, MENU_ELEMENTS_LIST, ITEMS_LIST = 1, NUMBER_OF_INVISIBLE_ELEMENTS_LISTS };
// Enumeration for dialogs.
enum { ORPHANED_MENUS, MISSING_LABELS };

typedef struct {
    gchar *line;
    gint line_number;

    GSList *tree_build;
    guint current_path_depth;
    guint previous_path_depth;
    guint max_path_depth;

    GSList *menu_ids;
    GSList *toplevel_menu_ids[NUMBER_OF_MENU_LEVELS];

    gboolean ignore_all_upcoming_errs;

    gchar *current_elements[NUMBER_OF_CURRENT_ELEMENTS];
    gboolean open_option_element;
    gchar *previous_type;

    gboolean deprecated_exe_cmds_cvtd;

    guint8 loading_stage;
    gboolean change_done_loading_proc;
    gboolean root_menu_finished;
} MenuBuildingData;

static void start_tags_their_attr_and_val__empty_element_tags (GMarkupParseContext *parse_context, const gchar *element_name, 
                                                               const gchar **attribute_names, const gchar **attribute_values, 
                                                               gpointer menu_building_pnt, GError **error);
static void end_tags (G_GNUC_UNUSED GMarkupParseContext  *parse_context, 
                      G_GNUC_UNUSED const gchar          *element_name,
                                    gpointer              menu_building_pnt, 
                      G_GNUC_UNUSED GError              **error);
static void element_content (G_GNUC_UNUSED GMarkupParseContext  *parse_context, 
                                           const gchar          *text, 
                             G_GNUC_UNUSED gsize                 text_len, 
                                           gpointer              menu_building_pnt, 
                             G_GNUC_UNUSED GError              **error);
static gboolean elements_visibility (GtkTreeModel *foreach_model, GtkTreePath *foreach_path,
                                     GtkTreeIter *foreach_iter, GSList **invisible_elements_lists);
static void create_dialogs_for_invisible_menus_and_items (guint8 dialog_type, GtkTreeSelection *selection, 
                                                          GSList **invisible_elements_lists);
void get_menu_elements_from_file (gchar *new_filename);
void open_menu (void);

/* 

    Parses start-tags, their attributes and the values of the latter. Parses also empty element-tags.
    Besides the syntax check done by GLib's XML subset parser, this function also checks for the correct nesting 
    of Openbox-specific tags, uniqueness of menu IDs and other things. It also prevents repeated appearances of 
    the same attribute inside a start-tag.

    GErrors constructed by g_set_error () don't receive a specific GQuark value since there is no differentiation, 
    the value is always set to one.

*/

static void start_tags_their_attr_and_val__empty_element_tags (GMarkupParseContext  *parse_context, 
                                                               const gchar          *element_name,
                                                               const gchar         **attribute_names,
                                                               const gchar         **attribute_values,
                                                               gpointer              menu_building_pnt,
                                                               GError              **error) {
    MenuBuildingData *menu_building = (MenuBuildingData *) menu_building_pnt; 

    // XML after the root menu is discarded by Openbox.
    if (menu_building->root_menu_finished || streq (element_name, "openbox_menu")) {
        return;
    }

    const gchar *current_attribute_name;
    const gchar *current_attribute_value;
    guint number_of_attributes = g_strv_length ((gchar **) attribute_names);
    GSList **menu_ids = &(menu_building->menu_ids);
    guint current_path_depth = menu_building->current_path_depth;
    gchar *current_action = menu_building->current_elements[ACTION];
    gint line_number = menu_building->line_number;
    guint8 *loading_stage = &(menu_building->loading_stage);
    GSList **toplevel_menu_ids = menu_building->toplevel_menu_ids;

    gchar *valid;

    // This is a local variant of txt_fields for constant values.
    const gchar *txt_fields[NUMBER_OF_TXT_FIELDS] = { NULL }; // Defaults

    gboolean menu_id_found = FALSE; // Default
    const gchar *menu_label = NULL;

    // Defaults = no icon
    GdkPixbuf *icon_img = NULL;
    guint8 icon_img_status = NONE_OR_NORMAL;
    gchar *icon_modification_time = NULL;
    gchar *icon_path = NULL;

    guint8 attribute_cnt, attribute_cnt2, ts_build_cnt;


    // --- Error checking ---


    // Check if current element is a valid child of another element.

    if (current_path_depth > 1) {
        const GSList *element_stack = g_markup_parse_context_get_element_stack (parse_context);
        const gchar *parent_element_name = (g_slist_next (element_stack))->data;
        gchar *error_txt = NULL;

        if (G_UNLIKELY (streq (element_name, "menu") && !streq (parent_element_name, "menu"))) {
            error_txt = "A menu can only have another menu";
        }
        else if (G_UNLIKELY (streq (element_name, "item") && !streq (parent_element_name, "menu"))) {
            error_txt = "An item can only have a menu";
        }
        else if (G_UNLIKELY (streq (element_name, "separator") && !streq (parent_element_name, "menu"))) {
            error_txt = "A separator can only have a menu";
        }
        else if (G_UNLIKELY (streq (element_name, "action") && !streq (parent_element_name, "item"))) {
            error_txt = "An action can only have an item";
        }
        else if (G_UNLIKELY (streq (element_name, "prompt") && !(streq (parent_element_name, "action") && 
                             streq_any (current_action, "Execute", "Exit", "SessionLogout", NULL)))) {
            error_txt = "A 'prompt' option can only have an 'Execute', 'Exit' or 'SessionLogout' action";
        }
        else if (G_UNLIKELY (streq (element_name, "command") && !(streq (parent_element_name, "action") && 
                             streq_any (current_action, "Execute", "Restart", NULL)))) {
            error_txt = "A 'command' option can only have an 'Execute' or 'Restart' action";
        }
        else if (G_UNLIKELY (streq (element_name, "startupnotify") && !(streq (parent_element_name, "action") && 
                             streq (current_action, "Execute")))) {
            error_txt = "A 'startupnotify' option can only have an 'Execute' action";
        }
        else if (G_UNLIKELY (streq_any (element_name, "enabled", "icon", "name", "wmclass", NULL) && 
                             !streq (parent_element_name, "startupnotify"))) {
            g_set_error (error, 1, line_number, "A%s '%s' option can only have a 'startupnotify' option as a parent", 
                         (streq_any (element_name, "enabled", "icon", NULL)) ? "n" : "", element_name);
            return;
        }
        else if (G_UNLIKELY (streq (menu_building->previous_type, "pipe menu") && 
                             menu_building->previous_path_depth < current_path_depth)) {
            error_txt = "A pipe menu is a self-closing element; it can't be used";
        }  

        if (G_UNLIKELY (error_txt)) {
            g_set_error (error, 1, line_number, "%s as a parent", error_txt);
            return;
        }
    }

    // Too many attributes

    if (G_UNLIKELY ((streq (element_name, "menu") && number_of_attributes > 4) || 
                    (streq (element_name, "item") && number_of_attributes > 2) || 
                    (streq_any (element_name, "separator", "action", NULL) && number_of_attributes > 1))) {
        g_set_error (error, 1, line_number, "Too many attributes for element '%s'", element_name);
        return;
    }

    for (attribute_cnt = 0; attribute_cnt < number_of_attributes; attribute_cnt++) {
        current_attribute_name = attribute_names[attribute_cnt];
        current_attribute_value = attribute_values[attribute_cnt];

        // Duplicate attributes

        for (attribute_cnt2 = 0; attribute_cnt2 < number_of_attributes; attribute_cnt2++) {
            if (attribute_cnt == attribute_cnt2) {
                continue;
            }
            if (G_UNLIKELY (streq (current_attribute_name, attribute_names[attribute_cnt2]))) {
                g_set_error (error, 1, line_number, "Element '%s' has more than one '%s' attribute", 
                             element_name, current_attribute_name);
                return;
            }
        }

        // Invalid attributes

        if (G_UNLIKELY ((streq (element_name, "menu") && 
                        !streq_any (current_attribute_name, "id", "label", "icon", "execute", NULL)) || 
                        (streq (element_name, "item") && !streq_any (current_attribute_name, "label", "icon", NULL)) || 
                        (streq (element_name, "separator") && !streq (current_attribute_name, "label")) || 
                        (streq (element_name, "action") && !streq (current_attribute_name, "name")))) {

            if (streq (element_name, "menu")) {
                valid = "are 'id', 'label', 'icon' and 'execute'";
            }
            else if (streq (element_name, "item")) {
                valid = "are 'label' and 'icon'";
            }
            else if (streq (element_name, "separator")) {
                valid = "is 'label'";
            }
            else { // action
                valid = "is 'name'";
            }
            g_set_error (error, 1, line_number, "Element '%s' has an invalid attribute '%s', valid %s", 
                         element_name, current_attribute_name, valid);
            return;
        }

        if (streq (element_name, "menu")) {

            // Check if a found menu ID fulfills the necessary conditions.

            if (streq (current_attribute_name, "id")) {
                menu_id_found = TRUE;

                // Check if the ID is a root menu ID, if so, whether it's at toplevel.

                if (streq (current_attribute_value, "root-menu")) {
                    if (G_UNLIKELY (current_path_depth > 1)) {
                        g_set_error (error, 1, line_number, "%s", 
                                     "The root menu can't be defined as a child of another menu");
                        return;
                    }

                    *loading_stage = ROOT_MENU;
                }

                // Check if the ID is unique.

                if (G_UNLIKELY (g_slist_find_custom (*menu_ids, current_attribute_value, (GCompareFunc) strcmp) &&
                                !(*loading_stage == ROOT_MENU && current_path_depth == 2 && 
                                g_slist_find_custom (toplevel_menu_ids[MENUS], current_attribute_value, 
                                    (GCompareFunc) strcmp) && 
                                !g_slist_find_custom (toplevel_menu_ids[ROOT_MENU], current_attribute_value, 
                                    (GCompareFunc) strcmp)))) {
                    g_set_error (error, 1, line_number, 
                                "'%s' is a menu ID that has already been defined before", current_attribute_value);
                    return;
                }
            }
            else if (streq (current_attribute_name, "label")) {
                menu_label = current_attribute_value;
            }
        }
    }

    // Missing or duplicate menu IDs

    if (G_UNLIKELY (streq (element_name, "menu") && !menu_id_found)) {
        g_set_error (error, 1, line_number, "Menu%s%s%s has no 'id' attribute", 
                     (menu_label) ? " '" : "",  (menu_label) ? menu_label : "", (menu_label) ? "'" : "");
        return;
    }


    // --- Retrieve attribute values


    if (streq_any (element_name, "menu", "item", "separator", NULL)) {
        txt_fields[TYPE_TXT] = element_name;

        for (attribute_cnt = 0; attribute_cnt < number_of_attributes; attribute_cnt++) {
            current_attribute_name = attribute_names[attribute_cnt];
            current_attribute_value = attribute_values[attribute_cnt];

            if (streq (current_attribute_name, "label")) {
                txt_fields[MENU_ELEMENT_TXT] = current_attribute_value;
                if (streq (element_name, "item")) {
                    free_and_reassign (menu_building->current_elements[ITEM], g_strdup (current_attribute_value));
                }
            }
            if (!streq (element_name, "separator")) { // menu or item
                if (streq (current_attribute_name, "icon")) {
                    icon_path = g_strdup (current_attribute_value);
                }
                if (streq (element_name, "menu")) {
                    if (streq (current_attribute_name, "id")) {
                        /*
                            Root menu IDs are only included inside the menu_ids list 
                            if they did not already appear in an extern menu definition.
                        */
                        if (!(*loading_stage == ROOT_MENU && 
                            (streq (current_attribute_value, "root-menu") || 
                             g_slist_find_custom (toplevel_menu_ids[MENUS], current_attribute_value, (GCompareFunc) strcmp)))) {
                            *menu_ids = g_slist_prepend (*menu_ids, g_strdup (current_attribute_value));
                        }

                        if ((*loading_stage == MENUS && current_path_depth == 1) || 
                            (*loading_stage == ROOT_MENU && current_path_depth == 2)) { // This excludes the "root-menu" id.
                            // TRUE -> ROOT_MENU, FALSE -> MENUS
                            GSList **menu_ids_list = &toplevel_menu_ids[(*loading_stage == ROOT_MENU)];

                            *menu_ids_list = g_slist_prepend (*menu_ids_list, g_strdup (current_attribute_value));
                        }
 
                        txt_fields[MENU_ID_TXT] = current_attribute_value;
                    }
                    else if (streq (current_attribute_name, "execute")) {
                        txt_fields[TYPE_TXT] = "pipe menu"; // Overwrites "menu".
                        txt_fields[EXECUTE_TXT] = current_attribute_value;
                    }
                }
            }
        }

        /*
            Create icon images.

            This has to follow after the retrieval loop, because if icon was the first attribute, 
            the other attributes would not have been retrieved yet; the latter are needed in case of an error 
            to compose an error message that informs about the ID (menu) or label (item), 
            so the element can be identified by the user.
        */

        if (icon_path) {
            GdkPixbuf *icon_in_original_size;

            GError *icon_creation_error = NULL;
            gboolean *ignore_all_upcoming_errs = &(menu_building->ignore_all_upcoming_errs);

            GtkWidget *dialog;
            GString *dialog_txt;

            gchar *icon_path_error_loop, *icon_path_selected;

            gint result;

            if (G_UNLIKELY (!(icon_in_original_size = gdk_pixbuf_new_from_file (icon_path, &icon_creation_error)))) {
                gboolean file_exists = g_file_test (icon_path, G_FILE_TEST_EXISTS);
                icon_img = gdk_pixbuf_copy (invalid_icon_imgs[(file_exists)]); // INVALID_FILE_ICON (TRUE) or INVALID_PATH_ICON
                icon_img_status = (file_exists) ? INVALID_FILE : INVALID_PATH;
                if (file_exists) {
                    icon_modification_time = get_modification_time_for_icon (icon_path);
                }

                if (*ignore_all_upcoming_errs) {
                    g_error_free (icon_creation_error);
                }
                else {
                    enum { CHOOSE_FILE = 1, CHECK_LATER, IGNORE_ALL_UPCOMING_ERRORS_AND_CHECK_LATER };

                    icon_path_error_loop = g_strdup (icon_path);
                    while (icon_creation_error) {
                        dialog_txt = g_string_new ("");
                        g_string_append_printf (dialog_txt, "<b>Line %i:\nThe following error occurred " 
                                                "while trying to create an icon for %s %s with ", line_number, 
                                                (txt_fields[MENU_ELEMENT_TXT]) ? 
                                                "the" : ((streq (txt_fields[TYPE_TXT], "menu")) ? "a" : "an"), 
                                                txt_fields[TYPE_TXT]);
                        if (streq_any (txt_fields[TYPE_TXT], "menu", "pipe menu", NULL)) {
                            g_string_append_printf (dialog_txt, "the menu ID '%s'", txt_fields[MENU_ID_TXT]);
                        }
                        else if (txt_fields[MENU_ELEMENT_TXT]) {
                            g_string_append_printf (dialog_txt, "the label '%s'", txt_fields[MENU_ELEMENT_TXT]);
                        }
                        else {
                            g_string_append (dialog_txt, "no assigned label");
                        }
                        g_string_append_printf (dialog_txt, " from '%s':\n\n<span foreground='#8a1515'>%s</span></b>\n\n"
                                                            "If you don't want to choose the correct/another file now, "
                                                            "you may ignore this or all following icon creation "
                                                            "error messages (closing this dialog window causes the latter) now "
                                                            "and check later from inside the program. "
                                                            "In this case all nodes that contain menus and items "
                                                            "with invalid icon paths will be shown expanded "
                                                            "after the loading process.", 
                                                            icon_path_error_loop, icon_creation_error->message);

                        create_dialog (&dialog, "Icon creation error", "dialog-error", dialog_txt->str, 
                                       "Choose file", "Check later", "Ignore all errors and check later", TRUE);

                        // Cleanup
                        g_string_free (dialog_txt, TRUE);

                        result = gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                        switch (result) {
                            case CHOOSE_FILE:
                                if ((icon_path_selected = choose_icon ())) {
                                    // Cleanup and reset
                                    g_error_free (icon_creation_error);
                                    icon_creation_error = NULL;
                                    free_and_reassign (icon_path_error_loop, icon_path_selected);

                                    if (G_LIKELY ((icon_in_original_size = gdk_pixbuf_new_from_file (icon_path_error_loop, 
                                                                                                     &icon_creation_error)))) {
                                        free_and_reassign (icon_path, g_strdup (icon_path_error_loop));
                                        g_object_unref (icon_img);
                                        icon_img_status = NONE_OR_NORMAL;
                                        menu_building->change_done_loading_proc = TRUE;
                                    }
                                }
                                break;
                            default: // If the dialog window is closed all upcoming errors will be ignored.
                                *ignore_all_upcoming_errs = (result != CHECK_LATER);

                                // Cleanup and reset
                                g_error_free (icon_creation_error);
                                icon_creation_error = NULL;
                        }
                    }
                    // Cleanup
                    g_free (icon_path_error_loop);
                }
            }

            if (G_LIKELY (!icon_img_status)) {
                icon_img = gdk_pixbuf_scale_simple (icon_in_original_size, font_size + 10, font_size + 10, GDK_INTERP_BILINEAR);
                free_and_reassign (icon_modification_time, get_modification_time_for_icon (icon_path));

                // Cleanup
                g_object_unref (icon_in_original_size);
            }
        }
    }
    else if (streq (element_name, "action")) {
        txt_fields[TYPE_TXT] = element_name;
        txt_fields[MENU_ELEMENT_TXT] = attribute_values[0]; // There is only one attribute.
        free_and_reassign (menu_building->current_elements[ACTION], g_strdup (attribute_values[0]));
    }
    else if (g_regex_match_simple ("prompt|command|execute|startupnotify|enabled|wmclass|name|icon", 
            element_name, G_REGEX_ANCHORED, 0)) {
        if (!streq (element_name, "startupnotify")) {
            txt_fields[TYPE_TXT] = "option";
            menu_building->open_option_element = TRUE;
        }
        else  {
            txt_fields[TYPE_TXT] = "option block";
        }
        txt_fields[MENU_ELEMENT_TXT] = element_name;
        if (G_UNLIKELY (streq (txt_fields[MENU_ELEMENT_TXT], "execute"))) {
            txt_fields[MENU_ELEMENT_TXT] = "command";
            menu_building->deprecated_exe_cmds_cvtd = TRUE;
        }
    }

    // --- Store all values that are needed later to create a treeview row. ---


    gpointer *tree_data = g_slice_alloc (sizeof (gpointer) * NUMBER_OF_TS_BUILD_FIELDS);

    tree_data[TS_BUILD_ICON_IMG] = icon_img;
    tree_data[TS_BUILD_ICON_IMG_STATUS] = GUINT_TO_POINTER (icon_img_status);
    tree_data[TS_BUILD_ICON_MODIFICATION_TIME] = icon_modification_time;
    tree_data[TS_BUILD_ICON_PATH] = icon_path;
    /*
        txt_fields starts with ICON_PATH_TXT, but this array element is not used here and replaced by icon_path, 
        since there are separate variables here for all treestore fields that refer to an icon image.
    */
    for (ts_build_cnt = TS_BUILD_MENU_ELEMENT; ts_build_cnt < TS_BUILD_PATH_DEPTH; ts_build_cnt++) {
        tree_data[ts_build_cnt] = g_strdup (txt_fields[ts_build_cnt - 3]);
    }
    tree_data[TS_BUILD_PATH_DEPTH] = GUINT_TO_POINTER (current_path_depth);

    menu_building->tree_build = g_slist_prepend (menu_building->tree_build, tree_data);


    // --- Preparations for further processing ---


    menu_building->previous_path_depth = current_path_depth;
    free_and_reassign (menu_building->previous_type, g_strdup (txt_fields[TYPE_TXT]));

    if (current_path_depth > menu_building->max_path_depth) {
        menu_building->max_path_depth = current_path_depth;
    }
    (menu_building->current_path_depth)++;
}

/* 

    Parses end elements.

*/

static void end_tags (G_GNUC_UNUSED GMarkupParseContext  *parse_context, 
                      G_GNUC_UNUSED const gchar          *element_name,
                                    gpointer              menu_building_pnt, 
                      G_GNUC_UNUSED GError              **error)
{
    MenuBuildingData *menu_building = (MenuBuildingData *) menu_building_pnt;

    if (menu_building->root_menu_finished) { // XML after the root menu is discarded by Openbox.
        return;
    }

    if (streq_any (element_name, "item", "action", NULL)) {
        free_and_reassign (menu_building->current_elements[streq (element_name, "action")], NULL);
    }
    guint current_path_depth = --(menu_building->current_path_depth);

    if (menu_building->loading_stage == ROOT_MENU && current_path_depth == 1) {
        menu_building->root_menu_finished = TRUE;
    }
}

/* 

    Parses text from inside an element.

*/

static void element_content (G_GNUC_UNUSED GMarkupParseContext  *parse_context, 
                                           const gchar          *text, 
                             G_GNUC_UNUSED gsize                 text_len, 
                                           gpointer              menu_building_pnt, 
                             G_GNUC_UNUSED GError              **error)
{
    MenuBuildingData *menu_building = (MenuBuildingData *) menu_building_pnt;

    // XML after the root menu is discarded by Openbox.
    if (!menu_building->tree_build || menu_building->root_menu_finished) {
        return;
    }

    gpointer *tree_data = menu_building->tree_build->data;

    if (menu_building->open_option_element) {
        gchar *nul_terminated_text = g_strndup (text, text_len);
        gchar *current_element = tree_data[TS_BUILD_MENU_ELEMENT];
        gchar *current_action = menu_building->current_elements[ACTION];

        /*
            Avoids that spaces and returns are taken over in cases like
            <command>
                some_program
            </command>

            This will be written back as <command>some_program</command>
        */
        if (streq (tree_data[TS_BUILD_TYPE], "option")) {
            g_strstrip (nul_terminated_text);
        }

        if (G_LIKELY (!((streq (current_element, "enabled") || 
                        (streq (current_element, "prompt") && 
                        streq_any (current_action, "Exit", "SessionLogout", NULL))) && 
                        !streq_any (nul_terminated_text, "yes", "no", NULL)))) {
            tree_data[TS_BUILD_VALUE] = nul_terminated_text;
        }
        else {
            gchar *current_item = menu_building->current_elements[ITEM];

            GtkWidget *dialog;
            gchar *dialog_title_txt, *dialog_txt, *line_with_escaped_markup_txt;
            gint result;

            #define YES 1

            dialog_title_txt = g_strconcat (streq (current_element, "enabled") ? "Enabled" : "Prompt", 
                                            " option has invalid value", NULL);
            dialog_txt = g_strdup_printf ("<b>Line %i:</b>\n<tt>%s</tt>\n\n" 
                                          "An item %s%s%scontains a%s <b>'%s' action</b> "
                                          "that has a%s <b>'%s' option</b> with the <b>invalid value "
                                          "<span foreground='#8a1515'>%s</span></b>."
                                          "\n\nPlease choose <b>either 'yes' or 'no'</b> for the option "
                                          "(Closing this dialog window sets value to 'no').", 

                                          menu_building->line_number, 
                                          g_strstrip (line_with_escaped_markup_txt = 
                                              g_markup_escape_text (menu_building->line, -1)),
                                          (current_item) ? "labeled <b>'" : "", 
                                          (current_item) ? (current_item) : "", 
                                          (current_item) ? "'</b> " : "", 
                                          (streq (current_action, "SessionLogout")) ? "" : "n", 
                                          current_action, 
                                          (streq (current_element, "enabled")) ? "n" : "", 
                                          current_element, 
                                          nul_terminated_text);

            create_dialog (&dialog, dialog_title_txt, "dialog-error", dialog_txt, "yes", "no", NULL, TRUE);

            // Cleanup
            g_free (dialog_title_txt);
            g_free (dialog_txt);
            g_free (line_with_escaped_markup_txt);

            result = gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
            tree_data[TS_BUILD_VALUE] = g_strdup ((result == YES) ? "yes" : "no");
            menu_building->change_done_loading_proc = TRUE;
        }
        menu_building->open_option_element = FALSE;
    }
}

/* 

    Sets visibility value of menus, pipe menus, items and separators and adds menus and items without labels to a list.

*/

static gboolean elements_visibility (GtkTreeModel  *foreach_model,
                                     GtkTreePath   *foreach_path,
                                     GtkTreeIter   *foreach_iter,
                                     GSList       **invisible_elements_lists)
{
    gchar *type_txt, *element_visibility_txt;

    gtk_tree_model_get (foreach_model, foreach_iter, 
                        TS_TYPE, &type_txt, 
                        TS_ELEMENT_VISIBILITY, &element_visibility_txt, -1);

    if (streq_any (type_txt, "action", "option", "option block", NULL) || streq (element_visibility_txt, "visible")) {
        // Cleanup
        g_free (type_txt);
        g_free (element_visibility_txt);

        return FALSE;
    }

    guint8 element_visibility_ancestor = check_if_invisible_ancestor_exists (foreach_model, foreach_path);
    gchar *new_element_visibility_txt;
    gchar *menu_element_txt;

    gtk_tree_model_get (foreach_model, foreach_iter, TS_MENU_ELEMENT, &menu_element_txt, -1);

    if (!streq (element_visibility_txt, "invisible orphaned menu")) {
        if (element_visibility_ancestor == INVISIBLE_ORPHANED_ANCESTOR) {
            new_element_visibility_txt = "invisible dsct. of invisible orphaned menu";
        }
        else {
            if (element_visibility_ancestor) {
                new_element_visibility_txt = "invisible dsct. of invisible menu";
            }
            else {
                if (menu_element_txt || streq (type_txt, "separator")) {
                    new_element_visibility_txt = "visible";
                }
                else {
                    new_element_visibility_txt = (streq (type_txt, "item")) ? "invisible item" : "invisible menu";
                }
            }
        }

        gtk_tree_store_set (treestore, foreach_iter, TS_ELEMENT_VISIBILITY, new_element_visibility_txt, -1);
    }

    /*
        If the function is called from the "Missing Labels" dialog, the invisible elements lists were already built in a 
        previous call of this function. In this case, NULL instead of the invisible elements lists has been sent as 
        a parameter, so the following if/else statement is not executed.
    */
    if (invisible_elements_lists && !menu_element_txt && !streq (type_txt, "separator")) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gint current_path_depth = gtk_tree_path_get_depth (foreach_path);
        GSList **label_list = &invisible_elements_lists[(streq (type_txt, "item")) ? ITEMS_LIST : MENUS_LIST];
        gchar *list_elm_txt = NULL; // Default, symbolises a toplevel item without label.

        if (!streq (type_txt, "item")) {
            gtk_tree_model_get (foreach_model, foreach_iter, TS_MENU_ID, &list_elm_txt, -1);
        }
        else if (current_path_depth > 1) {
            GtkTreeIter iter_ancestor;
            gchar *menu_id_txt_ancestor;

            gtk_tree_model_iter_parent (foreach_model, &iter_ancestor, foreach_iter);
            gtk_tree_model_get (foreach_model, &iter_ancestor, TS_MENU_ID, &menu_id_txt_ancestor, -1);

            list_elm_txt = g_strdup_printf ("Child of menu with id '%s'", menu_id_txt_ancestor);

            // Cleanup
            g_free (menu_id_txt_ancestor);
        }
 
        *label_list = g_slist_prepend (*label_list, list_elm_txt);

        // To select an iter, the equivalent row has to be visible.
        if (current_path_depth > 1 &&
            !gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), foreach_path)) {
             gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), foreach_path);
        }
        gtk_tree_selection_select_iter (selection, foreach_iter);
        
    }

    // Cleanup
    g_free (menu_element_txt);
    g_free (type_txt);
    g_free (element_visibility_txt);

    return FALSE;
}

/* 

    Creates dialogs that ask about the handling of invisible menus and items.

*/

static void create_dialogs_for_invisible_menus_and_items (guint8             dialog_type, 
                                                          GtkTreeSelection  *selection, 
                                                          GSList           **invisible_elements_lists)
{
    GtkWidget *dialog, *content_area;
    gchar *dialog_title_txt, *dialog_headline_txt, *dialog_txt;
    gchar *dialog_txt_status_change1 = " are not shown by Openbox. ";
    gchar *dialog_txt_status_change2 = g_strdup_printf (" If you choose '<b>Keep status</b>' now, "
                                                        "you can still change their status from inside the program. "
                                                        "Select the menus%s and either choose "
                                                        "'<b>Edit</b> -> <b>Visualise</b>/<b>Visualise recursively</b>' "
                                                        "from the menu bar or rightclick them and choose "
                                                        "'<b>Visualise</b>/<b>Visualise recursively</b>' "
                                                        "from the context menu.\n\n", 
                                                        (dialog_type == ORPHANED_MENUS) ? "" : "/items");

    gint result;

    GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

    GList *selected_rows;

    GtkWidget *menu_grid;
    gchar *cell_txt;
    gchar *plural;

    guint  menus_list_len, items_list_len;
    guint *lists_len[] = { &menus_list_len, &items_list_len };
    guint  number_of_toplvl_items_without_label = 0;

    guint grid_row_cnt = 0;
    guint8 grid_column_cnt, index, lists_cnt;

    GList *selected_rows_loop;
    GSList *invisible_elements_lists_loop[2], *invisible_elements_lists_subloop;
    GtkTreeIter iter_loop;
    gboolean valid;

    gchar *menu_element_txt_loop, *menu_id_txt_loop, *element_visibility_txt_loop;

    enum { VISUALISE = 1, KEEP_STATUS, DELETE };

    // Preliminary work - Creating dialog title and headline, lists for the orphaned menus dialog.

    if (dialog_type == ORPHANED_MENUS) {
        guint menu_ids_list_len;

         // Create menu element and menu ID lists. 
        valid = gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, gtk_tree_model_iter_n_children (model, NULL) - 1);
        while (valid) {
            gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);

            if (!streq (element_visibility_txt_loop, "invisible orphaned menu")) {
                // Cleanup
                g_free (element_visibility_txt_loop);
                break;
            }

            gtk_tree_model_get (model, &iter_loop, 
                                TS_MENU_ELEMENT, &menu_element_txt_loop,
                                TS_MENU_ID, &menu_id_txt_loop,
                                -1);

            invisible_elements_lists[MENU_IDS_LIST] = g_slist_prepend (invisible_elements_lists[MENU_IDS_LIST], 
                                                                       menu_id_txt_loop);
            invisible_elements_lists[MENU_ELEMENTS_LIST] = g_slist_prepend (invisible_elements_lists[MENU_ELEMENTS_LIST], 
                                                                            menu_element_txt_loop);

            gtk_tree_selection_select_iter (selection, &iter_loop);
            valid = gtk_tree_model_iter_previous (model, &iter_loop);

            // Cleanup
            g_free (element_visibility_txt_loop);
        }

        invisible_elements_lists_loop[0] = invisible_elements_lists[MENU_IDS_LIST];
        invisible_elements_lists_loop[1] = invisible_elements_lists[MENU_ELEMENTS_LIST];

        // Create dialog title and headline.
        menu_ids_list_len = g_slist_length (menu_ids);
        plural = (menu_ids_list_len == 1) ? "" : "s";

        dialog_title_txt = g_strdup_printf ("Invisible orphaned menu%s found", plural);
        dialog_headline_txt = g_strdup_printf ("The following menu%s %s defined outside the root menu, "
                                               "but <b>%s used inside it</b>:", plural, 
                                               (menu_ids_list_len == 1) ? "is" : "are", 
                                               (menu_ids_list_len == 1) ? "isn't" : "aren't");
    }
    else { // dialog_type == MISSING_LABELS
        // Create dialog title and headline
        menus_list_len = g_slist_length (invisible_elements_lists[MENUS_LIST]);
        items_list_len = g_slist_length (invisible_elements_lists[ITEMS_LIST]);

        gchar *menus_items_txt = g_strconcat ((menus_list_len > 0) ? "Menu" : "", 
                                              (menus_list_len <= 1) ? "" : "s", 
                                              (menus_list_len > 0 && items_list_len > 0) ? " and " : "", 
                                              (menus_list_len == 0) ? "I" : ((items_list_len > 0) ? "i" : ""), 
                                              (items_list_len > 0) ? "tem" : "", 
                                              (items_list_len <= 1) ? "" : "s", 
                                              NULL);
        gchar *menus_items_txt_lower_case;

        dialog_title_txt = g_strdup_printf ("%s without label found", menus_items_txt);
        menus_items_txt_lower_case = g_ascii_strdown (menus_items_txt, -1);
        dialog_headline_txt = g_strdup_printf ("The following %s %s <b>no label</b>:", 
                                               menus_items_txt_lower_case, 
                                               ((menus_list_len == 1 && items_list_len == 0) || 
                                               (menus_list_len == 0 && items_list_len == 1)) ? "has" : "have"); 

        // Cleanup
        g_free (menus_items_txt);
        g_free (menus_items_txt_lower_case);
    }

    // Create menu grid that contains the list(s) of invisible menu elements.

    menu_grid = gtk_grid_new ();

    for (lists_cnt = 0; lists_cnt <= dialog_type; lists_cnt++) {
        if (dialog_type == MISSING_LABELS) {
            if (invisible_elements_lists[lists_cnt]) {
                plural = (*(lists_len[lists_cnt]) > 1) ? "s" : "";

                // Display headlines for the missing labels lists.
                invisible_elements_lists[lists_cnt] = g_slist_reverse (invisible_elements_lists[lists_cnt]);

                cell_txt = g_strdup_printf ("\n<b>%s%s:</b> (%s%s shown)\n", 
                                            (lists_cnt == MENUS_LIST) ? "Menu" : "Item", plural, 
                                            (lists_cnt == MENUS_LIST) ? "Menu ID" : "Location", plural);
                gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt, FALSE), 0, grid_row_cnt++, 1, 1);

                // Cleanup
                g_free (cell_txt);

                // Display the number of toplevel items without label.
                if (lists_cnt == ITEMS_LIST) {
                    for (invisible_elements_lists_subloop = invisible_elements_lists[ITEMS_LIST]; 
                         invisible_elements_lists_subloop; 
                         invisible_elements_lists_subloop = invisible_elements_lists_subloop->next) {
                        if (!invisible_elements_lists_subloop->data) {
                            number_of_toplvl_items_without_label++;
                        }
                    }
 
                    if (number_of_toplvl_items_without_label) {
                        /*
                            If the number of toplevel items without label is below 10, 
                            the number is replaced with its spelled out equivalent so 
                            the whole sentence looks less machinery processed.
                        */
                        gchar *small_numbers_spelled_out[] = { "One", "Two", "Three", "Four", "Five", 
                                                               "Six", "Seven", "Eight", "Nine" };

                        gchar *toplvl_items_str = (number_of_toplvl_items_without_label < 10) ? "%s%s%s%s" : "%i%s%s%s";
                        cell_txt = g_strdup_printf (toplvl_items_str,
                                                    (number_of_toplvl_items_without_label < 10) ? 
                                                    small_numbers_spelled_out[number_of_toplvl_items_without_label - 1] : 
                                                    GUINT_TO_POINTER (number_of_toplvl_items_without_label), 
                                                    " toplevel item", 
                                                    (number_of_toplvl_items_without_label > 1) ? "s" : "", 
                                                    (number_of_toplvl_items_without_label == items_list_len) ? "\n" : "");

                        gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt, FALSE), 0, grid_row_cnt++, 1, 1);

                        // Cleanup
                        g_free (cell_txt);
                        // The number of toplevel items w/o label has been displayed, so these labels can be removed from the list.
                        invisible_elements_lists[lists_cnt] = g_slist_remove_all (invisible_elements_lists[ITEMS_LIST], NULL);
                    }
                }
            }
            invisible_elements_lists_loop[lists_cnt] = invisible_elements_lists[lists_cnt];
        }
        // Display the orphaned menus and missing labels lists.
        while (invisible_elements_lists_loop[lists_cnt]) {
            for (grid_column_cnt = 0; grid_column_cnt <= (dialog_type == ORPHANED_MENUS); grid_column_cnt++) {
                index = ((dialog_type == ORPHANED_MENUS && grid_column_cnt == 1) || 
                         (dialog_type == MISSING_LABELS && lists_cnt == 1));

                /*
                    The list construction loop covers both dialogs:
                    For the 
                    - ORPHANED_MENUS dialog it creates one list with two columns
                    - MISSING_LABELS     dialog it creates one or two lists with one column

                    If dialog_type == ORPHANED_MENUS, there are two columns (=two loop iterations): 
                    One for the menu ID and one for the label (if it doesn't exist, the column is left blank).
                    The loop is called once, if there are orphaned menus.

                    If dialog_type == MISSING_LABELS, there is one column (=one loop iteration).
                    The loop is called once, if there are either menus OR items without labels, 
                    or twice, if there are menus AND items without labels.
                    An additional new line is added to the end of the list of menus with missing labels if 
                    it is followed by a list of items with missing labels.
                */

                cell_txt = g_strdup_printf ("%s%s%s%s  ", (dialog_type == ORPHANED_MENUS && grid_row_cnt == 0) ? "\n" : "", 
                                            (dialog_type == MISSING_LABELS || 
                                            (grid_column_cnt == 1 && !invisible_elements_lists_loop[MENU_ELEMENTS_LIST]->data)) ? 
                                            "" : ((grid_column_cnt == 0) ? "<b>Menu ID:</b> " : 
                                            (g_slist_length (invisible_elements_lists[grid_column_cnt]) == 1) ? 
                                            " <b>Label:</b> " : "<b>Label:</b> "),  
                                            (dialog_type == ORPHANED_MENUS && 
                                            !invisible_elements_lists_loop[grid_column_cnt]->data) ? 
                                            "" : (gchar *) invisible_elements_lists_loop[index]->data, 
                                            (invisible_elements_lists_loop[index]->next || 
                                            (!invisible_elements_lists_loop[index]->next && dialog_type == MISSING_LABELS && 
                                            lists_cnt == MENUS_LIST && invisible_elements_lists[ITEMS_LIST])) ? "" : "\n");
                gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt, FALSE), 
                                 grid_column_cnt, grid_row_cnt, 1, 1);

                // Cleanup
                g_free (cell_txt);

                invisible_elements_lists_loop[index] = invisible_elements_lists_loop[index]->next;
            }
            grid_row_cnt++;
        }
    }

    content_area = create_dialog (&dialog, dialog_title_txt, "dialog-information", dialog_headline_txt, 
                                  "Visualise", "Keep status", "Delete", FALSE);

    // Add the rest of the dialog components and show the dialog.

#if GTK_CHECK_VERSION(3,8,0)
    gtk_container_add (GTK_CONTAINER (scrolled_window), menu_grid);
#else
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), menu_grid);
#endif

    gtk_container_add (GTK_CONTAINER (content_area), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
    gtk_container_add (GTK_CONTAINER (content_area), scrolled_window);
    gtk_container_add (GTK_CONTAINER (content_area), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

    dialog_txt = g_strdup_printf ((dialog_type == ORPHANED_MENUS) ? 
                                  "\n%sdefined outside the root menu that don't appear inside it%s"
                                  "To integrate them, %s%s"
                                  "Invisible orphaned menus%s%s%sblue%s.\n" : // dialog_type == MISSING_LABELS
                                  "\n%sand items without label%s"
                                  "They can be visualised via creating a label for each of them; to do so %s%s"
                                  "Menus and items without label%s%s%sgrey%s.\n", 

                                  "Menus ", 
                                  dialog_txt_status_change1, 
                                  "choose '<b>Visualise</b>' here.", 
                                  dialog_txt_status_change2, 
                                  " and their children are <b><span background='", 
                                  (dialog_type == ORPHANED_MENUS) ? "#364074" : "#656772", 
                                  "' foreground='white'>highlighted in ", 
                                  "</span></b>");

    gtk_container_add (GTK_CONTAINER (content_area), new_label_with_formattings (dialog_txt, TRUE));

    // Cleanup
    for (lists_cnt = 0; lists_cnt < NUMBER_OF_INVISIBLE_ELEMENTS_LISTS; lists_cnt++) {
        g_slist_free_full (invisible_elements_lists[lists_cnt], (GDestroyNotify) g_free);
        invisible_elements_lists[lists_cnt] = NULL;
    }
    g_free (dialog_title_txt);
    g_free (dialog_headline_txt);
    g_free (dialog_txt_status_change2);
    g_free (dialog_txt);

    gtk_widget_show_all (dialog);
    gtk_widget_set_size_request (scrolled_window, 570, MIN (gtk_widget_get_allocated_height (menu_grid), 125));

    result = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    switch (result) {
        case VISUALISE:
            selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

            for (selected_rows_loop = selected_rows; selected_rows_loop; selected_rows_loop = selected_rows_loop->next) {
                gtk_tree_model_get_iter (model, &iter_loop, selected_rows_loop->data);

                if (dialog_type == ORPHANED_MENUS) {
                    gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);

                    // The visibility of subrows is set after the function has been left.
                    gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, 
                                        (menu_element_txt_loop) ? "visible" : "invisible menu", 
                                        -1);

                    // Cleanup
                    g_free (menu_element_txt_loop);
                }
                else { // dialog_type == MISSING_LABELS
                    gtk_tree_store_set (treestore, &iter_loop, TS_MENU_ELEMENT, "= Newly created label =", -1);
                    gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);
                    // The visibility of subrows is set after the function has been left.
                    if (!g_str_has_suffix (element_visibility_txt_loop, "invisible orphaned menu")) {
                        gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, "visible", -1);
                    }

                    // Cleanup
                    g_free (element_visibility_txt_loop);
                }
            }

            change_done = TRUE;

            // Cleanup
            g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
            break;
        case KEEP_STATUS:
            break;
        case DELETE:
            remove_rows ("load menu");
            change_done = TRUE;
    }

    gtk_tree_selection_unselect_all (selection);
}

/* 

    Parses a menu file and appends collected elements to the tree view.

*/

void get_menu_elements_from_file (gchar *new_filename)
{
    FILE *file;

    if (!(file = fopen (new_filename, "r"))) {
        gchar *err_txt = g_strdup_printf ("<b>Could not open menu</b>\n<tt>%s</tt><b>!</b>", new_filename);
        show_errmsg (err_txt);

        // Cleanup
        g_free (err_txt);
        g_free (new_filename);

        return;
    }

    MenuBuildingData menu_building = {
        .line =                     NULL, 
        .line_number =              1, 
        .tree_build =               NULL, 
        .current_path_depth =       1, 
        .previous_path_depth =      1, 
        .max_path_depth =           1, 
        .menu_ids =                 NULL, 
        .toplevel_menu_ids =        { NULL }, 
        .ignore_all_upcoming_errs = FALSE, 
        .current_elements =         { NULL }, 
        .open_option_element =      FALSE, 
        .previous_type =            NULL, 
        .deprecated_exe_cmds_cvtd = FALSE, 
        .loading_stage =            MENUS, 
        .change_done_loading_proc = FALSE, 
        .root_menu_finished =       FALSE
    };

    gsize length; // Used for getline (); since menu_building.line is set to NULL, the value for length is ignored.

    // No passthrough and error functions -> NULL for both corresponding arguments.
    GMarkupParser parser = { start_tags_their_attr_and_val__empty_element_tags, end_tags, element_content, NULL, NULL };
    // No user data destroy notifier called when the parse context is freed -> NULL
    GMarkupParseContext *parse_context = g_markup_parse_context_new (&parser, 0, &menu_building, NULL);

    GError *error = NULL;

    guint8 ts_build_cnt;

    while (getline (&menu_building.line, &length, file) != -1) {
        if (G_UNLIKELY (!g_markup_parse_context_parse (parse_context, menu_building.line, 
                        strlen (menu_building.line), &error))) {
            gchar *part_of_err_msg_with_escaped_markup_txt, *pure_errmsg;
            GString *full_errmsg = g_string_new ("");

            /*
                Remove leading and trailing (incl. newline) whitespace from line and 
                escape all special characters so the markup is used properly.
            */
            part_of_err_msg_with_escaped_markup_txt = g_markup_escape_text (g_strstrip (menu_building.line), -1);
            g_string_append_printf (full_errmsg, "<b>Line %i:</b>\n<tt>%s</tt>\n\n", 
                                    menu_building.line_number, part_of_err_msg_with_escaped_markup_txt);

            // Cleanup
            g_free (part_of_err_msg_with_escaped_markup_txt);

            /*
                Since the line number of the error message provided by GLib is often imprecise, it is removed. 
                Instead, the line number provided by the program has already been added before.
                The character position is removed, too, because displaying the line number should be sufficient for 
                menus used in practice.
            */
            if (g_regex_match_simple ("Error", error->message, G_REGEX_ANCHORED, 0)) {
                // "Error on line 15 char 8: Element..." -> "Element..."
                pure_errmsg = extract_substring_via_regex (error->message, "(?<=: ).*"); // Regex with positive lookbehind.
            }
            else {
                pure_errmsg = g_strdup (error->message);
            }

            /*
                Escape the error message text so it is displayed correctly if it contains markup. 
                This program does not generate error messages that contain markup, but the GLib markup parser does.
                An example is "Document must begin with an element (e.g. <book>)".
            */
            part_of_err_msg_with_escaped_markup_txt = g_markup_escape_text (pure_errmsg, -1);

            g_string_append_printf (full_errmsg, "<b><span foreground='#8a1515'>%s!</span>\n\n"
                                                 "Please&#160;correct&#160;your&#160;menu&#160;file</b>\n<tt>%s</tt>\n"
                                                 "<b>before reloading it.</b>", part_of_err_msg_with_escaped_markup_txt, new_filename);

            // Cleanup
            g_free (pure_errmsg);
            g_free (part_of_err_msg_with_escaped_markup_txt);

            show_errmsg (full_errmsg->str);

            // Cleanup
            g_string_free (full_errmsg, TRUE);
            g_error_free (error);
            g_free (new_filename);
            g_slist_free_full (menu_building.menu_ids, (GDestroyNotify) g_free);

            goto parsing_abort;
        }
        (menu_building.line_number)++;
    }

    // --- Menu file loaded without erros, now (re)set global variables. ---


    clear_global_data ();
    change_done = menu_building.change_done_loading_proc;
    menu_ids = menu_building.menu_ids;
    set_filename_and_window_title (new_filename);


    // --- Fill treestore. ---


    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

    guint allocated_size = sizeof (GtkTreeIter) * menu_building.max_path_depth;
    GtkTreeIter *levels = g_slice_alloc (allocated_size);

    GSList *menus_and_items_with_inaccessible_icon_image = NULL;
    GSList *invisible_elements_lists[NUMBER_OF_INVISIBLE_ELEMENTS_LISTS] = { NULL };

    guint number_of_toplevel_menu_ids = 0;
    guint number_of_used_toplevel_root_menus = 0;

    gboolean root_menu_stage = FALSE;
    gboolean add_row = TRUE; // Default
    gboolean menu_or_item_or_separator_at_root_toplevel;

    guint current_level;
    gint row_number = 0;

    GSList *tree_build_loop;
    gpointer *tree_data;

    GSList *menus_and_items_with_inaccessible_icon_image_loop;
    GtkTreeIter iter_loop;
    GtkTreePath *path_loop;
    gboolean valid;

    gchar *type_txt_loop, *menu_id_txt_loop;

    menu_building.tree_build = g_slist_reverse (menu_building.tree_build);

    for (tree_build_loop = menu_building.tree_build; tree_build_loop; tree_build_loop = tree_build_loop->next) {
        tree_data = tree_build_loop->data;
        if (streq (tree_data[TS_BUILD_MENU_ID], "root-menu")) {
             root_menu_stage = TRUE;
 
            // Parentheses avoid gcc warning.
            if ((number_of_toplevel_menu_ids = gtk_tree_model_iter_n_children (model, NULL))) {
                GSList *menu_ids_of_root_menus_defined_outside_root_first = NULL;

                guint invisible_menu_outside_root_index;

                GtkTreeIter iter_swap;

                guint root_menus_cnt;

                GSList *menu_ids_of_root_menus_defined_outside_root_first_loop, *toplevel_menu_ids_root_menu_loop; 

                gchar *menu_element_txt_loop, *element_visibility_txt_loop;
                gchar *menu_id_txt_subloop;
                gchar *menu_id_of_root_menu_defined_outside_root_first_loop;

                /*

                    Generate a list that contains all toplevel menus defined first outside the root menu that are visible.

                    menu_building.toplevel_menu_ids[ROOT_MENU] is in reverse order, 
                    so looping through it and prepending all matches with the treeview to the new list 
                    menu_ids_of_root_menus_defined_outside_root_first results in a correct order for the latter.

                    menu_ids_of_root_menus_defined_outside_root_first is a subset of menu_building.toplevel_menu_ids[ROOT_MENU], 
                    if the number of root menus defined outside root first is smaller than the number of root menus, 
                    or it is identical, if both numbers are equal.

                    The outer loop has to iterate through menu_building.toplevel_menu_ids[ROOT_MENU] whilst the inner one 
                    has to iterate through the treeview, because if it would be done vice versa the order of 
                    menu_ids_of_root_menus_defined_outside_root_first would be according to the treeview, 
                    but it has to be according to the order of the root menu inside the menu file. 

                    Example menu:

                    <openbox_menu xmlns="http://openbox.org/3.4/menu">

                    // Only the menu definitions here have been added to the treeview so far.

                    <menu id="menu3" label="menu3" />
                    <menu id="menu5" label="menu5" />
                    <menu id="menu1" label="menu1" />
                    <menu id="menu2" label="menu2" />
                    <menu id="menu6" label="menu6" />

                    <menu id="root-menu" label="Openbox 3">

                    // This is the current position inside the menu file up to which its elements have been added to 
                    // the treeview yet, elements inside the root menu that have not been defined before haven't been 
                    // added yet; regarding this example these are the separator, item and menu4.
                    // The menus 1-4 are part of the menu_building.toplevel_menu_ids[ROOT_MENU] list.

                    <menu id="menu1" />
                    <separator />
                    item label="item" />
                    <menu id="menu2" />
                    <menu id="menu3" />
                    <menu id="menu4" label="menu4" />
                    </menu>

                    </openbox_menu>

                    menu_building.toplevel_menu_ids[ROOT_MENU] contains:

                    menu4
                    menu3
                    menu2
                    menu1

                    This is the reverse order in comparison to the root menu of the menu file.

                    menu_ids_of_root_menus_defined_outside_root_first will contain:

                    menu1
                    menu2
                    menu3

                    menu4 has not been defined outside first -> no part of menu_ids_of_root_menus_defined_outside_root_first.
                */

                for (toplevel_menu_ids_root_menu_loop = menu_building.toplevel_menu_ids[ROOT_MENU]; 
                     toplevel_menu_ids_root_menu_loop; 
                     toplevel_menu_ids_root_menu_loop = toplevel_menu_ids_root_menu_loop->next) {
                    valid = gtk_tree_model_get_iter_first (model, &iter_loop);
                    while (valid) {
                        gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
                        if (streq (toplevel_menu_ids_root_menu_loop->data, menu_id_txt_loop)) {
                            gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
                            gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, 
                                                (G_LIKELY (menu_element_txt_loop)) ? "visible" : "invisible menu", 
                                                -1);
                            menu_ids_of_root_menus_defined_outside_root_first = 
                                g_slist_prepend (menu_ids_of_root_menus_defined_outside_root_first, g_strdup (menu_id_txt_loop));

                            // Cleanup
                            g_free (menu_element_txt_loop);

                            break;
                        }
                        valid = gtk_tree_model_iter_next (model, &iter_loop);

                        // Cleanup
                        g_free (menu_id_txt_loop);
                    }
                }

                /*
                    Move menus that don't show up inside the root menu to the bottom, 
                    keeping their original order, and mark them as invisible. 

                    Example menu:

                    <openbox_menu xmlns="http://openbox.org/3.4/menu">

                    // Only the menu definitions here have been added to the treeview so far.

                    <menu id="menu3" label="menu3" />
                    <menu id="menu5" label="menu5" />
                    <menu id="menu1" label="menu1" />
                    <menu id="menu2" label="menu2" />
                    <menu id="menu6" label="menu6" />

                    <menu id="root-menu" label="Openbox 3">

                    // This is the current position inside the menu file up to which its elements have been added to 
                    // the treeview yet, elements inside the root menu that have not been defined before haven't been 
                    // added yet, regarding this example these are the separator, item and menu4.

                    <menu id="menu1" />
                    <separator />
                    <item label="item" />
                    <menu id="menu2" />
                    <menu id="menu3" />
                    <menu id="menu4" label="menu4" />
                    </menu>

                    </openbox_menu>

                    Treeview looks like this so far:

                    menu3
                    menu5
                    menu1
                    menu6
                    menu2

                    After the orphaned menus have been moved to the end:

                    menu3
                    menu1
                    menu2
                    menu5 (invisible orphaned menu, sorted to the end)
                    menu6                     ""
                */

                invisible_menu_outside_root_index = number_of_toplevel_menu_ids - 1;
                valid = gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, invisible_menu_outside_root_index);
                while (valid) {
                    gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);
                    if (G_UNLIKELY (!element_visibility_txt_loop)) {
                        gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, "invisible orphaned menu", -1);
                        gtk_tree_model_iter_nth_child (model, &iter_swap, NULL, invisible_menu_outside_root_index--);
                        gtk_tree_store_swap (treestore, &iter_loop, &iter_swap);
                        iter_loop = iter_swap;
                    }
                    valid = gtk_tree_model_iter_previous (model, &iter_loop);

                    // Cleanup
                    g_free (element_visibility_txt_loop);
                }

                /*
                    The order of the toplevel menus depends on the order inside the root menu, 
                    so the menus are sorted accordingly to it. 

                    Toplevel menus defined inside the root menu as well as toplevel items and separators have not been added yet, 
                    so the number of elements of the menu_ids_of_root_menus_defined_first_outside_root list 
                    is equal to the number of menus inside the root menu that are not invisible orphaned menus 
                    (=element visibilty "visible" or "invisible menu"). 

                    Example menu:

                    <openbox_menu xmlns="http://openbox.org/3.4/menu">

                    // Only the menu definitions here have been added to the treeview so far.

                    <menu id="menu3" label="menu3" />
                    <menu id="menu5" label="menu5" />
                    <menu id="menu1" label="menu1" />
                    <menu id="menu2" label="menu2" />
                    <menu id="menu6" label="menu6" />

                    <menu id="root-menu" label="Openbox 3">

                    // This is the current position inside the menu file up to which its elements have been added to 
                    // the treeview yet, elements inside the root menu that have not been defined before haven't been 
                    // added yet, regarding this example these are the separator, item and menu4.
                    // Since the menu processing hasn't gone beyond this position yet, the sorting of the toplevel menus is  
                    // done via using the menu_ids_of_root_menus_defined_outside_root_first list as reference.

                    <menu id="menu1" />
                    <separator />
                    <item label="item" />
                    <menu id="menu2" />
                    <menu id="menu3" />
                    <menu id="menu4" label="menu4" />
                    </menu>

                     </openbox_menu>

                    Treeview looks like this so far:

                    menu3 (order according to the menu definitions done prior to the root menu)
                    menu1                                ""            
                    menu2                                ""
                    menu5 (invisible orphaned menu)
                    menu6                                ""

                    After the sorting has been done:

                    menu1 (order according to root menu of the menu file)
                    menu2                    ""
                    menu3                    ""
                    menu5 (invisible orphaned menu)
                    menu6                    ""

                    Invisible orphaned menus are unaffected by the sorting.
                */

                gtk_tree_model_get_iter_first (model, &iter_loop);
                for (menu_ids_of_root_menus_defined_outside_root_first_loop = menu_ids_of_root_menus_defined_outside_root_first; 
                     menu_ids_of_root_menus_defined_outside_root_first_loop; 
                     menu_ids_of_root_menus_defined_outside_root_first_loop = 
                       menu_ids_of_root_menus_defined_outside_root_first_loop->next) {
                    gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
                    menu_id_of_root_menu_defined_outside_root_first_loop = 
                    menu_ids_of_root_menus_defined_outside_root_first_loop->data;
                    if (!streq (menu_id_of_root_menu_defined_outside_root_first_loop, menu_id_txt_loop)) {
                        for (root_menus_cnt = number_of_used_toplevel_root_menus + 1; // = 1 at first time.
                            root_menus_cnt <= invisible_menu_outside_root_index; 
                            root_menus_cnt++) {
                            gtk_tree_model_iter_nth_child (model, &iter_swap, NULL, root_menus_cnt);
                            gtk_tree_model_get (model, &iter_swap, TS_MENU_ID, &menu_id_txt_subloop, -1);
                            if (streq (menu_id_of_root_menu_defined_outside_root_first_loop, menu_id_txt_subloop)) {
                            // Cleanup
                                g_free (menu_id_txt_subloop);
                                break;
                            }
                            // Cleanup
                            g_free (menu_id_txt_subloop);
                        }
                        gtk_tree_store_swap (treestore, &iter_loop, &iter_swap);
                    }

                    gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, ++number_of_used_toplevel_root_menus);

                    // Cleanup
                    g_free (menu_id_txt_loop);
                }

                // Cleanup
                g_slist_free_full (menu_ids_of_root_menus_defined_outside_root_first, (GDestroyNotify) g_free);
            }

            continue; // Nothing to add for a "root-menu" menu ID. 
        }

        menu_or_item_or_separator_at_root_toplevel = FALSE; // Default

        type_txt_loop = tree_data[TS_BUILD_TYPE];

        current_level = GPOINTER_TO_UINT (tree_data[TS_BUILD_PATH_DEPTH]) - 1;

        if (root_menu_stage) {
            current_level--;
            if (current_level == 0) { // toplevel -> type_txt_loop is "menu", "pipe menu", "item" or "separator".
                menu_or_item_or_separator_at_root_toplevel = TRUE;
                /*
                    Toplevel root menus are only added if they have not yet been defined before, 
                    since the question whether to add a menu element or not only arises in this case, 
                    a default setting is only done here and not for every menu element.
                */
                add_row = TRUE; // Default
                if (streq (type_txt_loop, "menu")) {
                    /*
                        If the current row inside menu_building.tree_build is a menu with an icon, look for a corresponding 
                        toplevel menu inside the treeview (it will exist if it has already been defined outside the root menu),
                        and if one exists, add the icon data to this toplevel menu.
                    */
                    if (tree_data[TS_BUILD_ICON_IMG]) {
                        valid = gtk_tree_model_get_iter_first (model, &iter_loop);
                        while (valid) {
                            gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
                            if (streq (tree_data[TS_BUILD_MENU_ID], menu_id_txt_loop)) {
                                for (ts_build_cnt = 0; ts_build_cnt <= TS_BUILD_ICON_PATH; ts_build_cnt++) {
                                    gtk_tree_store_set (treestore, &iter_loop, ts_build_cnt, tree_data[ts_build_cnt], -1);
                                }

                                // Cleanup
                                g_free (menu_id_txt_loop);
                                break;
                            }
                            // Cleanup
                            g_free (menu_id_txt_loop);
          
                            valid = gtk_tree_model_iter_next (model, &iter_loop);
                        }
                    }

                    if (!tree_data[TS_BUILD_MENU_ELEMENT] && 
                        g_slist_find_custom (menu_building.toplevel_menu_ids[MENUS], 
                                             tree_data[TS_BUILD_MENU_ID], (GCompareFunc) strcmp)) {
                        add_row = FALSE; // Is a menu defined outside root that is already inside the treestore.
                    }
                }
            }
        }

        if (add_row) {
            GtkTreePath *path;

            gtk_tree_store_insert (treestore, &levels[current_level], 
                                   (current_level == 0) ? NULL : &levels[current_level - 1], 
                                   (menu_or_item_or_separator_at_root_toplevel) ? row_number : -1);

            iter = levels[current_level];
            path = gtk_tree_model_get_path (model, &iter);

            for (ts_build_cnt = 0; ts_build_cnt < TS_BUILD_PATH_DEPTH; ts_build_cnt++) {
                gtk_tree_store_set (treestore, &iter, ts_build_cnt, tree_data[ts_build_cnt], -1);
            }

            if (GPOINTER_TO_UINT (tree_data[TS_ICON_IMG_STATUS]) && gtk_tree_path_get_depth (path) > 1) {
                /*
                    Add a row reference of a path of a menu, pipe menu or item that has an invalid icon path or 
                    a path that points to a file that contains no valid image data.
                */
                menus_and_items_with_inaccessible_icon_image = g_slist_prepend (menus_and_items_with_inaccessible_icon_image, 
                                                                                gtk_tree_row_reference_new (model, path));
            }

            // Cleanup
            gtk_tree_path_free (path);
        }

        if (menu_or_item_or_separator_at_root_toplevel) {
            row_number++;
        }
    }

    g_signal_handler_block (selection, handler_id_row_selected);

    // Show a message if there are invisible menus outside root.
    if (G_UNLIKELY (number_of_used_toplevel_root_menus < number_of_toplevel_menu_ids)) {
        create_dialogs_for_invisible_menus_and_items (ORPHANED_MENUS, selection, invisible_elements_lists);
    }

    /*
        Set element visibility status for all those (pipe) menus, items and separators that don't already have one.
        If invisible orphaned menus have been visualised and they had descendant (pipe) menus, items or separators, 
        readjust the visibility status of the latter.
    */
    gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) elements_visibility, invisible_elements_lists);

    // Show a message if there are menus and items without a label (=invisible).
    if (G_UNLIKELY (invisible_elements_lists[MENUS_LIST] || invisible_elements_lists[ITEMS_LIST])) {
        create_dialogs_for_invisible_menus_and_items (MISSING_LABELS, selection, invisible_elements_lists);
        /*
            If (pipe) menus and/or items without label have received a label now and they had descendant 
            (pipe) menus, items or separators, readjust the visibility status of the latter.
        */
        gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) elements_visibility, NULL);
    }

    g_signal_handler_unblock (selection, handler_id_row_selected);

    gtk_tree_view_collapse_all (GTK_TREE_VIEW (treeview));


    // --- Finalisation ---


    // Pre-sort options of Execute action and startupnotify, if autosorting is activated.
    if (autosort_options) {
        gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) sort_loop_after_sorting_activation, NULL);
    }

    // Expand nodes that contain a broken icon.
    if (G_UNLIKELY (menus_and_items_with_inaccessible_icon_image)) {
        for (menus_and_items_with_inaccessible_icon_image_loop = menus_and_items_with_inaccessible_icon_image; 
             menus_and_items_with_inaccessible_icon_image_loop; 
             menus_and_items_with_inaccessible_icon_image_loop = menus_and_items_with_inaccessible_icon_image_loop->next) {
            path_loop = gtk_tree_row_reference_get_path (menus_and_items_with_inaccessible_icon_image_loop->data);
            gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), path_loop);
            gtk_tree_view_collapse_row (GTK_TREE_VIEW (treeview), path_loop);

            // Cleanup
            gtk_tree_path_free (path_loop);
        }

        // Cleanup
        g_slist_free_full (menus_and_items_with_inaccessible_icon_image, (GDestroyNotify) gtk_tree_row_reference_free);
    }

    // Notify about a conversion of deprecated execute to command options.
    if (G_UNLIKELY (menu_building.deprecated_exe_cmds_cvtd && 
                    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM 
                                        (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS])))) {
        GtkWidget *dialog, *content_area;
        GtkWidget *chkbt_exe_opt_conversion_notification = gtk_check_button_new_with_label 
            (gtk_menu_item_get_label ((GTK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]))));

        content_area = create_dialog (&dialog, "Conversion of deprecated execute option", 
                                               "dialog-information", 
                                               "This menu contains deprecated 'execute' options; "
                                               "they have been converted to 'command' options. "
                                               "These conversions have not been written back to the menu file yet; "
                                               "to do so, simply save the menu from inside the program.", 
                                               "_OK", NULL, NULL, FALSE);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chkbt_exe_opt_conversion_notification), TRUE);

        gtk_container_add (GTK_CONTAINER (content_area), chkbt_exe_opt_conversion_notification);

        gtk_widget_show_all (dialog);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]),
                                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                        (chkbt_exe_opt_conversion_notification)));
        gtk_widget_destroy (dialog);

        change_done = TRUE;
    }

    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));

    create_list_of_icon_occurrences ();


    // --- Cleanup ---


    g_slice_free1 (allocated_size, levels);

    parsing_abort:
    fclose (file);
    g_markup_parse_context_free (parse_context);

    // -- tree_build --

    for (tree_build_loop = menu_building.tree_build; tree_build_loop; tree_build_loop = tree_build_loop->next) {
        tree_data = tree_build_loop->data;
        if (tree_data[TS_BUILD_ICON_IMG]) {
            g_object_unref (tree_data[TS_BUILD_ICON_IMG]);
        }
        for (ts_build_cnt = TS_BUILD_ICON_MODIFICATION_TIME; ts_build_cnt <= TS_BUILD_ELEMENT_VISIBILITY; ts_build_cnt++) {
            g_free (tree_data[ts_build_cnt]);
        }
        g_slice_free1 (sizeof (gpointer) * NUMBER_OF_TS_BUILD_FIELDS, tree_data);
    }
    g_slist_free (menu_building.tree_build);

    // -- Other menu_building lists and variables --

    g_free (menu_building.line);

    g_slist_free_full (menu_building.toplevel_menu_ids[MENUS], (GDestroyNotify) g_free);
    g_slist_free_full (menu_building.toplevel_menu_ids[ROOT_MENU], (GDestroyNotify) g_free);

    g_free (menu_building.current_elements[ITEM]);
    g_free (menu_building.current_elements[ACTION]);
    g_free (menu_building.previous_type);
}

/* 

    Lets the user choose a menu xml file for opening.

*/

void open_menu (void)
{
    if (change_done && !continue_despite_unsaved_changes ()) {
        return;
    }

    GtkWidget *dialog;
    gchar *new_filename;

    create_file_dialog (&dialog, TRUE); // TRUE == "open" (FALSE would be "Save as ...")

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        new_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        gtk_widget_destroy (dialog);
        get_menu_elements_from_file (new_filename);
        row_selected (); // Resets settings for menu- and toolbar.
    }
    else {
        gtk_widget_destroy (dialog);
    }
}
