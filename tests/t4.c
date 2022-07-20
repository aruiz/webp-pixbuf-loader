#include <gdk-pixbuf/gdk-pixbuf.h>

#define IMAGE_READ_BUFFER_SIZE 500
gint
main(gint argc, gchar **argv) {
        gchar **env = g_get_environ();
        const gchar *file_path = g_environ_getenv(env, "TEST_FILE");
        g_warning("%s", file_path);

        /* setup animation. */
        gchar *mime_type = "image/webp";

        GFile *file1 = g_file_new_for_path(file_path);
        GFileInputStream *input_stream = g_file_read (file1, NULL, NULL);

        guchar buffer[IMAGE_READ_BUFFER_SIZE];
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_mime_type(mime_type, NULL);
        g_assert(GDK_IS_PIXBUF_LOADER(loader));

        goffset bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
                                          &buffer,
                                          IMAGE_READ_BUFFER_SIZE,
                                          NULL, NULL);
        goffset bytes_read_total = bytes_read;
        while (bytes_read > 0) {
                /* Check if data accumulation has failed. */
                g_assert(gdk_pixbuf_loader_write(loader, (guchar*)&buffer, bytes_read, NULL));

                bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
                                                  &buffer,
                                                  IMAGE_READ_BUFFER_SIZE,
                                                  NULL, NULL);
                bytes_read_total += bytes_read;
        }

        g_assert((bytes_read == 0) && (bytes_read_total > 0)); /* Finished reading all data. */

        GdkPixbufAnimation *anim = gdk_pixbuf_loader_get_animation(loader);
        g_assert((anim != NULL)); /* animation has been created. */

        if (gdk_pixbuf_animation_is_static_image(anim)) {
                GdkPixbuf *static_pixbuf = gdk_pixbuf_animation_get_static_image(anim);
                g_assert((static_pixbuf != NULL));
                g_object_unref(static_pixbuf);
                g_object_unref(anim);
                goto end_of_test;
        }

        int cntFrames = 0;
        GdkPixbufAnimationIter *anim_iter = gdk_pixbuf_animation_get_iter(anim, NULL);
        g_assert(anim_iter != NULL); /* animation iterator has been created. */
        while (TRUE) {
                G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                GTimeVal curTime;
                g_get_current_time(&curTime);
                G_GNUC_END_IGNORE_DEPRECATIONS

                if (gdk_pixbuf_animation_iter_advance(anim_iter, &curTime)) {
                        cntFrames += 1;
                        if (cntFrames == 1) {
                                GdkPixbuf *pixbuf =  gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                                int w = gdk_pixbuf_get_width(pixbuf);
                                int h = gdk_pixbuf_get_height(pixbuf);
                                g_print("Width is %d and Height is %d .\n", w, h);
                                g_assert(w == 300);
                                g_assert(h == 300);
                                g_object_unref(pixbuf);
                        }
                } else {
                        break;
                }
                gint delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                if ((delay < 0) || (cntFrames > 100)) {
                        break;
                }
        }
        g_print("Total frames parsed: %d\n", cntFrames);
        g_assert(cntFrames == 10);
        /* note there should be 2 loops in the test t3.webp file. */

        end_of_test:
        g_object_unref(G_OBJECT (input_stream));
        g_object_unref(G_OBJECT (file1));
        gdk_pixbuf_loader_close(loader, NULL);
        g_object_unref(loader);
        g_object_unref(anim_iter);
        g_object_unref(anim);
        g_strfreev(env);
        return 0;
}
