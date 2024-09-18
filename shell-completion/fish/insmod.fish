# insmod(8) completion                                      -*- shell-script -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>

# globally disable file completions
complete -c insmod -f

complete -c insmod -s f -l force   -d "DANGEROUS: forces a module load, may cause data corruption and crash your machine"
complete -c insmod -s s -l syslog  -d 'print to syslog, not stderr'
complete -c insmod -s v -l verbose -d 'enables more messages'
complete -c insmod -s V -l version -d 'show version'
complete -c insmod -s h -l help    -d 'show this help'

# provide an exclusive (x) list of required (r) answers (a), keeping (k) the
# matches at the top.
# TODO: match only one file
# BUG: fish lists everything, even when only the given suffix is requested. Plus
# we need to explicitly keep them sorted.
complete -c insmod -x -ra "(__fish_complete_suffix .ko .ko.gz .ko.xz .ko.zst)" -k
# TODO: add module parameter completion, based of modinfo
