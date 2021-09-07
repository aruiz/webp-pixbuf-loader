/* GdkPixbuf library - WebP Image Loader
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * Copyright (C) 2021 Alan Hawrelak
 *
 * Authors: Alan Hawrelak <alangh@shaw.ca>
 *      with some code copied from io-webp.c with its authors.
 */

#include "io-webp-anim.h"


G_DEFINE_TYPE (GdkPixbufWebpAnim, gdk_pixbuf_webp_anim, GDK_TYPE_PIXBUF_ANIMATION);
G_DEFINE_TYPE (GdkPixbufWebpAnimIter, gdk_pixbuf_webp_anim_iter, GDK_TYPE_PIXBUF_ANIMATION_ITER);

static void
destroy_data(guchar *pixels, gpointer data) {
        g_free(pixels);
}

/* gdk_pixbuf_webp_anim_class code =============================== */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static GdkPixbufAnimationIter *
gdk_pixbuf_webp_anim_get_iter(GdkPixbufAnimation *anim,
                              const GTimeVal *start_time);

G_GNUC_END_IGNORE_DEPRECATIONS

static GdkPixbuf *gdk_pixbuf_webp_anim_iter_get_pixbuf(GdkPixbufAnimationIter *iter);

static gboolean gdk_pixbuf_webp_image_is_static_image(GdkPixbufAnimation *animation) {
        GdkPixbufWebpAnim *webp_anim;
        gboolean is_static = TRUE;
        webp_anim = GDK_PIXBUF_WEBP_ANIM (animation);

        if (webp_anim != NULL && webp_anim->context != NULL) {
                /* WebPGetFeatures already called on webp_anim->context->features in creating webp_anim. */
                if (webp_anim->context->features.has_animation != 0) {
                        is_static = FALSE;
                }
        }
        return is_static;
}

static GdkPixbuf *gdk_pixbuf_webp_anim_get_static_image(GdkPixbufAnimation *anim) {
        GdkPixbufWebpAnim *webp_anim;
        GdkPixbufAnimationIter *iter;
        webp_anim = GDK_PIXBUF_WEBP_ANIM (anim);

        if (!webp_anim->webp_iter) {
                iter = gdk_pixbuf_webp_anim_get_iter(anim, NULL);
                webp_anim->webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
                if (webp_anim->webp_iter == NULL) {
                        return NULL;
                }
        } else {
                iter = GDK_PIXBUF_ANIMATION_ITER (webp_anim->webp_iter);
        }

        return gdk_pixbuf_webp_anim_iter_get_pixbuf(iter);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
iter_clear(GdkPixbufWebpAnimIter *iter) {
        iter->cur_frame = 0;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
iter_restart(GdkPixbufWebpAnimIter *iter) {
        iter_clear(iter);
}

/*
 * This assumes that WebPDemuxGetFrame(demuxer, 1, webp_iter->wpiter) or
 *   WebPDemuxNextFrame(webp_iter->wpiter)
 * has just been called and there is data in wpiter to create a pixbuf.
 */
static
GdkPixbuf *
data_to_pixbuf(GdkPixbufWebpAnimIter *iter, gboolean *has_error) {
        GdkPixbufWebpAnimIter *webp_iter;
        gint w, h;
        GdkPixbuf *pixbuf;
        guint32 data_size;
        gpointer data;
        guint8 *out;

        webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
        data = (gpointer) webp_iter->wpiter->fragment.bytes;    /* casting away const. */
        data_size = webp_iter->wpiter->fragment.size;
        gboolean use_alpha = webp_iter->webp_anim->context->features.has_alpha;
        if (use_alpha) {
                out = WebPDecodeRGBA(data, data_size, &w, &h);
        } else {
                out = WebPDecodeRGB(data, data_size, &w, &h);
        }

        if (!out) {
                *has_error = TRUE;
                return NULL;
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
                *has_error = TRUE;
                return NULL;
        }

        (void) g_object_ref(pixbuf);  /* if absent, eog fails to load animation. */
        GdkPixbuf *pixb = webp_iter->webp_anim->context->pixbuf;
        if (pixb) {
                g_object_unref(pixb);
        }
        if (!GDK_IS_PIXBUF (pixbuf)) {
                *has_error = TRUE;
                return NULL;
        }
        webp_iter->webp_anim->context->pixbuf = pixbuf;
        return pixbuf;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static GdkPixbufAnimationIter *
gdk_pixbuf_webp_anim_get_iter(GdkPixbufAnimation *anim,
                              const GTimeVal *start_time) {
        if (!anim) {
                return NULL;
        }
        GdkPixbufWebpAnim *old_anim = GDK_PIXBUF_WEBP_ANIM (anim);
        if (old_anim && old_anim->webp_iter) {
                return GDK_PIXBUF_ANIMATION_ITER (old_anim->webp_iter);
        }

        GdkPixbufWebpAnimIter *webp_iter = NULL;
        webp_iter = g_object_new(GDK_TYPE_PIXBUF_WEBP_ANIM_ITER, NULL);
        (void) g_object_ref(webp_iter);
        webp_iter->webp_anim = GDK_PIXBUF_WEBP_ANIM (anim);
        webp_iter->webp_anim->webp_iter = webp_iter;
        g_object_ref (webp_iter->webp_anim);

        webp_iter->wpiter = g_try_new0(WebPIterator, 1);
        /* Attempt to initialize the WebPIterator. */
        (void) WebPDemuxGetFrame(webp_iter->webp_anim->demuxer, 1, webp_iter->wpiter);
        gboolean has_err = FALSE;
        (void) data_to_pixbuf(webp_iter, &has_err);
        if (has_err) {
                return NULL;
        }
        iter_restart(webp_iter);
        return GDK_PIXBUF_ANIMATION_ITER (webp_iter);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void gdk_pixbuf_webp_anim_get_size(GdkPixbufAnimation *animation,
                                          int *width,
                                          int *height) {
        GdkPixbufWebpAnim *anim = GDK_PIXBUF_WEBP_ANIM (animation);
        if (anim && anim->context) {
                if (width) {
                        *width = anim->context->features.width;
                }
                if (height) {
                        *height = anim->context->features.height;
                }
        }
}

static void gdk_pixbuf_webp_anim_finalize(GObject *object) {
        GdkPixbufWebpAnim *anim = GDK_PIXBUF_WEBP_ANIM (object);

        if (anim->context->anim_incr.filedata) {
                g_free(anim->context->anim_incr.filedata);
                anim->context->anim_incr.filedata = NULL;
        }

        if (anim->context->anim_incr.data) {
                g_free(anim->context->anim_incr.data);
                anim->context->anim_incr.data = NULL;
                anim->context->anim_incr.used_len = 0;
        }

        WebPDemuxReleaseIterator(anim->webp_iter->wpiter);
        if (anim->demuxer) {
                WebPDemuxDelete(anim->demuxer);
        }

        WebPAnimDecoderDelete(anim->dec);
        g_free(anim->animInfo);
        g_free(anim->decOptions);
        if (anim->context->pixbuf) {
                g_object_unref(anim->context->pixbuf);
        }
        g_free(anim->context);   /* not handled in io-webp.c */
        anim->context = NULL;

        G_OBJECT_CLASS (gdk_pixbuf_webp_anim_parent_class)->finalize(object);
}

static GdkPixbuf *gdk_pixbuf_webp_anim_iter_get_pixbuf(GdkPixbufAnimationIter *iter) {
        /* WebPBitstreamFeatures   features; */
        GdkPixbufWebpAnimIter *webp_iter;

        webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
        if (webp_iter && webp_iter->webp_anim && webp_iter->webp_anim->context) {
                /* features = webp_iter->webp_anim->context->features; */
        } else {
                return NULL;
        }

        WebPIterator *curr = webp_iter->wpiter;
        if (!curr) {
                return NULL;
        }

        gboolean ispix = GDK_IS_PIXBUF (webp_iter->webp_anim->context->pixbuf);
        if (ispix) {
                return webp_iter->webp_anim->context->pixbuf;
        } else {
                return NULL;
        }
}

static void
gdk_pixbuf_webp_anim_class_init(GdkPixbufWebpAnimClass *klass) {
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationClass *anim_class = GDK_PIXBUF_ANIMATION_CLASS (klass);

        object_class->finalize = gdk_pixbuf_webp_anim_finalize;

        anim_class->is_static_image = gdk_pixbuf_webp_image_is_static_image;
        anim_class->get_static_image = gdk_pixbuf_webp_anim_get_static_image;
        anim_class->get_size = gdk_pixbuf_webp_anim_get_size;
        anim_class->get_iter = gdk_pixbuf_webp_anim_get_iter;
}

static void
gdk_pixbuf_webp_anim_init(GdkPixbufWebpAnim *anim) {
}
/* end gdk_pixbuf_webp_anim_class code =============================== */


/* gdk_pixbuf_webp_anim_iter class code ============================== */
static void
gdk_pixbuf_webp_anim_iter_init(GdkPixbufWebpAnimIter *iter) {
}

static void
gdk_pixbuf_webp_anim_iter_finalize(GObject *object) {
        GdkPixbufWebpAnimIter *iter = GDK_PIXBUF_WEBP_ANIM_ITER (object);

        /* See the note on WebPDemuxGetFrame in webp/demux.h.
         * Also apparently WebPDemuxDelete does not need to be called
         * because we are not using the demux chunk calls.
         */
        WebPDemuxReleaseIterator(iter->wpiter);
        g_free(iter->wpiter);
        iter->wpiter = NULL;

        g_object_unref(iter->webp_anim);
        iter->webp_anim = NULL;
        G_OBJECT_CLASS (gdk_pixbuf_webp_anim_iter_parent_class)->finalize(object);
}

static int gdk_pixbuf_webp_anim_iter_get_delay_time(GdkPixbufAnimationIter *iter) {
        int dur = 0;
        GdkPixbufWebpAnimIter *webp_iter;

        webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
        if (webp_iter && webp_iter->wpiter) {
                dur = webp_iter->wpiter->duration;

                /* It the loops are completed,
                 * give the delay time of -1, to indicate
                 * the current image stays forever.
                 */
                if ((webp_iter->webp_anim->loops_completed >= 1) &&
                    (webp_iter->webp_anim->animInfo->loop_count > 0) && /* loop_count == 0 means loop forever. */
                    (webp_iter->webp_anim->loops_completed >=
                     webp_iter->webp_anim->animInfo->loop_count)) {
                        dur = -1;
                }
        }
        return dur;
}

static gboolean gdk_pixbuf_webp_anim_iter_on_currently_loading_frame(GdkPixbufAnimationIter *iter) {
        gboolean is_loading = FALSE;
        GdkPixbufWebpAnimIter *webp_iter;
        webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
        if (webp_iter && webp_iter->wpiter) {
                is_loading = !webp_iter->wpiter->complete;
        }
        return is_loading;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static gboolean gdk_pixbuf_webp_anim_iter_advance(GdkPixbufAnimationIter *iter,
                                                  const GTimeVal *current_time) {
        gboolean hasFrameChanged = FALSE;
        GdkPixbufWebpAnimIter *webp_iter;
        webp_iter = GDK_PIXBUF_WEBP_ANIM_ITER (iter);
        if (!webp_iter || !webp_iter->wpiter) {
                return FALSE;
        }

        /* Now move to the proper frame */
        const WebPDemuxer *demuxer = webp_iter->webp_anim->demuxer;
        /* uint32_t num_frames = WebPDemuxGetI(demuxer, WEBP_FF_FRAME_COUNT); */
        uint32_t num_frames = webp_iter->webp_anim->animInfo->frame_count;
        int cur_frame = webp_iter->cur_frame;
        if (webp_iter->wpiter->complete) {
                /* then change frames. */
                if (webp_iter->cur_frame >= num_frames) {
                        webp_iter->webp_anim->loops_completed += 1;
                        if ((webp_iter->webp_anim->animInfo->loop_count > 0) &&
                            (webp_iter->webp_anim->loops_completed >=
                             webp_iter->webp_anim->animInfo->loop_count)) {
                                /* This allows breaking the, otherwise endless, loop in eog at
                                 *  eog-image.c:line 2434 in private_timeout().
                                 */
                                return TRUE;
                        }

                        /* Clean up for the next loop. */
                        WebPDemuxReleaseIterator(webp_iter->wpiter);
                        cur_frame = 0;  /* so it will loop at the end of the animation. */
                }
                if (!webp_iter->wpiter->complete) {   /* This does not handle partial loads. All data must be loaded. */
                        return FALSE;
                }
                cur_frame += 1;

                if (cur_frame == 1) {
                        if (WebPDemuxGetFrame(demuxer, 1, webp_iter->wpiter) == FALSE) {
                                return FALSE;
                        }
                } else {
                        if (WebPDemuxNextFrame(webp_iter->wpiter) == FALSE) {
                                return FALSE;
                        }
                }

                gboolean has_err = FALSE;
                (void) data_to_pixbuf(webp_iter, &has_err);
                if (has_err) {
                        return FALSE;
                }

                /* set the status variables. */
                webp_iter->cur_frame = cur_frame;
                hasFrameChanged = TRUE;
        }
        return hasFrameChanged;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
gdk_pixbuf_webp_anim_iter_class_init(GdkPixbufWebpAnimIterClass *klass) {
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationIterClass *anim_iter_class =
                GDK_PIXBUF_ANIMATION_ITER_CLASS (klass);

        object_class->finalize = gdk_pixbuf_webp_anim_iter_finalize;

        anim_iter_class->get_delay_time = gdk_pixbuf_webp_anim_iter_get_delay_time;
        anim_iter_class->get_pixbuf = gdk_pixbuf_webp_anim_iter_get_pixbuf;
        anim_iter_class->on_currently_loading_frame = gdk_pixbuf_webp_anim_iter_on_currently_loading_frame;
        anim_iter_class->advance = gdk_pixbuf_webp_anim_iter_advance;
}
/* end gdk_pixbuf_webp_anim_iter class code ============================== */

/* returns raw data in pdata.
 * If error, pdata-> size and bytes are set to 0 or NULL.
 * Updates context->features if successful. */
void
get_data_from_file(FILE *f, WebPContext *context, GError **error, WebPData *pdata) {
        guint32 data_size;
        gpointer data;
        WebPBitstreamFeatures features;

        pdata->size = 0;
        pdata->bytes = NULL;

        /* Get data size */
        fseek(f, 0, SEEK_END);
        data_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        /* Get data */
        data = g_malloc(data_size);
        gboolean ok = (fread(data, data_size, 1, f) == 1);
        if (!ok) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_FAILED,
                            "Failed to read file");
                return;
        }

        /* Check for RIFF and WEBP tags in WebP container header. */
        char tag[5];
        tag[4] = 0;
        for (int i = 0; i < 4; i++) { tag[i] = *(u_int8_t *) (data + i); }
        int rc2 = strcmp(tag, "RIFF");
        if (rc2 != 0) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                            "Cannot read WebP image header...");
                return;
        }
        for (int i = 0; i < 4; i++) { tag[i] = *(u_int8_t *) (data + 8 + i); }
        rc2 = strcmp(tag, "WEBP");
        if (rc2 != 0) {
                g_set_error(error,
                            GDK_PIXBUF_ERROR,
                            GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                            "Cannot read WebP image header...");
                return;
        }

        pdata->bytes = data;
        pdata->size = data_size;
        if (context->anim_incr.filedata) {
                g_free(context->anim_incr.filedata);
        }
        context->anim_incr.filedata = data;

        if (WebPGetFeatures(data, data_size, &features) == VP8_STATUS_OK) {
                context->features = features;
        }
}

/* in_context parameter can be NULL. */
extern
GdkPixbufAnimation *gdk_pixbuf__webp_image_load_animation_data(const guchar *buf, guint size,
                                                               WebPContext *in_context,
                                                               GError **error) {
        g_return_val_if_fail (buf != NULL, NULL);
        g_return_val_if_fail (size != 0, NULL);
        WebPContext *context = in_context;
        GdkPixbufWebpAnim *animation = NULL;
        WebPAnimDecoderOptions *dec_options = NULL;
        WebPAnimDecoder *dec = NULL;
        WebPAnimInfo *ptr_anim_info = NULL;

        animation = g_object_new(GDK_TYPE_PIXBUF_WEBP_ANIM, NULL);
        if (animation == NULL) {
                g_set_error_literal(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    "Not enough memory to load WebP file");
                goto error_end;
        }

        dec_options = g_try_new0(WebPAnimDecoderOptions, 1);
        if (!dec_options || !WebPAnimDecoderOptionsInit(dec_options)) {
                g_set_error_literal(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "WebPAnimDecoderOptionsInit() failed.");
                goto error_end;
        }
        dec_options->color_mode = MODE_RGBA;
        if (context == NULL) {
                context = g_try_new0(WebPContext, 1);
                if (context == NULL) {
                        g_set_error_literal(error,
                                            GDK_PIXBUF_ERROR,
                                            GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                            "Not enough memory to load WebP file");
                        goto error_end;
                }
        }

        animation->decOptions = dec_options;
        animation->pdata.bytes = buf;
        animation->pdata.size = size;
        /* pdata has the same lifetime as dec, below. */

        dec = WebPAnimDecoderNew(&animation->pdata, dec_options);
        ptr_anim_info = g_try_new0 (WebPAnimInfo, 1);
        if (ptr_anim_info == NULL) {
                g_set_error_literal(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    "Not enough memory to load WebP file");
                goto error_end;
        }

        if (!WebPAnimDecoderGetInfo(dec, ptr_anim_info)) {
                g_set_error_literal(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_FAILED,
                                    "WebPAnimDecoderGetInfo could not get animation info.");
                goto error_end;
        }
        animation->dec = dec;
        animation->decOptions = dec_options;
        animation->demuxer = (WebPDemuxer *) WebPAnimDecoderGetDemuxer(dec); /* cast to discard const. */
        animation->animInfo = ptr_anim_info;
        context->error = error;
        animation->context = context;

        return GDK_PIXBUF_ANIMATION (animation);

        error_end:
        if (context && context->error && *(context->error))
                g_print("%s\n", (*(context->error))->message);

        if (ptr_anim_info != NULL) {
                g_free(ptr_anim_info);
        }
        if (dec != NULL) {
                WebPAnimDecoderDelete(dec);
        }
        if (animation != NULL) {
                (void) g_object_unref(animation);
        }
        if (dec_options != NULL) {
                g_free(dec_options);
        }
        return NULL;
}

extern
GdkPixbufAnimation *gdk_pixbuf__webp_image_load_animation(FILE *file,
                                                          GError **error) {
        g_return_val_if_fail (file != NULL, NULL);
        WebPContext *context = NULL;

        context = g_try_new0(WebPContext, 1);
        if (context == NULL) {
                g_set_error_literal(error,
                                    GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    "Not enough memory to load WebP file");
                return NULL;
        }

        /* read in the file to get data. This updates context->features. */
        WebPData pdata;
        get_data_from_file(file, context, error, &pdata);
        return gdk_pixbuf__webp_image_load_animation_data(pdata.bytes, pdata.size, context, error);
}
