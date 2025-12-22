# Install script for directory: C:/Users/jcbus/source/repos/Flatearth/engine

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/FlatearthSolution")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/Debug/flatearth.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/Release/flatearth.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/MinSizeRel/flatearth.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/RelWithDebInfo/flatearth.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/Debug/flatearth.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/Release/flatearth.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/MinSizeRel/flatearth.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/jcbus/source/repos/Flatearth/build/engine/RelWithDebInfo/flatearth.dll")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Containers/DArray.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Containers/HashSet.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Containers/LinkedList.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Containers/Queue.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/Application.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/ApplicationConfig.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/Event.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/FeMemory.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/Input.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Core/Logger.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Defines.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Entrypoint.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Error.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/GameTypes.hpp"
    "C:/Users/jcbus/source/repos/Flatearth/engine/src/Platform/Platform.hpp"
    )
endif()

