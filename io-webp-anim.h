/* GdkPixbuf library - WebP Image Loader
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * Copyright (C) 2021 Alan Hawrelak
 * Copyright (C) 2022 Alberto Ruiz
 *
 * Authors: Alan Hawrelak <alangh@shaw.ca>
 *          Alberto Ruiz  <aruiz@gnome.org>
 */

#ifndef __IO_WEBP_ANIM_H__
#define __IO_WEBP_ANIM_H__

#define GDK_PIXBUF_ENABLE_BACKEND

#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#undef GDK_PIXBUF_ENABLE_BACKEND

G_BEGIN_DECLS

#define GDK_WEBP_ANIMATION_TYPE gdk_webp_animation_get_type ()
G_DECLARE_FINAL_TYPE (GdkWebpAnimation, gdk_webp_animation, GDK_WEBP, ANIMATION, GdkPixbufAnimation)

GdkWebpAnimation *gdk_webp_animation_new_from_bytes (GByteArray *data, GError **error);

#define GDK_WEBP_ANIMATION_ITER_TYPE gdk_webp_animation_iter_get_type ()
G_DECLARE_FINAL_TYPE (GdkWebpAnimationIter, gdk_webp_animation_iter, GDK_WEBP, ANIMATION_ITER, GdkPixbufAnimationIter)

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
GdkWebpAnimationIter *gdk_webp_animation_new_from_buffer_and_time (const GByteArray *buf,
                                                                   const GTimeVal *start_time,
                                                                   GError **error);
G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __IO_WEBP_ANIM_H__ */
