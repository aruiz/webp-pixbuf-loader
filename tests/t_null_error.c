#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>

static gchar *
create_filename ()
{
  GString *pathbuild = g_string_new (g_get_tmp_dir ());
  gchar   *id        = g_uuid_string_random ();
  g_string_append_printf (pathbuild, "%s%s.webp", G_DIR_SEPARATOR_S, id);
  g_free (id);
  return g_string_free (pathbuild, FALSE);
}

gboolean
save_func (const gchar *buffer, gsize count, GError **error, gpointer data)
{
  return TRUE;
}

void
test_pixbufv (GdkPixbuf *pixbuf, gchar **keys, gchar **values)
{
  GStatBuf tmpstat = { 0 };
  gchar   *path    = create_filename ();
  g_assert (gdk_pixbuf_savev (pixbuf, path, "webp", keys, values, NULL));
  g_stat (path, &tmpstat);
  g_remove (path);
  g_free (path);

  g_assert (gdk_pixbuf_save_to_callbackv (pixbuf, save_func, NULL, "webp", keys, values, NULL));
}

int
main ()
{
  gchar **env = g_get_environ ();

  GdkPixbuf *pixbuf
      = gdk_pixbuf_new_from_file (g_environ_getenv (env, "TEST_FILE"), NULL);
  g_assert (pixbuf != NULL);
  g_clear_pointer (&env, g_strfreev);

  gchar *keys[2]   = { "icc-profile", NULL };
  gchar *values[2] = { "MQo=", NULL };

  test_pixbufv (pixbuf, NULL, NULL);
  test_pixbufv (pixbuf, keys, values);

  g_object_unref (pixbuf);
}