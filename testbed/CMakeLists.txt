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
#   DEPENDENCIES

find_package(Vulkan REQUIRED)

target_link_libraries(${BINARY_NAME}
    PRIVATE
        flatearth
        Vulkan::Vulkan
)

############################################################################
#   PLATFORM-SPECIFIC

if(UNIX AND NOT APPLE)
    target_link_libraries(${BINARY_NAME}
        PRIVATE
            X11
            xcb
            X11-xcb
            xcb-keysyms
    )
endif()

if(MSVC)
    target_compile_options(${BINARY_NAME}
        PRIVATE
            /W4
            /permissive-
            /std:c++latest
    )
else()
    target_compile_options(${BINARY_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_custom_command(TARGET flatearth_testbed POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:flatearth>
        $<TARGET_FILE_DIR:flatearth_testbed>
)