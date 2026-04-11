#!/bin/bash
set -e

# Usage:
#   ./post-build.sh            -> debug (default)
#   ./post-build.sh debug      -> debug
#   ./post-build.sh release    -> release
#   ./post-build.sh debug debug    -> debug (compatible with passing mode + outRoot)
#   ./post-build.sh release release -> release

mode="${1:-debug}"
out_root="${2:-}"

# Normalize mode
mode=$(echo "$mode" | tr '[:upper:]' '[:lower:]')

# Decide output root:
# - if 2nd arg provided, trust it
# - otherwise derive from mode
if [[ -n "$out_root" ]]; then
  out_root=$(echo "$out_root" | tr '[:upper:]' '[:lower:]')
else
  if [[ "$mode" == "release" ]]; then
    out_root="release"
  else
    out_root="debug"
  fi
fi

BIN_DIR="${out_root}/bin"
ASSET_DIR="${BIN_DIR}/assets"
SHADER_DIR="${ASSET_DIR}/shaders"

mkdir -p "${SHADER_DIR}"

echo "Compiling shaders into ${SHADER_DIR}..."

echo "assets/shaders/Builtin.ObjectShader.vert.glsl -> ${SHADER_DIR}/Builtin.ObjectShader.vert.spv"
"${VULKAN_SDK}/bin/glslc" -fshader-stage=vert \
  assets/shaders/Builtin.ObjectShader.vert.glsl \
  -o "${SHADER_DIR}/Builtin.ObjectShader.vert.spv"

echo "assets/shaders/Builtin.ObjectShader.frag.glsl -> ${SHADER_DIR}/Builtin.ObjectShader.frag.spv"
"${VULKAN_SDK}/bin/glslc" -fshader-stage=frag \
  assets/shaders/Builtin.ObjectShader.frag.glsl \
  -o "${SHADER_DIR}/Builtin.ObjectShader.frag.spv"

echo "Copying assets into ${BIN_DIR}..."

mkdir -p "${ASSET_DIR}/textures"
cp -r assets/textures/. "${ASSET_DIR}/textures/" 

echo "Done."
