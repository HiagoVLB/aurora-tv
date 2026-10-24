#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cc -std=gnu11 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -D_DEFAULT_SOURCE -DCURL_DISABLE_TYPECHECK \
  -Icore/libgamestream/libgamestream -Icore/libgamestream/src \
  -Ithird_party/commons/util/logging \
  tests/core/test_http_lifecycle.c core/libgamestream/src/http.c \
  -pthread -o "$test_dir/http"
"$test_dir/http"
"$test_dir/http" fail-init
