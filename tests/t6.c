#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main (gint argc, gchar **argv)  {
  GError *error = NULL;
  gchar **env = g_get_environ();

  g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
  GdkPixbufFormat *format = gdk_pixbuf_get_file_info (g_environ_getenv(env, "TEST_FILE"), NULL, NULL);
  if (!format) {
    g_error("%s", error->message);
  };

  g_assert(format != NULL);
  gchar *name = gdk_pixbuf_format_get_name (format);
  g_assert_cmpstr(name, ==, "webp");
  g_free (name);

  g_strfreev (env);
  gdk_pixbuf_format_free (format);
  return 0;
}
