#!/usr/bin/env bash
# Applies the patches in 3ds/patches/ to the pinned third_party submodules.
# Safe to run more than once: already-applied patches are skipped.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# apply_series <submodule> <patch>...
#
# The patches are listed oldest first and build on each other. Once a later
# patch is applied, an earlier one can no longer be reversed, because the lines
# around its changes have moved on. So only the newest applied patch can be
# detected by reversing it: find it, treat it and everything before it as
# applied, and apply the rest in order.
apply_series() {
    local submodule="$1"
    shift
    local patches=("$@")
    local dir="$ROOT/$submodule"
    if [ ! -e "$dir/.git" ]; then
        echo "error: $submodule is not checked out (run: git submodule update --init --recursive)" >&2
        exit 1
    fi

    local newest_applied=-1 i
    for ((i = ${#patches[@]} - 1; i >= 0; i--)); do
        if git -C "$dir" apply --reverse --check "$ROOT/3ds/patches/${patches[i]}" >/dev/null 2>&1; then
            newest_applied=$i
            break
        fi
    done

    for ((i = 0; i < ${#patches[@]}; i++)); do
        local file="$ROOT/3ds/patches/${patches[i]}"
        if ((i <= newest_applied)); then
            echo "already applied: ${patches[i]}"
        else
            git -C "$dir" apply --check "$file"
            git -C "$dir" apply "$file"
            echo "applied: ${patches[i]}"
        fi
    done
}

apply_series third_party/moonlight-common-c \
    moonlight-common-c-h263.patch \
    moonlight-common-c-mpeg2.patch
