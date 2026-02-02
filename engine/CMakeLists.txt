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
    )
elseif(WIN32)
    if(MSVC)
        target_compile_options(${BINARY_NAME} PRIVATE /W4 /permissive-)
        target_link_libraries(${BINARY_NAME} PRIVATE Vulkan::Vulkan)
    endif()
endif()

# Define FEXPORT so your code can detect and handle export logic internally
target_compile_definitions(${BINARY_NAME} PRIVATE FEXPORT)

# Set library version properties
set_target_properties(
    ${BINARY_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION_STRING}
    SOVERSION ${LIBRARY_VERSION_MAJOR}
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
#   BUILD OPTIONS

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(${BINARY_NAME} PRIVATE /Zi)
    else()
        target_compile_options(${BINARY_NAME} PRIVATE -g)
    endif()
endif()