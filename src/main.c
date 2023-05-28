#include <gtk/gtk.h>

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

struct EntryData* get_data()
{
  struct EntryData* data = malloc(STRINT_MAX_COUNT * sizeof(struct EntryData));

  for (size_t i = 0; i < STRINT_MAX_COUNT; ++i) {
    data[i].text = calloc(255, sizeof(char));
  }

  strcpy(data[0].text, "hello");
  strcpy(data[1].text, "world");
  strcpy(data[2].text, "more");
  strcpy(data[3].text, "buttons");
  strcpy(data[3].text, "rossi long strings big sentence even");
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
