#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <webp/decode.h>

static gchar *
create_filename ()
{
  GString *pathbuild = g_string_new (g_get_tmp_dir ());
  gchar   *id        = g_uuid_string_random ();
  g_string_append_printf (pathbuild, "%s%s.webp", G_DIR_SEPARATOR_S, id);
  g_free (id);
  return g_string_free (pathbuild, FALSE);
}

static gboolean
save_func (const gchar *buf, gsize size, GError **error, gpointer user_data)
{
  gchar *path = create_filename ();

  g_file_set_contents (path, buf, size, error);
  if (*error)
    {
      g_free (path);
      return FALSE;
    }

  *(gchar **) user_data = path;
  return TRUE;
}

gint
main (gint argc, gchar **argv)
{
  GError *error = NULL;
  gchar **env   = g_get_environ ();

  GdkPixbuf *pixbuf
      = gdk_pixbuf_new_from_file (g_environ_getenv (env, "TEST_FILE"), &error);
  if (error)
    g_error ("%s", error->message);

  g_assert (! gdk_pixbuf_get_has_alpha (pixbuf));

  g_assert (gdk_pixbuf_get_width (pixbuf) == 200);
  g_assert (gdk_pixbuf_get_height (pixbuf) == 200);

  g_strfreev (env);

  /* We compare if creating by file and by callback creates the same content */

  char  *keys[3] = { "preset", "quality", NULL };
  char  *vals[3] = { "photo", "98", NULL };
  gchar *path_1  = NULL;
  gdk_pixbuf_save_to_callbackv (pixbuf, save_func, &path_1, "webp", keys, vals, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert_nonnull (path_1);

  gchar *path_2 = create_filename ();
  gdk_pixbuf_save (pixbuf, path_2, "webp", &error, keys[0], vals[0], keys[1], vals[1], NULL);
  if (error)
    g_error ("%s", error->message);

  gsize  length    = 0;
  gchar *contents1 = NULL;
  g_file_get_contents (path_1, &contents1, &length, &error);
  if (error)
    g_error ("%s", error->message);

  gchar *sum1 = g_compute_checksum_for_data (G_CHECKSUM_SHA256, (guchar *) contents1, length);
  g_assert_nonnull (sum1);

  gchar *contents2 = NULL;
  g_file_get_contents (path_2, &contents2, &length, &error);
  if (error)
    g_error ("%s", error->message);

  gchar *sum2 = g_compute_checksum_for_data (G_CHECKSUM_SHA256, (guchar *) contents2, length);
  g_assert_nonnull (sum2);

  /* check if they share the same checksum */
  g_assert (g_strcmp0 (sum1, sum2) == 0);

  GdkPixbuf *pb_from_file = gdk_pixbuf_new_from_file (path_1, &error);

  g_assert_cmpint (gdk_pixbuf_get_width (pb_from_file), ==, 200);
  g_assert_cmpint (gdk_pixbuf_get_height (pb_from_file), ==, 200);
  g_assert (! gdk_pixbuf_get_has_alpha (pb_from_file));

  g_remove (path_1);
  g_remove (path_2);
  g_free (sum1);
  g_free (sum2);
  g_free (contents1);
  g_free (contents2);
  g_free (path_1);
  g_free (path_2);
  g_object_unref (pixbuf);
  g_object_unref (pb_from_file);
  return 0;
}
