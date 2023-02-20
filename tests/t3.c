#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main(gint argc, gchar **argv) {
        GdkPixbufAnimation *anim = gdk_pixbuf_animation_new_from_file(g_getenv("TEST_FILE"), NULL);
        g_assert(anim != NULL);
        g_assert(!gdk_pixbuf_animation_is_static_image(anim)); // Make sure it is an animation

        GdkPixbufAnimationIter *anim_iter = gdk_pixbuf_animation_get_iter(anim, NULL);
        g_assert(anim_iter != NULL);

        int nframes;
        for (nframes = 1; gdk_pixbuf_animation_iter_advance(anim_iter, NULL); nframes++) {
                g_assert_cmpint(nframes, <=, 15); // avoid infinite loop (test image has 5 frames and 3 loops)
                if (nframes == 1) { // check canvas size
                        GdkPixbuf *pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                        int w = gdk_pixbuf_get_width(pixbuf);
                        int h = gdk_pixbuf_get_height(pixbuf);
                        g_assert_cmpint(w, ==, 300);
                        g_assert_cmpint(h, ==, 300);
                }

                int delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);  
                if (delay == -1)
                        break;

                // check frame duration
                if ((nframes % 5) == 0 || (nframes % 5) == 4)
                        g_assert_cmpint(delay, ==, 1000);
                else
                        g_assert_cmpint(delay, ==, 300);
        }
        g_assert(nframes == 15);
        g_object_unref (anim_iter);
        g_object_unref (anim);
        return 0;
}