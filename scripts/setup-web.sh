#!/usr/bin/env bash
# Explicit networked setup. Subsequent builds and runtime use local files only.
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PODPLAYER_DEPS_DIR="${PODPLAYER_DEPS_DIR:-/workspace/deps}"
mkdir -p "$PODPLAYER_DEPS_DIR"
checkout() {
  local url="$1" destination="$2" commit="$3"
  if [[ ! -d "$destination/.git" ]]; then
    if [[ -e "$destination" ]]; then
      echo "Refusing to overwrite existing dependency path: $destination" >&2; exit 1
    fi
    git clone --no-checkout "$url" "$destination"
    git -C "$destination" checkout --detach "$commit"
  fi
  [[ "$(git -C "$destination" rev-parse HEAD)" == "$commit" ]] || {
    echo "Dependency commit mismatch at $destination; inspect it manually." >&2; exit 1;
  }
  git -C "$destination" diff --quiet && git -C "$destination" diff --cached --quiet || {
    echo "Dependency contains local source changes: $destination" >&2; exit 1;
  }
}
checkout https://github.com/lvgl/lvgl.git "$PODPLAYER_DEPS_DIR/lvgl-8.3.10" 06e229cc7679c01119ca5d905a1f9dad96f475f0
checkout https://github.com/emscripten-core/emsdk.git "$PODPLAYER_DEPS_DIR/emsdk" d7b4130688bfa348392b921ab04015aa15c4f154
"$PODPLAYER_DEPS_DIR/emsdk/emsdk" install 3.1.64
"$PODPLAYER_DEPS_DIR/emsdk/emsdk" activate 3.1.64
cd "$REPO_ROOT"
EMSDK="$PODPLAYER_DEPS_DIR/emsdk" LVGL_DIR="$PODPLAYER_DEPS_DIR/lvgl-8.3.10" python3 scripts/build-web.py
