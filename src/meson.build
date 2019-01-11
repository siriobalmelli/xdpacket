src_files = files([
    'iface.c',
    'field2.c',
    'fval.c',
    'parse2.c',
    'process.c',
    'rule.c',
    'xdpacket_globals.c',
    'yamlutils.c'
])

xdpacket = executable(meson.project_name(),
			['xdpacket.c'] + src_files,
			dependencies : deps,
			include_directories : inc,
			install: true
			)

