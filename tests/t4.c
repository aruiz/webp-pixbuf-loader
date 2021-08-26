#include <gdk-pixbuf/gdk-pixbuf.h>

#define GDK_TEST        1       /* There is a test using just GDK, or one using GTK. */

#ifndef GDK_TEST        /* Then do the test using GTK. */
#include <gtk/gtk.h>

typedef struct _AnimationStructure {
        GtkWindow *window;
        GdkPixbufAnimation *anim;
        GdkPixbufAnimationIter *anim_iter;
        GtkWidget *image;
        int delay;
} AnimationStructure;

gboolean
delete_objects(GtkWidget *widget, GdkEvent *event, gpointer data) {
        AnimationStructure *ani = (AnimationStructure *) data;
        if (ani->anim) {
                g_object_unref(ani->anim);
        }
        g_free(ani);
        return FALSE;
}

static void activate(GtkApplication *app, gpointer user_data) {
        GtkWidget *window;
        GtkWidget *label;
        AnimationStructure *ani = (AnimationStructure *) user_data;

        window = gtk_application_window_new(app);
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        label = gtk_label_new("Test WebP Animation");
        gtk_container_add(GTK_CONTAINER (box), label);

        GtkWidget *image = NULL;

        /*GdkPixbuf *staticPixbuf = NULL;
          staticPixbuf = gdk_pixbuf_animation_get_static_image (ani->anim);
          image = gtk_image_new_from_pixbuf (staticPixbuf);
        */

        image = gtk_image_new_from_animation(ani->anim);
        gtk_container_add(GTK_CONTAINER (box), image);
        gtk_container_add(GTK_CONTAINER (window), box);
        gtk_window_set_title(GTK_WINDOW (window), "Test");
        gtk_window_set_default_size(GTK_WINDOW (window), 500, 500);
        g_signal_connect(G_OBJECT(window),
                         "delete-event", G_CALLBACK(delete_objects), ani);
        gtk_widget_show_all(window);
} /* end of function activate */


gint
main(gint argc, gchar **argv) {
        GError *error = NULL;
        gchar **env = g_get_environ();
        g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
        gtk_init(&argc, &argv);

        /* setup animation. */
        GdkPixbufAnimation *anim = NULL;
        GdkPixbufAnimationIter *anim_iter = NULL;
        anim = gdk_pixbuf_animation_new_from_file(g_environ_getenv(env, "TEST_FILE"), &error);
        gboolean isStatic = gdk_pixbuf_animation_is_static_image(anim);
        if (!isStatic) {
                GtkApplication *app;
                GTimeVal curTime;
                AnimationStructure *ani = g_new0(AnimationStructure, 1);
                ani->anim = anim;
                g_get_current_time(&curTime);
                anim_iter = gdk_pixbuf_animation_get_iter(anim, &curTime);
                int delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                ani->anim_iter = anim_iter;
                ani->delay = delay;
                app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
                g_signal_connect(app, "activate", G_CALLBACK(activate), ani);
                (void) g_application_run(G_APPLICATION(app), argc, argv);
                g_object_unref(app);
        }

        g_strfreev(env);
        return 0;
}

#else

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

        if (! GDK_IS_PIXBUF_LOADER(loader)) {
                g_assert(FALSE);
        }
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
                cntFrames += 1;
                g_get_current_time(&curTime);
                hasAdv = gdk_pixbuf_animation_iter_advance(anim_iter, &curTime);
                if (hasAdv) {
                        pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                        int w = gdk_pixbuf_get_width(pixbuf);
                        int h = gdk_pixbuf_get_height(pixbuf);
                        if (cntFrames == 1) {
                                g_print("Width is %d and Height is %d .", w, h);
                                g_assert(w == 67);
                                g_assert(h == 76);
                        }
                }
                delay = gdk_pixbuf_animation_iter_get_delay_time(anim_iter);
                if ((delay < 0) || (cntFrames > 100)) {
                        break;
                }
        }
        g_print("Total frames parsed: %d", cntFrames);
        g_assert(cntFrames == 11);
        /* note there should be 2 loops in the test t3.webp file. */

        end_of_test:
        g_object_unref(G_OBJECT (input_stream));
        g_object_unref(G_OBJECT (file1));
        gdk_pixbuf_loader_close(loader, error);
        g_object_unref(loader);
        g_object_unref(anim_iter);
        g_object_unref(anim);
        g_object_unref(anim_iter);
        g_free(buffer);
        g_strfreev(env);
        return 0;
}
#endif