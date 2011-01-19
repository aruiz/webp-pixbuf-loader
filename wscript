def options(opt):
	opt.load('compiler_c')

def configure(conf):
	conf.load('compiler_c glib2')
	conf.find_program('gdk-pixbuf-query-loaders')
	conf.check_cc(lib='webpdecode', header_name='webp/decode.h',
	              uselib_store='WEBP', auto_add_header_name=True,
	              mandatory=True)
	conf.check_cfg(package='gdk-pixbuf-2.0', uselib_store='GDK_PIXBUF',
	               atleast_version='2.22.0', args='--cflags --libs',
	               mandatory=True)
	
def build(bld):
	bld.new_task_gen(
		features = 'c cshlib',
		source = 'io-webp.c',
		target = 'pixbufloader-webp',
		uselib = 'GDK_PIXBUF')
