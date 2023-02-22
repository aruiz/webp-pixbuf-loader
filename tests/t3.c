#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

gint
main (gint argc, gchar **argv)
{
  GdkPixbufAnimation *anim
      = gdk_pixbuf_animation_new_from_file (g_getenv ("TEST_FILE"), NULL);
  g_assert (anim != NULL);
  g_assert (! gdk_pixbuf_animation_is_static_image (anim)); // Make sure it is an animation

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GTimeVal time = { 0 };
  G_GNUC_END_IGNORE_DEPRECATIONS

  GdkPixbufAnimationIter *iter = gdk_pixbuf_animation_get_iter (anim, &time);
  g_assert (iter != NULL);

  /* Frame 1 */
  time.tv_usec = 1000;
  g_assert (gdk_pixbuf_animation_iter_advance (iter, &time) == FALSE); // Step 1ms
  g_assert (gdk_pixbuf_animation_iter_get_pixbuf (iter) != NULL);
  g_assert_cmpint (gdk_pixbuf_animation_iter_get_delay_time (iter), ==, 1000);

  time.tv_sec += 1;
  g_assert (gdk_pixbuf_animation_iter_advance (iter, &time) == TRUE); // Step 1s and 1ms
  g_assert (gdk_pixbuf_animation_iter_get_pixbuf (iter) != NULL);
  g_assert_cmpint (gdk_pixbuf_animation_iter_get_delay_time (iter), ==, 300);

  time.tv_usec += 300000;
  g_assert (gdk_pixbuf_animation_iter_advance (iter, &time) == TRUE); // Step 300ms
  g_assert (gdk_pixbuf_animation_iter_get_pixbuf (iter) != NULL);
  g_assert_cmpint (gdk_pixbuf_animation_iter_get_delay_time (iter), ==, 300);

  time.tv_usec += 300000;
  g_assert (gdk_pixbuf_animation_iter_advance (iter, &time) == TRUE); // Step 300ms
  g_assert (gdk_pixbuf_animation_iter_get_pixbuf (iter) != NULL);
  g_assert_cmpint (gdk_pixbuf_animation_iter_get_delay_time (iter), ==, 300);

  time.tv_usec += 300000;
  g_assert (gdk_pixbuf_animation_iter_advance (iter, &time) == TRUE); // Step 300ms
  g_assert (gdk_pixbuf_animation_iter_get_pixbuf (iter) != NULL);
  g_assert_cmpint (gdk_pixbuf_animation_iter_get_delay_time (iter), ==, 1000);

  g_object_unref (iter);
  g_object_unref (anim);
  return 0;
}
