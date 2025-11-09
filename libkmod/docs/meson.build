gnome = import('gnome')

version_file = configure_file(
  input: 'version.xml.in',
  output: 'version.xml',
  configuration: cdata,
)

built_docs = gnome.gtkdoc(
  'libkmod',
  content_files : version_file,
  ignore_headers : [
    f'@project_source_root@/libkmod/libkmod-index.h',
    f'@project_source_root@/libkmod/libkmod-internal-file.h',
    f'@project_source_root@/libkmod/libkmod-internal.h',
  ],
  scan_args : '--ignore-decorators="KMOD_EXPORT"',
  src_dir : f'@project_source_root@/libkmod/',
  namespace : 'kmod',
  module_version : '3',
  main_xml : 'libkmod-docs.xml',
)

test(
  'test-gtkdoc',
  test_gtkdoc,
  args : meson.current_build_dir(),
  depends : built_docs,
)
