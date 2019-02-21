# NOTE this metadata DUPLICATED in 'default.nix'. We must MANUALLY sync both because:
#   - The Meson build setup must stand alone adn *not* depend on Nix
#   - Sirio sees no way to have Nix parse these variables when building the derivation
project('xdpacket', 'c',
  version : '0.2.0',
  license : 'GPL2',
  default_options : [
    'b_coverage=true',
    'b_lto=true',
    'c_std=gnu11'
    ])
description = 'the eXtremely Direct Packet handler'
homepage = 'https://siriobalmelli.github.io/xdpacket/'


# whole-project build flags (most of these are hardening flags)
add_project_arguments([
    '-D_FORTIFY_SOURCE=2',
    '-fasynchronous-unwind-tables',
    '-fexceptions',
    '-fstack-protector-strong',
    '-pipe',
    '-Wall',
    '-Werror=format-security',
    '-Werror=implicit-function-declaration'
    ],
  language : 'c')

# _all_ variables visible in gdb and core dumps
if get_option('buildtype') == 'debug'
  add_project_arguments([
    '-DDEBUG',
    '-fno-omit-frame-pointer',
    '-grecord-gcc-switches',
    '-O0',
    '-Wno-cpp'
    ],
    language : 'c')

elif get_option('buildtype') == 'debugoptimized'
  add_project_arguments([
    '-fno-omit-frame-pointer',
    '-grecord-gcc-switches',
    '-march=native',
    '-mtune=native',
    '-O2'
    ],
  language : 'c')

elif get_option('buildtype') == 'release'
  add_project_arguments([
    '-DNDEBUG',
    '-march=native',
    '-mtune=native',
    '-Ofast'
    ],
  language : 'c')

# use this to cross check the effect of flags on the other build types.
elif get_option('buildtype') == 'plain'
  add_project_arguments([
    '-DNDEBUG',
    '-march=core2'
    ],
  language : 'c')
endif


nonlibc_dep = dependency('nonlibc', fallback : ['nonlibc', 'nonlibc_dep'])
Judy_dep = dependency('Judy', fallback : ['Judy', 'Judy_dep'])
yaml_dep = dependency('yaml-0.1', fallback : ['yaml', 'yaml_dep'])
# All deps in a single arg. Use THIS ONE in compile calls
deps = [nonlibc_dep, Judy_dep, yaml_dep]


#build
inc = include_directories('include')
subdir('src')
subdir('test')
subdir('man')
#inc_old = include_directories('include_old')
#subdir('src_old')
#subdir('test_old')
