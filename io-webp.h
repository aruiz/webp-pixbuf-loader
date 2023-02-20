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

/* Progressive loader context */
typedef struct {
    GdkPixbufModuleSizeFunc size_func;
    GdkPixbufModuleUpdatedFunc update_func;
    GdkPixbufModulePreparedFunc prepare_func;
    gpointer user_data;
    WebPDecoderConfig deccfg;
    gboolean got_header;
    GdkPixbuf *pixbuf;
    WebPIDecoder *idec;
    GByteArray    *anim_buffer;
} WebPContext;

#endif /* IO_WEBP_H */
