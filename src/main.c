/*
* Copyright (c) 2023 Mike Salmela
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

#define G_APPLICATION_DEFAULT_FLAGS (GApplicationFlags) 0
#define STRINT_MAX_COUNT 255

#define COMMUNICATOR_UNIX_ADDRESS "unix:abstract=pantryiocommunication"
#define COMMUNICATOR_BUS_NAME "org.pantryio.GDBus.StatusInterface"
#define COMMUNICATOR_INTERFACE_NAME COMMUNICATOR_BUS_NAME
#define COMMUNICATOR_OBJECT_PATH "/org/gtk/GDBus/PantryIOCommunicator"

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
  char* name;            /**< PantryItem#name (char*) contains the name of the product.*/
  enum ItemState status; /**< PantryItem#status(itemState) contains a ItemState enum that tells if the item is in stock or not.*/
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
                                         COMMUNICATOR_BUS_NAME, /* bus_name */
                                         COMMUNICATOR_OBJECT_PATH,
                                         COMMUNICATOR_INTERFACE_NAME,
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

static void
on_pantry_data_changed(GDBusConnection *connection,
                       const gchar     *sender_name,
                       const gchar     *object_path,
                       const gchar     *interface_name,
                       const gchar     *signal_name,
                       GVariant        *parameters,
                       gpointer         user_data)
{
  // TODO
  printf("on pantry data changed\n");
}

/**
 * @brief Subscribes @cb_function to an update signal.
 * @param callback a function to be called when the update signal happens.
 */
GDBusConnection* subscribe_to_update_signal(/*void(*callback)()*/)
{
  GDBusConnection *connection;
  GError *error;
  error = NULL;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (error != NULL) {
      g_printerr("Failed to get bus connection: %s\n", error->message);
      g_error_free(error);
      return NULL;
  }

  g_dbus_connection_signal_subscribe(connection,
                                     NULL,
                                     COMMUNICATOR_INTERFACE_NAME,
                                     "PantryDataChanged",
                                     COMMUNICATOR_OBJECT_PATH,
                                     NULL,
                                     G_DBUS_SIGNAL_FLAGS_NONE,
                                     on_pantry_data_changed,
                                     NULL,
                                     NULL);
  return connection;
}

/**
 * @brief Free a PantryItem array.
 */
void pantry_item_free(struct PantryItem* items)
{
  if (items == NULL) {
    return;
  }

  size_t i = 0;

  while (items[i].name != NULL) {
    free(items[i].name);
    ++i;
  }
  free(items);
}

/**
 * @brief Get an array of PantryItem data from the DBus interface.
 * If out_data != NULL, the caller must free the memory after usage using
 * pantry_item_free.
 * @param out_data mallocs the PantryItem array here.
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
  if (status_data == NULL) {
    free(data);
    *out_data = NULL;
    return 0;
  }

  for (size_t i = 0; i < STRINT_MAX_COUNT; ++i) {
    ++str_count;
    data[i].name = calloc(255, sizeof(char));

    while (status_data[str_end] && status_data[str_end] != ',') {
      ++str_end;
    }

    memcpy(data[i].name, &status_data[str_begin], (str_end - str_begin - 2) * sizeof(char));

    data[i].status = status_data[str_end - 1] - '0';

    if (status_data[str_end] == 0) {
      data[i+1].name = NULL;
      break;
    }

    str_end = str_end + 1;
    str_begin = str_end;
  }

  free(status_data);
  *out_data = data;
  return str_count;
}

/**
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

/**
 * @brief Create a window for GthApplication* app
 * @param app the running GtkApplication
 */
GtkWidget* create_window(GtkApplication *app)
{
  GtkWidget *window;
  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW(window), "MainWindow");
  gtk_window_set_default_size(GTK_WINDOW(window), 100, 100);
  //gtk_window_fullscreen(GTK_WINDOW(window));
  return window;
}

/**
 * @brief Constructs window with data about pantry items.
 * Queries the data from the pantry-io-communication DBus interface.
 */
static void activate(GtkApplication *app,
                     gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *box;

  subscribe_to_update_signal();

  window = create_window(app);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

  gtk_window_set_child(GTK_WINDOW (window), box);

  struct PantryItem* data;
  size_t lenght = get_pantry_data(&data);

  for (size_t i = 0; i < lenght; ++i) {
    create_button(box, &data[i]);
  }
  pantry_item_free(data);
  gtk_widget_show(window);
}

/**
 * @brief Refresh contents of the UI. Gets new data from the DBus interface.
 */
static void refresh(GtkApplication *app)
{
  // TODO
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
