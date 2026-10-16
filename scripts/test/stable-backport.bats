#!/usr/bin/env bats
# SPDX-FileCopyrightText: 2026 Lucas De Marchi <demarchi@kernel.org>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Tests for scripts/stable-backport.sh. Each test creates its own git
# repository with an upstream branch (master) and a stable branch (stable).

bats_require_minimum_version 1.5.0

SCRIPT="$BATS_TEST_DIRNAME/../stable-backport.sh"

setup() {
    # Isolate from the user's git configuration
    export GIT_CONFIG_GLOBAL=/dev/null
    export GIT_CONFIG_NOSYSTEM=1
    export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com
    export GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com

    cd "$BATS_TEST_TMPDIR"
    git init -q -b master repo
    cd repo

    OLD=$(commit old)
    BASE=$(commit base)
    git branch stable
}

# commit <name> [<message body>]: create a commit touching its own file and
# print its hash
commit() {
    echo "$1" > "$1"
    git add "$1"
    git commit -q -m "$1" ${2:+-m "$2"}
    git rev-parse HEAD
}

# pick <commit> [<cherry-pick args>...]: cherry-pick a commit to stable
pick() {
    local c=$1
    shift

    git checkout -q stable
    git cherry-pick "$@" "$c" >/dev/null
    git checkout -q master
}

# run_backport [<args>...]: run the script with stable and master branches
run_backport() {
    run --separate-stderr "$SCRIPT" "$@" stable master
}

# check_result <commit> <result> <reason pattern>: check the line reported
# for a commit
check_result() {
    local line

    line=$(grep "^$1 " <<< "$output") || {
        echo "No result for $1 in output:" >&2
        echo "$output" >&2
        return 1
    }

    [[ $line == "$1 $2 "$3 ]] || {
        echo "Unexpected result for $1: $line" >&2
        echo "Expected: $1 $2 $3" >&2
        return 1
    }
}

@test "fix of commit reachable from stable is a candidate" {
    local fix
    fix=$(commit fix "Fixes: ${BASE:0:12}")

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix" 1 "Fixed commit $BASE is in stable"
}

@test "fix of commit not in stable is not a candidate" {
    local a fix
    a=$(commit a)
    fix=$(commit fix "Fixes: ${a:0:12}")

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix" 0 "Fixed commit(s) $a not in stable"
}

@test "fix of commit cherry-picked with -x is a candidate" {
    local a fix
    a=$(commit a)
    fix=$(commit fix "Fixes: ${a:0:12}")
    pick "$a" -x

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix" 1 "Fixed commit $a was cherry-picked to stable"
}

@test "commit cherry-picked without -x is detected by patch-id" {
    local a b fix
    a=$(commit a)
    b=$(commit b "Fixes: ${BASE:0:12}")
    fix=$(commit fix "Fixes: ${b:0:12}")
    pick "$b"

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$b" 0 "already cherry-picked to stable"
    check_result "$fix" 1 "Fixed commit $b was cherry-picked to stable"
}

@test "commit cherry-picked with -x is not a candidate again" {
    local fix
    fix=$(commit fix "Fixes: ${BASE:0:12}")
    pick "$fix" -x

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix" 0 "already cherry-picked to stable"
}

@test "fix of a fix is a candidate" {
    local fix1 fix2
    fix1=$(commit fix1 "Fixes: ${BASE:0:12}")
    fix2=$(commit fix2 "Fixes: ${fix1:0:12}")

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix1" 1 "Fixed commit $BASE is in stable"
    check_result "$fix2" 1 "Fixed commit $fix1 is a fix of a fix*"
}

@test "commits without Fixes: are ignored" {
    local a
    a=$(commit a)

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    [[ -z $output ]]
    [[ $stderr == *"Found 0 candidate commits"* ]]
}

@test "invalid Fixes: commit is ignored with a warning" {
    local bad good
    bad=$(commit bad "Fixes: deadbeef1234")
    good=$(commit good "Fixes: ${BASE:0:12}")

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    [[ $stderr == *"$bad: ignoring invalid commit(s): deadbeef1234"* ]]
    check_result "$bad" 0 "*not in stable"
    check_result "$good" 1 "Fixed commit $BASE is in stable"
}

@test "only results are printed to stdout" {
    local fix
    fix=$(commit fix "Fixes: ${BASE:0:12}")

    run_backport --starting-point="$BASE"
    [[ $status -eq 0 ]]
    [[ ${#lines[@]} -eq 1 ]]
    [[ $stderr == *"Found 1 candidate commits"* ]]
}

@test "starting point defaults to the last commit cherry-picked with -x" {
    local a fix1 fix2
    fix1=$(commit fix1 "Fixes: ${BASE:0:12}")
    a=$(commit a)
    fix2=$(commit fix2 "Fixes: ${BASE:0:12}")
    pick "$a" -x

    run_backport
    [[ $status -eq 0 ]]
    [[ $stderr == *"Starting point (from stable): $a"* ]]
    [[ ${#lines[@]} -eq 1 ]]
    check_result "$fix2" 1 "Fixed commit $BASE is in stable"
}

@test "missing automatic starting point is an error" {
    commit fix "Fixes: ${BASE:0:12}" >/dev/null

    run_backport
    [[ $status -eq 1 ]]
    [[ $stderr == *"Could not find starting point for stable"* ]]
}

@test "starting point older than the branch point is an error" {
    commit fix "Fixes: ${BASE:0:12}" >/dev/null

    run_backport --starting-point="$OLD"
    [[ $status -eq 1 ]]
    [[ $stderr == *"is older than the branch point ($BASE)"* ]]
}

@test "--starting-point with separate argument" {
    local fix
    fix=$(commit fix "Fixes: ${BASE:0:12}")

    run_backport --starting-point "$BASE"
    [[ $status -eq 0 ]]
    check_result "$fix" 1 "Fixed commit $BASE is in stable"
}

@test "--starting-point without argument prints usage" {
    run --separate-stderr "$SCRIPT" stable master --starting-point
    [[ $status -eq 1 ]]
    [[ $stderr == Usage:* ]]
}

@test "invalid branch is an error" {
    run --separate-stderr "$SCRIPT" --starting-point="$BASE" nonexistent master
    [[ $status -eq 1 ]]
    [[ $stderr == *"Invalid branch/commit: nonexistent"* ]]
}
