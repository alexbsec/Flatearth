cmake_minimum_required(VERSION 3.20)

############################################################################
#   PROJECT
project(FlatearthTestbed LANGUAGES CXX)

set(BINARY_NAME flatearth_testbed)

############################################################################
#   SOURCES
add_executable(${BINARY_NAME}
    src/Game.cc
    src/Entrypoint.cc
)

############################################################################
#   LANGUAGE / STANDARD
# Keep it consistent across compilers.
set_target_properties(${BINARY_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

############################################################################
#   DEPENDENCIES
find_package(Vulkan REQUIRED)

# ----------------------------------------------------------------------------
# Flatearth (prebuilt engine lib) for Linux builds where engine is built separately
# ----------------------------------------------------------------------------
if(UNIX AND NOT APPLE AND NOT TARGET flatearth)
    set(FLATEARTH_ENGINE_BUILD_DIR
        "${CMAKE_CURRENT_LIST_DIR}/../engine/build"
        CACHE PATH "Path to engine build dir containing libflatearth.so"
    )

    # Try common output layouts
    find_library(FLATEARTH_LIB
        NAMES flatearth libflatearth
        PATHS
            "${FLATEARTH_ENGINE_BUILD_DIR}"
            "${FLATEARTH_ENGINE_BUILD_DIR}/lib"
            "${CMAKE_CURRENT_LIST_DIR}/../bin"
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

    # Optional: if you include engine headers from testbed
    target_include_directories(flatearth INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/../engine/src"
    )
endif()

target_link_libraries(${BINARY_NAME}
    PRIVATE
        flatearth
        Vulkan::Vulkan
)

############################################################################
#   PLATFORM-SPECIFIC (Linux/X11)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)

    target_link_libraries(${BINARY_NAME}
        PRIVATE
            X11::X11
    )

    # xcb bits: prefer pkg-config, fallback to raw link names.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-keysyms x11-xcb)
        target_link_libraries(${BINARY_NAME} PRIVATE PkgConfig::XCB)
    else()
        target_link_libraries(${BINARY_NAME}
            PRIVATE
                xcb
                X11-xcb
                xcb-keysyms
        )
    endif()
endif()

############################################################################
#   WARNINGS / COMPILER OPTIONS
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

if(NOT WIN32)
    target_compile_options(flatearth_testbed PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_link_options(flatearth_testbed PRIVATE
        -fsanitize=address,undefined
    )
endif()

############################################################################
#   COPY ENGINE LIB NEXT TO TESTBED (post-build)
add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:flatearth>
        $<TARGET_FILE_DIR:${BINARY_NAME}>
)
