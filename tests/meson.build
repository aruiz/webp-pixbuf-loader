

t1 = executable('t1', 't1.c', dependencies : [gdkpb, webp, webpdemux])
t2 = executable('t2', 't2.c', dependencies : [gdkpb, webp, webpdemux])
t3 = executable('t3', 't3.c', dependencies : [gdkpb, webp, webpdemux])
t4 = executable('t4', 't4.c', dependencies : [gdkpb, webp, webpdemux])

if meson.version().version_compare('>=0.54.0')
  # The Meson fs.as_posix() feature was introduced in 0.54.0
  fs = import('fs')
  module_path = fs.as_posix(pbl_webp.full_path())
else
  module_path = pbl_webp.full_path().replace('\\', '/')
endif

loaders_data = configuration_data()
loaders_data.set('MODULE_PATH', module_path)
loaders = configure_file(input: 'loaders.cache.in',
                         output : 'loaders.cache',
                         configuration : loaders_data)

test('load 1x1 image', t1, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t1.webp'])
test('load 200x200 image', t2, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t2.webp'])
test('load animation', t3, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t3.webp'])
test('get file info', t4, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t1.webp'])
