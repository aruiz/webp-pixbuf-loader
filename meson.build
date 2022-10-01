project('webp-pixbuf-loader', 'c', meson_version: '>=0.53')
cc = meson.get_compiler('c')
gdkpb = dependency('gdk-pixbuf-2.0', version: '>2.22.0', method: 'pkg-config')
gdk_pb_moddir = get_option('gdk_pixbuf_moduledir')
if gdk_pb_moddir == ''
  gdk_pb_moddir = gdkpb.get_variable(pkgconfig: 'gdk_pixbuf_moduledir', pkgconfig_define: ['prefix', get_option('prefix')])
endif
gdk_pb_query_loaders = gdkpb.get_variable(pkgconfig: 'gdk_pixbuf_query_loaders')
webp = dependency('libwebp', version: '>0.4.3')
webpdemux = dependency('libwebpdemux', version: '>0.4.3')

# This is a workaround since debian doesn't specify the query-loaders path in the .pc file
# and it uses the platform triad in its path
dirs = []
if host_machine.system() == 'linux'
  machine_triplet = run_command(cc.get_id(), '-dumpmachine', check: true).stdout().strip()
  dirs = ['/usr/lib' / machine_triplet / 'gdk-pixbuf-2.0']
endif

# -34/-64 is to overcome a Fedora bug in the .pc file
gdk_pb_query_loaders = find_program(get_option('gdk_pixbuf_query_loaders_path'), gdk_pb_query_loaders, gdk_pb_query_loaders+'-32', gdk_pb_query_loaders+'-64', dirs: dirs)

headers = [
  'endian.h',
  'sys/endian.h',
  'sys/byteorder.h',
]

foreach h : headers
  if cc.has_header(h)
    add_global_arguments('-DHAVE_' + h.underscorify().to_upper(), language : 'c')
  endif
endforeach

pbl_webp = shared_module('pixbufloader-webp',
                         sources: ['io-webp.c', 'io-webp-anim.c'],
                         dependencies: [gdkpb, webp, webpdemux],
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

meson.add_install_script(gdk_pb_query_loaders.path(), '--update-cache')

subdir('tests')

