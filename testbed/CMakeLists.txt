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
#   RUNTIME ASSETS (assets/ next to executable + compile shaders to .spv)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(ASSET_SRC_DIR  "${REPO_ROOT}/assets")
set(SHADER_SRC_DIR "${ASSET_SRC_DIR}/shaders")

set(RUNTIME_DIR     "$<TARGET_FILE_DIR:${BINARY_NAME}>")
set(ASSET_OUT_DIR   "${RUNTIME_DIR}/assets")
set(SHADER_OUT_DIR  "${ASSET_OUT_DIR}/shaders")

# Find glslc (Windows + Linux)
if(WIN32)
  set(GLSLC "$ENV{VULKAN_SDK}/Bin/glslc.exe")
else()
  set(GLSLC "$ENV{VULKAN_SDK}/bin/glslc")
endif()

if(NOT EXISTS "${GLSLC}")
  find_program(GLSLC glslc)
endif()

if(NOT GLSLC)
  message(FATAL_ERROR "glslc not found. Install Vulkan SDK or put glslc in PATH / set VULKAN_SDK.")
endif()

add_custom_command(
  TARGET ${BINARY_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSET_OUT_DIR}"

  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"

  COMMAND "${GLSLC}" -fshader-stage=vert
          "${SHADER_SRC_DIR}/Builtin.ObjectShader.vert.glsl"
          -o "${SHADER_OUT_DIR}/Builtin.ObjectShader.vert.spv"

  COMMAND "${GLSLC}" -fshader-stage=frag
          "${SHADER_SRC_DIR}/Builtin.ObjectShader.frag.glsl"
          -o "${SHADER_OUT_DIR}/Builtin.ObjectShader.frag.spv"

  COMMENT "Copying assets/ next to executable + compiling shaders to assets/shaders/*.spv"
  VERBATIM
)
