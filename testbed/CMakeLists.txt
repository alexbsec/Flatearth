cmake_minimum_required(VERSION 3.20)

############################################################################
#   PROJECT
project(FlatearthTestbed LANGUAGES CXX)

set(BINARY_NAME flatearth_testbed)

############################################################################
#   SOURCES
add_executable(${BINARY_NAME}
    src/Entrypoint.cc
    src/Game.cc
    src/Director.cc
    src/Systems/PlayerSystem.cc
    src/Systems/CameraFollowSystem.cc
    src/Systems/WorldSystem.cc
    src/Systems/HPBarSystem.cc
    src/Systems/MainMenuSystem.cc
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
        "${FLATEARTH_ENGINE_BUILD_DIR}/_deps/imgui-src"
    )
endif()

if(UNIX AND NOT APPLE)
  set_target_properties(${BINARY_NAME} PROPERTIES
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    SKIP_BUILD_RPATH FALSE
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH_USE_LINK_PATH FALSE
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
            /Zc:preprocessor
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
    target_compile_options(flatearth_testbed PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -O1
        -g
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

############################################################################
#   COPY GAME ASSETS NEXT TO TESTBED (post-build)
set(_assets_src "${CMAKE_CURRENT_LIST_DIR}/../assets")
foreach(_subdir textures fonts bgm tiles)
    add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_assets_src}/${_subdir}"
            "$<TARGET_FILE_DIR:${BINARY_NAME}>/assets/${_subdir}"
        COMMENT "Copying assets/${_subdir}"
    )
endforeach()

############################################################################
#   RUNTIME ASSETS
# Built-in shaders are embedded in the engine library at compile time.
# Only game assets (textures, tiles, audio) need to be deployed next to
# the executable — handled by the root Makefile's asset-copy step.
