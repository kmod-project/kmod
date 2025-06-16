project(
  'kmod',
  'c',
  version : '34',
  license : ['LGPLv2.1', 'GPL-2.0-or-later'],
  meson_version : '>=0.61.0',
  default_options : [
    'c_std=gnu11',
    'b_pie=true',
    'warning_level=2',
    'prefix=/usr',
    'sysconfdir=/etc',
  ]
)

cdata = configuration_data()
cdata.set_quoted('PACKAGE', meson.project_name())
cdata.set_quoted('VERSION', meson.project_version())

cdata.set10('ENABLE_LOGGING', get_option('logging'))
cdata.set10('ENABLE_DEBUG', get_option('debug-messages'))
cdata.set10('ENABLE_ELFDBG', false)

pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# We rely on the glibc variant of basename, et al.
cdata.set10('_GNU_SOURCE', true)

################################################################################
# Function and structure checks
################################################################################

_funcs = [
  'open64', 'stat64', 'fopen64', '__stat64_time64',
  'secure_getenv',
]
foreach func : _funcs
  cdata.set10('HAVE_@0@'.format(func.to_upper()), cc.has_function(func, args : '-D_GNU_SOURCE'))
endforeach

# Meson has some amount of support for finding builtins by passing the symbol
# name without the "__builtin_" prefix to cc.has_function(). In practice, it
# doesn't seem to detect everything we need.
_builtins = [
  ['__builtin_clz', '0', true],
  ['__builtin_types_compatible_p', 'int, int', true],
  ['__builtin_uadd_overflow', '0U, 0U, (void*)0', false],
  ['__builtin_uaddl_overflow', '0UL, 0UL, (void*)0', false],
  ['__builtin_uaddll_overflow', '0ULL, 0ULL, (void*)0', false],
  ['__builtin_umul_overflow', '0U, 0U, (void*)0', false],
  ['__builtin_umull_overflow', '0UL, 0UL, (void*)0', false],
  ['__builtin_umulll_overflow', '0ULL, 0ULL, (void*)0', false],
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

cdata.set10('HAVE_STATIC_ASSERT', cc.compiles('_Static_assert(1, "Test");', name : '_Static_assert'))
cdata.set10('HAVE_NORETURN', cc.compiles('#include <stdlib.h>; _Noreturn int foo(void) { exit(0); }', name : '_Noreturn'))

################################################################################
# Default CFLAGS and LDFLAGS
################################################################################

add_project_arguments(
  cc.get_supported_arguments([
    '-fdata-sections',
    '-fdiagnostics-show-option',
    '-ffunction-sections',
    '-fno-common',
    '-Wchar-subscripts',
    '-Wendif-labels',
    '-Wfloat-equal',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Winit-self',
    '-Wlogical-op',
    '-Wmissing-declarations',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-attributes',
    '-Wno-declaration-after-statement',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wsign-compare',
    '-Wstrict-aliasing=3',
    '-Wstrict-prototypes',
    '-Wtype-limits',
    '-Wundef',
    '-Wuninitialized',
    '-Wvla',
    '-Wwrite-strings',
  ]),
  language : 'c'
)

add_project_link_arguments(
  cc.get_supported_link_arguments([
    '-Wl,--gc-sections',
  ]),
  language : 'c'
)

# Clang as of v18, relies on statically linking the sanitizers. This causes two
# distinct issues:
#  - the shared library is underlinked, so the build fails
#  - the modules (that we dlopen/ld_preload) are underlinked so the tests fail
#
# Force shared libasan (GCC defaults to shared and this toggle doesn't exist),
# which combined with the LD_PRELOAD in our wrapper makes everything happy.
if get_option('b_sanitize') != 'none' and cc.get_id() == 'clang'
  add_project_arguments('-shared-libasan', language : 'c')
  add_project_link_arguments('-shared-libasan', language : 'c')
endif
################################################################################
# Options
################################################################################

module_compressions = ''
module_signatures = ''

features = []
dep_map = {}

# keep in sync with meson_options.txt
dlopen_all = get_option('dlopen').contains('all')

#-------------------------------------------------------------------------------
# Directories
#-------------------------------------------------------------------------------

prefixdir = get_option('prefix')
sysconfdir = get_option('sysconfdir')
bindir = prefixdir / get_option('bindir')
sbindir = prefixdir / get_option('sbindir')
includedir = prefixdir / get_option('includedir')
libdir = prefixdir / get_option('libdir')
datadir = prefixdir / get_option('datadir')

distconfdir = get_option('distconfdir')
moduledir = get_option('moduledir')

bashcompletiondir = get_option('bashcompletiondir')
fishcompletiondir = get_option('fishcompletiondir')
zshcompletiondir = get_option('zshcompletiondir')

cdata.set_quoted('SYSCONFDIR', sysconfdir)

_customdirs = [
  # The defaults are hard-coded due to historical reasons
  ['distconfdir', prefixdir / 'lib',  'DISTCONFDIR'],
  ['moduledir',   '/lib/modules',     'MODULE_DIRECTORY'],
]

foreach tuple : _customdirs
  dir_option = tuple[0]
  def_path = tuple[1]
  quoted = tuple[2]

  customdir = get_variable(dir_option)
  if customdir == ''
    customdir = def_path
  else
    if not customdir.startswith('/')
      error('User provided @0@, \'@1@\' is not an absolute path.'
          .format(dir_option, customdir))
    endif
    # Strip all leading/trailing and re-add only the leading one.
    customdir = '/' / customdir.strip('/')
  endif
  cdata.set_quoted(quoted, customdir)
  set_variable(dir_option, customdir)
endforeach

foreach confdir : [sysconfdir, distconfdir]
  install_emptydir(confdir / 'depmod.d')
  install_emptydir(confdir / 'modprobe.d')
endforeach

_completiondirs = [
  ['bash', 'bash-completion', 'bash-completion' / 'completions', '@0@'],
  ['fish', 'fish',            'fish' / 'vendor_functions.d',     '@0@.fish'],
  ['zsh',  '',                'zsh' / 'site-functions',          '_@0@'],
]

foreach tuple : _completiondirs
  dir_option = tuple[0] + 'completiondir'
  pkg_dep = tuple[1]
  def_path = tuple[2]
  ins_name = tuple[3]

  completiondir = get_variable(dir_option)
  if completiondir == ''
    completion = dependency(pkg_dep, required : false)
    if completion.found()
      completion_prefix = completion.get_variable(pkgconfig : 'prefix')
      if completion_prefix != prefixdir
        warning('User provided prefix \'@0@\' differs from @1@ one \'@2@\'.'
                .format(prefixdir, pkg_dep, completion_prefix))
        warning('Not installing completion. To re-enable, manually set @0@.'
                .format(dir_option))
        completiondir = 'no'
      else
        completiondir = completion.get_variable(pkgconfig : 'completionsdir')
      endif
    else
      completiondir = datadir / def_path
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
        files('shell-completion' / tuple[0] / ins_name.format(comp)),
        install_dir : completiondir,
      )
    endforeach
  endif

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

_compression = [
  ['zstd', 'libzstd', '>= 1.4.4'],
  ['xz',   'liblzma', '>= 4.99'],
  ['zlib', 'zlib',    '>= 0'],
]

foreach tuple : _compression
  opt = tuple[0]
  pkg_dep = tuple[1]
  pkg_dep_version = tuple[2]

  dlopen = dlopen_all or get_option('dlopen').contains(opt)
  if not dlopen_all and dlopen and get_option(opt).disabled()
    error('Incompatiable options: dlopen=@0@ for disabled @0@'.format(opt))
  endif

  dep = dependency(pkg_dep, version : pkg_dep_version, required : get_option(opt))
  have = dep.found()
  if have and dlopen
    dep = dep.partial_dependency(compile_args : true, includes : true)
  endif

  cdata.set10('ENABLE_' + opt.to_upper(), have)
  cdata.set10('ENABLE_' + opt.to_upper() + '_DLOPEN', have and dlopen)

  if have
    module_compressions += '@0@ '.format(opt)
  endif

  features += ['@0@@1@'.format(have ? '+' : '-', opt.to_upper())]
  dep_map += {opt : dep}
endforeach

#-------------------------------------------------------------------------------
# Signed modules
#-------------------------------------------------------------------------------

opt = 'openssl'
dep = dependency('libcrypto', version : '>= 1.1.0', required : get_option(opt))
have = dep.found()

if have
  module_signatures = 'PKCS7 legacy'
else
  module_signatures = 'legacy'
endif
cdata.set10('ENABLE_' + opt.to_upper(), have)
features += ['@0@@1@'.format(have ? '+' : '-', opt.to_upper())]
dep_map += {opt : dep}

#-------------------------------------------------------------------------------
# Config output
#-------------------------------------------------------------------------------

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
    'shared/elf-note.h',
    'shared/hash.c',
    'shared/hash.h',
    'shared/macro.h',
    'shared/missing.h',
    'shared/strbuf.c',
    'shared/strbuf.h',
    'shared/util.c',
    'shared/util.h',
    'shared/tmpfile-util.c',
    'shared/tmpfile-util.h',
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
cdeps = []

if not cc.has_function('dlopen')
  cdeps += cc.find_library('dl', required : true)
endif

if dep_map.get('zstd').found()
  libkmod_files += files('libkmod/libkmod-file-zstd.c')
  libkmod_deps += dep_map['zstd']
endif

if dep_map.get('xz').found()
  libkmod_files += files('libkmod/libkmod-file-xz.c')
  libkmod_deps += dep_map['xz']
endif

if dep_map.get('zlib').found()
  libkmod_files += files('libkmod/libkmod-file-zlib.c')
  libkmod_deps += dep_map['zlib']
endif

if dep_map.get('openssl').found()
  libkmod_deps += dep_map['openssl']
endif

install_headers('libkmod/libkmod.h')

libkmod = shared_library(
  'kmod',
  libkmod_files,
  dependencies : libkmod_deps + cdeps,
  link_with : libshared,
  link_args : ['-Wl,--version-script', meson.current_source_dir() /
                                                  'libkmod/libkmod.sym'],
  link_depends : files('libkmod/libkmod.sym'),
  gnu_symbol_visibility : 'hidden',
  version : '2.5.1',
  install : true,
)

pkg.generate(
  name : 'libkmod',
  description : 'Library to deal with kernel modules',
  libraries : libkmod,
  requires_private : libkmod_deps,
  libraries_private : cdeps,
)

libkmod_internal = static_library(
  'kmod-internal',
  objects : libkmod.extract_all_objects(recursive : true),
  dependencies : libkmod_deps + cdeps,
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
  'tools/opt.c',
  'tools/opt.h',
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
]

# Don't (space) escape variables with space-separated lists, for consistency
# with the autotools build.
_kmod_unescaped_variables = [
  'module_signatures=' + module_signatures,
]

# XXX: Support for empty variables was added in meson v1.4.0.
# pkgconf behaves identically on missing and empty variable.
if module_compressions != ''
  _kmod_unescaped_variables += ['module_compressions=' + module_compressions]
endif

pkg.generate(
  name : 'kmod',
  description : 'Tools to deal with kernel modules',
  install_dir : datadir / 'pkgconfig',
  unescaped_variables : _kmod_unescaped_variables,
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

if get_option('tools')
  mkdir_p = 'mkdir -p "$DESTDIR@0@"'
  meson.add_install_script('sh', '-c', mkdir_p.format(sbindir))

  ln_s = 'ln -sfr "$DESTDIR@0@/kmod" "$DESTDIR@1@"'
  foreach tool : _tools
    meson.add_install_script('sh', '-c', ln_s.format(bindir, sbindir / tool))
  endforeach
endif

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
  bash = find_program('bash')
  sanitizer_env = find_program('scripts/sanitizer-env.sh')
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

# ------------------------------------------------------------------
# summary
# ------------------------------------------------------------------

summary({
  'prefix'      : prefixdir,
  'sysconfdir'  : sysconfdir,
  'bindir'      : bindir,
  'sbindir'     : sbindir,
  'includedir'  : includedir,
  'libdir'      : libdir,
  'datadir'     : datadir,
}, section : 'Directories')

summary({
  'distconfdir' : distconfdir,
  'moduledir'   : moduledir,
}, section : 'Kmod specific')

summary({
  'bashcompletiondir' : bashcompletiondir,
  'fishcompletiondir' : fishcompletiondir,
  'zshcompletiondir'  : zshcompletiondir,
}, section : 'Shell completions')

summary({
  'tools'           : get_option('tools'),
  'logging'         : get_option('logging'),
  'debug-messages'  : get_option('debug-messages'),
  'build-tests'     : get_option('build-tests'),
  'manpages'        : get_option('manpages'),
  'docs'            : get_option('docs'),
  'dlopen'          : get_option('dlopen'),
}, section : 'Options')

summary({
  'features'    : ' '.join(features)
}, section : '')

summary({
  'Compiler'        : cc.get_id() + ' (' + cc.version() + ')',
  'Linker'          : cc.get_linker_id(),
}, section : 'Build tools')
