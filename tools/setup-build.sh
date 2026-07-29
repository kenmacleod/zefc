#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
filc_cpp="${FILC_CPP:-/home/macken01/pizlo/filc-0.678-linux-x86_64/build/bin/fil++}"
build_arg="${1:-build}"
shift || true

if [[ "$build_arg" = /* ]]; then
  build_dir="$build_arg"
else
  build_dir="$root/$build_arg"
fi

if [[ ! -x "$filc_cpp" ]]; then
  echo "Fil-C++ not found or not executable: $filc_cpp" >&2
  echo "Set FILC_CPP or -Dfilc_cpp= in meson_options.txt" >&2
  exit 1
fi

if [[ -d "$build_dir/meson-private" ]]; then
  CXX="$filc_cpp" meson setup "$build_dir" "$root" --reconfigure "$@"
else
  CXX="$filc_cpp" meson setup "$build_dir" "$root" "$@"
fi
