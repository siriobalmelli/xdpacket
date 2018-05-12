lib_files =  [ 'xdpk.c', 'field.c' ]

xdpk = shared_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : true,
			dependencies : deps)
# Make linking work on linux systems without breaking nix
p = get_option('prefix')
if host_machine.system() == 'linux' and not p.startswith('/nix')
	meson.add_install_script('ldconfig', p + '/' + get_option('libdir'))
endif

xdpk_static = static_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : true,
			dependencies : deps)

# don't set anything here, rely on variables declared in toplevel file
pkg = import('pkgconfig')
pkg.generate(libraries : [ xdpk_static, xdpk ],
		name : meson.project_name(),
		version : meson.project_version(),
		filebase : meson.project_name(),
		description : description)

#   Export a static dependency (straightforward)
xdpk_dep_static = declare_dependency(link_with : xdpk_static,
					include_directories : inc,
					dependencies : deps)

#   Export a shared dependency (a royal pain).
#
# Force dependers to have an rpath entry pointing to where we KNOW we'll install.
# This fixes both Nix and non-nix projects (providing 'prefix' is set correctly),
#+  and does OPAQUELY (depender doesn't have to add an 'install_rpath' directive).
rpath = get_option('prefix') + '/' + get_option('libdir')
xdpk_dep_shared = declare_dependency(link_with : xdpk,
					include_directories : inc,
					dependencies : deps,
					link_args : '-Wl,-rpath,' + rpath)
