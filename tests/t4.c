#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main (gint argc, gchar **argv)
{
  gchar **env = g_get_environ ();

  gint             w = 0, h = 0;
  GdkPixbufFormat *format
      = gdk_pixbuf_get_file_info (g_environ_getenv (env, "TEST_FILE"), &w, &h);

  g_assert_cmpint (w, ==, 1);
  g_assert_cmpint (h, ==, 1);

  g_assert (format != NULL);
  gchar *name = gdk_pixbuf_format_get_name (format);
  g_assert_cmpstr (name, ==, "webp");
  g_free (name);

  g_strfreev (env);
  return 0;
}
