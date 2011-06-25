/* GdkPixbuf library - WebP Image Loader
 *
 * Copyright (C) 2011 Alberto Ruiz
 * Copyright (C) 2011 David Mazary
 *
 * Authors: Alberto Ruiz <aruiz@gnome.org>
 *          David Mazary <dmaz@vt.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <webp/decode.h>

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>
#undef  GDK_PIXBUF_ENABLE_BACKEND

/* Progressive loader context */
typedef struct {
        GdkPixbufModuleSizeFunc size_func;
        GdkPixbufModuleUpdatedFunc update_func;
        GdkPixbufModulePreparedFunc prepare_func;
        gpointer user_data;
        GdkPixbuf *pixbuf;
        gboolean got_header;
        WebPIDecoder *idec;
        GError **error;
} WebPContext;

static void
destroy_data (guchar *pixels, gpointer data)
{
  g_free (pixels);
}

/* Shared library entry point */
static GdkPixbuf *
gdk_pixbuf__webp_image_load (FILE *f, GError **error)
{
  GdkPixbuf * volatile pixbuf = NULL;
  guint32 data_size;
  guint8 *out;
  gint w, h, ok;
  gpointer data;
  
  /* Get data size */
  fseek(f, 0, SEEK_END);
  data_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  /* Get data */
  data = g_malloc(data_size);
  ok = (fread(data, data_size, 1, f) == 1);
  if (!ok)
  {
    /*TODO: Return GError*/
    g_free (data);
    return;
  }
  out = WebPDecodeRGB(data, data_size, &w, &h);
  g_free (data);
  
  if (!out)
  {
    /*TODO: Return GError*/
    return;
  }

  /*FIXME: libwebp does not support alpha channel detection, once supported
   * we need to make the alpha channel conditional to save memory if possible */
  pixbuf = gdk_pixbuf_new_from_data ((const guchar *)out,
                                     GDK_COLORSPACE_RGB,
                                     FALSE ,
                                     8,
                                     w, h,
                                     3 * w,
                                     destroy_data,
                                     NULL);

  if (!pixbuf) {
    /*TODO: Return GError?*/
    return;
  }
  return pixbuf;
}

static gpointer
gdk_pixbuf__webp_image_begin_load (GdkPixbufModuleSizeFunc size_func,
                                  GdkPixbufModulePreparedFunc prepare_func,
                                  GdkPixbufModuleUpdatedFunc update_func,
                                  gpointer user_data,
                                  GError **error)
{
        WEBP_CSP_MODE mode = MODE_RGB;
        WebPContext *context = g_new0 (WebPContext, 1);
        context->size_func = size_func;
        context->prepare_func = prepare_func;
        context->update_func  = update_func;
        context->user_data = user_data;
        context->idec = WebPINew(mode);
        return context;
}

static gboolean
gdk_pixbuf__webp_image_stop_load (gpointer context, GError **error)
{
        WebPContext *data = (WebPContext *) context;
        g_return_val_if_fail(data != NULL, TRUE);
        if (data->pixbuf) {
                g_object_unref (data->pixbuf);
        }
        if (data->idec) {
                WebPIDelete (data->idec);
        }
        return TRUE;
}

static gboolean
gdk_pixbuf__webp_image_load_increment(gpointer context,
                                     const guchar *buf, guint size,
                                     GError **error)
{
        gint width, height;
        WebPContext *data = (WebPContext *) context;
        g_return_val_if_fail(data != NULL, FALSE);

        /* TODO: Loop while(TRUE), continue on vp8 status suspend */
        if (!data->got_header) {
                gint rc;
                rc = WebPGetInfo (data->user_data, size,
                                 &width, &height);
                if (rc == 0) {
                        /* Header formatting error */
                }
                data->got_header = TRUE;

                if (data->size_func) {
                        (* data->size_func) (&width, &height,
                                             data->user_data);
                }

                /* TODO: Initialize pixbuf using width and height */

                if (data->prepare_func) {
                        (* data->prepare_func) (data->pixbuf,
                                                NULL,
                                                data->user_data);
                }
                return TRUE;
        }

        VP8StatusCode status = WebPIUpdate (data->idec, (guchar *) buf, size);
        printf("Updated buffer with code %d\n", status);
        if (status == VP8_STATUS_SUSPENDED) {
                if (data->update_func) {
                        (* data->update_func) (data->pixbuf, 0, 0,
                                               width,
                                               height,
                                               data->user_data);
                }
                return TRUE;
        }
        g_return_val_if_fail (status == VP8_STATUS_OK, FALSE);
        if (data->update_func) {
                (* data->update_func) (data->pixbuf, 0, 0,
                                       width,
                                       height,
                                       data->user_data);
        }
        return TRUE;
}

void
fill_vtable (GdkPixbufModule *module)
{
        module->load = gdk_pixbuf__webp_image_load;
        module->begin_load = gdk_pixbuf__webp_image_begin_load;
        module->stop_load = gdk_pixbuf__webp_image_stop_load;
        module->load_increment = gdk_pixbuf__webp_image_load_increment;
}

void
fill_info (GdkPixbufFormat *info)
{
  /* WebP TODO: figure out how to represent the full pattern */
  static GdkPixbufModulePattern signature[] = {
    { "RIFF", NULL, 100 },
    { NULL, NULL, 0 }
  };

  static gchar *mime_types[] = {
    "image/webp",
    NULL
  };

  static gchar *extensions[] = {
    "webp",
    NULL
  };

  info->name        = "webp";
  info->signature   = signature;
  info->description = "The WebP image format";
  info->mime_types  = mime_types;
  info->extensions  = extensions;
  info->flags       = GDK_PIXBUF_FORMAT_THREADSAFE;
  info->license     = "LGPL";
}
