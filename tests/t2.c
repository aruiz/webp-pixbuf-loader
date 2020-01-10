#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main (gint argc, gchar **argv)  {
  GError *error = NULL;
  gchar **env = g_get_environ();

  g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (g_environ_getenv(env, "TEST_FILE"), &error);
  if (error) {
    g_error("%s", error->message);
  };
  g_assert(error == NULL);

  g_assert(gdk_pixbuf_get_width(pixbuf) == 200);
  g_assert(gdk_pixbuf_get_height(pixbuf) == 200);

  g_strfreev(env);
  g_object_unref(pixbuf);
  return 0;
}
