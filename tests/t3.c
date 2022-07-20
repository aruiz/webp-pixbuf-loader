/*
 * This tests duration for various frames in t5.webp .
 * The first frame and the fifth (last) frame have duration 1000,
 * and the other frames have duration 300.
 * The webp container has 3 loops.
*/

#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main(gint argc, gchar **argv)
{
        const gchar *file_path = g_getenv("TEST_FILE");

        /* setup animation. */
        g_autoptr(GdkPixbufAnimation) anim = gdk_pixbuf_animation_new_from_file (file_path, NULL);
        g_assert(anim != NULL); /* animation has been created. */

        if (gdk_pixbuf_animation_is_static_image(anim)) {
                g_error("TEST_FILE is not an animated WebP sample");
                return 1;
        }


        g_autoptr(GdkPixbufAnimationIter) anim_iter = gdk_pixbuf_animation_get_iter(anim, NULL);
        g_assert(anim_iter != NULL); /* animation iterator has been created. */

        int nframes = 1;
        while (gdk_pixbuf_animation_iter_advance(anim_iter, NULL)) {
                if (nframes == 1) {
                        g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                        g_assert(gdk_pixbuf_get_width(pixbuf) == 300 &&
                                        gdk_pixbuf_get_height(pixbuf) == 300);
                }

                int delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                if (delay < 0 ||
                    nframes > 15) {
                        break;
                }

                /* check duration for various frames. */
                switch (nframes) {
                        case 5:
                        case 10:
                        case 4:
                        case 9:
                        case 14:
                                g_assert(delay == 1000);
                                break;
                        case 1:
                        case 6:
                        case 11:
                                g_assert(delay == 300);
                                break;
                        default:
                                break;

                }

                nframes++;
        }

        g_assert(nframes == 15);
        return 0;
}