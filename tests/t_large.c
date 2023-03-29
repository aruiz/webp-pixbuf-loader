#include <gdk-pixbuf/gdk-pixbuf.h>

#define READ_BUFFER_SIZE 65535

gint
main (gint argc, gchar **argv)
{
  GError *error = NULL;
  gchar **env   = g_get_environ ();

  GdkPixbuf *pixbuf
      = gdk_pixbuf_new_from_file (g_environ_getenv (env, "TEST_FILE"), &error);

  if (error)
    g_error ("%s", error->message);

  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 4096);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 4096);

  g_clear_object (&pixbuf);
  g_strfreev (env);
  return 0;
}