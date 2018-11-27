src_files = files([
	'hook.c',
    'iface.c',
    'field.c',
    'matcher.c',
    'packet.c',
    'parse.c',
	'parse_util.c'
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
			)

