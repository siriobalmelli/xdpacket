src_files = files([
	'hook.c',
    'iface.c',
    'field.c',
    'field2.c',
    'fval.c',
    'matcher.c',
    'node2.c',
    'packet.c',
    'parse.c',
    'parse2.c',
    'parse_util.c',
    'xdpacket_globals.c',
    'yamlutils.c'
])

xdpk = executable('xdpk',
			['xdpk.c'] + src_files,
			dependencies : deps,
			include_directories : inc,
			)

xdpacket = executable(meson.project_name(),
			['xdpacket.c'] + src_files,
			dependencies : deps,
			include_directories : inc,
			install: true
			)

