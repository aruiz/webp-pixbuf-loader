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

enum {
    AIDstate_need_initialize = 0,
    AIDstate_need_data = 1,
    AIDstate_have_all_data = 2,
    AIDstate_sending_frames = 3
} AIDstate;

struct _anim_incr_decode {
    int     state;          /* AIDstate */
    guchar  *filedata;
    guchar  *data;
    size_t  used_len;
    size_t  cur_max_len;
    size_t  total_data_len;
};
typedef struct _anim_incr_decode anim_incr_decode;

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
    anim_incr_decode anim_incr;         /* used by animation. */
    gint last_y;
    GError **error;
} WebPContext;

#endif /* IO_WEBP_H */
