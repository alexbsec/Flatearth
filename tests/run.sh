#!/bin/bash
set -e

type="Debug"
OUT_ROOT="${2:-debug}"

if [[ "$1" =~ ^([Rr]elease)$ ]]; then
  type="Release"
  OUT_ROOT="release"
else
  type="Debug"
  OUT_ROOT="debug"
fi

buildDir="./build-${OUT_ROOT}"
binDir="../${OUT_ROOT}/bin"

mkdir -p "$buildDir"
mkdir -p "$binDir"

GREEN='\033[0;32m'
CYAN='\033[1;36m'
RESET='\033[0m'

echo -e "${CYAN}############################### BUILDING TESTS (${type}) ###############################${RESET}"

cmake -DCMAKE_BUILD_TYPE="$type" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -S . \
      -B "$buildDir"

linkPath=$(pwd)
ln -sf "${linkPath}/${buildDir}/compile_commands.json" compile_commands.json

echo -e "${CYAN}Running CMake build...${RESET}"
cmake --build "$buildDir"

echo -e "${CYAN}Moving test executable to bin...${RESET}"
cp -f "${buildDir}/flatearth_tests" "${binDir}/flatearth_tests"
cp -f "${buildDir}/libflatearth.so" "${binDir}/libflatearth.so" 2>/dev/null || true

echo -e "${CYAN}############################# FINISHED ########################################${RESET}"
echo -e "${GREEN}Tests built successfully!${RESET}"
