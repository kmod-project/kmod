if not get_option('tools')
  error('The test suite requires also building the tools')
endif

build_module_playground = custom_target(
  'build-module-playground',
  command : [
    setup_modules,
    meson.project_source_root(),
    meson.project_build_root(),
    'testsuite/module-playground', # do not prepend source/build root
  ],
  output : 'bb-rootfs',
  console : true,
  build_always_stale : true, # TODO: only when the playground has changed
  build_by_default : false,
)

create_rootfs = custom_target(
  'create-rootfs',
  command : [
    setup_rootfs,
    join_paths(meson.project_source_root(), 'testsuite/rootfs-pristine'),
    join_paths(meson.project_build_root(), 'testsuite/rootfs'),
    join_paths(meson.project_build_root(), 'testsuite/module-playground'),
    join_paths(meson.project_build_root(), 'config.h'),
    sysconfdir,
    moduledir,
  ],
  output : 'stamp-rootfs',
  console : true,
  depends : build_module_playground,
  build_always_stale : true, # TODO: only when the rootfs has changed
  build_by_default : false,
)

_test_override_mods = [
  'delete_module',
  'init_module',
  'path',
  'uname',
]

libdl = cc.find_library('dl')
test_override_mods = []

foreach mod : _test_override_mods
  test_override_mods += shared_module(
    mod,
    files('@0@.c'.format(mod)),
    include_directories : top_include,
    dependencies : libdl,
    link_with : [libshared, libkmod_internal],
    gnu_symbol_visibility : 'hidden',
    name_prefix : '',
    build_rpath : '',
    build_by_default : false,
  )
endforeach

testsuite_c_args = [
  '-DTESTSUITE_ROOTFS="@0@/testsuite/rootfs/"'.format(meson.project_build_root()),
  '-DTOOLS_DIR="@0@/"'.format(meson.project_build_root()),
  '-DOVERRIDE_LIBDIR="@0@/testsuite/"'.format(meson.project_build_root()),
]

libtestsuite = static_library(
  'testsuite',
  files('testsuite.c'),
  include_directories : top_include,
  c_args : testsuite_c_args,
  dependencies : cc.find_library('rt'),
  build_by_default : false,
  install : false,
)

_testsuite = [
  'test-array',
  'test-blacklist',
  'test-dependencies',
  'test-depmod',
  'test-hash',
  'test-init',
  'test-initstate',
  'test-list',
  'test-loaded',
  'test-modinfo',
  'test-modprobe',
  'test-new-module',
  'test-scratchbuf',
  'test-strbuf',
  'test-testsuite',
  'test-util',
  'test-weakdep',
]

foreach input : _testsuite
  test(
    input,
    executable(
      input,
      files('@0@.c'.format(input)),
      include_directories : top_include,
      c_args : testsuite_c_args,
      link_with : [libshared, libkmod_internal, libtestsuite],
      build_by_default : false,
    ),
    depends : [internal_kmod_symlinks, create_rootfs, test_override_mods],
  )
endforeach
