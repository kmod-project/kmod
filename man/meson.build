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
    f'man_@section@_@man@',
    command : [
      build_scdoc,
      scdoc,
      '@INPUT@',
      's|@SYSCONFDIR@|@0@|g;'.format(sysconfdir) +
      's|@DISTCONFDIR@|@0@|g;'.format(distconfdir) +
      's|@MODULE_DIRECTORY@|@0@|g;'.format(moduledir),
    ],
    input : f'@man@.@section@.scd',
    output : f'@man@.@section@',
    capture : true,
    install : true,
    install_dir : get_option('mandir') / f'man@section@',
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
    f'man_@section@_@man@',
    command : ['echo', f'.so @dest@'],
    output : f'@man@.@section@',
    capture : true,
    install : true,
    install_dir : get_option('mandir') / f'man@section@',
  )
endforeach
