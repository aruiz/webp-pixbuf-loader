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

#include "io-webp.h"
#include "io-webp-anim.h"

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
  context->pixbuf       = NULL;
  context->idec         = NULL;
  context->anim_buffer  = NULL;

  return context;
}

static gboolean
load_increment (gpointer data, const guchar *buf, guint size, GError **error)
{
  WebPContext *context = (WebPContext *) data;
  if (context->got_header == FALSE)
    {
      gint width, height;
      if (WebPGetInfo (buf, size, &width, &height) == FALSE)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not get WebP header information");
          return FALSE;
        }

      context->got_header = TRUE;

      gint scaled_w = width;
      gint scaled_h = height;

      context->size_func (&scaled_w, &scaled_h, context->user_data);

      if (scaled_w != 0 && scaled_h != 0 && (scaled_w != width || scaled_h != height))
        {
          context->deccfg.options.use_scaling   = TRUE;
          context->deccfg.options.scaled_width  = scaled_w;
          context->deccfg.options.scaled_height = scaled_h;
          width                                 = scaled_w;
          height                                = scaled_h;
        }

      WebPBitstreamFeatures features;
      if (WebPGetFeatures (buf, size, &features) != VP8_STATUS_OK)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not get WebP image feature information");
          return FALSE;
        }

      if (features.has_animation)
        {
          context->anim_buffer = g_byte_array_new ();
          g_byte_array_append (context->anim_buffer, buf, size);
          return TRUE;
        }

      context->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, features.has_alpha,
                                        8, width, height);
      if (context->pixbuf == NULL)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not allocate GdkPixbuf");
          return FALSE;
        }

      context->deccfg.output.colorspace = features.has_alpha ? MODE_RGBA : MODE_RGB;

      guint len                                 = 0;
      context->deccfg.output.is_external_memory = TRUE;
      context->deccfg.output.u.RGBA.rgba
          = gdk_pixbuf_get_pixels_with_length (context->pixbuf, &len);
      context->deccfg.output.u.RGBA.stride = gdk_pixbuf_get_rowstride (context->pixbuf);
      context->deccfg.output.u.RGBA.size = (size_t) len;
      context->idec = WebPIDecode (NULL, 0, &context->deccfg);

      if (context->idec == NULL)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                       "Could not allocate WebPIDecode");
          g_clear_object (&context->pixbuf);
          return FALSE;
        }

      context->prepare_func (context->pixbuf, NULL, context->user_data);
    }

  if (context->anim_buffer)
    {
      g_byte_array_append (context->anim_buffer, buf, size);
      return TRUE;
    }

  gint status = WebPIAppend (context->idec, buf, (size_t) size);
  if (status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not decode WebP buffer chunk");
      return FALSE;
    }

  gint last_y = 0, width = 0;
  if (WebPIDecGetRGB (context->idec, &last_y, &width, NULL, NULL) == NULL
      && status != VP8_STATUS_SUSPENDED)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not get RGP data from WebP incremental decoder");
      return FALSE;
    }

  context->update_func (context->pixbuf, 0, 0, width, last_y, context->user_data);

  return TRUE;
}

static gboolean
stop_load (gpointer data, GError **error)
{
  WebPContext *context = (WebPContext *) data;
  gboolean     ret     = FALSE;

  if (context->anim_buffer)
    {
      GdkWebpAnimation *anim = gdk_webp_animation_new_from_bytes (context->anim_buffer, error);
      context->anim_buffer = NULL;

      GdkPixbufAnimationIter *iter
          = gdk_pixbuf_animation_get_iter (GDK_PIXBUF_ANIMATION (anim), NULL);

      GdkPixbuf *pb = gdk_pixbuf_animation_iter_get_pixbuf (iter);
      if (pb == NULL)
        {
          g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                       "Could not get Pixbuf from WebP animation iter");

          WebPFreeDecBuffer (&context->deccfg.output);
          g_clear_pointer (&context->idec, WebPIDelete);
        }
      else
        {
          context->prepare_func (pb, GDK_PIXBUF_ANIMATION (anim), context->user_data);
          ret = TRUE;
        }

      g_object_unref (anim);
      g_object_unref (iter);
    }

  if (context->pixbuf != NULL)
    {
      gint w = gdk_pixbuf_get_width (context->pixbuf);
      gint h = gdk_pixbuf_get_height (context->pixbuf);
      context->update_func (context->pixbuf, 0, 0, w, h, context->user_data);
      ret = TRUE;
    }

  g_clear_pointer (&context->idec, WebPIDelete);
  g_clear_object (&context->pixbuf);
  g_free (context);

  return ret;
}

/* pixbuf save logic */

/* Encoder write callback to accumulate output data in a FILE stream */
static int
write_file (const uint8_t *data, size_t data_size, const WebPPicture *const pic)
{
  FILE *const out = (FILE *) pic->custom_ptr;
  return data_size ? (fwrite (data, data_size, 1, out) == 1) : 1;
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
  char *options[3] = { "quality", "preset", NULL };
  for (char **o = options; *o; o++)
    {
      if (g_strcmp0 (*o, option_key) == 0)
        return TRUE;
    }
  return FALSE;
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
          if (strncmp (*kiter, "quality", 7) == 0)
            {
              guint64 quality;
              if (! g_ascii_string_to_unsigned (*viter, 10, 0, 100, &quality, error))
                return FALSE;
              config.quality = (float) quality;
            }
          else if (strncmp (*kiter, "preset", 6) == 0)
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

  if (save_func)
    {
      picture.writer     = write_array;
      picture.custom_ptr = (void *) g_byte_array_new ();
    }
  else if (f)
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
      if (save_func && picture.custom_ptr)
        g_byte_array_free ((GByteArray *) picture.custom_ptr, TRUE);
      WebPPictureFree (&picture);
      return FALSE;
    }

  if (save_func)
    {
      GByteArray *arr = (GByteArray *) picture.custom_ptr;
      save_func ((const gchar *) arr->data, arr->len, error, user_data);
      g_byte_array_free (arr, TRUE);

      if (*error)
        {
          WebPPictureFree (&picture);
          return FALSE;
        }
    }

  WebPPictureFree (&picture);
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
