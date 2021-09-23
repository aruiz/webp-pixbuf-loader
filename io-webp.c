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

#define  IMAGE_READ_BUFFER_SIZE 65535

typedef enum {
        ACCUMstate_need_initialize = 0,
        ACCUMstate_need_data,
        ACCUMstate_have_all_data,
        ACCUMstate_sending_frames
} ACCUMstate;

/* from io-webp-anim.c */
extern
GdkPixbufAnimation *gdk_pixbuf__webp_image_load_animation(FILE *file,
                                                          GError **error);

extern
GdkPixbufAnimation *gdk_pixbuf__webp_image_load_animation_data(const guchar *buf, guint size,
                                                               WebPContext *in_context,
                                                               GError **error);

static void
destroy_data(guchar *pixels, gpointer data) {
        g_free(pixels);
}

/* Shared library entry point */
static GdkPixbuf *
gdk_pixbuf__webp_image_load(FILE *f, GError **error) {
        GdkPixbuf *volatile pixbuf = NULL;
        guint32 data_size;
        guint8 *out;
        gint w, h, ok;
        gpointer data;
        WebPBitstreamFeatures features;
        gboolean use_alpha = TRUE;

        /* Get data size */
        fseek(f, 0, SEEK_END);
        data_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        /* Get data */
        data = g_malloc(data_size);
        ok = (fread(data, data_size, 1, f) == 1);
        if (!ok) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Failed to read file");
                return FALSE;
        }

        /* Take the safe route and only disable the alpha channel when
           we're sure that there is not any. */
        if (WebPGetFeatures(data, data_size, &features) == VP8_STATUS_OK
            && features.has_alpha == FALSE) {
                use_alpha = FALSE;
        }

        if (use_alpha) {
                out = WebPDecodeRGBA(data, data_size, &w, &h);
        } else {
                out = WebPDecodeRGB(data, data_size, &w, &h);
        }
        g_free(data);

        if (!out) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Cannot create WebP decoder.");
                return FALSE;
        }

        pixbuf = gdk_pixbuf_new_from_data((const guchar *) out,
                                          GDK_COLORSPACE_RGB,
                                          use_alpha,
                                          8,
                                          w, h,
                                          w * (use_alpha ? 4 : 3),
                                          destroy_data,
                                          NULL);

        if (!pixbuf) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Failed to decode image");
                return FALSE;
        }
        return pixbuf;
}

static gpointer
gdk_pixbuf__webp_image_begin_load(GdkPixbufModuleSizeFunc size_func,
                                  GdkPixbufModulePreparedFunc prepare_func,
                                  GdkPixbufModuleUpdatedFunc update_func,
                                  gpointer user_data,
                                  GError **error) {
        WebPContext *context = g_new0 (WebPContext, 1);
        context->size_func = size_func;
        context->prepare_func = prepare_func;
        context->update_func = update_func;
        context->user_data = user_data;
        return context;
}

static gboolean
gdk_pixbuf__webp_image_stop_load(gpointer context, GError **error) {
        WebPContext *data = (WebPContext *) context;
        g_return_val_if_fail(data != NULL, TRUE);

        if (data->idec) {
                WebPIDelete(data->idec);
        }
        return TRUE;
}

/*
 * anim_context will contain the new ptr and size if one has to be made.
 * newbuf incoming data to add
 * size of the incoming data
 * returns the new ptr
 */
static
gpointer realloc_copy_mem(AnimIncrDecode *anim_context, const guchar *newbuf, guint size) {
        guchar *core_data = anim_context->accum_data;
        size_t core_used_len = anim_context->used_len;
        size_t core_cur_max_len = anim_context->cur_max_len;
        size_t size_to_add = IMAGE_READ_BUFFER_SIZE;
        if (core_data == NULL) {
                guchar *ptr = g_try_malloc0(core_cur_max_len + size + size_to_add);
                if (ptr == NULL) {
                        core_used_len = 0;
                        core_cur_max_len = 0;
                } else {
                        memcpy(ptr, newbuf, size);
                        core_data = ptr;
                        core_used_len = size;
                        core_cur_max_len = size + size_to_add;
                }
        } else {  /* (core_data != NULL) */
                if (core_cur_max_len >= core_used_len + size) {
                        memcpy(core_data + core_used_len, newbuf, size);
                        core_used_len += size;
                } else {
                        size_t extended_size = 0;
                        guchar *newptr = g_try_realloc(core_data,
                                                       core_cur_max_len + size + size_to_add);
                        extended_size = core_cur_max_len + size + size_to_add;
                        if (!newptr) {
                                newptr = g_try_realloc(core_data, core_cur_max_len + size);
                                extended_size = core_cur_max_len + size;
                        }

                        if (newptr) {
                                core_data = newptr;
                                core_cur_max_len = extended_size;
                                memcpy(core_data + core_used_len, newbuf, size);
                                core_used_len += size;
                        }
                }
        }

        anim_context->accum_data = core_data;
        anim_context->cur_max_len = core_cur_max_len;
        anim_context->used_len = core_used_len;

        return core_data;
}

/*
 * Only called after all data is present.
 * At that point the size is data->anim_incr.total_data_len .
 */
void
static create_anim(WebPContext *data,
                   guchar *ptr, guint size,
                   GError **error) {
        data->anim_incr.accum_data = ptr;
        if (data->anim_incr.used_len == data->anim_incr.total_data_len) {
                data->anim_incr.state = ACCUMstate_have_all_data;
                GdkPixbufAnimation *anim = gdk_pixbuf__webp_image_load_animation_data(
                        data->anim_incr.accum_data,
                        data->anim_incr.used_len,
                        data,
                        error);

                GdkPixbufAnimationIter *anim_iter = gdk_pixbuf_animation_get_iter(anim, NULL);
                GdkPixbuf *pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(anim_iter);
                data->pixbuf = pixbuf;
                data->anim_incr.state = ACCUMstate_sending_frames;
                if (data->prepare_func) {
                        (*data->prepare_func)(data->pixbuf,
                                              anim,
                                              data->user_data);
                }
        }
}

/* handle animation.
 * The WebP animation API was not designed for incremental load.
 * Defeat the purpose of incremental load, by loading all increments
 *  into one data buffer. Do not create a gdk_pixbuf until all data
 *  is present. Then issue data->prepare_func call.
 */
static gboolean
gdk_pixbuf__webp_anim_load_increment(gpointer context,
                                     const guchar *buf, guint size,
                                     GError **error) {
        WebPContext *data = (WebPContext *) context;

        if (data->anim_incr.state == ACCUMstate_need_initialize) {
                if (size < 12) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header...");
                        return FALSE;
                }

                /* check for "RIFF" tag, and then the "WEBP" tag. */
                char tag[5];
                tag[4] = 0;
                for (int i = 0; i < 4; i++) { tag[i] = *(gchar *) (buf + i); }
                int rc2 = strcmp(tag, "RIFF");
                if (rc2 != 0) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header...");
                        return FALSE;
                }
                for (int i = 0; i < 4; i++) { tag[i] = *(gchar *) (buf + 8 + i); }
                rc2 = strcmp(tag, "WEBP");
                if (rc2 != 0) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header...");
                        return FALSE;
                }

                /* The next 4 bytes give the size of the webp container less the 8 byte header. */
                uint32_t anim_size = *(uint32_t *) (buf + 4); /* gives file size not counting the first 8 bytes. */
                uint32_t file_size = anim_size + 8;
                if (file_size < size) {
                        /* user asking to insert data larger than the image size set in the file data. */
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header..");
                        return FALSE;
                }
                data->anim_incr.total_data_len = file_size; /* allow for 8 byte header. "RIFF" + SIZE . */
                guchar *ptr = calloc(sizeof(guchar), file_size);
                if (ptr == NULL) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    "Failed to allocate memory for the WebP image.");
                        return FALSE;

                }
                gint w, h;
                gint rc = WebPGetInfo(buf, size, &w, &h);
                if (rc == 0) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header.");
                        return FALSE;
                }
                data->got_header = TRUE;

                if (data->size_func) {
                        gint scaled_w = w;
                        gint scaled_h = h;

                        (*data->size_func)(&scaled_w, &scaled_h,
                                           data->user_data);
                        if (scaled_w != w || scaled_h != h) {
                                data->config.options.use_scaling = TRUE;
                                data->config.options.scaled_width = scaled_w;
                                data->config.options.scaled_height = scaled_h;
                                w = scaled_w;
                                h = scaled_h;
                        }
                }

                memcpy(ptr, buf, size);
                data->anim_incr.accum_data = ptr;
                data->anim_incr.used_len = size;
                data->anim_incr.cur_max_len = anim_size + 8;
                data->anim_incr.total_data_len = anim_size + 8;
                data->anim_incr.state = ACCUMstate_need_data;
                data->config.options.dithering_strength = 50;
                data->config.options.alpha_dithering_strength = 100;
                if (size == data->anim_incr.total_data_len) {
                        data->anim_incr.state = ACCUMstate_have_all_data;
                        create_anim(data, ptr, size, error);
                } else if (size > data->anim_incr.total_data_len) {
                        return FALSE;
                }
                return TRUE;
        } else if (data->anim_incr.state == ACCUMstate_need_data) {
                gpointer ptr = realloc_copy_mem(&data->anim_incr, buf, size);
                if (data->anim_incr.used_len == data->anim_incr.total_data_len) {
                        data->anim_incr.state = ACCUMstate_have_all_data;
                        create_anim(data, ptr, data->anim_incr.used_len, error);
                } else if (data->anim_incr.used_len > data->anim_incr.total_data_len) {
                        return FALSE;
                }
                return TRUE;
        } else if (data->anim_incr.state == ACCUMstate_have_all_data) {
                return TRUE;
        } else if (data->anim_incr.state == ACCUMstate_sending_frames) {
                return TRUE;
        }
        return FALSE;
}

static gboolean
gdk_pixbuf__webp_image_load_increment(gpointer context,
                                      const guchar *buf, guint size,
                                      GError **error) {
        gint w, h, stride;
        WebPContext *data = (WebPContext *) context;
        g_return_val_if_fail(data != NULL, FALSE);
        gboolean use_animation = FALSE;

        if (data->got_header) {
                if (data->config.input.has_animation) {
                        return gdk_pixbuf__webp_anim_load_increment(context, buf, size, error);
                }
        } else {
                gint rc = WebPGetInfo(buf, size, &w, &h);
                if (rc == 0) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    "Cannot read WebP image header.");
                        return FALSE;
                }
                data->got_header = TRUE;

                if (data->size_func) {
                        gint scaled_w = w;
                        gint scaled_h = h;

                        (*data->size_func)(&scaled_w, &scaled_h,
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
                if (WebPGetFeatures(buf, size, &features) == VP8_STATUS_OK) {
                        if (features.has_alpha == FALSE)
                                use_alpha = FALSE;
                        if (features.has_animation) {
                                use_animation = TRUE;
                                data->config.input.has_animation = TRUE;
                                data->features = features;
                        }
                }

                data->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                              use_alpha,
                                              8,
                                              w,
                                              h);

                guint pixbuf_length;
                stride = gdk_pixbuf_get_rowstride(data->pixbuf);
                guchar *pixbuf_data = gdk_pixbuf_get_pixels_with_length(data->pixbuf,
                                                                        &pixbuf_length);

                /* Initialise the picture to transparent black. */
                memset(pixbuf_data, 0x00, pixbuf_length);

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

                if (use_animation) {
                        return gdk_pixbuf__webp_anim_load_increment(context, buf, size, error);
                }

                data->idec = WebPIDecode(NULL, 0, &data->config);
                if (!data->idec) {
                        g_set_error(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "Cannot create WebP decoder.");
                        return FALSE;
                }

                if (data->prepare_func) {
                        (*data->prepare_func)(data->pixbuf,
                                              NULL,
                                              data->user_data);
                }
        }

        /* Append size bytes to decoder's buffer */
        const VP8StatusCode status = WebPIAppend(data->idec, buf, size);
        if (status != VP8_STATUS_SUSPENDED && status != VP8_STATUS_OK) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                            "WebP decoder failed with status code %d.",
                            status);
                return FALSE;
        }

        /* Decode decoder's updated buffer */
        guint8 *dec_output;
        dec_output = WebPIDecGetRGB(data->idec, &data->last_y, &w, &h, &stride);
        if (dec_output == NULL && status != VP8_STATUS_SUSPENDED) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Bad inputs to WebP decoder.");
                return FALSE;
        }

        if (data->update_func) {
                (*data->update_func)(data->pixbuf, 0, 0,
                                     w,
                                     data->last_y,
                                     data->user_data);
        }
        return TRUE;
}

static int
write_file(const uint8_t *data, size_t data_size, const WebPPicture *const pic) {
        FILE *const out = (FILE *) pic->custom_ptr;
        return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

typedef struct {
        GdkPixbufSaveFunc func;
        gpointer data;
} save_context;

static int
save_callback(const uint8_t *data, size_t data_size, const WebPPicture *const pic) {
        save_context *env = (save_context *) pic->custom_ptr;
        return (*env->func)((const char *) data, data_size, NULL, env->data);
}

static gboolean
real_save_webp(GdkPixbuf *pixbuf,
               gchar **keys,
               gchar **values,
               GError **error,
               gboolean to_callback,
               FILE *f,
               save_context *context) {
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
                        if (strncmp(*kiter, "quality", 7) == 0) {
                                float quality = (float) g_ascii_strtod(*viter, NULL);
                                if (quality < 0 || quality > 100) {
                                        g_set_error(error,
                                                    GDK_PIXBUF_ERROR,
                                                    GDK_PIXBUF_ERROR_BAD_OPTION,
                                                    "WebP quality must be a value between 0 and 100.");
                                        return FALSE;
                                }
                                config.quality = quality;
                        } else if (strncmp(*kiter, "preset", 6) == 0) {
                                WebPPreset preset;
                                if (strncmp(*viter, "default", 7) == 0) {
                                        preset = WEBP_PRESET_DEFAULT;
                                } else if (strncmp(*viter, "photo", 5) == 0) {
                                        preset = WEBP_PRESET_PHOTO;
                                } else if (strncmp(*viter, "picture", 7) == 0) {
                                        preset = WEBP_PRESET_PICTURE;
                                } else if (strncmp(*viter, "drawing", 7) == 0) {
                                        preset = WEBP_PRESET_DRAWING;
                                } else if (strncmp(*viter, "icon", 4) == 0) {
                                        preset = WEBP_PRESET_ICON;
                                } else if (strncmp(*viter, "text", 4) == 0) {
                                        preset = WEBP_PRESET_TEXT;
                                } else {
                                        g_set_error(error,
                                                    GDK_PIXBUF_ERROR,
                                                    GDK_PIXBUF_ERROR_BAD_OPTION,
                                                    "WebP encoder invalid preset.");
                                        return FALSE;
                                }
                                if (WebPConfigPreset(&config, preset, config.quality) == 0) {
                                        g_set_error(error,
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

        if (WebPValidateConfig(&config) != 1) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_BAD_OPTION,
                            "Invalid encoding configuration");
                return FALSE;
        }

        w = gdk_pixbuf_get_width(pixbuf);
        h = gdk_pixbuf_get_height(pixbuf);
        rowstride = gdk_pixbuf_get_rowstride(pixbuf);
        has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
        pixels = gdk_pixbuf_get_pixels(pixbuf);

        picture.width = w;
        picture.height = h;

        if (has_alpha) {
                rc = WebPPictureImportRGBA(&picture, pixels, rowstride);
        } else {
                rc = WebPPictureImportRGB(&picture, pixels, rowstride);
        }
        if (rc == 0) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                            "Failed to allocate picture");
                return FALSE;
        }

        if (to_callback) {
                picture.writer = save_callback;
                picture.custom_ptr = (void *) context;
        } else {
                picture.writer = write_file;
                picture.custom_ptr = (void *) f;
        }

        if (WebPEncode(&config, &picture) == 0) {
                fprintf(stderr, "Error! Cannot encode picture as WebP\n");
        }

        return TRUE;
}

static gboolean
gdk_pixbuf__webp_image_save(FILE *f,
                            GdkPixbuf *pixbuf,
                            gchar **keys,
                            gchar **values,
                            GError **error) {
        return real_save_webp(pixbuf, keys, values, error,
                              FALSE, f, NULL);
}

static gboolean
gdk_pixbuf__webp_image_save_to_callback(GdkPixbufSaveFunc save_func,
                                        gpointer user_data,
                                        GdkPixbuf *pixbuf,
                                        gchar **keys,
                                        gchar **values,
                                        GError **error) {
        save_context *context = g_new0 (save_context, 1);
        context->func = save_func;
        context->data = user_data;
        return real_save_webp(pixbuf, keys, values, error,
                              TRUE, NULL, context);
}

void
fill_vtable(GdkPixbufModule *module) {
        module->load = gdk_pixbuf__webp_image_load;
        module->begin_load = gdk_pixbuf__webp_image_begin_load;
        module->stop_load = gdk_pixbuf__webp_image_stop_load;
        module->load_increment = gdk_pixbuf__webp_image_load_increment;
        module->save = gdk_pixbuf__webp_image_save;
        module->save_to_callback = gdk_pixbuf__webp_image_save_to_callback;
        module->load_animation = gdk_pixbuf__webp_image_load_animation;
}

void
fill_info(GdkPixbufFormat *info) {
        static GdkPixbufModulePattern signature[] = {
                {"RIFFsizeWEBP", "    xxxx    ", 100},
                {NULL, NULL,                     0}
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

        info->name = "webp";
        info->signature = signature;
        info->description = "The WebP image format";
        info->mime_types = mime_types;
        info->extensions = extensions;
        info->flags = GDK_PIXBUF_FORMAT_WRITABLE | GDK_PIXBUF_FORMAT_THREADSAFE;
        info->license = "LGPL";
}
