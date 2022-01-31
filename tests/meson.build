

t1 = executable('t1', 't1.c',
    link_with : pbl_webp, dependencies : [gdkpb, webp, webpdemux])
t2 = executable('t2', 't2.c', link_with : pbl_webp, dependencies : [gdkpb, webp, webpdemux])
t3 = executable('t3', 't3.c', link_with : pbl_webp, dependencies : [gdkpb, webp, webpdemux])
t4 = executable('t4', 't4.c', link_with : pbl_webp, dependencies : [gdkpb, webp, webpdemux, gio2])
# t4-gtk3 see below.
t5 = executable('t5', 't5.c', link_with : pbl_webp, dependencies : [gdkpb, webp, webpdemux, gio2])

loaders_data = configuration_data()
loaders_data.set('MODULE_PATH', pbl_webp.full_path())
loaders = configure_file(input: 'loaders.cache.in',
                         output : 'loaders.cache',
                         configuration : loaders_data)

test('load 1x1 image', t1, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t1.webp'])
test('load 200x200 image', t2, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t2.webp'])
test('load animation and parse', t3, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t3.webp'])
test('load animation and display', t4, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t3.webp'])

if gtk3.found()
t4_gtk3_exe = executable('t4_gtk3_exe',
                          sources: ['t4-gtk3.c'],
                          link_with : pbl_webp,
                          dependencies: [gdkpb, webp, webpdemux, gtk3],
                          install: false)
#test('t4_gtk3_exe', t4_gtk3_exe)
endif

test('load animation test duration', t5, env : ['GDK_PIXBUF_MODULE_FILE=' +
                                  meson.current_build_dir() + '/loaders.cache',
                          'TEST_FILE=' + meson.current_source_dir() + '/t5.webp'])
