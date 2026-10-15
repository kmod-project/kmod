#!/bin/bash
# SPDX-FileCopyrightText: 2026 Lucas De Marchi <demarchi@kernel.org>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Report the commits in <upstream-branch> that should be cherry-picked to
# <stable-branch>.
#
# A commit is a candidate when it has a "Fixes: <commit>" line and the
# commit being fixed is part of the stable branch:
#   - it is reachable from the tip of the stable branch
#   - it was cherry-picked to <stable-branch> (found via the
#     "(cherry picked from commit <commit>)" line);
#   - it is itself a candidate already selected (fix of a fix).
#
# Commits are walked oldest first, so a fix of a fix is only evaluated after
# the fix it refers to.
#
# Examples:
#
# 1) Any backport since the last commit backported to the stable branch:
#
#   ./stable-backport.sh kmod-34.y master
#
# 2) Any backport since the stable branch diverged from the upstream branch:
#
#  ./stable-backport.sh --starting-point=v34 kmod-34.y master

set -euo pipefail

usage() {
    echo "Usage: $0 [--starting-point=<commit>] <stable-branch> <upstream-branch>" >&2
    exit 1
}

git_resolve() {
    git rev-parse --verify --quiet "$1^{commit}"
}

# Resolve the refs from stdin, ignoring (with a warning) the invalid ones
git_resolve_all() {
    local context=$1
    local failed=()

    while read -r ref; do
        git_resolve "$ref" || failed+=("$ref")
    done

    if [[ ${#failed[@]} -gt 0 ]]; then
        echo "$context: ignoring invalid commit(s): ${failed[*]}" >&2
    fi
}

find_starting_point() {
    local stable=$1
    git log --notes -n1 -E --grep='^\(cherry picked from commit [0-9a-f]{7,40}\)' \
        --format=%B%N "$stable" |
        grep -E -m1 '^\(cherry picked from commit [0-9a-f]{7,40}\)' |
        sed -nE 's/^\(cherry picked from commit ([0-9a-f]+)\).*/\1/p'
}

# State from options
STABLE_BRANCH=
UPSTREAM_BRANCH=
START=

parse_args() {
    local STARTING_POINT=
    local POSITIONAL=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
        --starting-point=*) STARTING_POINT=${1#*=} ;;
        --starting-point)
            [[ $# -ge 2 ]] || usage
            STARTING_POINT="$2"
            shift
            ;;
        -h | --help) usage ;;
        -*)
            echo "Unknown option: $1" >&2
            usage
            ;;
        *)
            POSITIONAL+=("$1")
        esac
        shift
    done

    [[ ${#POSITIONAL[@]} -eq 2 ]] || usage

    STABLE_BRANCH=${POSITIONAL[0]}
    UPSTREAM_BRANCH=${POSITIONAL[1]}

    # Validate args
    for ref in "$STABLE_BRANCH" "$UPSTREAM_BRANCH"; do
        git_resolve "$ref" >/dev/null || {
            echo "Invalid branch/commit: $ref" >&2
            exit 1
        }
    done

    if [[ -z $STARTING_POINT ]]; then
        STARTING_POINT=$(find_starting_point "$STABLE_BRANCH") || {
            echo "Could not find starting point for $STABLE_BRANCH" >&2
            exit 1
        }
        echo "Starting point (from $STABLE_BRANCH): $STARTING_POINT" >&2
    else
        echo "Starting point: $STARTING_POINT" >&2
    fi

    START=$(git_resolve "$STARTING_POINT") || {
        echo "Invalid starting point: $STARTING_POINT" >&2
        exit 1
    }

    local branch_point
    branch_point=$(git merge-base "$STABLE_BRANCH" "$UPSTREAM_BRANCH") || {
        echo "No common ancestor between $STABLE_BRANCH and $UPSTREAM_BRANCH" >&2
        exit 1
    }

    git merge-base --is-ancestor "$branch_point" "$START" || {
        echo "Starting point $STARTING_POINT is older than the branch point ($branch_point)" >&2
        exit 1
    }
}

parse_args "$@"

# Global state
declare -A PICKED=()
declare -A FIX=()
declare -A REASON=()
declare -A RESULT=()
CANDIDATE=()

# Find all commits in the upstream branch that have a "Fixes: <commit>" line
# and mark them as candidates. The "Fixes: <commit>" line is used to find the
# commit that was fixed, so we can check if it is already in the stable branch.
readarray -t CANDIDATE < <(git log --notes --reverse --format=%H -E \
                           --grep='^Fixes: *[0-9a-fA-F]{7,40}' "$START..$UPSTREAM_BRANCH")
echo "Found ${#CANDIDATE[@]} candidate commits in $START..$UPSTREAM_BRANCH" >&2
for c in "${CANDIDATE[@]}"; do
    f=$(git show -s --format=%B%N "$c" | \
        sed -nE 's/^Fixes: *([0-9a-fA-F]{7,40}).*/\1/p' | \
        git_resolve_all "$c" | xargs)
    FIX[$c]="$f"
done

# Find all commits in the stable branch that were cherry-picked (assume it was
# from upstream). We will use this to check if a candidate commit was already
# picked to the stable branch.
while read -r h; do
    [[ -n $h ]] || continue
    full=$(git_resolve "$h") || continue
    PICKED[$full]=1
done < <(git log "$UPSTREAM_BRANCH..$STABLE_BRANCH" --format=%B%N |
         sed -nE 's/^\(cherry picked from commit ([0-9a-f]{7,40})\).*/\1/p')

# Also find upstream commits that have an equivalent patch in the stable
# branch (by looking at their patch-id), which covers commits picked
# without -x or applied with git am. We still look at the cherry-pick lines
# above, because only looking at patch-id doesn't cover cases where there
# were conflict resolutions
while read -r mark h; do
    [[ $mark == - ]] && PICKED[$h]=1
done < <(git cherry "$STABLE_BRANCH" "$UPSTREAM_BRANCH")

for c in "${CANDIDATE[@]}"; do
    [[ -z ${PICKED[$c]-} ]] || {
        RESULT[$c]=0
        REASON[$c]="already cherry-picked to $STABLE_BRANCH"
        continue
    }

    for f in ${FIX[$c]}; do
        if git merge-base --is-ancestor "$f" "$STABLE_BRANCH"; then
            RESULT[$c]=1
            REASON[$c]="Fixed commit $f is in $STABLE_BRANCH"
            continue 2
        fi

        if [[ -n ${PICKED[$f]-} ]]; then
            RESULT[$c]=1
            REASON[$c]="Fixed commit $f was cherry-picked to $STABLE_BRANCH"
            continue 2
        fi

        if [[ ${RESULT[$f]-} == 1 ]]; then
            RESULT[$c]=1
            REASON[$c]="Fixed commit $f is a fix of a fix to be added to $STABLE_BRANCH"
            continue 2
        fi
    done
    RESULT[$c]=0
    REASON[$c]="Fixed commit(s) ${FIX[$c]} not in $STABLE_BRANCH"
done

for c in "${CANDIDATE[@]}"; do
    printf "%s %d %s\n" "$c" "${RESULT[$c]}" "${REASON[$c]}"
done
