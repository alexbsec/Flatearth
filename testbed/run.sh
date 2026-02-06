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
linkPath=$(pwd)
testingPath=$(pwd)
binDir="../${OUT_ROOT}/bin"

mkdir -p "$buildDir"
mkdir -p "$binDir"

GREEN='\033[0;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RESET='\033[0m'

echo -e "${CYAN}############################### BUILDING TESTBED (${type}) ###############################${RESET}"

if [ -f compile_commands.json ]; then
  echo -e "${YELLOW}Cleaning old compile commands...${RESET}"
  rm compile_commands.json
fi

cmake -DCMAKE_BUILD_TYPE="$type" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -S . \
      -B "$buildDir"

ln -sf "${linkPath}/${buildDir}/compile_commands.json" compile_commands.json

echo -e "${CYAN}Running CMake build...${RESET}"
cmake --build "$buildDir"

echo -e "${CYAN}Moving test executable to bin...${RESET}"
mv -f "${testingPath}/${buildDir}/flatearth_testbed" "${binDir}/flatearth_testbed"

echo -e "${CYAN}############################# FINISHED ########################################${RESET}"
echo -e "${GREEN}Testbed executable created successfully!${RESET}"
