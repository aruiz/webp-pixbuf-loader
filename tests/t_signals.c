#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct
{
  gint area_prepared_cb_count;
  gint area_updated_cb_count;
  gint closed_cb_count;
  gint size_prepared_cb_count;
} SignalsCounters;

void
area_prepared_cb (GdkPixbufLoader *self, gpointer user_data)
{
  SignalsCounters *counters = (SignalsCounters *) user_data;

  counters->area_prepared_cb_count++;
}

void
area_updated_cb (GdkPixbufLoader *self, gint x, gint y, gint width, gint height, gpointer user_data)
{
  SignalsCounters *counters = (SignalsCounters *) user_data;

  counters->area_updated_cb_count++;

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  g_assert_cmpint (width, ==, 200);
  g_assert_cmpint (height, ==, 200);
}

void
closed_cb (GdkPixbufLoader *self, gpointer user_data)
{
  SignalsCounters *counters = (SignalsCounters *) user_data;

  counters->closed_cb_count++;
}

void
size_prepared_cb (GdkPixbufLoader *self, gint width, gint height, gpointer user_data)
{
  SignalsCounters *counters = (SignalsCounters *) user_data;

  counters->size_prepared_cb_count++;

  g_assert_cmpint (width, ==, 200);
  g_assert_cmpint (height, ==, 200);
}

gint
main (gint argc, gchar **argv)
{
  gchar          **env = g_get_environ ();
  GError          *error = NULL;
  gsize            pixbuf_size;
  guchar          *pixbuf_data = NULL;
  SignalsCounters  counters = { 0 };

  GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();

  g_signal_connect (G_OBJECT (loader), "area-prepared",
                    G_CALLBACK (area_prepared_cb), &counters);
  g_signal_connect (G_OBJECT (loader), "area-updated",
                    G_CALLBACK (area_updated_cb), &counters);
  g_signal_connect (G_OBJECT (loader), "closed", G_CALLBACK (closed_cb), &counters);
  g_signal_connect (G_OBJECT (loader), "size-prepared",
                    G_CALLBACK (size_prepared_cb), &counters);

  g_file_get_contents (g_environ_getenv (env, "TEST_FILE"),
                       (gchar **) &pixbuf_data, &pixbuf_size, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  gdk_pixbuf_loader_write (loader, pixbuf_data, pixbuf_size, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  g_clear_pointer (&pixbuf_data, g_free);

  gdk_pixbuf_loader_close (loader, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  g_clear_object (&loader);

  g_clear_pointer (&env, g_strfreev);

  g_assert_cmpint (counters.area_prepared_cb_count, ==, 1);
  g_assert_cmpint (counters.area_updated_cb_count, ==, 1);
  g_assert_cmpint (counters.closed_cb_count, ==, 1);
  g_assert_cmpint (counters.size_prepared_cb_count, ==, 1);

  return 0;
}
