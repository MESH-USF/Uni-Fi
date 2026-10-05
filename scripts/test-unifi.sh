#!/usr/bin/env bash
set -euo pipefail
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d /tmp/unifi-tests.XXXXXX)
for source in "$repo_root"/firmware/test/unifi_host/*.cpp; do
  binary="$build_dir/$(basename "$source" .cpp)"
  g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I "$repo_root/firmware/src" \
    -I "$repo_root/firmware/examples/companion_radio" \
    "$source" -o "$binary"
  "$binary"
done
if [ -f "$repo_root/web/package.json" ]; then
  (cd "$repo_root/web" && npm test)
fi
