# NOTE this metadata DUPLICATED in 'default.nix'. We must MANUALLY sync both because:
#   - The Meson build setup must stand alone adn *not* depend on Nix
#   - Sirio sees no way to have Nix parse these variables when building the derivation
project('xdpacket', 'c',
  version : '0.1.0',
  license : 'GPL2',
  default_options : [
    'b_coverage=true',
    'b_lto=true',
    'c_std=gnu11'
    ])
description = 'the eXtremely Direct Packet handler'
homepage = 'https://siriobalmelli.github.io/xdpacket/'


# _all_ variables visible in gdb and core dumps
if get_option('buildtype') == 'debug'
  add_project_arguments(
    ['-DDEBUG', '-O0'],
    language : 'c'
    )

elif get_option('buildtype') == 'debugoptimized'
  add_project_arguments(
    ['-march=native', '-mtune=native'],
    language : 'c'
    )

elif get_option('buildtype') == 'release'
  add_project_arguments(
    ['-DNDEBUG', '-march=native', '-mtune=native', '-Ofast'],
    language : 'c'
    )

# use this to cross check the effect of flags on the other build types.
elif get_option('buildtype') == 'plain'
  add_project_arguments(
    ['-DNDEBUG', '-march=core2'],
    language : 'c'
    )

endif


nonlibc = dependency('nonlibc')
judy = dependency('Judy')
yaml = dependency('yaml-0.1')
# All deps in a single arg. Use THIS ONE in compile calls
deps = [nonlibc, judy, yaml]


#build
inc = include_directories('include')
subdir('src')
subdir('test')
#inc_old = include_directories('include_old')
#subdir('src_old')
#subdir('test_old')
