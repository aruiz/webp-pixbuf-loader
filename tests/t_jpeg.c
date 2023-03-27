#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main (gint argc, gchar **argv)
{
  GError *error = NULL;
  gchar **env   = g_get_environ ();

  gchar *contents = NULL;
  gsize  len      = 0;
  g_assert (g_file_get_contents (g_environ_getenv (env, "TEST_FILE"), &contents, &len, NULL));

  GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type ("webp", NULL);
  g_assert (loader != NULL);
  gdk_pixbuf_loader_write (loader, (const guchar *) contents, len, &error);
  g_assert (error != NULL);

  g_clear_pointer (&contents, g_free);
  g_clear_object (&loader);
  g_clear_error (&error);
  g_strfreev (env);
  return 0;
}