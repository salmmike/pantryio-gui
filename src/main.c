#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

#define G_APPLICATION_DEFAULT_FLAGS (GApplicationFlags) 0
#define STRINT_MAX_COUNT 255

#define COMMUNICATOR_UNIX_ADDRESS "unix:abstract=pantryiocommunication"

/**
 * @enum PantryState
 * @brief Info about an item status in pantry.
 */
enum ItemState
{
  out_of_stock = 0,
  in_stock = 1,
};

/**
 * @struct PantryItem
 * @brief Holds name and status of a pantry item
 */
struct PantryItem
{
  char* name;            /**< PantryItem#name contains the name of the product.*/
  enum ItemState status; /**< PantryItem#status cantains a ItemState enum that tells if the item is in stock or not.*/
};

/**
 * @brief Ask the DBus interface for current pantry data. Caller must free the returned pointer.
 * @return a string containing item names and their status.
 * The name and status are separated by an equal sign (=) and the items by a comma (,)
 */
gchar* get_pantry_dbus_status()
{
    GDBusConnection *connection;
    const gchar *response;
    GVariant *value;
    GError *error;
    gchar *opt_address = COMMUNICATOR_UNIX_ADDRESS;

    error = NULL;
    connection = g_dbus_connection_new_for_address_sync(opt_address,
                                                        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                        NULL, /* GDBusAuthObserver */
                                                        NULL, /* GCancellable */
                                                        &error);
    if (connection == NULL) {
          g_printerr ("Error connecting to D-Bus address %s: %s\n", opt_address, error->message);
          g_error_free (error);
          return NULL;
    }

    value = g_dbus_connection_call_sync (connection,
                                        NULL, /* bus_name */
                                        "/org/gtk/GDBus/PantryIOCommunicator",
                                        "org.pantryio.GDBus.StatusInterface",
                                        "GetStatus",
                                        NULL,
                                        G_VARIANT_TYPE ("(s)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);

    if (value == NULL) {
          g_printerr ("Error invoking GetStatus(): %s\n", error->message);
          g_error_free (error);
        return NULL;
    }

    g_variant_get(value, "(&s)", &response);
    char* ret = calloc(strlen(response), sizeof(char));
    strcpy(ret, response);

    g_object_unref (connection);
    return ret;
}

/**
 * @brief Free a PantryItem array.
 */
void pantry_item_free(struct PantryItem* items)
{
  size_t i = 0;

  while (items[i].name[0] != 0) {
    free(items[i].name);
    ++i;
  }
  free(items[i].name);
  free(items);
}

/**
 * @brief Get an array of PantryItem data from the DBus interface.
 * @param data mallocs the PantryItem array here.
 * @return PantryItem count
 */
size_t get_pantry_data(struct PantryItem **out_data)
{
  struct PantryItem *data = malloc(STRINT_MAX_COUNT * sizeof(struct PantryItem));

  size_t str_begin = 0;
  size_t str_end = 0;
  size_t str_count = 0;
  gchar* status_data = NULL;

  status_data = get_pantry_dbus_status();

  for (size_t i = 0; i < STRINT_MAX_COUNT; ++i) {
    ++str_count;
    data[i].name = calloc(255, sizeof(char));

    while (status_data[str_end] && status_data[str_end] != ',') {
      ++str_end;
    }

    memcpy(data[i].name, &status_data[str_begin], (str_end - str_begin - 2) * sizeof(char));

    data[i].status = status_data[str_end - 1] - '0';

    if (status_data[str_end] == 0) {
      data[i+1].name = calloc(255, sizeof(char));
      break;
    }
    str_end = str_end + 1;
    str_begin = str_end;
  }

  free(status_data);

  *out_data = data;
  return str_count;
}

/*
 * @brief Creates a button inside @box
 * @param box GtkWidget* to a gtk_box
 * @param label_info PantryItem* with name and status.
 */
void create_button(GtkWidget*         box,
                   struct PantryItem* label_info)
{
  GtkWidget *button;
  button = gtk_button_new_with_label(label_info->name);

  if (label_info->status == out_of_stock) {
    GtkWidget *label = gtk_button_get_child(GTK_BUTTON(button));
    PangoAttribute *textColor = pango_attr_foreground_new(65535,0,0);
    PangoAttrList *const Attrs = pango_attr_list_new();
    pango_attr_list_insert(Attrs, textColor);
    gtk_label_set_attributes((GtkLabel *)label, Attrs);
  }

  gtk_box_append(GTK_BOX(box), button);
}

/*
 * @brief Create a window for GthApplication* app
 */
GtkWidget* create_window(GtkApplication *app,
                         int             width,
                         int             height)
{
  GtkWidget *window;
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size(GTK_WINDOW (window), width, height);
  return window;
}

/*
 * @brief Constructs window with data about pantry items.
 * Queries the data from the pantry-io-communication DBus interface.
 */
static void activate(GtkApplication *app,
                     gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *box;

  window = create_window(app, 200, 200);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

  gtk_window_set_child(GTK_WINDOW (window), box);

  struct PantryItem* data;
  size_t lenght = get_pantry_data(&data);

  for (size_t i = 0; i < lenght; ++i) {
    create_button(box, &data[i]);
  }
  gtk_widget_show (window);
}

int main(int    argc,
         char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run(G_APPLICATION (app), argc, argv);
  g_object_unref(app);

  return status;
}
