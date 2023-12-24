#include <gdk-pixbuf/gdk-pixbuf.h>

gint area_prepared_cb_count = 0;
gint area_updated_cb_count = 0;
gint closed_cb_count = 0;
gint size_prepared_cb_count = 0;

void
area_prepared_cb (GdkPixbufLoader* self, gpointer user_data)
{
  area_prepared_cb_count++;
}

void
area_updated_cb (GdkPixbufLoader* self, gint x, gint y, gint width, gint height, gpointer user_data)
{
  area_updated_cb_count++;

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  g_assert_cmpint (width, ==, 200);
  g_assert_cmpint (height, ==, 200);
}

void
closed_cb (GdkPixbufLoader* self, gpointer user_data)
{
  closed_cb_count++;
}

void
size_prepared_cb (GdkPixbufLoader* self, gint width, gint height, gpointer user_data)
{
  size_prepared_cb_count++;

  g_assert_cmpint (width, ==, 200);
  g_assert_cmpint (height, ==, 200);
}

gint
main (gint argc, gchar **argv)
{
  gchar **env   = g_get_environ ();
  GdkPixbufLoader *loader;
  GError *error = NULL;
  gsize pixbuf_size;
  guchar *pixbuf_data;

  loader = gdk_pixbuf_loader_new();

  g_signal_connect (G_OBJECT(loader), "area-prepared", G_CALLBACK(area_prepared_cb), NULL);
  g_signal_connect (G_OBJECT(loader), "area-updated", G_CALLBACK(area_updated_cb), NULL);
  g_signal_connect (G_OBJECT(loader), "closed", G_CALLBACK(closed_cb), NULL);
  g_signal_connect (G_OBJECT(loader), "size-prepared", G_CALLBACK(size_prepared_cb), NULL);

  g_file_get_contents (g_environ_getenv (env, "TEST_FILE"), (gchar**)&pixbuf_data, &pixbuf_size, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  gdk_pixbuf_loader_write(loader, pixbuf_data, pixbuf_size, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  g_free(pixbuf_data);

  gdk_pixbuf_loader_close(loader, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (error == NULL);

  g_object_unref(G_OBJECT(loader));

  g_strfreev (env);

  g_assert_cmpint (area_prepared_cb_count, ==, 1);
  g_assert_cmpint (area_updated_cb_count, ==, 1);
  g_assert_cmpint (closed_cb_count, ==, 1);
  g_assert_cmpint (size_prepared_cb_count, ==, 1);

  return 0;
}
