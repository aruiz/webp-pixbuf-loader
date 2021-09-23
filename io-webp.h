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

#ifndef IO_WEBP_H
#define IO_WEBP_H

#include <webp/decode.h>
#include <webp/encode.h>
#include <string.h>

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>
#undef  GDK_PIXBUF_ENABLE_BACKEND

/* Animation progressive loader state */
typedef struct {
    int     state;          /* ACCUMstate */
    guchar  *filedata;
    guchar  *accum_data;
    size_t  used_len;
    size_t  cur_max_len;
    size_t  total_data_len;
} AnimIncrDecode;

/* Progressive loader context */
typedef struct {
    GdkPixbufModuleSizeFunc size_func;
    GdkPixbufModuleUpdatedFunc update_func;
    GdkPixbufModulePreparedFunc prepare_func;
    WebPDecoderConfig config;
    gpointer user_data;
    GdkPixbuf *pixbuf;
    gboolean got_header;
    WebPIDecoder *idec;
    WebPBitstreamFeatures features;     /* used by animation. */
    AnimIncrDecode anim_incr;         /* used by animation. */
    gint last_y;
    GError **error;
} WebPContext;

#endif /* IO_WEBP_H */
