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

G_BEGIN_DECLS

/* GdkPixbufWebpAnim  */
#define GDK_TYPE_PIXBUF_WEBP_ANIM              (gdk_pixbuf_webp_anim_get_type ())
G_DECLARE_FINAL_TYPE(GdkPixbufWebpAnim, GDK_TYPE_PIXBUF_WEBP_ANIM, GdkPixbufWebp, ANIM, GdkPixbufAnimation)
#define GDK_PIXBUF_WEBP_ANIM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_WEBP_ANIM, GdkPixbufWebpAnim))
#define GDK_IS_PIXBUF_WEBP_ANIM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_WEBP_ANIM))

/* GdkPixbufWebpAnimIter  */
#define GDK_TYPE_PIXBUF_WEBP_ANIM_ITER              (gdk_pixbuf_webp_anim_iter_get_type ())
G_DECLARE_FINAL_TYPE(GdkPixbufWebpAnimIter, GDK_PIXBUF_WEBP_ANIM_ITER, GdkPixbufWebp, ANIM_ITER, GdkPixbufAnimationIter)
#define GDK_PIXBUF_WEBP_ANIM_ITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_WEBP_ANIM_ITER, GdkPixbufWebpAnimIter))
#define GDK_IS_PIXBUF_WEBP_ANIM_ITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_WEBP_ANIM_ITER))

G_END_DECLS

#endif /* IO_WEBP_ANIM_H */
