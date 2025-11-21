#!/bin/bash

set -e  # Exit on any error

buildDir="./build"
linkPath=$(pwd)
enginePath=$(pwd)
binDir="../bin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RESET='\033[0m'

echo -e "${CYAN}############################### BUILDING ENGINE ###############################${RESET}"

# Check build directory
echo -e "${YELLOW}>> Checking if build directory exists...${RESET}"
if [ ! -d "$buildDir" ]; then
    echo -e "${GREEN}Creating build directory...${RESET}"
    mkdir "$buildDir"
    pushd "$buildDir" > /dev/null
    echo -e "${CYAN}Running 'cmake ..'${RESET}"
    cmake ..
    popd > /dev/null
else
    echo -e "${GREEN}Build directory exists!${RESET}"
fi

# CMake step
echo -e "${CYAN}############################# CMAKE STEP ######################################${RESET}"

if [ -f compile_commands.json ]; then
    echo -e "${YELLOW}Cleaning old compile commands...${RESET}"
    rm compile_commands.json
fi

echo -e "${GREEN}Creating compile_commands.json...${RESET}"
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B "$buildDir"
ln -sf "${linkPath}/build/compile_commands.json" compile_commands.json 

echo -e "${CYAN}Running CMake build...${RESET}"
cmake --build "$buildDir"

echo -e "${CYAN}Running make inside build directory...${RESET}"
pushd "$buildDir" > /dev/null
make
popd > /dev/null

# Copying step
echo -e "${CYAN}############################# COPYING STEP ####################################${RESET}"

echo -e "${YELLOW}Cleaning old library links...${RESET}"
rm -rf "${binDir}/libflatearth.so"*

echo -e "${GREEN}Creating new symlinks...${RESET}"
ln -sf "${enginePath}/build/libflatearth.so" "${binDir}/libflatearth.so"
ln -sf "${enginePath}/build/libflatearth.so.0" "${binDir}/libflatearth.so.0"
ln -sf "${enginePath}/build/libflatearth.so.0.1" "${binDir}/libflatearth.so.0.1"

echo -e "${CYAN}############################# FINISHED ########################################${RESET}"
echo -e "${GREEN}Flatearth library built successfully!${RESET}"

