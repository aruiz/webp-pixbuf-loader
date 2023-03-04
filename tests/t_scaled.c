#include <gdk-pixbuf/gdk-pixbuf.h>

int
main ()
{
  gchar **env = g_get_environ ();

  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size (g_environ_getenv (env, "TEST_FILE"),
                                                        110, 40, NULL);
  g_assert (pixbuf);

  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 40);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 40);
  g_clear_object (&pixbuf);

  pixbuf = gdk_pixbuf_new_from_file_at_size (g_environ_getenv (env, "TEST_FILE_"
                                                                    "ANIM"),
                                             110, 40, NULL);
  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 40);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 40);

  g_clear_pointer (&env, g_strfreev);
  g_object_unref (pixbuf);
}