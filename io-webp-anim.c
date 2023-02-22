#include "io-webp-anim.h"

#include <webp/decode.h>
#include <webp/encode.h>

typedef struct
{
  gint w;
  gint h;
} FrameSize;

typedef struct
{
  GByteArray *anim_data;
  FrameSize   frame_size;
  gboolean    is_static;
  GdkPixbuf  *cached_static_image;
} GdkWebpAnimationPrivate;

struct _GdkWebpAnimation
{
  GdkPixbufAnimation parent_instance;
};

G_DEFINE_TYPE_WITH_PRIVATE (GdkWebpAnimation, gdk_webp_animation, GDK_TYPE_PIXBUF_ANIMATION)

static void
anim_finalize (GObject *self)
{
  G_OBJECT_CLASS (gdk_webp_animation_parent_class)->finalize (self);
}

static void
anim_dispose (GObject *self)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (self));

  if (priv->anim_data)
    {
      g_byte_array_free (priv->anim_data, TRUE);
      priv->anim_data = NULL;
    }

  g_clear_object (&priv->cached_static_image);

  G_OBJECT_CLASS (gdk_webp_animation_parent_class)->dispose (self);
}

static gboolean
is_static_image (GdkPixbufAnimation *self)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (self));
  return priv->is_static;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static GdkPixbufAnimationIter *
get_iter (GdkPixbufAnimation *self, const GTimeVal *start_time)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (self));
  GError               *error = NULL;
  GdkWebpAnimationIter *iter
      = gdk_webp_animation_new_from_buffer_and_time (priv->anim_data, start_time, &error);
  if (error)
    {
      g_warning ("Could not instantiate WebP implementation of "
                 "GdkPixbufAnimationIter: %s",
                 error->message);
      g_error_free (error);
      return NULL;
    }

  return GDK_PIXBUF_ANIMATION_ITER (iter);
}
G_GNUC_END_IGNORE_DEPRECATIONS

static GdkPixbuf *
get_static_image (GdkPixbufAnimation *self)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (self));
  if (! priv->cached_static_image)
    {
      GdkPixbufAnimationIter *iter = get_iter (self, NULL);
      priv->cached_static_image    = gdk_pixbuf_animation_iter_get_pixbuf (
          GDK_PIXBUF_ANIMATION_ITER (iter));
      g_object_ref (priv->cached_static_image);
      g_object_unref (iter);
    }

  return priv->cached_static_image;
}

static void
get_size (GdkPixbufAnimation *self, gint *width, gint *height)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (self));
  if (width)
    *width = priv->frame_size.w;
  if (height)
    *height = priv->frame_size.h;
}

static void
gdk_webp_animation_class_init (GdkWebpAnimationClass *klass)
{
  GObjectClass            *object_class = G_OBJECT_CLASS (klass);
  GdkPixbufAnimationClass *anim_class   = GDK_PIXBUF_ANIMATION_CLASS (klass);

  object_class->finalize = anim_finalize;
  object_class->dispose  = anim_dispose;

  anim_class->is_static_image  = is_static_image;
  anim_class->get_static_image = get_static_image;
  anim_class->get_size         = get_size;
  anim_class->get_iter         = get_iter;
}

static void
gdk_webp_animation_init (GdkWebpAnimation *self)
{
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (self);
  *priv = (GdkWebpAnimationPrivate){ 0 };
}

GdkWebpAnimation *
gdk_webp_animation_new_from_bytes (GByteArray *data, GError **error)
{
  WebPBitstreamFeatures features = (WebPBitstreamFeatures){ 0 };
  if (WebPGetFeatures (data->data, data->len, &features) != VP8_STATUS_OK)
    {
      g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
                   "Could not get WebP feature information from file data");
      return NULL;
    }

  GdkWebpAnimation *anim = GDK_WEBP_ANIMATION (g_object_new (GDK_WEBP_ANIMATION_TYPE, NULL));
  GdkWebpAnimationPrivate *priv = gdk_webp_animation_get_instance_private (
      GDK_WEBP_ANIMATION (anim));

  priv->is_static  = features.has_animation == FALSE;
  priv->frame_size = (FrameSize){ features.width, features.height };
  priv->anim_data  = data;

  return anim;
}
