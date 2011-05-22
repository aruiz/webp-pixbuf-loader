/* GdkPixbuf library - WebP Image Loader
 *
 * Copyright (C) 2011 Alberto Ruiz
 *
 * Authors: Alberto Ruiz <aruiz@gnome.org>
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

static void
destroy_data (guchar *pixels, gpointer data)
{
  g_free (pixels);
}

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

void
fill_vtable (GdkPixbufModule *module)
{
        module->load = gdk_pixbuf__webp_image_load;
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
