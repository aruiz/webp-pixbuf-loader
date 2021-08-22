
project('webp-pixbuf-loader', 'c')
add_project_arguments('-pthread','-I/usr/include/gtk-3.0','-I/usr/include/at-spi2-atk/2.0','-I/usr/include/at-spi-2.0','-I/usr/include/dbus-1.0','-I/usr/lib/x86_64-linux-gnu/dbus-1.0/include','-I/usr/include/gtk-3.0','-I/usr/include/gio-unix-2.0/','-I/usr/include/cairo','-I/usr/include/pango-1.0','-I/usr/include/harfbuzz','-I/usr/include/pango-1.0','-I/usr/include/atk-1.0','-I/usr/include/cairo','-I/usr/include/pixman-1','-I/usr/include/freetype2','-I/usr/include/libpng16','-I/usr/include/freetype2','-I/usr/include/libpng16','-I/usr/include/gdk-pixbuf-2.0','-I/usr/include/libpng16','-I/usr/include/glib-2.0','-I/usr/lib/x86_64-linux-gnu/glib-2.0/include', language : 'c')
add_project_link_arguments('-lgtk-3','-lgdk-3','-lpangocairo-1.0','-lpango-1.0','-latk-1.0','-lcairo-gobject','-lcairo','-lgdk_pixbuf-2.0','-lgio-2.0','-lgobject-2.0','-lglib-2.0', language : 'c')
gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = get_option('gdk_pixbuf_moduledir')
if gdk_pb_moddir == ''
  gdk_pb_moddir = gdkpb.get_pkgconfig_variable('gdk_pixbuf_moduledir')
endif
gdk_pb_query_loaders = gdkpb.get_pkgconfig_variable('gdk_pixbuf_query_loaders')
webp = dependency('libwebp', version: '>0.4.3')
webpdemux = dependency('libwebpdemux', version: '>0.4.3')   # AGH added.

# -34/-64 is to overcome a Fedora bug in the .pc file
# debian has a similar problem with its .pc file but it requires knowing the platform triad

gdk_pb_query_loaders = find_program(get_option('gdk_pixbuf_query_loaders_path'), gdk_pb_query_loaders, gdk_pb_query_loaders+'-32', gdk_pb_query_loaders+'-64')


pbl_webp = shared_library('pixbufloader-webp',
                          sources: ['io-webp.c', 'io-webp-anim.c'],
                          dependencies: [gdkpb, webp, webpdemux],
                          install: true,
                          install_dir: gdk_pb_moddir)

cdata = configuration_data()
cdata.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(input: 'webp-pixbuf.thumbnailer.in',
               output: 'webp-pixbuf.thumbnailer',
               configuration: cdata,
               install: true,
               install_dir: get_option('datadir') / 'thumbnailers')

meson.add_install_script(gdk_pb_query_loaders.path(), '--update-cache')

subdir('tests')

