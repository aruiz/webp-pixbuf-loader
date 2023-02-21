#include "io-webp-anim.h"

#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/demux.h>

typedef struct {
  GdkPixbuf *pb;
  gint length;
} Frame;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
typedef struct
{
        GTimeVal start_time;
        GTimeVal curr_time;
        GArray *frames;
        gint loop_length;
        gsize curr_frame;
} GdkWebpAnimationIterPrivate;


struct _GdkWebpAnimationIter
{
  GdkPixbufAnimationIter *parent_instance;
};

static void
clear_frame (Frame *fr) {
  g_object_unref (fr->pb);
}

G_DEFINE_TYPE_WITH_PRIVATE (GdkWebpAnimationIter, gdk_webp_animation_iter, GDK_TYPE_PIXBUF_ANIMATION_ITER)

static void
iter_finalize (GObject *self)
{
  //GdkWebpAnimationIterPrivate *priv = gdk_webp_animation_iter_get_instance_private (
  //    GDK_WEBP_ANIMATION_ITER (self));

  G_OBJECT_CLASS (gdk_webp_animation_iter_parent_class)->finalize (self);
}

static void
iter_dispose (GObject *self)
{
  GdkWebpAnimationIterPrivate *priv = gdk_webp_animation_iter_get_instance_private (
      GDK_WEBP_ANIMATION_ITER (self));

  if (priv->frames) {
    g_array_free(priv->frames, TRUE);
    priv->frames = NULL;
  }

  G_OBJECT_CLASS (gdk_webp_animation_iter_parent_class)->dispose (self);
}

static void
gdk_webp_animation_iter_class_init (GdkWebpAnimationIterClass *klass)
{
  GObjectClass            *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = iter_finalize;
  object_class->dispose  = iter_dispose;
}
#include <webp/encode.h>
static void
gdk_webp_animation_iter_init (GdkWebpAnimationIter *self)
{
  GdkWebpAnimationIterPrivate *priv = gdk_webp_animation_iter_get_instance_private (self);
  *priv = (GdkWebpAnimationIterPrivate){ 0 };

  priv->frames = g_array_new (FALSE, FALSE, sizeof(Frame));
  g_array_set_clear_func (priv->frames, (GDestroyNotify)clear_frame);
}

GdkWebpAnimationIter *
gdk_webp_animation_new_from_buffer_and_time (const GByteArray *buf, GTimeVal *start_time, GError **error)
{
  WebPAnimDecoderOptions opts;
  if (!WebPAnimDecoderOptionsInit (&opts)) {
    g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED, "Could not initialize WebP decoder options");
    return NULL;
  }

  opts.color_mode = MODE_RGBA;

  WebPData data = { .bytes = buf->data, .size = (size_t)buf->len };
  WebPAnimDecoder *adec = WebPAnimDecoderNew (&data, &opts);
  if (!adec) {
    g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED, "Could not instantiate WebPAnimDecoder");
    return NULL;
  }

  WebPAnimInfo ainfo;
  if (!WebPAnimDecoderGetInfo (adec, &ainfo)) {
    g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED, "Could not get animation info from WebP decoder");
    g_clear_pointer(&adec, WebPAnimDecoderDelete);
    return NULL;
  }

  GdkWebpAnimationIter *iter = g_object_new (GDK_WEBP_ANIMATION_ITER_TYPE, NULL);
  GdkWebpAnimationIterPrivate *priv = gdk_webp_animation_iter_get_instance_private (iter);

  if (start_time == NULL) {
    g_get_current_time (&priv->start_time);
  } else {
    priv->start_time = *start_time;
  }

  guchar *tmp_data = NULL;
  gint timestamp = 0;
  gint loop_length = 0;

  while (WebPAnimDecoderHasMoreFrames (adec)) {
    if (!WebPAnimDecoderGetNext (adec, &tmp_data, &timestamp)) {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED, "Could not get frame from WebP animation decoder");
      g_clear_pointer(&adec, WebPAnimDecoderDelete);
      return NULL;
    }

    GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, (gint)ainfo.canvas_width, (gint)ainfo.canvas_height);

    guchar *pb_pixels = gdk_pixbuf_get_pixels (pb);
    gsize   pb_stride = (gsize)gdk_pixbuf_get_rowstride(pb);
    gsize data_stride = (gsize)ainfo.canvas_width * 4;
    for (int i=0; i<ainfo.canvas_height; i++) {
      guchar *data_row = tmp_data + (i * data_stride);
      guchar *pb_row = pb_pixels + (i * pb_stride);
      memcpy (pb_row, data_row, data_stride);
    }

    Frame f = { .pb = pb, .length = timestamp - loop_length };
    g_array_append_val (priv->frames, f);

    loop_length = timestamp;
    tmp_data = NULL;
    timestamp = 0;
  }

  g_clear_pointer(&adec, WebPAnimDecoderDelete);
  return iter;
}
G_GNUC_END_IGNORE_DEPRECATIONS
