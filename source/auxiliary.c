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
#include <stdlib.h>

#include "general_header_files/enum__ancestor_visibility.h"
#include "general_header_files/enum__expansion_statuses.h"
#include "general_header_files/enum__invalid_icon_imgs.h"
#include "general_header_files/enum__ts_elements.h"
#include "auxiliary.h"

void expand_row_from_iter (GtkTreeIter *local_iter);
gchar *extract_substring_via_regex (gchar *string, gchar *regex_str);
void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
guint get_font_size (void);
gchar *get_modification_time_for_icon (gchar *icon_path);
void get_toplevel_iter_from_path (GtkTreeIter *local_iter, GtkTreePath *local_path);
gboolean multiple_str_replacement_callback_func (const GMatchInfo *match_info, GString *result, gpointer hash_table);
gboolean streq_any (const gchar *string, ...);

GtkWidget *new_label_with_formattings (gchar *label_txt, gboolean wrap);
GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *icon_name, gchar *label_txt, 
                          gchar *button_txt_1, gchar *button_txt_2, gchar *button_txt_3, gboolean show_immediately);
void create_file_dialog (GtkWidget **dialog, gboolean open);
void create_invalid_icon_imgs (void);
gboolean check_expansion_statuses_of_nodes (GtkTreeModel *foreach_or_filter_model, 
                                            GtkTreePath  *foreach_or_filter_path, 
                                            GtkTreeIter  *foreach_or_filter_iter, 
                                            gboolean     *expansion_statuses_of_nodes);
void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
                                 gchar **options_array, gboolean *opts_exist);
gboolean check_if_invisible_descendant_exists (              GtkTreeModel *filter_model, 
                                               G_GNUC_UNUSED GtkTreePath *filter_path,
                                                             GtkTreeIter *filter_iter, 
                                                             gboolean    *at_least_one_descendant_is_invisible);
guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path);
static gboolean add_icon_occurrence_to_list (G_GNUC_UNUSED GtkTreeModel *foreach_model, 
                                                           GtkTreePath  *foreach_path,
                                                           GtkTreeIter  *foreach_iter);
void create_list_of_icon_occurrences (void);
void wrong_or_missing (GtkWidget *widget, GtkCssProvider *css_provider);


/*

    -------------------------------------------------------------------
      GENERAL - Auxiliary functions not necessarily bound to Kickshaw
    -------------------------------------------------------------------

*/


/* 

    Expands a row by retrieving the required path from a passed iterator.

*/

void expand_row_from_iter (GtkTreeIter *local_iter)
{
    GtkTreePath *path = gtk_tree_model_get_path (model, local_iter);

    gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, FALSE); // FALSE == just expand immediate children.

    // Cleanup
    gtk_tree_path_free (path);
}

/* 

   Extracts a substring from a string by using a regular expression.

   This function returns a newly-allocated string that should be freed with g_free () after use.

*/

gchar *extract_substring_via_regex (gchar *string, 
                                    gchar *regex_str)
{
    // No compile and match options, no return location for a GError.
    GRegex *regex = g_regex_new (regex_str, 0, 0, NULL); 
    GMatchInfo *match_info; // match_info is created even if g_regex_match returns FALSE.
    gchar *match;

    g_regex_match (regex, string, 0, &match_info); // No match options.
    match = g_match_info_fetch (match_info, 0); // 0 == full text of the match.

    // Cleanup
    g_regex_unref (regex);
    g_match_info_free (match_info);

    return match;
}

/* 

    Frees dyn. alloc. memory of all strings of a static string array.

*/

void free_elements_of_static_string_array (gchar    **string_array, 
                                           gint8      number_of_fields, 
                                           gboolean   set_to_NULL)
{
    while (--number_of_fields + 1) { // x fields == 0 ... x-1
        g_free (string_array[number_of_fields]);

        if (set_to_NULL) {
            string_array[number_of_fields] = NULL;
        }
    }
}

/* 

    Retrieves the current font size so the size of the icon images can be adjusted to it.

*/

guint get_font_size (void)
{
    gchar *font_str;
    gchar *font_substr;
    guint  font_size;

    g_object_get (gtk_settings_get_default (), "gtk-font-name", &font_str, NULL);
    font_size = atoi (font_substr = extract_substring_via_regex (font_str, "\\d+$")); // e.g. Sans 12 -> 12

    // Cleanup
    g_free (font_str);
    g_free (font_substr);

    return font_size;
}

/* 

    Returns the time of the last modification of an icon image file as an RFC 3339 encoded string.

    Since memory has been allocated for it, the string should be freed with g_free () after use.

*/

gchar *get_modification_time_for_icon (gchar *icon_path)
{
    GFile *file = g_file_new_for_path (icon_path);
    // The last two arguments (cancellable and error) are not used.
    GFileInfo *file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GTimeVal icon_modification_time;

    g_file_info_get_modification_time (file_info, &icon_modification_time);

    // Cleanup
    g_object_unref (file);
    g_object_unref (file_info);

    return g_time_val_to_iso8601 (&icon_modification_time);
}

/* 

    Sets an iterator for the toplevel of a given path.

*/

void get_toplevel_iter_from_path (GtkTreeIter *local_iter, GtkTreePath *local_path)
{
    gchar *toplevel_str = g_strdup_printf ("%i", gtk_tree_path_get_indices (local_path)[0]);

    gtk_tree_model_get_iter_from_string (model, local_iter, toplevel_str);

    // Cleanup
    g_free (toplevel_str);
}

/* 

    Used for replacing several matches at once by g_regex_replace_eval ().

    g_regex_replace_eval () constructs a string, which it later returns, by alternatively adding 
    the unchanged parts inside itself and the replaced parts inside the callback function, 
    which can be written freely by the user, using its parameters.

*/

gboolean multiple_str_replacement_callback_func (const GMatchInfo *match_info, 
                                                 GString          *result, 
                                                 gpointer          hash_table)
{
    gchar *match = g_match_info_fetch (match_info, 0);
    gchar *replacement = g_hash_table_lookup ((GHashTable *) hash_table, match);

    g_string_append (result, replacement);

    // Cleanup
    g_free (match);

    return FALSE; // Continue constructing the result string.
}

/* 

    Short replacement for the check if a string equals to one of several values
    (strcmp (x,y) == 0 || strcmp (x,z) == 0).

*/

gboolean streq_any (const gchar *string, ...)
{
    va_list arguments;
    gchar *check;

    va_start (arguments, string);
    while ((check = va_arg (arguments, gchar *))) { // Parentheses avoid gcc warning.
        if (streq (string, check)) {
            va_end (arguments); // Mandatory for safety and implementation/platform neutrality.
            return TRUE;
        }
    }
    va_end (arguments); // Mandatory for safety and implementation/platform neutrality.
    return FALSE;
}


/* 

    --------------------------------------------------------------------
      KICKSHAW-SPECIFIC - Auxiliary functions for use in Kickshaw only
    --------------------------------------------------------------------

*/


/* 

    Creates a new label with some formattings.

*/

GtkWidget *new_label_with_formattings (gchar *label_txt, gboolean wrap)
{
    return gtk_widget_new (GTK_TYPE_LABEL, "label", label_txt, "xalign", 0.0, "use-markup", TRUE, "wrap", wrap, "max-width-chars", 1, NULL);
}

/* 

    Convenience function for the creation of a dialog window.

    The dialog can have up to three buttons. If less than three are needed, 
    the "button_txt_2" and/or "button_txt_3" argument should be set to NULL.
    If something else than a label text shall be added to the content area, 
    the "show_immediately" argument should be set to FALSE. 
    gtk_widget_show_all () should be called then after all additional widgets have been added to the content area.

*/

GtkWidget *create_dialog (GtkWidget **dialog, 
                          gchar      *dialog_title, 
                          gchar      *icon_name, 
                          gchar      *label_txt, 
                          gchar      *button_txt_1, 
                          gchar      *button_txt_2, 
                          gchar      *button_txt_3, 
                          gboolean    show_immediately)
{
    GtkWidget *content_area, *label;
    gchar *label_txt_with_addl_border = g_strdup_printf ("\n%s\n", label_txt);

    *dialog = gtk_dialog_new_with_buttons (dialog_title, GTK_WINDOW (window), GTK_DIALOG_MODAL, 
                                           button_txt_1, 1, button_txt_2, 2, button_txt_3, 3,
                                           NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (*dialog));
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 10);
    gtk_container_add (GTK_CONTAINER (content_area), gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG));
    label = new_label_with_formattings (label_txt_with_addl_border, TRUE);
    gtk_widget_set_size_request (label, 570, -1);
    gtk_container_add (GTK_CONTAINER (content_area), label);
    if (show_immediately) {
        gtk_widget_show_all (*dialog);
    }

    // Cleanup
    g_free (label_txt_with_addl_border);

    return content_area;
}

/* 

    Creates a file dialog for opening a new and saving an existing menu.

*/

void create_file_dialog (GtkWidget **dialog, gboolean open)
{
    gchar *menu_folder;
    GtkFileFilter *file_filter;

    *dialog = gtk_file_chooser_dialog_new ((open) ? "Open Openbox Menu File" : "Save Openbox Menu File As ...", 
                                           GTK_WINDOW (window),
                                           (open) ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE, 
                                           "_Cancel", GTK_RESPONSE_CANCEL,
                                           (open) ? "_Open" : "_Save", GTK_RESPONSE_ACCEPT,
                                           NULL);

    menu_folder = g_strconcat (g_getenv ("HOME"), "/.config/openbox", NULL);
    if (g_file_test (menu_folder, G_FILE_TEST_EXISTS)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (*dialog), menu_folder);
    }

    // Cleanup
    g_free (menu_folder);

    file_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (file_filter, "Openbox Menu Files");
    gtk_file_filter_add_pattern (file_filter, "*.xml");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (*dialog), file_filter);
}

/* 

    Creates icon images for the case of a broken path or an invalid image file.

*/

void create_invalid_icon_imgs (void)
{
    GdkPixbuf *invalid_icon_img_pixbuf_dialog_size;
    gchar *icon_name;

    for (guint8 invalid_icon_img_cnt = 0; invalid_icon_img_cnt < NUMBER_OF_INVALID_ICON_IMGS; invalid_icon_img_cnt++) {
        // This becomes only true if the font size or icon theme have been changed before during the runtime of this program.
        if (G_UNLIKELY (invalid_icon_imgs[invalid_icon_img_cnt])) {
            g_object_unref (invalid_icon_imgs[invalid_icon_img_cnt]);
        }

        icon_name = (invalid_icon_img_cnt == INVALID_PATH_ICON) ? "dialog-question" : "image-missing";
        invalid_icon_img_pixbuf_dialog_size = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon_name, 
                                                                        48, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        invalid_icon_imgs[invalid_icon_img_cnt] = gdk_pixbuf_scale_simple (invalid_icon_img_pixbuf_dialog_size, 
                                                                           font_size + 10, font_size + 10, 
                                                                           GDK_INTERP_BILINEAR);

        // Cleanup
        g_object_unref (invalid_icon_img_pixbuf_dialog_size);
    }
}

/* 

    Checks expansion statuses of nodes to determine 
    - how to set the sensivity of the menu bar items and toolbar buttons for the expansion and collapse of all nodes.

    In this case this function has been called after a selection.
    It is only necessary to check whether there is at least one expanded and collapsed node, 
    regardless of their position inside the treeview.
    AT_LEAST_ONE_IS_EXPANDED and AT_LEAST_ONE_IS_COLLAPSED are set to TRUE, if the respective conditions apply.

    - which expansion and collapse options to add to the context menu 
    (expand recursively, expand only immediate children and collapse).

    In this case this function has been called after one or more rows, all of them containing at least one child, 
    were rightlicked.
    It has to be checked whether an immediate child of a row is expanded. If this applies to one of the selected rows 
    or at least one of the selected rows is currently not expanded, the option to expand only the immediate children is 
    added later to the context menu.
    The second check is for whether there is at least one collapsed node, since if one is collapsed it should 
    be possible to recursively expand all nodes. To determine whether the collapse option should be added it is 
    sufficient to check if the root node is expanded, which is not done here.
    AT_LEAST_ONE_IMD_CH_IS_EXP and AT_LEAST_ONE_IS_COLLAPSED are set to TRUE, if the respective conditions apply.

*/

gboolean check_expansion_statuses_of_nodes (GtkTreeModel *foreach_or_filter_model, 
                                            GtkTreePath  *foreach_or_filter_path, 
                                            GtkTreeIter  *foreach_or_filter_iter, 
                                            gboolean     *expansion_statuses_of_nodes)
{
    GtkTreePath *model_path = NULL; // Default

    if (gtk_tree_model_iter_has_child (foreach_or_filter_model, foreach_or_filter_iter)) {
        if (model != foreach_or_filter_model) { // = filter, called from create_context_menu ()
            // The path of the model, not filter model, is needed to check whether the row is expanded.
            model_path = gtk_tree_model_filter_convert_path_to_child_path ((GtkTreeModelFilter *) foreach_or_filter_model, 
                                                                           foreach_or_filter_path);
        }

        if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), (model_path) ? model_path : foreach_or_filter_path)) {
            if (!model_path) { // = called from row_selected ()
                expansion_statuses_of_nodes[AT_LEAST_ONE_IS_EXPANDED] = TRUE;
            }
            else if (gtk_tree_path_get_depth (foreach_or_filter_path) == 1) {
                expansion_statuses_of_nodes[AT_LEAST_ONE_IMD_CH_IS_EXP] = TRUE;
            }
        }
        else {
            expansion_statuses_of_nodes[AT_LEAST_ONE_IS_COLLAPSED] = TRUE;
        }
    }

    // Iterate only as long as not for all queried statuses a positive match has been found.
    gboolean stop_iterating = (((model_path) ? expansion_statuses_of_nodes[AT_LEAST_ONE_IMD_CH_IS_EXP] : 
                                               expansion_statuses_of_nodes[AT_LEAST_ONE_IS_EXPANDED]) && 
                                               expansion_statuses_of_nodes[AT_LEAST_ONE_IS_COLLAPSED]);

    // Cleanup
    gtk_tree_path_free (model_path);

    return stop_iterating;
}

/* 

    Checks for existing options of an Execute action or startupnotify option.

*/

void check_for_existing_options (GtkTreeIter  *parent, 
                                 guint8        number_of_opts, 
                                 gchar       **options_array, 
                                 gboolean     *opts_exist)
{
    GtkTreeIter iter_loop;
    gint ch_cnt;
    guint8 opts_cnt;

    gchar *menu_element_txt_loop;

    for (ch_cnt = 0; ch_cnt < gtk_tree_model_iter_n_children (model, parent); ch_cnt++) {
        gtk_tree_model_iter_nth_child (model, &iter_loop, parent, ch_cnt);
        gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
        for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++) {
            if (streq (menu_element_txt_loop, options_array[opts_cnt])) {
                opts_exist[opts_cnt] = TRUE;
            }
        }
        // Cleanup
        g_free (menu_element_txt_loop);
    }
}

/* 

    Looks for invisible descendants of a row.

*/

gboolean check_if_invisible_descendant_exists (              GtkTreeModel *filter_model, 
                                               G_GNUC_UNUSED GtkTreePath  *filter_path,
                                                             GtkTreeIter  *filter_iter, 
                                                             gboolean     *at_least_one_descendant_is_invisible)
{
    gchar *menu_element_txt_loop, *type_txt_loop;

    gtk_tree_model_get (filter_model, filter_iter, 
                        TS_MENU_ELEMENT, &menu_element_txt_loop, 
                        TS_TYPE, &type_txt_loop, 
                        -1);

    *at_least_one_descendant_is_invisible = !menu_element_txt_loop && !streq (type_txt_loop, "separator");

    // Cleanup
    g_free (menu_element_txt_loop);
    g_free (type_txt_loop);

    // Stop iterating if an invisible descendant has been found.
    return *at_least_one_descendant_is_invisible;
}

/* 

    Looks for invisible ancestors of a given path and
    returns the status of the elements visibility column for the first found one.

*/

guint8 check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *local_path)
{
    if (gtk_tree_path_get_depth (local_path) == 1) {
        return NONE_OR_VISIBLE_ANCESTOR;
    }

    guint8 visibility_of_ancestor;

    GtkTreePath *path_loop = gtk_tree_path_copy (local_path);
    GtkTreeIter iter_loop;

    gchar *element_ancestor_visibility_txt_loop;

    do {
        gtk_tree_path_up (path_loop);
        gtk_tree_model_get_iter (local_model, &iter_loop, path_loop);
        gtk_tree_model_get (local_model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_ancestor_visibility_txt_loop, -1);
        if (G_UNLIKELY (element_ancestor_visibility_txt_loop && 
                        g_str_has_prefix (element_ancestor_visibility_txt_loop, "invisible"))) {
            visibility_of_ancestor = (g_str_has_suffix (element_ancestor_visibility_txt_loop, "orphaned menu")) ? 
                                      INVISIBLE_ORPHANED_ANCESTOR : INVISIBLE_ANCESTOR;

            // Cleanup
            gtk_tree_path_free (path_loop);
            g_free (element_ancestor_visibility_txt_loop);

            return visibility_of_ancestor;
        }

        // Cleanup
        g_free (element_ancestor_visibility_txt_loop);
    } while (gtk_tree_path_get_depth (path_loop) > 1);

    // Cleanup
    gtk_tree_path_free (path_loop);

    return NONE_OR_VISIBLE_ANCESTOR;
}

/* 

    Adds a row that contains an icon to a list.

*/

static gboolean add_icon_occurrence_to_list (G_GNUC_UNUSED GtkTreeModel *foreach_model, 
                                                           GtkTreePath  *foreach_path,
                                                           GtkTreeIter  *foreach_iter)
{
    GdkPixbuf *icon;

    gtk_tree_model_get (model, foreach_iter, TS_ICON_IMG, &icon, -1);
    if (icon) {
        rows_with_icons = g_slist_prepend (rows_with_icons, gtk_tree_path_copy (foreach_path));

        // Cleanup
        g_object_unref (icon);
    }

    return FALSE;
}

/* 

    Creates a list that contains all rows with an icon.

*/

void create_list_of_icon_occurrences (void)
{
    gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) add_icon_occurrence_to_list, NULL);
    if (rows_with_icons) {
        g_timeout_add (1000, (GSourceFunc) check_for_external_file_and_settings_changes, "timeout");
    }
}

/* 

    Gives Widget a red background color to indicate that something is wrong or missing.

*/

void wrong_or_missing (GtkWidget *widget, GtkCssProvider *css_provider)
{
    GtkStyleContext *context = gtk_widget_get_style_context (widget);

    if (!gtk_style_context_has_class (context, "bg_class")) {
        gtk_style_context_add_class (context, "bg_class");
        gtk_css_provider_load_from_data (css_provider, (GTK_IS_ENTRY (widget)) ? 
        ".entry.bg_class { background: rgba(235,199,199,100) }" : 
        ".bg_class { background: rgba(235,199,199,100) }", 
        -1, NULL);
    }
}
