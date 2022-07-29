#! /bin/bash
export TIMEFORMAT="elapsed time: %2Rs (%2U user / %2S system)"
export LC_ALL=C

set -e -E
trap 'echo $ME: error in $BASH_COMMAND >&2; exit 1' ERR

CLEANUP=:
trap 'trap - ERR; eval "$CLEANUP"' 0

ME=${0##*/}
BASEDIR=
OUTPUTDIR=
KVER=
SYMVERS=
FILESYMS=
DEPMOD=
KEEP=
ERR=0
: "${DEPMOD_OPTS:=}"

usage() {
    cat <<EOF
Usage: $ME [OPTION]... [MODULE]...

MODULE is a module basename such as 'e1000' or 'sd_mod'.
If no MODULE arguments are given, they are read from stdin.

Options:
  -d|--depmod=PATH	depmod executable to call (default: autodetected)
  -b|--basedir=DIR	base directory to look for modules, default: /
  -o|--outputdir=PATH	output directory, CONTENTS WILL BE OVERWRITTEN
			default: temporary directory under ${TMPDIR:-/tmp}
  -k|--kver=VERSION	kernel version to run for
			default: autodetected from installed kernel
  -E|--symvers FILE	symvers file for depmod -E (default: autodetected)
  -F|--filesyms FILE	System.map file for depmod -F (default: autodetected)
  -K|--keep			do not clean up results
  -h|--help			print this help
EOF
}

OPTIONS=b:k:E:d:o:F:Kh
LONGOPTS=basedir:,kver:,symvers:,depmod:,outputdir:,filesyms:,keep,help
# shellcheck disable=SC2207
ARGS=$(getopt -o "$OPTIONS" -l "$LONGOPTS" -- "$@")
eval set -- "$ARGS"

while [[ $# -gt 0 ]]; do
    case $1 in
	-d|--depmod)
	    shift
	    DEPMOD=$1
	    ;;
	-b|--basedir)
	    shift
	    BASEDIR=$1
	    ;;
	-k|--kver)
	    shift
	    KVER=$1
	    ;;
	-E|--symvers)
	    shift
	    SYMVERS=$1
	    ;;
	-F|--filesyms)
	    shift
	    FILESYMS=$1
	    ;;
	-o|--outputdir)
	    shift
	    OUTPUTDIR=$1
	    ;;
	-K|--keep)
	    KEEP=y
	    ;;
	-h|--help)
	    usage
	    exit 0
	    ;;
	--)
	    shift
	    break
	    ;;
    esac
    shift
done
MODULES=("$@")

find_glob() {
    local files

    # shellcheck disable=SC2206
    files=($1)
    case "${files[0]}" in
	"$1")
	    ;;
	*)
	    echo "${files[0]}"
	    ;;
    esac
}

find_installed_kernel() {
    local kernel
    kernel=$(find_glob "$BASEDIR/lib/modules/*/modules.dep")
    [[ "$kernel" ]] || {
	echo "$ME: no installed kernel found" >&2
	return
    }
    kernel=${kernel%/*}
    kernel=${kernel##*/}
    echo "$ME: found kernel $kernel" >&2
    echo "$kernel"
}

find_symvers() {
    local symvers

    symvers=$(find_glob "$BASEDIR/boot/symvers-$1*")
    [[ ! "$symvers" ]] || echo "$symvers"
}

find_filesyms() {
    local fsyms

    fsyms=$(find_glob "$BASEDIR/boot/System.map-$1*")
    [[ ! "$fsyms" ]] || echo "$fsyms"
}

[[ ! "$BASEDIR" ]] || BASEDIR=$(cd "$BASEDIR" && echo "$PWD")

[[ "$DEPMOD" ]] || {
    for _x in "${0%/*}/../depmod" "${0%/*}/../tools/depmod" \
		/usr/local/sbin/depmod /sbin/depmod /usr/sbin/depmod; do
	[[ -f "$_x" && -x "$_x" ]] && {
	    DEPMOD=$_x
	    break
	}
    done
}

[[ "$DEPMOD" && -f "$DEPMOD" && -x "$DEPMOD" ]]
echo "$ME: using DEPMOD=$DEPMOD" >&2
if [[ "$OUTPUTDIR" ]]; then
    [[ -d "$OUTPUTDIR" ]] || {
	mkdir -p "$OUTPUTDIR"
	# shellcheck disable=SC2016
	[[ "$KEEP" ]] || CLEANUP='rm -rf "$OUTPUTDIR";'"$CLEANUP"
    }
else
    OUTPUTDIR=$(mktemp -d "${TMPDIR:-/tmp}/$ME.XXXXXX")
    [[ "$OUTPUTDIR" ]]
    # shellcheck disable=SC2016
    [[ "$KEEP" ]] || CLEANUP='rm -rf "$OUTPUTDIR";'"$CLEANUP"
fi

OUTPUTDIR=$(cd "$OUTPUTDIR" && echo "$PWD")

[[ "$KVER" ]] || KVER=$(find_installed_kernel)
[[ "$KVER" && -d "$BASEDIR/lib/modules/$KVER" ]]

[[ "$SYMVERS" ]] || SYMVERS=$(find_symvers "$KVER")
[[ "$FILESYMS" ]] || FILESYMS=$(find_filesyms "$KVER")

[[ ("$SYMVERS" && -f "$SYMVERS") || ("$FILESYMS" && -f "$FILESYMS") ]]

# depmod can't handle compressed symvers
case $SYMVERS in
    *.gz)
	gzip -dc "$SYMVERS" >"${TMPDIR:-/tmp}/symvers-$KVER"
	SYMVERS="${TMPDIR:-/tmp}/symvers-$KVER"
	# shellcheck disable=SC2016
	CLEANUP='rm -f "$SYMVERS";'"$CLEANUP"
	;;
esac

[[ ! "$SYMVERS" ]] || FILESYMS=
echo "$ME: symbols resolved with ${SYMVERS:+-E "$SYMVERS"}${FILESYMS:+-F "$FILESYMS"}" >&2

[[ "${#MODULES[@]}" -gt 0 ]] || {
    echo "$ME: reading modules from stdin ..." >&2
    mapfile -t MODULES < <(sed -E 's/\s+/\n/g')
}
[[ "${#MODULES[@]}" -gt 0 ]]

find_module() {
    local f
    f=$(cd "$2" && find . -name "$1.ko*" -type f -print -quit)
    echo "${f#./}"
}

copy_results_to() {
    rm -rf "${OUTPUTDIR:?}/$1"
    mkdir -p "$OUTPUTDIR/$1"
    cp -a "$WORKDIR/lib/modules/$KVER/modules".* "$OUTPUTDIR/$1"
}

restore_results_from() {
    cp -a "$OUTPUTDIR/$1/"* "$WORKDIR/lib/modules/$KVER/"
}

delete_modules() {
    pushd "$WORKDIR/lib/modules/$KVER" &>/dev/null
    rm -f "${!EXTRAMODS[@]}"
    popd &>/dev/null
}

restore_deleted_modules() {
    pushd "$CLONEDIR/lib/modules/$KVER" &>/dev/null
    echo "${!EXTRAMODS[@]}" | sed -E 's/\s+/\n/g' | \
	cpio -p --link "$WORKDIR/lib/modules/$KVER" 2>/dev/null
    popd &>/dev/null
}

check_results_for() {
    local ref
    ref=${2:-reference}
    if diff -ruq "$OUTPUTDIR/$ref" "$OUTPUTDIR/$1"; then
	echo "$ME: $1: OK" >&2
    else
	echo "$ME: depmod output for in $OUTPUTDIR/$1 differs from $OUTPUTDIR/$ref" >&2
	: $((ERR++))
    fi
}

# MODPATHS holds the full module paths relative to the module directory,
# this is the same format as in modules.dep
# REGEX will be used to grep modules.dep for any of these paths occurring
# in the dependency list of some module (DEPENDS below).
MODPATHS=()
for x in "${!MODULES[@]}"; do
    [[ "$x" ]] || continue
    mp=$(find_module "${MODULES[$x]}" "$BASEDIR/lib/modules/$KVER")
    if [[ "$mp" ]]; then
	MODPATHS["$x"]=$mp
	REGEX="$REGEX|${mp//./\\.}"
    else
	echo "$ME: module ${MODULES[$x]} not found" >&2
    fi
done
REGEX=${REGEX#|}

# First, clone the module directory
CLONEDIR=$OUTPUTDIR/clone
[[ -f "$CLONEDIR/lib/modules/$KVER/modules.dep" ]] || {
    rm -rf "$CLONEDIR/lib/modules"
    mkdir -p "$CLONEDIR/lib/modules"
    cp -r "$BASEDIR/lib/modules/$KVER" "$CLONEDIR/lib/modules"

    echo "$ME: running depmod over entire tree"
    time "$DEPMOD" $DEPMOD_OPTS -b "$CLONEDIR" -e ${SYMVERS:+-E "$SYMVERS"} ${FILESYMS:+-F "$FILESYMS"} "$KVER"
}

# With modules.dep in place, find all modules that depend on MODULES.
mapfile -t DEPENDS < <(sed -nE 's,:.*\<('"$REGEX"')\>.*$,,p' "$CLONEDIR/lib/modules/$KVER/modules.dep")

# EXTRAMODS holds the paths for MODULES and all their dependencies
# use associative array to avoid duplicates
declare -A EXTRAMODS
for x in "${MODPATHS[@]}" "${DEPENDS[@]}"; do
    EXTRAMODS["$x"]=1
done

# Copy clone work dir and save the depmod results in "reference" dir
WORKDIR=$OUTPUTDIR/work
rm -rf "$WORKDIR/lib/modules"
mkdir -p "$WORKDIR/lib/modules"
cp -rl "$CLONEDIR/lib/modules/$KVER" "$WORKDIR/lib/modules/$KVER"
copy_results_to reference

# Delete EXTRAMODS, and run depmod again (pretend system state before adding EXTRAMODS)
# Save results in "orig"
delete_modules
echo "$ME: running full depmod after module removal"
time "$DEPMOD" $DEPMOD_OPTS -b "$WORKDIR" -e ${SYMVERS:+-E "$SYMVERS"} ${FILESYMS:+-F "$FILESYMS"} "$KVER"
copy_results_to orig

# 1. Restore deleted modules, and run depmod --incremental
# Save results in "incremental", and compare to "reference"
restore_deleted_modules
echo "$ME: running depmod --incremental (${#EXTRAMODS[@]} modules added)"
time "$DEPMOD" $DEPMOD_OPTS -b "$WORKDIR" -I -e ${SYMVERS:+-E "$SYMVERS"} ${FILESYMS:+-F "$FILESYMS"} "$KVER" "${!EXTRAMODS[@]}"
copy_results_to incremental
check_results_for incremental

# 2. Same thing, but with depmod --incremental --all
# Save results in "incremental-all", and compare to "reference"
restore_results_from orig
echo "$ME: running depmod --incremental --all"
time "$DEPMOD" $DEPMOD_OPTS -b "$WORKDIR" -I -a -e ${SYMVERS:+-E "$SYMVERS"} ${FILESYMS:+-F "$FILESYMS"} "$KVER"
copy_results_to incremental-all
check_results_for incremental-all

# 3. Delete modules again, and test depmod --incremental --all for removal
# Save results in "deleted", and compare to "orig"
delete_modules
echo "$ME: running depmod --incremental --all after module removal"
# this will spit out lots of "module xyz not found" error messages
time "$DEPMOD" $DEPMOD_OPTS -b "$WORKDIR" -I -a -e ${SYMVERS:+-E "$SYMVERS"} ${FILESYMS:+-F "$FILESYMS"} "$KVER" 2>/dev/null
copy_results_to deleted
check_results_for deleted orig

exit "$ERR"
