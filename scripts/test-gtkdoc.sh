#!/bin/bash

set -euo pipefail

err=0

warn() {
    (( ++err ))
    echo
    echo "WARNING: $1."
    echo "See file below for more details."
    echo
    echo "$2:"
    cat "$2"
    echo
}

cd "$1"

grep -q '100%' 'libkmod-undocumented.txt' || warn 'Some APIs are missing documentation' 'libkmod-undocumented.txt'

(( $(wc -l < 'libkmod-unused.txt') == 0)) || warn 'APIs are missing from the libkmod-section.txt index' 'libkmod-unused.txt'

exit "$err"
