
project('webp-pixbuf-loader', 'c', meson_version: '>=0.50')
gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = get_option('gdk_pixbuf_moduledir')
if gdk_pb_moddir == ''
  gdk_pb_moddir = gdkpb.get_pkgconfig_variable('gdk_pixbuf_moduledir')
endif
gdk_pb_query_loaders = gdkpb.get_pkgconfig_variable('gdk_pixbuf_query_loaders')
webp = dependency('libwebp', version: '>0.4.3')
webpdemux = dependency('libwebpdemux', version: '>0.4.3')
gio2 = dependency('gio-2.0', version: '>2.56.0', method: 'pkg-config')
gtk3 = dependency('gtk+-3.0', version: '>3.22.0', method: 'pkg-config', required: false)

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

