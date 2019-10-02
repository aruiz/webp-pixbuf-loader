project('webp-pixbuf-loader', 'c')

gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = gdkpb.get_pkgconfig_variable('gdk_pixbuf_moduledir')
gdk_pb_query_loaders = gdkpb.get_pkgconfig_variable('gdk_pixbuf_query_loaders')

webp = dependency('libwebp', version: '>0.4.3')

prog = find_program(gdk_pb_query_loaders, gdk_pb_query_loaders+'-64') # -64 is to overcome a Fedora bug in the .pc file

pbl_webp = shared_library('pixbufloader-webp', 'io-webp.c',
                          dependencies: [gdkpb, webp],
                          install: true,
                          install_dir: gdk_pb_moddir)

meson.add_install_script(prog.path())
