#!/usr/bin/env bash
# Linux standalone fixture tests. PROTOCOL_TEST_HEADERS may point to copied SDK
# mbedtls/expat headers; no ARM libraries or live TV/host are needed.
set -euo pipefail
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cc -std=gnu11 -g -fsanitize=address,undefined -ffunction-sections -fdata-sections \
  ${PROTOCOL_TEST_HEADERS:+-I"$PROTOCOL_TEST_HEADERS"} \
  -Icore/libgamestream/libgamestream -Icore/libgamestream/src \
  -Icore/moonlight-common-c/src -Ithird_party/commons/util/logging \
  -Ithird_party/commons/util/uuidstr \
  tests/core/test_vibeshine_protocol.c core/libgamestream/src/client.c \
  core/libgamestream/src/xml.c core/libgamestream/src/set_error.c \
  -Wl,--gc-sections -l:libexpat.so.1 -o "$test_dir/protocol"
"$test_dir/protocol"
