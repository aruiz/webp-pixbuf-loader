def options(opt):
    opt.load('compiler_c')


def configure(conf):
    conf.load('compiler_c glib2')
    conf.find_program('gdk-pixbuf-query-loaders')
    conf.check_cc(lib='webp', header_name='webp/decode.h',
                  uselib_store='WEBP',  mandatory=True)
    conf.check_cfg(package='gdk-pixbuf-2.0', uselib_store='GDK_PIXBUF',
                   atleast_version='2.22.0', args='--cflags --libs',
                   mandatory=True)
    conf.check_cfg(package='gdk-pixbuf-2.0', uselib_store='GDK_PIXBUF_VAR',
                   atleast_version='2.22.0',
                   variables='gdk_pixbuf_moduledir',
                   mandatory=True)


def gdk_pixbuf_query_loaders(bld):
    bld.exec_command('gdk-pixbuf-query-loaders --update-cache')


def build(bld):
    if bld.cmd == 'install':
        bld.add_post_fun(gdk_pixbuf_query_loaders)

    bld.new_task_gen(
        features='c cshlib',
        source='io-webp.c',
        target='pixbufloader-webp',
        uselib='GDK_PIXBUF WEBP',
        install_path='${GDK_PIXBUF_VAR_gdk_pixbuf_moduledir}')
