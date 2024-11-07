scdoc = find_program('scdoc')

_man_pages = [
  ['5', 'depmod.d'],
  ['5', 'modprobe.d'],
  ['5', 'modules.dep'],
  ['8', 'depmod'],
  ['8', 'insmod'],
  ['8', 'kmod'],
  ['8', 'lsmod'],
  ['8', 'modinfo'],
  ['8', 'modprobe'],
  ['8', 'rmmod'],
]
foreach tuple : _man_pages
  section = tuple[0]
  man = tuple[1]

  custom_target(
    'man_@0@_@1@'.format(section, man),
    command : [
      build_scdoc,
      scdoc,
      '@INPUT@',
      's|@SYSCONFDIR@|@0@|g;'.format(sysconfdir) +
      's|@DISTCONFDIR@|@0@|g;'.format(distconfdir) +
      's|@MODULE_DIRECTORY@|@0@|g;'.format(moduledir),
    ],
    input : '@0@.@1@.scd'.format(man, section),
    output : '@0@.@1@'.format(man, section),
    capture : true,
    build_by_default : get_option('manpages'),
    install : true,
    install_dir : get_option('mandir') / 'man@0@'.format(section)
  )
endforeach

_man_aliases = [
  ['5', 'modprobe.conf', 'modprobe.d.5'],
  ['5', 'modules.dep.bin', 'modules.dep.5'],
]

foreach tuple : _man_aliases
  section = tuple[0]
  man = tuple[1]
  dest = tuple[2]

  custom_target(
    'man_@0@_@1@'.format(section, man),
    command : ['echo', '.so @0@'.format(dest)],
    output : '@0@.@1@'.format(man, section),
    capture : true,
    build_by_default : get_option('manpages'),
    install : true,
    install_dir : get_option('mandir') / 'man@0@'.format(section)
  )
endforeach
