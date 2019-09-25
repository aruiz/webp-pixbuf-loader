/* GdkPixbuf library - WebP Image Loader
 *
 * Copyright (C) 2011 Alberto Ruiz
 * Copyright (C) 2011 David Mazary
 * Copyright (C) 2014 Přemysl Janouch
 *
 * Authors: Alberto Ruiz <aruiz@gnome.org>
 *          David Mazary <dmaz@vt.edu>
 *          Přemysl Janouch <p.janouch@gmail.com>
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
        WebPDecoderConfig config;
        gpointer user_data;
        GdkPixbuf *pixbuf;
        gboolean got_header;
        WebPIDecoder *idec;
        gint last_y;
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
        WebPBitstreamFeatures features;
        gboolean use_alpha = TRUE;

        /* Get data size */
        fseek (f, 0, SEEK_END);
        data_size = ftell(f);
        fseek (f, 0, SEEK_SET);

        /* Get data */
        data = g_malloc (data_size);
        ok = (fread (data, data_size, 1, f) == 1);
        if (!ok) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
                             "Failed to read file");
                return FALSE;
        }

        /* Take the safe route and only disable the alpha channel when
           we're sure that there is not any. */
        if (WebPGetFeatures (data, data_size, &features) == VP8_STATUS_OK
            && features.has_alpha == FALSE) {
                use_alpha = FALSE;
        }

        if (use_alpha) {
                out = WebPDecodeRGBA (data, data_size, &w, &h);
        } else {
                out = WebPDecodeRGB (data, data_size, &w, &h);
        }
        g_free (data);

        if (!out) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
                             "Cannot create WebP decoder.");
                return FALSE;
        }

        pixbuf = gdk_pixbuf_new_from_data ((const guchar *)out,
                                           GDK_COLORSPACE_RGB,
                                           use_alpha,
                                           8,
                                           w, h,
                                           w * (use_alpha ? 4 : 3),
                                           destroy_data,
                                           NULL);

        if (!pixbuf) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
                             "Failed to decode image");
                return FALSE;
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
        WebPContext *context = g_new0 (WebPContext, 1);
        context->size_func = size_func;
        context->prepare_func = prepare_func;
        context->update_func  = update_func;
        context->user_data = user_data;
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
gdk_pixbuf__webp_image_load_increment (gpointer context,
                                       const guchar *buf, guint size,
                                       GError **error)
{
        gint w, h, stride;
        WebPContext *data = (WebPContext *) context;
        g_return_val_if_fail(data != NULL, FALSE);

        if (!data->got_header) {
                gint rc = WebPGetInfo (buf, size, &w, &h);
                if (rc == 0) {
                        g_set_error (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     "Cannot read WebP image header.");
                        return FALSE;
                }
                data->got_header = TRUE;

                if (data->size_func) {
                        gint scaled_w = w;
                        gint scaled_h = h;

                        (* data->size_func) (&scaled_w, &scaled_h,
                                             data->user_data);
                        if (scaled_w != w || scaled_h != h) {
                            data->config.options.use_scaling = TRUE;
                            data->config.options.scaled_width = scaled_w;
                            data->config.options.scaled_height = scaled_h;
                            w = scaled_w;
                            h = scaled_h;
                        }
                }

                WebPBitstreamFeatures features;
                gboolean use_alpha = TRUE;

                /* Take the safe route and only disable the alpha channel when
                   we're sure that there is not any. */
                if (WebPGetFeatures (buf, size, &features) == VP8_STATUS_OK
                    && features.has_alpha == FALSE) {
                        use_alpha = FALSE;
                }

                data->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                               use_alpha,
                                               8,
                                               w,
                                               h);

                guint pixbuf_length;
                stride = gdk_pixbuf_get_rowstride (data->pixbuf);
                guchar *pixbuf_data = gdk_pixbuf_get_pixels_with_length (data->pixbuf,
                                                                         &pixbuf_length);

                /* Initialise the picture to transparent black. */
                memset (pixbuf_data, 0x00, pixbuf_length);

                data->config.output.colorspace = use_alpha ? MODE_RGBA : MODE_RGB;
                data->config.output.is_external_memory = TRUE;
                data->config.output.u.RGBA.rgba = pixbuf_data;
                data->config.output.u.RGBA.stride = stride;

                /* XXX: this may not be true: the last row doesn't strictly have
                   to include stride padding, even though it does in case of
                   gdk_pixbuf_new(), looking at its source.  There is an explicit
                   check in libwebp that requires that the following value equals
                   stride times height, however, even though it is fairly unlikely
                   that anyone would actually want to write into the padding. */
                data->config.output.u.RGBA.size = h * stride;

                data->idec = WebPIDecode (NULL, 0, &data->config);
                if (!data->idec) {
                        g_set_error (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     "Cannot create WebP decoder.");
                        return FALSE;
                }

                if (data->prepare_func) {
                        (* data->prepare_func) (data->pixbuf,
                                                NULL,
                                                data->user_data);
                }
        }

        /* Append size bytes to decoder's buffer */
        const VP8StatusCode status = WebPIAppend (data->idec, buf, size);
        if (status != VP8_STATUS_SUSPENDED && status != VP8_STATUS_OK) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                             "WebP decoder failed with status code %d.",
                             status);
                return FALSE;
        }

        /* Decode decoder's updated buffer */
        guint8 *dec_output;
        dec_output = WebPIDecGetRGB (data->idec, &data->last_y, &w, &h, &stride);
        if (dec_output == NULL && status != VP8_STATUS_SUSPENDED) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Bad inputs to WebP decoder.");
                return FALSE;
        }

        if (data->update_func) {
                (* data->update_func) (data->pixbuf, 0, 0,
                                       w,
                                       data->last_y,
                                       data->user_data);
        }
        return TRUE;
}

static int
write_file (const uint8_t* data, size_t data_size, const WebPPicture* const pic)
{
        FILE* const out = (FILE *) pic->custom_ptr;
        return data_size ? (fwrite (data, data_size, 1, out) == 1) : 1;
}

typedef struct {
        GdkPixbufSaveFunc func;
        gpointer          data;
} save_context;

static int
save_callback (const uint8_t* data, size_t data_size, const WebPPicture* const pic)
{
        save_context *env = (save_context *) pic->custom_ptr;
        return (* env->func) (data, data_size, NULL, env->data);
}

static gboolean
real_save_webp (GdkPixbuf        *pixbuf,
                gchar           **keys,
                gchar           **values,
                GError          **error,
                gboolean          to_callback,
                FILE             *f,
                save_context     *context)
{
        WebPPicture picture;
        WebPConfig config;
        gint w, h, rowstride, has_alpha, rc;
        guchar *pixels;

        if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_BAD_OPTION,
                            "WebP encoder version mismatch.");
                return FALSE;
        }

        if (keys && *keys) {
                gchar **kiter = keys;
                gchar **viter = values;

                while (*kiter) {
                        if (strncmp (*kiter, "quality", 7) == 0) {
                                float quality = (float) g_ascii_strtod (*viter, NULL);
                                if (quality < 0 || quality > 100) {
                                        g_set_error (error,
                                                     GDK_PIXBUF_ERROR,
                                                     GDK_PIXBUF_ERROR_BAD_OPTION,
                                                     "WebP quality must be a value between 0 and 100.");
                                        return FALSE;
                                }
                                config.quality = quality;
                        } else if (strncmp (*kiter, "preset", 6) == 0) {
                                WebPPreset preset;
                                if (strncmp (*viter, "default", 7) == 0) {
                                        preset = WEBP_PRESET_DEFAULT;
                                } else if (strncmp (*viter, "photo", 5) == 0) {
                                        preset = WEBP_PRESET_PHOTO;
                                } else if (strncmp (*viter, "picture", 7) == 0) {
                                        preset = WEBP_PRESET_PICTURE;
                                } else if (strncmp (*viter, "drawing", 7) == 0) {
                                        preset = WEBP_PRESET_DRAWING;
                                } else if (strncmp (*viter, "icon", 4) == 0) {
                                        preset = WEBP_PRESET_ICON;
                                } else if (strncmp (*viter, "text", 4) == 0) {
                                        preset = WEBP_PRESET_TEXT;
                                } else {
                                        g_set_error (error,
                                                     GDK_PIXBUF_ERROR,
                                                     GDK_PIXBUF_ERROR_BAD_OPTION,
                                                     "WebP encoder invalid preset.");
                                        return FALSE;
                                }
                                if (WebPConfigPreset (&config, preset, config.quality) == 0) {
                                         g_set_error (error,
                                                      GDK_PIXBUF_ERROR,
                                                      GDK_PIXBUF_ERROR_FAILED,
                                                      "Could not initialize decoder with preset.");
                                         return FALSE;
                                }
                        }
                        ++kiter;
                        ++viter;
                }
        }

        if (WebPValidateConfig (&config) != 1) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_BAD_OPTION,
                             "Invalid encoding configuration");
                return FALSE;
        }

        w = gdk_pixbuf_get_width (pixbuf);
        h = gdk_pixbuf_get_height (pixbuf);
        rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
        pixels = gdk_pixbuf_get_pixels (pixbuf);

        picture.width = w;
        picture.height = h;

        if (has_alpha) {
                rc = WebPPictureImportRGBA (&picture, pixels, rowstride);
        } else {
                rc = WebPPictureImportRGB (&picture, pixels, rowstride);
        }
        if (rc == 0) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                             "Failed to allocate picture");
                return FALSE;
        }

        if (to_callback) {
                picture.writer = save_callback;
                picture.custom_ptr = (void*) context;
        } else {
                picture.writer = write_file;
                picture.custom_ptr = (void*) f;
        }

        if (WebPEncode(&config, &picture) == 0) {
                fprintf(stderr, "Error! Cannot encode picture as WebP\n");
        }

        return TRUE;
}

static gboolean
gdk_pixbuf__webp_image_save (FILE          *f,
                             GdkPixbuf     *pixbuf,
                             gchar        **keys,
                             gchar        **values,
                             GError       **error)
{
        return real_save_webp (pixbuf, keys, values, error,
                               FALSE, f, NULL);
}

static gboolean
gdk_pixbuf__webp_image_save_to_callback (GdkPixbufSaveFunc   save_func,
                                         gpointer            user_data,
                                         GdkPixbuf          *pixbuf,
                                         gchar             **keys,
                                         gchar             **values,
                                         GError            **error)
{
        save_context *context = g_new0 (save_context, 1);
        context->func = save_func;
        context->data = user_data;
        return real_save_webp (pixbuf, keys, values, error,
                               TRUE, NULL, context);
}

void
fill_vtable (GdkPixbufModule *module)
{
        module->load = gdk_pixbuf__webp_image_load;
        module->begin_load = gdk_pixbuf__webp_image_begin_load;
        module->stop_load = gdk_pixbuf__webp_image_stop_load;
        module->load_increment = gdk_pixbuf__webp_image_load_increment;
        module->save = gdk_pixbuf__webp_image_save;
        module->save_to_callback = gdk_pixbuf__webp_image_save_to_callback;
}

void
fill_info (GdkPixbufFormat *info)
{
        static GdkPixbufModulePattern signature[] = {
                { "RIFFsizeWEBP", "    xxxx    ", 100 },
                { NULL, NULL, 0 }
        };

        static gchar *mime_types[] = {
                "image/webp",
                "audio/x-riff", /* FIXME hack around systems missing mime type */
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
        info->flags       = GDK_PIXBUF_FORMAT_WRITABLE | GDK_PIXBUF_FORMAT_THREADSAFE;
        info->license     = "LGPL";
}
