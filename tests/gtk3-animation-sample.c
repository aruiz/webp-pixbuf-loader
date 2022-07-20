#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

/* This test is not an automated test.
 * It is intended to allow one to eye-ball webp animated images
 * in a debug environment, to see if there are problems with
 * image reproduction, timing, etc.
 */

typedef struct _AnimationStructure {
        GtkWindow               *window;
        GdkPixbufAnimation      *anim;
        GdkPixbufAnimationIter  *anim_iter;
        GtkWidget               *image;
        int                      delay;
} AnimationStructure;

gboolean
delete_objects(GtkWidget *widget, GdkEvent *event, gpointer data) {
        AnimationStructure *ani = (AnimationStructure *) data;
        if (ani->anim)
                g_object_unref(ani->anim);

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

                G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                GTimeVal curTime;
                g_get_current_time(&curTime);
                G_GNUC_END_IGNORE_DEPRECATIONS

                AnimationStructure *ani = g_new0(AnimationStructure, 1);
                ani->anim = anim;

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
