# TODO: add helper to validate only_bits and skip_test
# Allowed values are 32, 64 and help.

#
# Simple helper executing the given function for all compiler/bit combinations
#
for_each_config() {
    [[ -n $1 ]] && compilers="$1" || compilers="gcc clang"
    [[ -n $2 ]] && bits="$2" || bits="32 64"
    [[ -n $3 ]] && suffix="-$3" || suffix=""
    meson_cmd=$4

    for compiler in ${compilers[@]}; do
        for bit in ${bits[@]}; do
            echo "::group::$compiler, $bit bit"
            builddir="builddir-$compiler-$bit$suffix/"

            if [[ "$bit" == "32" ]]; then
                CC="$compiler -m32" $meson_cmd "$builddir" "$bit"
            else
                CC=$compiler $meson_cmd "$builddir" "$bit"
            fi

            echo "::endgroup::"
        done
    done
}
