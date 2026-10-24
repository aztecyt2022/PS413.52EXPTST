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
cc -std=gnu11 -O2 -Wall -Wextra -Werror -pthread -Iplugin_mono/source \
  plugin_mono/source/mono_bootstrap.c tests/mono_bootstrap_test.c -o "$test_dir/bootstrap"
"$test_dir/bootstrap"
cc -std=gnu11 -O2 -Wall -Wextra -Werror -DDEXCORE_HOOK_TEST -Itests -Iplugin_mono/source \
  plugin_mono/source/overlay_hook.c plugin_mono/source/overlay_hook_native.c \
  tests/vendor/hde64/HDE64.c tests/overlay_hook_test.c -o "$test_dir/hook"
"$test_dir/hook"
