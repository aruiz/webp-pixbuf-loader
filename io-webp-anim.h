/* GdkPixbuf library - WebP Image Loader
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * Copyright (C) 2021 Alan Hawrelak
 *
 * Authors: Alan Hawrelak <alangh@shaw.ca>
 */

#ifndef IO_WEBP_ANIM_H
#define IO_WEBP_ANIM_H

#include <webp/types.h>
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux_types.h>
#include <webp/demux.h>
#include <string.h>

#define GDK_PIXBUF_ENABLE_BACKEND

#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>

#undef  GDK_PIXBUF_ENABLE_BACKEND

#include "io-webp.h"

typedef struct _GdkPixbufWebpAnim GdkPixbufWebpAnim;
typedef struct _GdkPixbufWebpAnimClass GdkPixbufWebpAnimClass;

#define GDK_TYPE_PIXBUF_WEBP_ANIM              (gdk_pixbuf_webp_anim_get_type ())
#define GDK_PIXBUF_WEBP_ANIM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_WEBP_ANIM, GdkPixbufWebpAnim))
#define GDK_IS_PIXBUF_WEBP_ANIM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_WEBP_ANIM))

#define GDK_PIXBUF_WEBP_ANIM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_PIXBUF_WEBP_ANIM, GdkPixbufWebpAnimClass))
#define GDK_IS_PIXBUF_WEBP_ANIM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_PIXBUF_WEBP_ANIM))
#define GDK_PIXBUF_WEBP_ANIM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_PIXBUF_WEBP_ANIM, GdkPixbufWebpAnimClass))

typedef struct _GdkPixbufWebpAnimIter GdkPixbufWebpAnimIter;


GType gdk_pixbuf_webp_anim_get_type(void) G_GNUC_CONST;

/* GdkPixbufWebpAnimIter -->   */
typedef struct _GdkPixbufWebpAnimIterClass GdkPixbufWebpAnimIterClass;

#define GDK_TYPE_PIXBUF_WEBP_ANIM_ITER              (gdk_pixbuf_webp_anim_iter_get_type ())
#define GDK_PIXBUF_WEBP_ANIM_ITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_WEBP_ANIM_ITER, GdkPixbufWebpAnimIter))
#define GDK_IS_PIXBUF_WEBP_ANIM_ITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_WEBP_ANIM_ITER))

#define GDK_PIXBUF_GIF_ANIM_ITER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_PIXBUF_GIF_ANIM_ITER, GdkPixbufGifAnimIterClass))
#define GDK_IS_PIXBUF_GIF_ANIM_ITER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_PIXBUF_GIF_ANIM_ITER))
#define GDK_PIXBUF_GIF_ANIM_ITER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_PIXBUF_GIF_ANIM_ITER, GdkPixbufGifAnimIterClass))


GType gdk_pixbuf_webp_anim_iter_get_type(void) G_GNUC_CONST;

#endif /* IO_WEBP_ANIM_H */
