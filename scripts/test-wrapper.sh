#!/bin/bash

set -euo pipefail

if [[ ${ASAN_OPTIONS-} || ${UBSAN_OPTIONS-} || ${MSAN_OPTIONS-} ]]; then
    ASAN_OPTIONS=verify_asan_link_order=0:halt_on_error=1:abort_on_error=1:print_summary=1
    export ASAN_OPTIONS
fi

exec "$@"
