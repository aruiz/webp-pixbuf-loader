/* GdkPixbuf library - WebP Image Loader
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * Copyright (C) 2011 Alberto Ruiz
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
gdk_pixbuf__webp_image_begin_load (GdkPixbufModuleSizeFunc     size_func,
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
gdk_pixbuf__webp_image_stop_load (gpointer data, GError **error)
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
          g_object_unref (anim);
          g_object_unref (iter);
          return FALSE;
        }

      context->prepare_func (pb, GDK_PIXBUF_ANIMATION (anim), context->user_data);

      g_object_unref (pb);
    }

  if (context->pixbuf != NULL)
    {
      gint w = gdk_pixbuf_get_width (context->pixbuf);
      gint h = gdk_pixbuf_get_height (context->pixbuf);
      context->update_func (context->pixbuf, 0, 0, w, h, context->user_data);
      ret = TRUE;
    }

  WebPFreeDecBuffer (&context->deccfg.output);

  g_clear_pointer (&context->idec, WebPIDelete);
  g_clear_object (&context->pixbuf);
  g_free (context);

  return ret;
}

static gboolean
gdk_pixbuf__webp_image_load_increment (gpointer data, const guchar *buf, guint size, GError **error)
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

      guint len = 0;
      context->deccfg.output.u.RGBA.rgba
          = gdk_pixbuf_get_pixels_with_length (context->pixbuf, &len);
      context->deccfg.output.u.RGBA.stride = gdk_pixbuf_get_rowstride (context->pixbuf);
      context->deccfg.output.u.RGBA.size = (size_t) len;
      context->idec = WebPIDecode (NULL, 0, &context->deccfg);

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

G_MODULE_EXPORT void
fill_vtable (GdkPixbufModule *module)
{
  // module->load = gdk_pixbuf__webp_image_load;
  module->begin_load     = gdk_pixbuf__webp_image_begin_load;
  module->stop_load      = gdk_pixbuf__webp_image_stop_load;
  module->load_increment = gdk_pixbuf__webp_image_load_increment;
  // module->save = gdk_pixbuf__webp_image_save;
  // module->save_to_callback = gdk_pixbuf__webp_image_save_to_callback;
  //  module->load_animation = gdk_pixbuf__webp_image_load_animation;
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
