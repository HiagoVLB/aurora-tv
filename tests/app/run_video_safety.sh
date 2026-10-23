#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
ndl=third_party/ss4s/modules/webos/ndl/webos5
cc -std=gnu11 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Icore/moonlight-common-c/src -Isrc/app \
  tests/app/test_decode_unit.c src/app/stream/video/decode_unit.c -o "$test_dir/decode_unit"
"$test_dir/decode_unit"
cc -std=gnu11 -g -fsanitize=address,undefined -fno-omit-frame-pointer -D_GNU_SOURCE \
  -Ithird_party/ss4s/include -Ithird_party/ss4s/modules/interface/include \
  -Ithird_party/ss4s/mocks/include/libndl-media -Ithird_party/ss4s/modules/webos/utils \
  -I"$ndl" $(pkg-config --cflags opus) \
  tests/app/test_ndl_serialization.c "$ndl/ndl_module.c" "$ndl/ndl_player.c" \
  "$ndl/ndl_video.c" "$ndl/ndl_audio.c" "$ndl/opus_empty.c" "$ndl/opus_fix.c" \
  third_party/ss4s/modules/webos/common/aurora_frame_diag.c \
  -pthread -ldl $(pkg-config --libs opus) -lm -o "$test_dir/ndl"
"$test_dir/ndl"
