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
#include "check_for_external_changes_timeout.h"

gboolean check_for_external_file_and_settings_changes (G_GNUC_UNUSED gpointer identifier);
void stop_timeout (void);

/* 

    Checks if...
    ...a valid icon file path has become invalid; if so, changes the icon to a broken one.
    ...an invalid icon file path has become valid; if so, replaces the broken icon with the icon image,
    if it is a proper image file.
    ...the font size has been changed; if so, changes the size of icons according to the new font size.

*/

// The identifier is not used by the function, but to switch the timeout on and off.
gboolean check_for_external_file_and_settings_changes (G_GNUC_UNUSED gpointer identifier)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ks.treeview));
    gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);

    gchar *font_desc;
    guint font_size_updated;
    gboolean recreate_icon_images = FALSE; // Default
    gboolean create_new_invalid_icon_imgs = FALSE; // Default
    gboolean replacement_done;

    gchar *current_icon_theme_name;

    gchar *time_stamp;

    GSList *rows_with_icons_loop;

    GtkTreePath *path_loop;
    GtkTreeIter iter_loop;

    guint icon_img_status_uint_loop;
    gchar *icon_modification_time_txt_loop;
    gchar *icon_path_txt_loop;

    g_object_get (gtk_settings_get_default (), "gtk-font-name", &font_desc, NULL);
    if (!(STREQ (font_desc, ks.font_desc))) {
        FREE_AND_REASSIGN (ks.font_desc, font_desc);

        // Check if font size has changed.
        if ((recreate_icon_images = (G_UNLIKELY (ks.font_size != (font_size_updated = get_font_size ()))))) {
            ks.font_size = font_size_updated;
            create_new_invalid_icon_imgs = TRUE;
        }

        if (gtk_widget_get_visible (ks.change_values_label)) {
            gchar *font_name = get_font_name ();
            const gchar *label_txt = gtk_label_get_text (GTK_LABEL (ks.change_values_label));

            gchar *label_markup = g_strdup_printf ("<span font_desc='%s %i'>%s</span>", font_name, ks.font_size + 2, label_txt);
            gtk_label_set_markup (GTK_LABEL (ks.change_values_label), label_markup);

            // Cleanup
            g_free (label_markup);
            g_free (font_name);
        }
    }
    else {
        // Cleanup
        g_free (font_desc);
    }

    // Check if icon theme has changed.
    g_object_get (gtk_settings_get_default (), "gtk-icon-theme-name", &current_icon_theme_name, NULL);
    if (G_UNLIKELY (!STREQ (ks.icon_theme_name, current_icon_theme_name))) {
        FREE_AND_REASSIGN (ks.icon_theme_name, g_strdup (current_icon_theme_name));
        create_new_invalid_icon_imgs = TRUE;
    }

    // If font size or icon theme has changed, recreate the invalid icon images.
    if (G_UNLIKELY (create_new_invalid_icon_imgs)) {
        create_invalid_icon_imgs ();
    }

    for (rows_with_icons_loop = ks.rows_with_icons; 
         rows_with_icons_loop; 
         rows_with_icons_loop = rows_with_icons_loop->next) {
        path_loop = rows_with_icons_loop->data;
        gtk_tree_model_get_iter (ks.model, &iter_loop, path_loop);
 
        gtk_tree_model_get (ks.model, &iter_loop,
                            TS_ICON_IMG_STATUS, &icon_img_status_uint_loop, 
                            TS_ICON_MODIFICATION_TIME, &icon_modification_time_txt_loop,
                            TS_ICON_PATH, &icon_path_txt_loop, 
                            -1);

        replacement_done = FALSE; // Default

        /*
            Case 1: Stored path no longer points to a valid icon image, so icon has to replaced with broken icon.
            Case 2: Invalid path to an icon image has become valid.
            Case 3: Icon image file is replaced with another one.
        */
        if (G_LIKELY (icon_img_status_uint_loop != INVALID_PATH) && 
            G_UNLIKELY (!g_file_test (icon_path_txt_loop, G_FILE_TEST_EXISTS))) { // Case 1
            gtk_tree_store_set (ks.treestore, &iter_loop, 
                                TS_ICON_IMG, ks.invalid_icon_imgs[INVALID_PATH_ICON], 
                                TS_ICON_IMG_STATUS, INVALID_PATH, 
                                TS_ICON_MODIFICATION_TIME, NULL, 
                                -1);

            if (number_of_selected_rows == 1 && gtk_tree_selection_iter_is_selected (selection, &iter_loop)) {
                wrong_or_missing (ks.entry_fields[ICON_PATH_ENTRY], ks.icon_path_entry_css_provider);
            }

            replacement_done = TRUE;
        }
        else if (G_LIKELY (g_file_test (icon_path_txt_loop, G_FILE_TEST_EXISTS))) {
            time_stamp = get_modification_time_for_icon (icon_path_txt_loop);

            if (G_UNLIKELY (icon_img_status_uint_loop == INVALID_PATH ||
                            !STREQ (time_stamp, icon_modification_time_txt_loop))) {
                // Case 2 & 3, FALSE == don't show error message when error occurs.
                if (G_UNLIKELY (!set_icon (&iter_loop, icon_path_txt_loop, FALSE))) { 
                    gtk_tree_store_set (ks.treestore, &iter_loop, 
                                        TS_ICON_IMG, ks.invalid_icon_imgs[INVALID_FILE_ICON], 
                                        TS_ICON_IMG_STATUS, INVALID_FILE, 
                                        TS_ICON_MODIFICATION_TIME, time_stamp, 
                                        -1);
                }

                if (number_of_selected_rows == 1 && gtk_tree_selection_iter_is_selected (selection, &iter_loop)) {
                    gtk_style_context_remove_class (gtk_widget_get_style_context (ks.entry_fields[ICON_PATH_ENTRY]), 
                                                    "mandatory_missing");
                }

                replacement_done = TRUE;
            }

            // Cleanup
            g_free (time_stamp);
        }

        /*
            If the font size or icon theme has changed, replace the icon images accordingly,
            but only if the icons have not been replaced already, since the replacement icons already have the new size.
        */
        if (G_UNLIKELY ((recreate_icon_images || (create_new_invalid_icon_imgs && icon_img_status_uint_loop)) && 
                        !replacement_done)) {
            if (!icon_img_status_uint_loop) { // Icon path is OK and valid icon exists = create and store larger or smaller icon
                set_icon (&iter_loop, icon_path_txt_loop, FALSE); // FALSE == don't show error message when error occurs.
            }
            else { // Icon path is wrong or icon image is invalid = store broken icon with larger or smaller size
                gtk_tree_store_set (ks.treestore, &iter_loop, TS_ICON_IMG, 
                                    ks.invalid_icon_imgs[(icon_img_status_uint_loop == INVALID_PATH) ? 
                                                         INVALID_PATH_ICON : INVALID_FILE_ICON], -1);
            }

            gtk_tree_view_columns_autosize (GTK_TREE_VIEW (ks.treeview)); // In case that the font size has been reduced.
        }

        // Cleanup
        g_free (icon_modification_time_txt_loop);
        g_free (icon_path_txt_loop);
    }

    // Cleanup
    g_free (current_icon_theme_name);

    return TRUE; // Keep timeout function up and running.
}

/* 

    Stops the timer and erases the list of icon occourrences.

*/

void stop_timeout (void)
{
    g_source_remove_by_user_data ("timeout");
    g_slist_free_full (ks.rows_with_icons, (GDestroyNotify) gtk_tree_path_free);
    ks.rows_with_icons = NULL;
}
