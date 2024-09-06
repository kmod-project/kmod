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
    '@0@/libkmod/libkmod-index.h'.format(meson.project_source_root()),
    '@0@/libkmod/libkmod-internal-file.h'.format(meson.project_source_root()),
    '@0@/libkmod/libkmod-internal.h'.format(meson.project_source_root()),
  ],
  scan_args : '--ignore-decorators="KMOD_EXPORT"',
  src_dir : '@0@/libkmod/'.format(meson.project_source_root()),
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
