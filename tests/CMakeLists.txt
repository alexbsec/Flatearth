cmake_minimum_required(VERSION 3.20)

############################################################################
#   PROJECT
project(FlatearthTests LANGUAGES CXX)

set(BINARY_NAME flatearth_tests)

############################################################################
#   SOURCES
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cc")

add_executable(${BINARY_NAME} ${TEST_SOURCES})

############################################################################
#   LANGUAGE / STANDARD
set_target_properties(${BINARY_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

############################################################################
#   ENGINE LIBRARY
# Mirror the testbed's prebuilt-lib lookup so `cmake -S tests -B ...` works
# standalone, and also works when built from the root CMakeLists.
if(NOT TARGET flatearth)
    set(_fe_engine_build_suffix "debug")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_fe_engine_build_suffix "release")
    endif()

    set(FLATEARTH_ENGINE_BUILD_DIR
        "${CMAKE_CURRENT_LIST_DIR}/../engine/build-${_fe_engine_build_suffix}"
        CACHE PATH "Path to engine build dir containing libflatearth.so"
    )

    find_library(FLATEARTH_LIB
        NAMES flatearth libflatearth
        PATHS
            "${CMAKE_CURRENT_LIST_DIR}/../${_fe_engine_build_suffix}/bin"
            "${FLATEARTH_ENGINE_BUILD_DIR}"
            "${FLATEARTH_ENGINE_BUILD_DIR}/lib"
        NO_DEFAULT_PATH
    )

    if(NOT FLATEARTH_LIB)
        message(FATAL_ERROR
            "Could not find libflatearth.so in:\n"
            "  ${FLATEARTH_ENGINE_BUILD_DIR}\n"
            "  ${FLATEARTH_ENGINE_BUILD_DIR}/lib\n"
            "Set FLATEARTH_ENGINE_BUILD_DIR accordingly."
        )
    endif()

    add_library(flatearth SHARED IMPORTED GLOBAL)
    set_target_properties(flatearth PROPERTIES
        IMPORTED_LOCATION "${FLATEARTH_LIB}"
    )

    target_include_directories(flatearth INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/../engine/src"
    )
endif()

############################################################################
#   RPATH (so the binary finds libflatearth.so at runtime)
if(UNIX AND NOT APPLE)
    set_target_properties(${BINARY_NAME} PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH FALSE
    )
endif()

target_link_libraries(${BINARY_NAME} PRIVATE flatearth)

############################################################################
#   COMPILER OPTIONS
if(MSVC)
    target_compile_options(${BINARY_NAME}
        PRIVATE
            /W4
            /permissive-
            /std:c++latest
            /Zc:__cplusplus
    )
    target_compile_definitions(${BINARY_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(${BINARY_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

if(NOT WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${BINARY_NAME} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -O1
        -g
    )
    target_link_options(${BINARY_NAME} PRIVATE
        -fsanitize=address,undefined
    )
endif()

############################################################################
#   COPY ENGINE LIB NEXT TO TEST BINARY (post-build)
add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:flatearth>
        $<TARGET_FILE_DIR:${BINARY_NAME}>
)

