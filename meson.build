project('webp-pixbuf-loader', 'c', version: '0.2.7', meson_version: '>=0.54')
cc = meson.get_compiler('c')
gio = dependency('gio-2.0', method: 'pkg-config')
gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = get_option('gdk_pixbuf_moduledir')
if gdk_pb_moddir == ''
  gdk_pb_moddir = gdkpb.get_variable(pkgconfig: 'gdk_pixbuf_moduledir', pkgconfig_define: ['prefix', get_option('prefix')])
endif

webp = dependency('libwebp', version: '>=1.3.2')
webpdemux = dependency('libwebpdemux', version: '>=1.3.2')
webpmux = dependency('libwebpmux', version: '>=1.3.2')

pbl_webp = shared_module('pixbufloader-webp',
                         sources: ['io-webp.c', 'io-webp-anim.c', 'io-webp-anim-iter.c'],
                         dependencies: [gdkpb, webp, webpdemux, webpmux],
                         # Workaround for https://gitlab.gnome.org/GNOME/glib/issues/1413
                         name_suffix: host_machine.system() == 'darwin' ? 'so' : [],
                         install: true,
                         install_dir: gdk_pb_moddir)

cdata = configuration_data()
cdata.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(input: 'webp-pixbuf.thumbnailer.in',
               output: 'webp-pixbuf.thumbnailer',
               configuration: cdata,
               install: true,
               install_dir: get_option('datadir') / 'thumbnailers')

warning('If you install this loader locally, make sure you rebuild the pixbuf loader cache')
warning('To rebuild the cache run: `pkg-config gdk-pixbuf-2.0 --variable=gdk_pixbuf_query_loaders` --update-cache')

subdir('tests')
