#ifndef __definitions_and_enumerations_h
#define __definitions_and_enumerations_h

// action option combo
enum { ACTION_OPTION_COMBO_ITEM, NUMBER_OF_ACTION_OPTION_COMBO_ELEMENTS };
// actions
enum { EXECUTE, EXIT, RECONFIGURE, RESTART, SESSIONLOGOUT, NUMBER_OF_ACTIONS };
// add buttons
enum { MENU, PIPE_MENU, ITEM, SEPARATOR, ACTION_OR_OPTION, NUMBER_OF_ADD_BUTTONS };
// ancestor visibility
enum { NONE_OR_VISIBLE_ANCESTOR, INVISIBLE_ANCESTOR, INVISIBLE_ORPHANED_ANCESTOR };
// columns
enum { COL_MENU_ELEMENT, COL_TYPE, COL_VALUE, COL_MENU_ID, COL_EXECUTE, COL_ELEMENT_VISIBILITY, NUMBER_OF_COLUMNS };
// entry fields
enum { MENU_ELEMENT_OR_VALUE_ENTRY, ICON_PATH_ENTRY, MENU_ID_ENTRY, EXECUTE_ENTRY, NUMBER_OF_ENTRY_FIELDS };
// execute options
enum { PROMPT, COMMAND, STARTUPNOTIFY, SN_OR_PROMPT = 2, NUMBER_OF_EXECUTE_OPTS };
// expansion statuses
enum { AT_LEAST_ONE_IS_EXPANDED, AT_LEAST_ONE_IMD_CH_IS_EXP, AT_LEAST_ONE_IS_COLLAPSED, NUMBER_OF_EXPANSION_STATUSES };
// find entry buttons
enum { CLOSE, BACK, FORWARD, NUMBER_OF_FIND_ENTRY_BUTTONS };
// invalid icon images
enum { INVALID_PATH_ICON, INVALID_FILE_ICON, NUMBER_OF_INVALID_ICON_IMGS };
// invalid icon image statuses
enum { NONE_OR_NORMAL, INVALID_PATH, INVALID_FILE };
// menu bar items file and edit
enum { MB_NEW, MB_OPEN, MB_SAVE, MB_SAVE_AS, MB_SEPARATOR_FILE, MB_QUIT, NUMBER_OF_FILE_MENU_ITEMS};
enum { MB_MOVE_TOP, MB_MOVE_UP, MB_MOVE_DOWN, MB_MOVE_BOTTOM, MB_SEPARATOR_EDIT1, MB_REMOVE, MB_REMOVE_ALL_CHILDREN, 
       MB_SEPARATOR_EDIT2, MB_VISUALISE, MB_VISUALISE_RECURSIVELY, NUMBER_OF_EDIT_MENU_ITEMS };
// menu bar items view and options
enum { SHOW_MENU_ID_COL, SHOW_EXECUTE_COL, SHOW_ELEMENT_VISIBILITY_COL_ACTVTD, SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL, 
       SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL, SHOW_ICONS, SET_OFF_SEPARATORS, DRAW_ROWS_IN_ALT_COLOURS, 
       SHOW_TREE_LINES, NO_GRID_LINES, SHOW_GRID_HOR, SHOW_GRID_VER, BOTH, CREATE_BACKUP_BEFORE_OVERWRITING_MENU, 
       SORT_EXECUTE_AND_STARTUPN_OPTIONS, NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS, NUMBER_OF_VIEW_AND_OPTIONS };
// move row
enum { TOP, UP, DOWN, BOTTOM };
// startupnotify options
enum { ENABLED, NAME, WM_CLASS, ICON, NUMBER_OF_STARTUPNOTIFY_OPTS };
// toolbar buttons
enum { TB_NEW, TB_OPEN, TB_SAVE, TB_SAVE_AS, TB_MOVE_UP, TB_MOVE_DOWN, TB_REMOVE, 
       TB_FIND, TB_EXPAND_ALL, TB_COLLAPSE_ALL, TB_QUIT, NUMBER_OF_TB_BUTTONS };
// treestore elements
enum { TS_ICON_IMG, TS_ICON_IMG_STATUS, TS_ICON_MODIFICATION_TIME, TS_ICON_PATH, TS_MENU_ELEMENT, 
       TS_TYPE, TS_VALUE, TS_MENU_ID, TS_EXECUTE, TS_ELEMENT_VISIBILITY, NUMBER_OF_TS_ELEMENTS };
// text fields
enum { ICON_PATH_TXT, MENU_ELEMENT_TXT, TYPE_TXT, VALUE_TXT, MENU_ID_TXT, 
       EXECUTE_TXT, ELEMENT_VISIBILITY_TXT, NUMBER_OF_TXT_FIELDS };
// renderer for treeview
enum { TXT_RENDERER, EXCL_TXT_RENDERER, PIXBUF_RENDERER, BOOL_RENDERER, NUMBER_OF_RENDERERS };

#define KICKSHAW_VERSION "0.5.11"
#define TREEVIEW_COLUMN_OFFSET NUMBER_OF_TS_ELEMENTS - NUMBER_OF_COLUMNS
#define FREE_AND_REASSIGN(string, new_value) { g_free (string); string = new_value; }
#define NOT_NULL_AND_NOT_EMPTY(string) (string && *string)
#define STREQ(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

typedef struct {
	GtkApplication *app;

	GtkWidget *window;
	GtkWidget *main_box;

	GtkWidget *mb_file_menu_items[NUMBER_OF_FILE_MENU_ITEMS];
	GtkWidget *mb_edit;
	GtkWidget *mb_edit_menu_items[NUMBER_OF_EDIT_MENU_ITEMS];
	GtkWidget *mb_search;
	GtkWidget *mb_expand_all_nodes, *mb_collapse_all_nodes;
	GtkWidget *mb_options;

	GtkWidget *mb_view_and_options[NUMBER_OF_VIEW_AND_OPTIONS];

	GtkToolItem *tb[NUMBER_OF_TB_BUTTONS];

	GtkWidget *button_grid;
	GtkWidget *add_image;
	GtkWidget *bt_bar_label;
	GtkWidget *bt_add[NUMBER_OF_ADD_BUTTONS];
	GtkWidget *bt_add_action_option_label;

	GtkWidget *change_values_label;

	GtkWidget *action_option_grid, *new_action_option_grid;
	GtkWidget *inside_menu_label, *inside_menu_check_button, *including_action_label, *including_action_check_button;
	GtkListStore *action_option_combo_box_liststore;
	GtkTreeModel *action_option_combo_box_model;
	GtkWidget *action_option, *action_option_cancel, *action_option_done;
	GtkWidget *options_grid, *suboptions_grid;
	gchar *options_label_txts[NUMBER_OF_EXECUTE_OPTS];
	GtkWidget *options_labels[NUMBER_OF_EXECUTE_OPTS], *options_fields[NUMBER_OF_EXECUTE_OPTS];
	GtkCssProvider *execute_options_css_providers[NUMBER_OF_EXECUTE_OPTS - 1]; // Only "command" and "prompt".
	GtkWidget *suboptions_labels[NUMBER_OF_STARTUPNOTIFY_OPTS], *suboptions_fields[NUMBER_OF_STARTUPNOTIFY_OPTS];
	GtkCssProvider *suboptions_fields_css_providers[NUMBER_OF_STARTUPNOTIFY_OPTS - 1]; // Only "name", "wmclass" and "icon".

	gchar *actions[NUMBER_OF_ACTIONS];
	gchar *execute_options[NUMBER_OF_EXECUTE_OPTS];
	gchar *execute_displayed_txts[NUMBER_OF_EXECUTE_OPTS];

	gchar *startupnotify_options[NUMBER_OF_STARTUPNOTIFY_OPTS];
	gchar *startupnotify_displayed_txts[NUMBER_OF_STARTUPNOTIFY_OPTS];

	GtkWidget *separator, *change_values_buttons_grid;

	GtkWidget *find_grid;
	GtkWidget *find_entry_buttons[NUMBER_OF_FIND_ENTRY_BUTTONS];
	GtkWidget *find_entry;
	GtkCssProvider *find_entry_css_provider;
	GtkWidget *find_in_columns[NUMBER_OF_COLUMNS - 1], *find_in_all_columns;
	GtkCssProvider *find_in_columns_css_providers[NUMBER_OF_COLUMNS - 1], *find_in_all_columns_css_provider;
	GtkWidget *find_match_case, *find_regular_expression; 
	GString *search_term; // = automatically NULL
	GList *rows_with_found_occurrences; // = automatically NULL

	GtkWidget *treeview;
	GtkTreeStore *treestore;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *column_header_txts[NUMBER_OF_COLUMNS];
	GtkTreeViewColumn *columns[NUMBER_OF_COLUMNS];
	GtkCellRenderer *renderers[NUMBER_OF_RENDERERS];

	gchar *txt_fields[NUMBER_OF_TXT_FIELDS]; // = automatically NULL

	GtkTargetEntry enable_list[3];

	GtkWidget *entry_grid;
	GtkWidget *entry_labels[NUMBER_OF_ENTRY_FIELDS], *entry_fields[NUMBER_OF_ENTRY_FIELDS];
	GtkCssProvider *menu_element_or_value_entry_css_provider, *icon_path_entry_css_provider;
	GtkWidget *icon_chooser, *remove_icon;

	GSList *source_paths; // = automatically NULL

	GSList *change_values_user_settings; // = automatically NULL

	GtkWidget *statusbar;
	gboolean statusbar_msg_shown; // = automatically FALSE

	GtkCssProvider *cm_css_provider;

	gchar *icon_theme_name; // = automatically NULL
	guint font_size; // = automatically 0

	GdkPixbuf *invalid_icon_imgs[NUMBER_OF_INVALID_ICON_IMGS]; // = automatically NULL

	gchar *filename; // = automatically NULL

	// = automatically NULL
	GSList *menu_ids;
	GSList *rows_with_icons;

	gboolean change_done; // = automatically FALSE
	gboolean autosort_options; // = automatically FALSE

	gint handler_id_row_selected, 
		 handler_id_action_option_combo_box, handler_id_action_option_button_clicked, 
		 handler_id_show_startupnotify_options, 
		 handler_id_find_in_columns[NUMBER_OF_COLUMNS], 
         handler_id_entry_fields[NUMBER_OF_ENTRY_FIELDS], 
		 handler_id_including_action_check_button;
} ks_data;

#endif
