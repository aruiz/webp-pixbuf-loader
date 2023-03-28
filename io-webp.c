/* GdkPixbuf library - WebP Image Loader
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * Copyright (C) 2022 Alberto Ruiz
 * Copyright (C) 2011 David Mazary
 * Copyright (C) 2014 Přemysl Janouch
 *
 * Authors: Alberto Ruiz <aruiz@gnome.org>
 *          David Mazary <dmaz@vt.edu>
 *          Přemysl Janouch <p.janouch@gmail.com>
 */

#include <string.h>

#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux.h>

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>
#undef GDK_PIXBUF_ENABLE_BACKEND

#include "io-webp-anim.h"

typedef struct
{
  GdkPixbufModuleSizeFunc     size_func;
  GdkPixbufModuleUpdatedFunc  update_func;
  GdkPixbufModulePreparedFunc prepare_func;
  gpointer                    user_data;
  WebPDecoderConfig           deccfg;
  gboolean                    got_header;
  gboolean                    is_animation;
  gboolean                    has_alpha;
  GByteArray                 *buffer;
  WebPIDecoder               *idec;
  GdkPixbuf                  *pb;
  gint                        width;
  gint                        height;
} WebPContext;

static gpointer
begin_load (GdkPixbufModuleSizeFunc     size_func,
            GdkPixbufModulePreparedFunc prepare_func,
            GdkPixbufModuleUpdatedFunc  update_func,
            gpointer                    user_data,
            GError                    **error)
{
  WebPContext *context  = g_new0 (WebPContext, 1);
  context->size_func    = size_func;
  context->prepare_func = prepare_func;
  context->update_func  = update_func;
  context->user_data    = user_data;
  context->deccfg       = (WebPDecoderConfig){ 0 };
  context->got_header   = FALSE;
  context->is_animation = FALSE;
  context->has_alpha    = FALSE;
  context->buffer       = NULL;
  context->idec         = NULL;
  context->pb           = NULL;
  context->width        = 0;
  context->height       = 0;

  return context;
}

// Create an empty pixbuf from the size function and target canvas size

static void
init_dec_config (WebPDecoderConfig *config, GdkPixbuf *pixbuf)
{
  guint pblen = 0;
  WebPInitDecoderConfig (config);
  config->options.use_scaling       = TRUE;
  config->options.scaled_width      = gdk_pixbuf_get_width (pixbuf);
  config->options.scaled_height     = gdk_pixbuf_get_height (pixbuf);
  config->output.is_external_memory = TRUE;
  config->output.colorspace = gdk_pixbuf_get_has_alpha (pixbuf) ? MODE_RGBA : MODE_RGB;
  config->output.u.RGBA.rgba = gdk_pixbuf_get_pixels_with_length (pixbuf, &pblen);
  config->output.u.RGBA.size   = (gsize) pblen;
  config->output.u.RGBA.stride = gdk_pixbuf_get_rowstride (pixbuf);
}

static gboolean
load_increment (gpointer data, const guchar *buf, guint size, GError **error)
{
  WebPContext *context = (WebPContext *) data;
  if (context->got_header == FALSE)
    {
      if (WebPGetInfo (buf, size, &context->width, &context->height) == FALSE)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not get WebP header information");
          return FALSE;
        }

      if (context->size_func)
        {
          context->size_func (&context->width, &context->height, context->user_data);

          // Check if this is a gdk_pixbuf_get_info call
          if (context->width == 0 || context->height == 0)
            {
              return TRUE;
            }
        }

      WebPBitstreamFeatures features;
      if (WebPGetFeatures (buf, size, &features) != VP8_STATUS_OK)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not get WebP image feature information");
          return FALSE;
        }
      context->got_header   = TRUE;
      context->has_alpha    = features.has_alpha;
      context->is_animation = features.has_animation;

      if (! context->is_animation)
        {
          gchar   *icc_data = NULL;
          WebPData wp_data  = { .bytes = buf, .size = size };
          WebPMux *mux      = WebPMuxCreate (&wp_data, FALSE);
          if (mux)
            {
              WebPData icc_profile = { 0 };
              if (WebPMuxGetChunk (mux, "ICCP", &icc_profile) == WEBP_MUX_OK
                  && icc_profile.bytes)
                icc_data = g_base64_encode (icc_profile.bytes, icc_profile.size);
              g_clear_pointer (&mux, WebPMuxDelete);
            }

          context->pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, context->has_alpha,
                                        8, context->width, context->height);

          if (! context->pb)
            {
              g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                           "Could not allocate GdkPixbuf");
              return FALSE;
            }

          if (icc_data)
            {
              gdk_pixbuf_set_option (context->pb, "icc-profile", icc_data);
              g_clear_pointer (&icc_data, g_free);
            }

          WebPDecoderConfig config;
          init_dec_config (&config, context->pb);

          context->idec = WebPIDecode (NULL, 0, &config);

          if (! context->idec)
            {
              g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                           "Could not allocate WebP incremental decoder");
              return FALSE;
            }

          if (context->prepare_func && context->pb)
            {
              context->prepare_func (context->pb, NULL, context->user_data);
            }
        }
      else
        {
          context->buffer = g_byte_array_new ();
        }
    }

  if (context->buffer)
    {
      g_byte_array_append (context->buffer, buf, size);
    }
  else if (context->pb && context->idec)
    {
      VP8StatusCode status = WebPIAppend (context->idec, buf, (size_t) size);
      if (status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                       "Could not incrementally decode WebP stream chunk");
          return FALSE;
        }

      gint last_y = 0;
      gint w      = 0;
      if (WebPIDecGetRGB (context->idec, &last_y, &w, NULL, NULL) == NULL
          && status != VP8_STATUS_SUSPENDED)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                       "Bad inputs to WebP decoder.");
          return FALSE;
        }

      if (status != VP8_STATUS_SUSPENDED && context->update_func)
        {
          context->update_func (context->pb, 0, 0, w, last_y, context->user_data);
        }
    }

  return TRUE;
}

static gboolean
stop_load (gpointer data, GError **error)
{
  WebPContext *context = (WebPContext *) data;
  gboolean     ret     = FALSE;

  if (context->got_header && context->is_animation)
    {
      GdkWebpAnimation *anim = gdk_webp_animation_new_from_bytes (context->buffer, error);
      context->buffer = NULL;

      GdkPixbufAnimationIter *iter
          = gdk_pixbuf_animation_get_iter (GDK_PIXBUF_ANIMATION (anim), NULL);

      GdkPixbuf *pb = gdk_pixbuf_animation_iter_get_pixbuf (iter);
      if (pb == NULL)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                       "Could not get Pixbuf from WebP animation iter");
        }
      else
        {
          if (context->prepare_func)
            context->prepare_func (pb, GDK_PIXBUF_ANIMATION (anim), context->user_data);
          ret = TRUE;
        }

      g_clear_object (&iter);
      g_clear_object (&anim);
    }
  else if (context->got_header && context->pb)
    {
      ret = TRUE;
    }

  if (context->buffer)
    {
      g_byte_array_free (context->buffer, TRUE);
      context->buffer = NULL;
    }

  g_clear_object (&context->pb);
  g_clear_pointer (&context->idec, WebPIDelete);
  g_clear_pointer (&context, g_free);
  return ret;
}

/* pixbuf save logic */

/* Encoder write callback to accumulate output data in a FILE stream */
static int
write_file (const uint8_t *data, size_t data_size, const WebPPicture *const pic)
{
  FILE *const out = (FILE *) pic->custom_ptr;
  return data_size == fwrite (data, sizeof (guchar), data_size, out) ? TRUE : FALSE;
}

/* Encoder write callback to accumulate output data in a GByteArray */
static int
write_array (const uint8_t *data, size_t data_size, const WebPPicture *const pic)
{
  GByteArray *arr = pic->custom_ptr;
  g_byte_array_append (arr, data, data_size);
  return TRUE;
}

static gboolean
is_save_option_supported (const gchar *option_key)
{
  char *options[4] = { "quality", "preset", "icc-profile", NULL };
  for (char **o = options; *o; o++)
    {
      if (g_strcmp0 (*o, option_key) == 0)
        return TRUE;
    }
  return FALSE;
}

/* Creates a new image data buffer with the ICC profile data in it */
WebPData
add_icc_data (WebPData *image_data, WebPData *icc_data, GError **error)
{
  WebPMux *mux = WebPMuxCreate (image_data, FALSE);

  if (mux == NULL)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not create WebPMux instance");
      return (WebPData){ 0 };
    }

  if (WebPMuxSetChunk (mux, "ICCP", icc_data, FALSE) != WEBP_MUX_OK)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not set ICC profile data WebP using Muxer");
      WebPMuxDelete (mux);
      return (WebPData){ 0 };
    }

  WebPData output = { 0 };
  if (WebPMuxAssemble (mux, &output) != WEBP_MUX_OK)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not assemble WebP data using Muxer");
      WebPMuxDelete (mux);
      return (WebPData){ 0 };
    }

  WebPMuxDelete (mux);
  return output;
}

static gboolean
save_webp (GdkPixbuf        *pixbuf,
           gchar           **keys,
           gchar           **values,
           GError          **error,
           GdkPixbufSaveFunc save_func,
           FILE             *f,
           gpointer         *user_data)
{
  WebPPicture picture;
  WebPConfig  config;
  uint8_t    *icc_data     = NULL;
  gsize       icc_data_len = 0;

  g_clear_error (error);

  if (! WebPPictureInit (&picture) || ! WebPConfigInit (&config))
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_BAD_OPTION,
                   "WebP encoder version mismatch.");
      return FALSE;
    }

  if (keys && *keys && values && *values)
    {
      gchar **kiter = keys;
      gchar **viter = values;

      while (*kiter)
        {
          if (g_strcmp0 (*kiter, "quality") == 0)
            {
              guint64 quality;
              if (! g_ascii_string_to_unsigned (*viter, 10, 0, 100, &quality, error))
                return FALSE;
              config.quality = (float) quality;
            }
          else if (g_strcmp0 (*kiter, "icc-profile") == 0)
            {
              icc_data = g_base64_decode (*viter, &icc_data_len);
            }
          else if (g_strcmp0 (*kiter, "preset") == 0)
            {
              gchar     *PRESET_KEYS[7] = { "default", "picture", "photo",
                                            "drawing", "icon",    "text",
                                            NULL };
              WebPPreset PRESET_VALS[7] = { WEBP_PRESET_DEFAULT,
                                            WEBP_PRESET_PICTURE,
                                            WEBP_PRESET_PHOTO,
                                            WEBP_PRESET_DRAWING,
                                            WEBP_PRESET_ICON,
                                            WEBP_PRESET_TEXT,
                                            0 };
              gboolean   preset_set     = FALSE;

              for (gchar **key = PRESET_KEYS; *key; key++)
                {
                  if (g_strcmp0 (*viter, *key) != 0)
                    continue;

                  if (WebPConfigPreset (&config, PRESET_VALS[key - PRESET_KEYS],
                                        config.quality)
                      == 0)
                    {
                      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                                   "Could not initialize decoder with "
                                   "preset.");
                      return FALSE;
                    }
                  preset_set = TRUE;
                  break;
                }

              if (! preset_set)
                {
                  g_warning ("Invalid WebP preset '%s', ignoring.", *viter);
                }
            }

          ++kiter;
          ++viter;
        }
    }

  if (WebPValidateConfig (&config) != 1)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_BAD_OPTION,
                   "Invalid WebP encoding configuration");
      return FALSE;
    }

  picture.width  = gdk_pixbuf_get_width (pixbuf);
  picture.height = gdk_pixbuf_get_height (pixbuf);
  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  if ((gdk_pixbuf_get_has_alpha (pixbuf)
           ? WebPPictureImportRGBA (&picture, gdk_pixbuf_get_pixels (pixbuf), rowstride)
           : WebPPictureImportRGB (&picture, gdk_pixbuf_get_pixels (pixbuf), rowstride))
      == FALSE)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                   "Failed to allocate picture");
      WebPPictureFree (&picture);
      return FALSE;
    }

  gboolean uses_array = save_func || icc_data;

  if (uses_array)
    {
      picture.writer     = write_array;
      picture.custom_ptr = (void *) g_byte_array_new ();
    }
  else if (f && ! icc_data)
    {
      picture.writer     = write_file;
      picture.custom_ptr = (void *) f;
    }
  else
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_BAD_OPTION,
                   "Save webp called without callback nor FILE stream value");
      WebPPictureFree (&picture);
      return FALSE;
    }

  if (WebPEncode (&config, &picture) == FALSE)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_BAD_OPTION,
                   "Could not encode WebP data");
      if ((save_func || icc_data) && picture.custom_ptr)
        g_byte_array_free ((GByteArray *) picture.custom_ptr, TRUE);
      WebPPictureFree (&picture);
      return FALSE;
    }

  gpointer custom_ptr = picture.custom_ptr;
  WebPPictureFree (&picture);

  if (uses_array)
    {
      WebPData data; // NOTE We can't do field initialization since we can't get length after free
      data.size  = ((GByteArray *) custom_ptr)->len;
      data.bytes = g_byte_array_free ((GByteArray *) custom_ptr, FALSE);

      if (icc_data)
        {
          WebPData icc_wpdata = { .bytes = icc_data, .size = icc_data_len };

          WebPData output = add_icc_data (&data, &icc_wpdata, error);
          g_clear_pointer (&icc_data, g_free);
          g_free ((gpointer) data.bytes);

          if (output.bytes == NULL)
            return FALSE;

          data = output;
        }

      gboolean ret = FALSE;
      if (save_func)
        ret = save_func ((const gchar *) data.bytes, data.size, error, user_data);
      else if (f)
        ret = fwrite (data.bytes, sizeof (guchar), data.size, f) == data.size ? TRUE : FALSE;

      WebPDataClear (&data);
      return ret;
    }

  return TRUE;
}

static gboolean
save (FILE *f, GdkPixbuf *pixbuf, gchar **keys, gchar **values, GError **error)
{
  return save_webp (pixbuf, keys, values, error, NULL, f, NULL);
}

static gboolean
save_to_callback (GdkPixbufSaveFunc save_func,
                  gpointer          user_data,
                  GdkPixbuf        *pixbuf,
                  gchar           **keys,
                  gchar           **values,
                  GError          **error)
{
  return save_webp (pixbuf, keys, values, error, save_func, NULL, user_data);
}

/* module entry points */

G_MODULE_EXPORT void
fill_vtable (GdkPixbufModule *module)
{
  module->begin_load     = begin_load;
  module->stop_load      = stop_load;
  module->load_increment = load_increment;

  module->save             = save;
  module->save_to_callback = save_to_callback;

  module->is_save_option_supported = is_save_option_supported;
}

G_MODULE_EXPORT void
fill_info (GdkPixbufFormat *info)
{
  static GdkPixbufModulePattern signature[] = {
    {"RIFFsizeWEBP", "    xxxx    ", 100},
    { NULL,          NULL,           0  }
  };

  static gchar *mime_types[] = { "image/webp", "audio/x-riff", /* FIXME hack around systems missing mime type */
                                 NULL };

  static gchar *extensions[] = { "webp", NULL };

  info->name        = "webp";
  info->signature   = signature;
  info->description = "The WebP image format";
  info->mime_types  = mime_types;
  info->extensions  = extensions;
  info->flags       = GDK_PIXBUF_FORMAT_WRITABLE | GDK_PIXBUF_FORMAT_THREADSAFE;
  info->license     = "LGPL";
}
