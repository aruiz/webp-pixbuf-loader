/*
 * This tests duration for various frames in t5.webp .
 * The first frame and the fifth (last) frame have duration 1000,
 * and the other frames have duration 300.
 * The webp container has 3 loops.
*/

#include <gdk-pixbuf/gdk-pixbuf.h>

#define IMAGE_READ_BUFFER_SIZE 500
gint
main(gint argc, gchar **argv) {
        GError *err = NULL;
        GError **error = &err;
        gchar **env = g_get_environ();
        g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
        const gchar *file_path = g_environ_getenv(env, "TEST_FILE");

        /* setup animation. */
        GFileInputStream *input_stream;
        goffset bytes_read = 0;
        goffset bytes_read_total = 0;
        guchar *buffer = NULL;
        gchar *mime_type = "image/webp";
        GdkPixbufLoader *loader = NULL;
        GdkPixbufAnimation *anim = NULL;
        GdkPixbufAnimationIter *anim_iter = NULL;
        gboolean failed = FALSE;

        GFile *file1 = g_file_new_for_path(file_path);
        input_stream = g_file_read (file1, NULL, error);

        buffer = g_new0 (guchar, IMAGE_READ_BUFFER_SIZE);
        loader = gdk_pixbuf_loader_new_with_mime_type(mime_type, error);

        if (! GDK_IS_PIXBUF_LOADER(loader))
                g_assert(FALSE);

        bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
                                          buffer,
                                          IMAGE_READ_BUFFER_SIZE,
                                          NULL, error);
        bytes_read_total += bytes_read;
        while (bytes_read > 0) {
                if (!gdk_pixbuf_loader_write(loader, buffer, bytes_read, error)) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "Failed to parse WebP input stream");
                        failed = TRUE;
                        break;
                }

                bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
                                                  buffer,
                                                  IMAGE_READ_BUFFER_SIZE,
                                                  NULL, error);
                bytes_read_total += bytes_read;
        }

        g_assert((bytes_read == 0) && (bytes_read_total > 0)); /* Finished reading all data. */
        g_assert(!failed); /* Data accumulation has not failed. */

        anim = gdk_pixbuf_loader_get_animation(loader);
        g_assert((anim != NULL)); /* animation has been created. */

        if (gdk_pixbuf_animation_is_static_image(anim)) {
                GdkPixbuf *staticPixbuf;
                staticPixbuf = gdk_pixbuf_animation_get_static_image(anim);
                g_assert((staticPixbuf != NULL));
                g_object_unref(staticPixbuf);
                g_object_unref(anim);
                goto end_of_test;
        }

        gboolean hasAdv = TRUE;
        int cntFrames = 0;
        int delay = 0;
        GTimeVal curTime;
        GdkPixbuf *pixbuf = NULL;
        anim_iter = gdk_pixbuf_animation_get_iter(anim, NULL);
        g_assert((anim_iter != NULL)); /* animation iterator has been created. */
        while (hasAdv) {
                if (cntFrames == 0) {
                        delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                        g_assert(delay == 1000);
                }

                gboolean has_new_frame = FALSE;
                g_get_current_time(&curTime);
                has_new_frame = gdk_pixbuf_animation_iter_advance(anim_iter, &curTime);
                if (has_new_frame) {
                        cntFrames += 1;
                        pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
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

                delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                if ((delay < 0) || (cntFrames > 100))
                        break;


                /* check duration for various frames. */
                if (cntFrames == 5 || cntFrames == 10)
                        g_assert(delay == 1000);

                if (cntFrames == 1 || cntFrames == 6 || cntFrames == 11)
                        g_assert(delay == 300);

                if (cntFrames == 4 || cntFrames == 9 || cntFrames == 14)
                        g_assert(delay == 1000);
        }
        g_print("Total frames parsed: %d\n", cntFrames);
        g_assert(cntFrames == 15);
        /* note there should be 3 loops in the test t5.webp file. */

        end_of_test:
        g_object_unref(G_OBJECT (input_stream));
        g_object_unref(G_OBJECT (file1));
        gdk_pixbuf_loader_close(loader, error);
        g_object_unref(loader);
        g_object_unref(anim_iter);
        g_object_unref(anim);
        g_free(buffer);
        g_strfreev(env);
        return 0;
}
