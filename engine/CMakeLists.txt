############################################################################
#   SELECTING CMAKE & C++ MINIMUM VERSION

cmake_minimum_required(VERSION 3.29)

set(CMAKE_CXX_STANDARD 23)

############################################################################
#   SETTING GLOBAL PARAMETERS

set(PROJECT_NAME "Flatearth Engine")
set(BINARY_NAME "flatearth")

project("${PROJECT_NAME}" LANGUAGES CXX)

set(LIBRARY_VERSION_MAJOR 0)
set(LIBRARY_VERSION_STRING 0.1)

# Use GNUInstallDirs to get platform-appropriate installation directories
include(GNUInstallDirs)

set(LIBRARY_BASE_PATH "${PROJECT_SOURCE_DIR}/src")

############################################################################
#   DEPENDENCIES

include(FetchContent)

# Box2D

FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v3.0.0
    GIT_SHALLOW    TRUE
)

set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED    OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS       OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(box2d)
set_target_properties(box2d PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Tiny XML 2

FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0
    GIT_SHALLOW    TRUE
)

set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(tinyxml2)
set_target_properties(tinyxml2 PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ImGui

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_SOURCE_DIR)

############################################################################
#   PROJECT DEFINITIONS

file(GLOB_RECURSE SOURCE_FILES "${LIBRARY_BASE_PATH}/*.cc")
file(GLOB_RECURSE HEADER_FILES "${LIBRARY_BASE_PATH}/*.hpp")

# Make the src available for includes
include_directories("${LIBRARY_BASE_PATH}")

# Add the shared library target
add_library(${BINARY_NAME} SHARED
    ${SOURCE_FILES}
    ${HEADER_FILES}
)


target_include_directories(${BINARY_NAME}
    PUBLIC
        ${LIBRARY_BASE_PATH}
)

target_sources(${BINARY_NAME} PRIVATE
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(${BINARY_NAME}
    PRIVATE
        ${IMGUI_SOURCE_DIR}/backends
    PUBLIC
        ${IMGUI_SOURCE_DIR}
)

############################################################################
#   PLATFORM-SPECIFIC SETTINGS

target_compile_features(${BINARY_NAME} PUBLIC cxx_std_23)
find_package(Vulkan REQUIRED)

if(UNIX AND NOT MSVC)
    # Use hidden symbol visibility by default
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)

    # Enable position-independent code for shared libs on Unix
    target_compile_options(${BINARY_NAME} PRIVATE -fPIC)

    target_link_libraries(${BINARY_NAME}
        PRIVATE
            X11
            xcb
            X11-xcb
            xcb-keysyms
            Vulkan::Vulkan
            box2d
            tinyxml2
            dl
            pthread
            m
    )
elseif(WIN32)
    if(MSVC)
        target_compile_options(${BINARY_NAME} PRIVATE /W4 /permissive-)
        target_link_libraries(${BINARY_NAME} PRIVATE Vulkan::Vulkan box2d tinyxml2)
    endif()
endif()

target_include_directories(${BINARY_NAME} PRIVATE
    $<TARGET_PROPERTY:box2d,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:tinyxml2,INTERFACE_INCLUDE_DIRECTORIES>
)

# Define FEXPORT so your code can detect and handle export logic internally
target_compile_definitions(${BINARY_NAME} PRIVATE FEXPORT)

# Set library version properties
set_target_properties(
    ${BINARY_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION_STRING}
    SOVERSION ${LIBRARY_VERSION_MAJOR}
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

############################################################################
#   LIBRARY INSTALLATION DIRECTIVES

# Install library (.so on Linux/.dll and .lib on Windows)
install(
    TARGETS ${BINARY_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install header files
install(
    FILES ${HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

############################################################################
#   BUILT-IN SHADER EMBEDDING

if(WIN32)
    set(_glslc_hint "$ENV{VULKAN_SDK}/Bin/glslc.exe")
else()
    set(_glslc_hint "$ENV{VULKAN_SDK}/bin/glslc")
endif()

find_program(GLSLC glslc HINTS "${_glslc_hint}")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Install the Vulkan SDK or add glslc to PATH.")
endif()

get_filename_component(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(_shader_src "${_repo_root}/assets/shaders")
set(_vert_glsl  "${_shader_src}/Builtin.ObjectShader.vert.glsl")
set(_frag_glsl  "${_shader_src}/Builtin.ObjectShader.frag.glsl")
set(_vert_spv   "${CMAKE_CURRENT_BINARY_DIR}/Builtin.ObjectShader.vert.spv")
set(_frag_spv   "${CMAKE_CURRENT_BINARY_DIR}/Builtin.ObjectShader.frag.spv")
set(_gen_dir    "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(_shaders_hpp "${_gen_dir}/BuiltinShaders.hpp")

file(MAKE_DIRECTORY "${_gen_dir}")

add_custom_command(
    OUTPUT "${_vert_spv}" "${_frag_spv}"
    COMMAND "${GLSLC}" -fshader-stage=vert "${_vert_glsl}" -o "${_vert_spv}"
    COMMAND "${GLSLC}" -fshader-stage=frag "${_frag_glsl}" -o "${_frag_spv}"
    DEPENDS "${_vert_glsl}" "${_frag_glsl}"
    COMMENT "Compiling built-in GLSL shaders to SPIR-V"
    VERBATIM
)

add_custom_command(
    OUTPUT "${_shaders_hpp}"
    COMMAND ${CMAKE_COMMAND}
        -DVERT_SPV=${_vert_spv}
        -DFRAG_SPV=${_frag_spv}
        -DOUT_FILE=${_shaders_hpp}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShaders.cmake"
    DEPENDS "${_vert_spv}" "${_frag_spv}"
    COMMENT "Embedding built-in shaders into BuiltinShaders.hpp"
    VERBATIM
)

add_custom_target(flatearth_builtin_shaders DEPENDS "${_shaders_hpp}")
add_dependencies(${BINARY_NAME} flatearth_builtin_shaders)
target_include_directories(${BINARY_NAME} PRIVATE "${_gen_dir}")

############################################################################
#   BUILD OPTIONS

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(${BINARY_NAME} PRIVATE /Zi)
    else()
        target_compile_options(${BINARY_NAME} PRIVATE -g)
    endif()
endif()
