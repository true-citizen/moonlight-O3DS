#!/usr/bin/env bash
# Applies the patches in 3ds/patches/ to the pinned third_party submodules.
# Safe to run more than once: already-applied patches are skipped.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

apply() {
    local submodule="$1" patch="$2"
    local dir="$ROOT/$submodule" file="$ROOT/3ds/patches/$patch"
    if [ ! -e "$dir/.git" ]; then
        echo "error: $submodule is not checked out (run: git submodule update --init --recursive)" >&2
        exit 1
    fi
    if git -C "$dir" apply --reverse --check "$file" >/dev/null 2>&1; then
        echo "already applied: $patch"
    else
        git -C "$dir" apply --check "$file"
        git -C "$dir" apply "$file"
        echo "applied: $patch"
    fi
}

apply third_party/moonlight-common-c moonlight-common-c-h263.patch
