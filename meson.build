project(
  'kmod',
  'c',
  version : '33',
  license : ['LGPLv2.1', 'GPL-2.0-or-later'],
  meson_version : '>=0.60.0',
  default_options : [
    'c_std=gnu11',
    'b_pie=true',
    'warning_level=2',
    'prefix=/usr',
    'sysconfdir=/etc',
    'localstatedir=/var',
  ]
)

cdata = configuration_data()
cdata.set_quoted('PACKAGE', meson.project_name())
cdata.set_quoted('VERSION', meson.project_version())

cdata.set10('ENABLE_LOGGING', get_option('logging'))
cdata.set10('ENABLE_DEBUG', get_option('debug-messages'))

pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# We rely on the glibc variant of basename, et al.
cdata.set10('_GNU_SOURCE', true)

# TODO: Once meson-only, adjust all #ifdef X to if X and convert .set to .set10

################################################################################
# Function and structure checks
################################################################################

_funcs = [
  'open64', 'stat64', 'fopen64', '__stat64_time64',
  'secure_getenv',
]
foreach func : _funcs
  if cc.has_function(func, args : '-D_GNU_SOURCE')
    cdata.set('HAVE_@0@'.format(func.to_upper()), true)
  endif
endforeach

# Meson has some amount of support for finding builtins by passing the symbol
# name without the "__builtin_" prefix to cc.has_function(). In practice, it
# doesn't seem to detect everything we need.
_builtins = [
  ['__builtin_clz', '0', true],
  ['__builtin_types_compatible_p', 'int, int', true],
  ['__builtin_uaddl_overflow', '0UL, 0UL, (void*)0', false],
  ['__builtin_uaddll_overflow', '0ULL, 0ULL, (void*)0', false],
]
foreach tuple : _builtins
  builtin = tuple[0]
  args = tuple[1]
  required = tuple[2]
  # XXX: meson 1.5.0 has links(... required ) flag
  have = cc.links('int main(void){@0@(@1@);return 0;}'.format(builtin, args))
  if required and not have
    error('required builtin function not found: @0@'.format(builtin))
  endif

  cdata.set10('HAVE_@0@'.format(builtin.to_upper()), have)
endforeach

# Check for __GLIBC__ as short-cut for the LFS64 wrappers
glibc = cc.has_header_symbol('features.h', '__GLIBC__')
# XXX: once meson only, rename the define to something more sensible ...
# NEED_64_WRAPPERS perhaps?
cdata.set10('HAVE_DECL___GLIBC__', glibc)

# dietlibc doesn't have st.st_mtim struct member
# XXX: we use both st_mtim and st_mtime ... unify and test
foreach tuple : [['struct stat', 'st_mtim', '#include <sys/stat.h>']]
  struct = tuple[0]
  member = tuple[1]
  prefix = tuple[2]
  if cc.has_member(struct, member, prefix : prefix, args : '-D_GNU_SOURCE')
    cdata.set('HAVE_@0@_@1@'.format(struct.underscorify().to_upper(),
                                    member.to_upper()), true)
  endif
endforeach

# basename may be only available in libgen.h with the POSIX behavior,
# not desired here
_decls = [
  ['basename', 'string.h'],
  ['__xstat', 'sys/stat.h'],
]
foreach tuple : _decls
  decl = tuple[0]
  header = tuple[1]
  have  = cc.has_header_symbol(header, decl, args : '-D_GNU_SOURCE')
  cdata.set10('HAVE_DECL_@0@'.format(decl.to_upper()), have)
endforeach

if cc.compiles('_Static_assert(1, "Test");', name : '_Static_assert')
  cdata.set('HAVE_STATIC_ASSERT', true)
endif

if cc.compiles('''
    #include <stdlib.h>
    _Noreturn int foo(void) { exit(0); }
    ''',
    name : '_Noreturn')
  cdata.set('HAVE_NORETURN', true)
endif

################################################################################
# Default CFLAGS and LDFLAGS
################################################################################

add_project_arguments(
  cc.get_supported_arguments([
    '-Wno-inline',
    '-Wvla',
    '-Wundef',
    '-Wformat=2',
    '-Wlogical-op',
    '-Wsign-compare',
    '-Wformat-security',
    '-Wmissing-include-dirs',
    '-Wformat-nonliteral',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wdeclaration-after-statement',
    '-Wfloat-equal',
    '-Wmissing-prototypes',
    '-Wstrict-prototypes',
    '-Wredundant-decls',
    '-Wmissing-declarations',
    '-Wmissing-noreturn',
    '-Wshadow',
    '-Wendif-labels',
    '-Wstrict-aliasing=3',
    '-Wwrite-strings',
    '-Wno-long-long',
    '-Wno-overlength-strings',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wnested-externs',
    '-Wchar-subscripts',
    '-Wtype-limits',
    '-Wuninitialized',
    '-fno-common',
    '-fdiagnostics-show-option',
    '-ffunction-sections',
    '-fdata-sections',
  ]),
  language : 'c'
)

add_project_link_arguments(
  cc.get_supported_link_arguments([
    '-Wl,--gc-sections',
  ]),
  language : 'c'
)

################################################################################
# Options
################################################################################

module_compressions = ''
module_signatures = ''

features = []

#-------------------------------------------------------------------------------
# Directories
#-------------------------------------------------------------------------------

sysconfdir = get_option('sysconfdir')
cdata.set_quoted('SYSCONFDIR', sysconfdir)

libdir = join_paths(get_option('prefix'), get_option('libdir'))

distconfdir = get_option('distconfdir')
if distconfdir == ''
  distconfdir = libdir
endif
cdata.set_quoted('DISTCONFDIR', distconfdir)

# The default moduledir is hard-coded due to historical reasons
moduledir = get_option('moduledir')
if moduledir == ''
  moduledir = '/lib/modules'
endif
cdata.set_quoted('MODULE_DIRECTORY', moduledir)

_completiondirs = [
  ['bashcompletiondir', 'bash-completion', 'bash-completion/completions', 'shell-completion/bash/@0@'],
  ['fishcompletiondir', 'fish',            'fish/vendor_functions.d',     'shell-completion/fish/@0@.fish'],
  ['zshcompletiondir',  '',                'zsh/site-functions',          'shell-completion/zsh/_@0@'],
]

foreach tuple : _completiondirs
  dir_option = tuple[0]
  pkg_dep = tuple[1]
  def_path = tuple[2]
  ins_path = tuple[3]

  completiondir = get_option(dir_option)
  if completiondir == ''
    completion = dependency(pkg_dep, required : false)
    if completion.found()
      completiondir = completion.get_variable(pkgconfig : 'completionsdir')
    else
      completiondir = join_paths(get_option('prefix'), get_option('datadir'),
                                 def_path)
    endif
  endif

  _completions = [
    'insmod',
    'lsmod',
    'rmmod',
  ]

  if completiondir != 'no'
    foreach comp : _completions
      install_data(
        files(ins_path.format(comp)),
        install_dir : completiondir,
      )
    endforeach
  endif

  # NEEDED solely for bash/kmod below
  set_variable(dir_option, completiondir)
endforeach

if bashcompletiondir != 'no'
  install_data(
    files('shell-completion/bash/kmod'),
    install_dir : bashcompletiondir,
  )
endif

#-------------------------------------------------------------------------------
# Compression support
#-------------------------------------------------------------------------------

zstd = dependency('libzstd', version : '>= 1.4.4', required : get_option('zstd'))
if zstd.found()
  cdata.set('ENABLE_ZSTD', true)
  module_compressions += 'zstd '
endif
features += ['@0@ZSTD'.format(zstd.found() ? '+' : '-')]

xz = dependency('liblzma', version : '>= 4.99', required : get_option('xz'))
if xz.found()
  cdata.set('ENABLE_XZ', true)
  module_compressions += 'xz '
endif
features += ['@0@XZ'.format(xz.found() ? '+' : '-')]

zlib = dependency('zlib', required : get_option('zlib'))
if zlib.found()
  cdata.set('ENABLE_ZLIB', true)
  module_compressions += 'zlib '
endif
features += ['@0@ZLIB'.format(zlib.found() ? '+' : '-')]

#-------------------------------------------------------------------------------
# Signed modules
#-------------------------------------------------------------------------------

crypto = dependency('libcrypto', version : '>= 1.1.0', required : get_option('openssl'))
if crypto.found()
  cdata.set('ENABLE_OPENSSL', true)
  module_signatures = 'PKCS7 legacy'
else
  module_signatures = 'legacy'
endif
features += ['@0@LIBCRYPTO'.format(crypto.found() ? '+' : '-')]

cdata.set_quoted('KMOD_FEATURES', ' '.join(features))

config_h = configure_file(
  output : 'config.h',
  configuration : cdata
)

add_project_arguments('-include', 'config.h', language : 'c')

################################################################################
# libraries and binaries
################################################################################

libshared = static_library(
  'shared',
  files(
    'shared/array.c',
    'shared/array.h',
    'shared/hash.c',
    'shared/hash.h',
    'shared/macro.h',
    'shared/missing.h',
    'shared/scratchbuf.c',
    'shared/scratchbuf.h',
    'shared/strbuf.c',
    'shared/strbuf.h',
    'shared/util.c',
    'shared/util.h',
  ),
  gnu_symbol_visibility : 'hidden',
  install : false,
)

libkmod_files = files(
  'libkmod/libkmod-builtin.c',
  'libkmod/libkmod.c',
  'libkmod/libkmod-config.c',
  'libkmod/libkmod-elf.c',
  'libkmod/libkmod-file.c',
  'libkmod/libkmod.h',
  'libkmod/libkmod-index.c',
  'libkmod/libkmod-index.h',
  'libkmod/libkmod-internal-file.h',
  'libkmod/libkmod-internal.h',
  'libkmod/libkmod-list.c',
  'libkmod/libkmod-module.c',
  'libkmod/libkmod-signature.c',
)

libkmod_deps = []

if zstd.found()
  libkmod_files += files('libkmod/libkmod-file-zstd.c')
  libkmod_deps += zstd
endif

if xz.found()
  libkmod_files += files('libkmod/libkmod-file-xz.c')
  libkmod_deps += xz
endif

if zlib.found()
  libkmod_files += files('libkmod/libkmod-file-zlib.c')
  libkmod_deps += zlib
endif

if crypto.found()
  libkmod_deps += crypto
endif

install_headers('libkmod/libkmod.h')

libkmod = shared_library(
  'kmod',
  libkmod_files,
  dependencies : libkmod_deps,
  link_with : libshared,
  link_args : ['-Wl,--version-script', join_paths(meson.current_source_dir(),
                                                  'libkmod/libkmod.sym')],
  link_depends : files('libkmod/libkmod.sym'),
  gnu_symbol_visibility : 'hidden',
  version : '2.5.0',
  install : true,
)

pkg.generate(
  name : 'libkmod',
  description : 'Library to deal with kernel modules',
  libraries : libkmod,
  requires_private : libkmod_deps,
)

libkmod_internal = static_library(
  'kmod-internal',
  objects : libkmod.extract_all_objects(recursive : true),
  dependencies : libkmod_deps,
  install : false,
)

kmod_sources = files(
  'tools/depmod.c',
  'tools/insmod.c',
  'tools/kmod.c',
  'tools/kmod.h',
  'tools/log.c',
  'tools/log.h',
  'tools/lsmod.c',
  'tools/modinfo.c',
  'tools/modprobe.c',
  'tools/rmmod.c',
  'tools/static-nodes.c',
)

kmod = executable(
  'kmod',
  kmod_sources,
  link_with : [libshared, libkmod_internal],
  gnu_symbol_visibility : 'hidden',
  build_by_default : get_option('tools'),
  install : get_option('tools'),
)

_kmod_variables = [
  'sysconfdir=' + sysconfdir,
  'distconfdir=' + distconfdir,
  'module_directory=' + moduledir,
  'module_signatures=' + module_signatures,
]

# XXX: Support for empty variables was added in meson v1.4.0.
# pkgconf behaves identically on missing and empty variable.
if module_compressions != ''
  _kmod_variables += ['module_compressions=' + module_compressions]
endif

pkg.generate(
  name : 'kmod',
  description : 'Tools to deal with kernel modules',
  install_dir : join_paths(get_option('datadir'), 'pkgconfig'),
  variables : _kmod_variables,
)

_tools = [
  'depmod',
  'insmod',
  'lsmod',
  'modinfo',
  'modprobe',
  'rmmod',
]

kmod_symlink = find_program('scripts/kmod-symlink.sh')
foreach tool : _tools
  if get_option('tools')
    symlink = join_paths(get_option('bindir'), tool)
    meson.add_install_script(kmod_symlink, symlink)
  endif
endforeach

internal_kmod_symlinks = []

foreach tool : _tools
  internal_kmod_symlinks += custom_target(
    tool,
    command : ['ln', '-sf', kmod, '@OUTPUT@'],
    output : tool,
    depends : kmod,
    build_by_default : true,
  )
endforeach

# ------------------------------------------------------------------------------
# TESTSUITE
# ------------------------------------------------------------------------------

if get_option('build-tests')
  setup_modules = find_program('scripts/setup-modules.sh')
  setup_rootfs = find_program('scripts/setup-rootfs.sh')
  top_include = include_directories('.')
  subdir('testsuite')
endif

# ------------------------------------------------------------------
# documentation
# ------------------------------------------------------------------

if get_option('manpages')
  build_scdoc = find_program('scripts/build-scdoc.sh')
  subdir('man')
endif

if get_option('docs')
  test_gtkdoc = find_program('scripts/test-gtkdoc.sh')
  subdir('libkmod/docs')
endif

summary({
  'moduledir'   : moduledir,
  'prefix'      : get_option('prefix'),
  'sysconfdir'  : sysconfdir,
  'distconfdir' : distconfdir,
  'libdir'      : libdir,
  'includedir'  : join_paths(get_option('prefix'), get_option('includedir')),
  'bindir'      : join_paths(get_option('prefix'), get_option('bindir')),
}, section : 'Directories')

summary({
  'tools'           : get_option('tools'),
  'logging'         : get_option('logging'),
  'debug-messages'  : get_option('debug-messages'),
  'build-tests'     : get_option('build-tests'),
  'manpages'        : get_option('manpages'),
  'docs'            : get_option('docs'),
}, section : 'Options')

summary({
  'features'    : ' '.join(features)
}, section : '')
