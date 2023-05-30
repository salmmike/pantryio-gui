#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

#define G_APPLICATION_DEFAULT_FLAGS (GApplicationFlags) 0
#define STRINT_MAX_COUNT 255

struct EntryData
{
  char* text;
  int status;
};


static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

gchar*
get_pantry_status()
{
    GDBusConnection *connection;
    const gchar *response;
    GVariant *value;
    GError *error;
    gchar *opt_address = "unix:abstract=pantryiocommunication";

    error = NULL;
    connection = g_dbus_connection_new_for_address_sync(opt_address,
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                       NULL, /* GDBusAuthObserver */
                                                       NULL, /* GCancellable */
                                                       &error);
    if (connection == NULL) {
          g_printerr ("Error connecting to D-Bus address %s: %s\n", opt_address, error->message);
          g_error_free (error);
          return EXIT_FAILURE;
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
          g_printerr ("Error invoking HelloWorld(): %s\n", error->message);
          g_error_free (error);
        return EXIT_FAILURE;
    }

    g_variant_get(value, "(&s)", &response);
    char* ret = calloc(strlen(response), sizeof(char));
    strcpy(ret, response);

    g_object_unref (connection);
    return ret;
}

struct EntryData* get_data()
{
  struct EntryData* data = malloc(STRINT_MAX_COUNT * sizeof(struct EntryData));

  size_t str_begin = 0;
  size_t str_end = 0;
  size_t str_count = 0;

  const gchar* status_data = get_pantry_status();

  for (size_t i = 0; i < STRINT_MAX_COUNT; ++i) {
    ++str_count;
    data[i].text = calloc(255, sizeof(char));

    while (status_data[str_end] && status_data[str_end] != ',') {
      ++str_end;
    }

    memcpy(data[i].text, &status_data[str_begin], (str_end - str_begin - 2) * sizeof(char));

    data[i].status = status_data[str_end - 1] - '0';

    if (status_data[str_end] == 0) {
      data[i+1].text = calloc(255, sizeof(char));
      break;
    }

    str_end = str_end + 1;
    str_begin = str_end;
  }

  free(status_data);

  return data;
}

void create_button(GtkWidget* box, struct EntryData* label_info)
{
  GtkWidget *button;
  button = gtk_button_new_with_label(label_info->text);
  gtk_box_append(GTK_BOX(box), button);
}

GtkWidget* create_window(GtkApplication *app)
{
  GtkWidget *window;
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);
  return window;
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *box;

  window = create_window(app);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_spacing(GTK_BOX(box), 10);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);

  gtk_window_set_child (GTK_WINDOW (window), box);

  struct EntryData* data = get_data();

  for (size_t i = 0; i < STRINT_MAX_COUNT; ++i) {
    if (data[i].text[0]) {
      create_button(box, &data[i]);
    } else {
      break;
    }
  }

  gtk_widget_show (window);
}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
