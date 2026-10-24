#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
cc -std=gnu11 -O2 -Wall -Wextra -Werror -pthread -DDEXCORE_METRICS_TEST \
  '-DDEXCORE_OVERLAY_STATE="/tmp/dexcore-overlay-ci/state"' \
  -Iplugin_mono/source plugin_mono/source/system_overlay.c \
  plugin_mono/source/system_metrics.c tests/overlay_runtime_test.c -o "$test_dir/runtime"
"$test_dir/runtime" off
"$test_dir/runtime" persist
${CXX:-clang++} -std=c++17 -O1 -Wall -Wextra -Werror -Wno-unused-function \
  -Wno-unused-variable -Wno-missing-field-initializers -fno-exceptions -fno-rtti \
  -pthread -Itests tests/overlay_ui_test.cc -o "$test_dir/ui"
"$test_dir/ui"
