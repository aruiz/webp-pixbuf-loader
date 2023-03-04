#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <webp/decode.h>
#include <webp/mux.h>

static gchar *
create_filename ()
{
  GString *pathbuild = g_string_new (g_get_tmp_dir ());
  gchar   *id        = g_uuid_string_random ();
  g_string_append_printf (pathbuild, "%s%s.webp", G_DIR_SEPARATOR_S, id);
  g_free (id);
  return g_string_free (pathbuild, FALSE);
}

void
test_webp_icc_output (guchar *buffer, gsize buf_size, const gchar *base64_string)
{
  WebPData data   = { .bytes = buffer, .size = buf_size };
  WebPData output = { 0 };
  WebPMux *mux    = WebPMuxCreate (&data, FALSE);
  WebPMuxGetChunk (mux, "ICCP", &output);
  WebPMuxDelete (mux);

  g_assert (output.bytes != NULL);
  gchar *encoded_icc = g_base64_encode (output.bytes, output.size);
  g_assert (encoded_icc != NULL);
  g_assert_cmpstr (encoded_icc, ==, base64_string);

  g_free (encoded_icc);
}

void
test_webp_icc_read (const gchar *path) {
  GError *error = NULL;

  GdkPixbuf *icc_pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (error)
    g_error ("%s", error->message);
  g_assert (icc_pixbuf != NULL);

  const gchar* icc_option = gdk_pixbuf_get_option (icc_pixbuf, "icc-profile");
  g_assert (icc_option != NULL);
  g_assert_cmpstr ("MQo=", ==, icc_option);

  g_clear_object (&icc_pixbuf);
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

  /* Test on disk contents */
  gchar *path = create_filename ();
  gdk_pixbuf_save (pixbuf, path, "webp", &error, "icc-profile", "MQo=", NULL);
  if (error)
    g_error ("%s", error->message);

  gchar *buffer;
  gsize  buf_size;
  g_file_get_contents (path, &buffer, &buf_size, &error);
  if (error)
    g_error ("%s", error->message);

  test_webp_icc_output ((guchar *) buffer, buf_size, "MQo=");

  /* Test icc-profile read */
  test_webp_icc_read (path);

  g_remove (path);
  g_clear_pointer (&buffer, g_free);
  g_clear_pointer (&path, g_free);

  /* Test on memory contents */
  gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buf_size, "webp", &error,
                             "icc-profile", "MQo=", NULL);
  if (error)
    g_error ("%s", error->message);

  test_webp_icc_output ((guchar *) buffer, buf_size, "MQo=");

  g_free (buffer);
  g_object_unref (pixbuf);
  return 0;
}
