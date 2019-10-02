project('webp-pixbuf-loader', 'c')

gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = gdkpb.get_pkgconfig_variable('gdk_pixbuf_moduledir')
gdk_pb_query_loaders = gdkpb.get_pkgconfig_variable('gdk_pixbuf_query_loaders')

webp = dependency('libwebp', version: '>0.4.3')

# -34/-64 is to overcome a Fedora bug in the .pc file
# debian has a similar problem with its .pc file but it requires knowing the platform triad

gdk_pb_query_loaders = find_program(get_option('gdk_pixbuf_query_loaders_path'), gdk_pb_query_loaders, gdk_pb_query_loaders+'-32', gdk_pb_query_loaders+'-64')


pbl_webp = shared_library('pixbufloader-webp', 'io-webp.c',
                          dependencies: [gdkpb, webp],
                          install: true,
                          install_dir: gdk_pb_moddir)

meson.add_install_script(gdk_pb_query_loaders.path(), '--update-cache')
