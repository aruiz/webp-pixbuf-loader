#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gslist.h>

#define  IMAGE_READ_BUFFER_SIZE 2000         /* 65535 */

typedef struct _AnimationStructure {
        GtkWindow *window;
        GdkPixbufAnimation *anim;
        GtkWidget *image;
        gboolean cancel_loading;
        GString *file_path;
        GError *error;
} AnimationStructure;

static void
callback_area_prepared(GdkPixbufLoader *loader,
                       AnimationStructure *ani);

/* callback seen at: eog-image.c */
static void
callback_image_size_prepared(GdkPixbufLoader *loader,
                             gint width,
                             gint height,
                             AnimationStructure *data) {
        AnimationStructure *ani = data;
        gtk_window_resize(ani->window, width, height);
        g_signal_connect(loader, "area-prepared",
                         (GCallback) callback_area_prepared, ani);

}

/* callbacks seen at: ./gdk-pixbuf/gdk-pixbuf/tests/pixbuf-area-updated.c */
#if 0
static void
callback_area_updated(GdkPixbufLoader *loader,
                      int x,
                      int y,
                      int width,
                      int height,
                      GdkPixbuf *pixbuf_old) {
        GdkPixbuf *pixbuf_new;

        pixbuf_new = gdk_pixbuf_loader_get_pixbuf(loader);

        /* update pixbuf */
        gdk_pixbuf_copy_area(pixbuf_new, x, y, width, height, pixbuf_old, x, y);
}
#endif

/* free copy of pixbuf used in area-updated callback. */
/*
static void
callback_closed (GdkPixbufLoader *loader,
                GdkPixbuf       *pixbuf_copy) {
   g_object_unref (pixbuf_copy);
}
*/

static void
callback_area_prepared(GdkPixbufLoader *loader,
                       AnimationStructure *ani) {
        GdkPixbufAnimation *anim = gdk_pixbuf_loader_get_animation(loader);
        /* GdkPixbuf *pixbuf_copy = gdk_pixbuf_copy (gdk_pixbuf_loader_get_pixbuf (loader)); */
        ani->anim = anim;
        if (gdk_pixbuf_animation_is_static_image(anim)) {
                GdkPixbuf *staticPixbuf = gdk_pixbuf_animation_get_static_image(anim);
                g_object_unref(anim);
                ani->anim = NULL;
                ani->image = gtk_image_new_from_pixbuf(staticPixbuf);
        } else {
                ani->image = gtk_image_new_from_animation(anim);
        }

        /* connect callbacks for other signals.
        g_signal_connect (loader, "area-updated",
                          (GCallback) callback_area_updated, (gpointer) pixbuf_copy);
        g_signal_connect (loader, "closed",
                          (GCallback) callback_closed, (gpointer) pixbuf_copy);
        */
}

/*
 * This test code is based on code in eog, eog-image.c and specifically
 * the function eog_image_real_load().
 * We need to input a stream of data.
 * Read from this stream in a loop.
 * Check each data read chunk
 * If it checks, add to the pixbuf data.
 * When finished show the pixbuf.
 */
void setup_image(AnimationStructure *ani, GError **error) {
        GFileInputStream *input_stream;
        GdkPixbufLoader *loader = NULL;
        gchar *mime_type = "image/webp";
        goffset bytes_read = 0;
        goffset bytes_read_total = 0;
        GdkPixbufAnimation *anim = NULL;

        char *astr = g_new0(char, ani->file_path->len + 1);
        memcpy(astr, ani->file_path->str, ani->file_path->len);
        GFile *file1 = g_file_new_for_path(astr);
        g_free(astr);
        input_stream = g_file_read(file1, NULL, error);
        guchar *buffer = g_new0 (guchar, IMAGE_READ_BUFFER_SIZE);

        /*
        GSList *formats;
        gchar **mimes;
        gchar *name;
        for (formats =gdk_pixbuf_get_formats (); formats ; formats = g_slist_next (formats)) {
            GdkPixbufFormat *info = (GdkPixbufFormat *)formats->data;
            name = gdk_pixbuf_format_get_name(info);
            mimes = gdk_pixbuf_format_get_mime_types(info);
        }
         */

        loader = gdk_pixbuf_loader_new_with_mime_type(mime_type, error);
        if (loader == NULL) {
                loader = gdk_pixbuf_loader_new_with_type("webp", error);
        }
        if (error && *error) {
                g_error_free(*error);
                *error = NULL;
        }
        g_signal_connect (loader,
                          "size-prepared",
                          G_CALLBACK(callback_image_size_prepared),
                          ani);

        gboolean failed = FALSE;
        gboolean have_compete_data = FALSE;
        while (!ani->cancel_loading) {
                bytes_read = g_input_stream_read(G_INPUT_STREAM (input_stream),
                                                 buffer,
                                                 IMAGE_READ_BUFFER_SIZE,
                                                 NULL, error);
                if (bytes_read == 0) {
                        /* End of the file */
                        have_compete_data = TRUE;
                        break;
                } else if (bytes_read == -1) {
                        failed = TRUE;

                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "Failed to read from input stream");

                        break;
                }
                bytes_read_total += bytes_read;
                if (!gdk_pixbuf_loader_write(loader, buffer, bytes_read, error)) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "Failed to parse WebP input stream");
                        failed = TRUE;
                        break;
                }
        }

        if (have_compete_data && !failed) {
                GtkWidget *image;

                anim = gdk_pixbuf_loader_get_animation(loader);
                ani->anim = anim;
                if (gdk_pixbuf_animation_is_static_image(anim)) {
                        GdkPixbuf *staticPixbuf;
                        staticPixbuf = gdk_pixbuf_animation_get_static_image(anim);
                        g_object_unref(anim);
                        ani->anim = NULL;
                        image = gtk_image_new_from_pixbuf(staticPixbuf);
                } else {
                        image = gtk_image_new_from_animation(anim);
                }
                ani->image = image;
        }

        g_free(buffer);
        g_object_unref(G_OBJECT (file1));
        g_object_unref(G_OBJECT (input_stream));
}

static void activate(GtkApplication *app, AnimationStructure *user_data) {
        GtkWidget *window;
        GtkWidget *label;
        GError *error;

        AnimationStructure *ani = user_data;
        error = ani->error;

        window = gtk_application_window_new(app);
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        label = gtk_label_new("Test WebP Animation");
        gtk_container_add(GTK_CONTAINER (box), label);
        ani->window = GTK_WINDOW (window);

        setup_image(ani, &error);
        if (ani->image) {
                gtk_container_add(GTK_CONTAINER (box), ani->image);
        }
        gtk_container_add(GTK_CONTAINER (window), box);
        gtk_window_set_title(GTK_WINDOW (window), "Test");
        gtk_window_set_default_size(GTK_WINDOW (window), 500, 500);
        gtk_widget_show_all(window);
} /* end of function activate */


gint
main(gint argc, gchar **argv) {
        GError *error = g_new0(GError, 1);
        gchar **env = g_get_environ();
        g_warning("%s", g_environ_getenv(env, "TEST_FILE"));
        const gchar *file_path = g_environ_getenv(env, "TEST_FILE");
        gtk_init(&argc, &argv);

        GtkApplication *app;
        AnimationStructure *ani = g_new0(AnimationStructure, 1);
        ani->file_path = g_string_new(file_path);
        ani->error = error;
        app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
        g_signal_connect(app, "activate", G_CALLBACK(activate), ani);
        (void) g_application_run(G_APPLICATION(app), argc, argv);
        g_object_unref(app);
        g_string_free(ani->file_path, TRUE);
        g_free(ani);
        g_strfreev(env);
        return 0;
}

