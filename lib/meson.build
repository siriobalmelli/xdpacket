lib_files = [
	'hook.c',
    'iface.c',
    'field.c',
    'matcher.c',
    'packet.c',
    'parse.c'
]

internal_static = static_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			dependencies : deps,
			install : true)
