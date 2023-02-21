#ifndef __IO_WEBP_ANIM_H__
#define __IO_WEBP_ANIM_H__

#define GDK_PIXBUF_ENABLE_BACKEND

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>

#undef GDK_PIXBUF_ENABLE_BACKEND

G_BEGIN_DECLS

#define GDK_WEBP_ANIMATION_TYPE gdk_webp_animation_get_type ()
G_DECLARE_FINAL_TYPE (GdkWebpAnimation, gdk_webp_animation, GDK_WEBP, ANIMATION, GdkPixbufAnimation)

GdkWebpAnimation* gdk_webp_animation_new_from_bytes (GByteArray *data, GError **error);
G_END_DECLS

#endif /* __IO_WEBP_ANIM_H__ */
