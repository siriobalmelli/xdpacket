src_files = files([
	'checksums.c',
    'iface.c',
    'field.c',
	'memref.c',
	'operations.c',
    'parse2.c',
    'process.c',
	'rout.c',
    'rule.c',
	'value.c',
    'xdpacket_globals.c',
    'yamlutils.c'
])

xdpacket = executable(meson.project_name(),
			['xdpacket.c'] + src_files,
			dependencies : deps,
			include_directories : inc,
			install: true
			)

