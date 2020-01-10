t1 = executable('t1', 't1.c', link_with : pbl_webp, dependencies : [gdkpb, webp])
loaders_data = configuration_data()
loaders_data.set('MODULE_PATH', pbl_webp.full_path())
loaders = configure_file(input: 'loaders.cache.in',
                         output : 'loaders.cache',
                         configuration : loaders_data)
test('load image', t1, env : ['GDK_PIXBUF_MODULE_FILE=' +
                              meson.current_build_dir() + '/loaders.cache',
                              'TEST_FILE=' + meson.current_source_dir() + '/t1.webp'])
