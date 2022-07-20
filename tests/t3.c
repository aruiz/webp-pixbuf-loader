#include <gdk-pixbuf/gdk-pixbuf.h>

gint
main(gint argc, gchar **argv) {
        gchar **env = g_get_environ();
        g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
        GdkPixbufAnimation *anim = NULL;
        anim = gdk_pixbuf_animation_new_from_file(g_environ_getenv(env, "TEST_FILE"), NULL);
        gboolean isStatic = gdk_pixbuf_animation_is_static_image(anim);
        if (!isStatic) {
                int cntFrames = 0;

                G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                GTimeVal curTime;
                g_get_current_time(&curTime);
                G_GNUC_END_IGNORE_DEPRECATIONS

                GdkPixbufAnimationIter *anim_iter = gdk_pixbuf_animation_get_iter(anim, &curTime);
                while (TRUE) {
                        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                        g_get_current_time(&curTime);
                        G_GNUC_END_IGNORE_DEPRECATIONS

                        if (gdk_pixbuf_animation_iter_advance(anim_iter, &curTime)) {
                                cntFrames += 1;
                                GdkPixbuf *pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                                int w = gdk_pixbuf_get_width(pixbuf);
                                int h = gdk_pixbuf_get_height(pixbuf);
                                if (cntFrames == 1) {
                                        g_print("Width is %d and Height is %d .\n", w, h);
                                        g_assert(w == 300);
                                        g_assert(h == 300);
                                }
                        } else {
                                break;
                        }
                        int delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                        if ((delay < 0) || (cntFrames > 100)) {
                                break;
                        }
                }
                g_print("Total frames parsed: %d\n", cntFrames);
                g_assert(cntFrames == 10);
                /* note there should be 2 loops in the test t3.webp file. */
                g_object_unref(anim_iter);
        }
        g_object_unref(anim);
        g_strfreev(env);

        return 0;
}
