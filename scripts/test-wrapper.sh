#!/bin/bash

set -euo pipefail

if [[ ${ASAN_OPTIONS-} || ${UBSAN_OPTIONS-} || ${MSAN_OPTIONS-} ]]; then
    LD_PRELOAD=$(gcc -print-file-name=libasan.so)
    export LD_PRELOAD
fi

exec "$@"
