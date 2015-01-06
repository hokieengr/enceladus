#include <libxfce4panel/xfce-panel-plugin.h>
#include <string.h>

static void constructor(XfcePanelPlugin *plugin);
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(constructor);


enum TreeColumns {
	FILE_NAME,
	TOTAL_COLUMNS
};

typedef struct {
	gchar **files;
	uint amount;
} Directory;

typedef struct {
	GtkTreeStore *model;
	GtkWidget *view;
	gchar *root;

	GtkTreeViewColumn *col[TOTAL_COLUMNS];
	GtkCellRenderer *renderer[TOTAL_COLUMNS];
	gchar *renderer_type[TOTAL_COLUMNS];
} GtkTree;

Directory *
new_directory() {
	Directory *dir = malloc(sizeof(Directory));
	if(dir == NULL)
		return NULL;
	dir->amount = 0;
	dir->files = NULL;
	return dir;
}

gchar *
gtk_tree_path_to_file_path(GtkTree *tree, GtkTreePath *path) {
	GtkTreeIter iter;

	GString *full_path = g_string_new(NULL);

	do {
		GValue directory = G_VALUE_INIT;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->model), &iter, path);
		gtk_tree_model_get_value(GTK_TREE_MODEL(tree->model), &iter, FILE_NAME, &directory);
		g_string_prepend(full_path, g_value_get_string(&directory));
		g_string_prepend(full_path, "/");
	} while(gtk_tree_path_up(path) && gtk_tree_path_get_depth(path) > 0);
	g_string_prepend(full_path, tree->root);

	return g_string_free(full_path, FALSE);
}

Directory *
get_directory_files(gchar *directory) {
	GDir *gdir = NULL;
	GError *error = NULL;
	gchar *filename = NULL;
	gdir = g_dir_open(directory, 0, &error);
	if(error != NULL)
		return NULL;
	Directory *dir = new_directory();

	while((filename = (gchar *)g_dir_read_name(gdir))) {
		dir->amount += 1;
		dir->files = realloc(dir->files, sizeof(gchar *)*dir->amount);
		dir->files[dir->amount-1] = filename;
	}
	return dir;
}

void
insert_directory_in_tree(Directory *dir, GtkTreeStore *tree, GtkTreeViewColumn *col, GtkTreeIter *parent) {
	GtkTreeIter row_inserted;
	int iterator;
	for(iterator = 0; iterator < dir->amount; iterator++) {
		gtk_tree_store_append(tree, &row_inserted, parent);
		gtk_tree_store_set(tree, &row_inserted, 0, dir->files[iterator], -1);
	}
}

void
reset_directory(GtkTreeStore *tree, GtkTreeIter *iter) {
	GtkTreeIter child;
	if(!gtk_tree_model_iter_children(GTK_TREE_MODEL(tree), &child, iter))
		return;

	while(gtk_tree_store_remove(tree, &child));
}

int
w_strcmp(const void *a, const void *b) {
	int result = strcmp((gchar *)a, (gchar *)b);
	if(result != 0)
		result *= -1;
	return result;
}


void
row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data) {
	GtkTree *tree = (GtkTree *)data;
	GtkTreeIter start;
	GError *error = NULL;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->model), &start, path);
	gchar *directory = gtk_tree_path_to_file_path(tree, path);
	if(directory == NULL)
		return;
	if(!g_file_test(directory, G_FILE_TEST_IS_DIR)) {
		GString *command = g_string_new("xdg-open ");
		g_string_append(command, directory);
		gchar * cmd = g_string_free(command, FALSE);
		g_spawn_command_line_async(cmd, &error);
		g_free(cmd);
		g_free(directory);
		return;
	}

	reset_directory(tree->model, &start);
	Directory *dir = get_directory_files(directory);
	g_free(directory);
	if(dir == NULL)
		return;
	qsort(dir->files, dir->amount, sizeof(gchar *), w_strcmp);
	insert_directory_in_tree(dir, tree->model, tree->col[FILE_NAME], &start);
}

static void
constructor(XfcePanelPlugin *plugin) {
	int iterator;

	GtkWidget *scroll_win = gtk_scrolled_window_new(NULL, NULL);

	GtkTree *tree = malloc(sizeof(GtkTree));
	tree->root = getenv("HOME");

	tree->model = gtk_tree_store_new(TOTAL_COLUMNS, G_TYPE_STRING);
	tree->view = gtk_tree_view_new();

	for(iterator = 0; iterator < TOTAL_COLUMNS; iterator += 1) {
		tree->col[iterator] = gtk_tree_view_column_new();
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree->view), tree->col[iterator]);	
	}
	gtk_tree_view_column_set_title(tree->col[FILE_NAME], tree->root);
	
	tree->renderer_type[FILE_NAME] = "text";
	for(iterator = 0; iterator < TOTAL_COLUMNS; iterator += 1) {
		tree->renderer[iterator] = gtk_cell_renderer_text_new();
		gtk_tree_view_column_add_attribute(tree->col[iterator], tree->renderer[iterator], tree->renderer_type[iterator], iterator);
	}

	gtk_tree_view_column_pack_start(tree->col[FILE_NAME], tree->renderer[FILE_NAME], TRUE);
	gtk_tree_view_column_add_attribute(tree->col[FILE_NAME], tree->renderer[FILE_NAME], tree->renderer_type[FILE_NAME], FILE_NAME);

	Directory *dir = get_directory_files(tree->root);
	if(dir == NULL)
		return;
	qsort(dir->files, dir->amount, sizeof(gchar *), w_strcmp);
	insert_directory_in_tree(dir, tree->model, tree->col[FILE_NAME], NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree->view), GTK_TREE_MODEL(tree->model));

	gtk_container_add(GTK_CONTAINER(scroll_win), tree->view);

	gtk_container_add(GTK_CONTAINER(plugin), scroll_win);

	g_signal_connect(tree->view, "row-activated", G_CALLBACK(row_activated), tree);

	gtk_widget_show_all(scroll_win);
	xfce_panel_plugin_set_expand (XFCE_PANEL_PLUGIN(plugin), TRUE);
}
