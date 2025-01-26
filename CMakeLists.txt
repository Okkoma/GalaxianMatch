cmake_minimum_required(VERSION 3.10.2)

project (SpaceMatch)

set (CMAKE_BUILD_TYPE Release)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/exe)
endif()

# Disable libdatachannel media,examples and tests
set (NO_MEDIA ON CACHE STRING "Disable media transport support" FORCE)
set (NO_EXAMPLES ON CACHE STRING "Disable examples" FORCE)
set (NO_TESTS ON CACHE STRING "Disable tests build" FORCE)
mark_as_advanced (FORCE NO_MEDIA NO_EXAMPLES NO_TESTS)

# Disable Wayland with this version of urho3d
set (VIDEO_WAYLAND FALSE CACHE STRING "Use Wayland video driver" FORCE)

# see more options in cmake/Modules/UrhoCommon.cmake

set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/app/src/main/cpp/ThirdParty/urho3d)
include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

if (URHO3D_HOME)
	add_subdirectory (app/src/main/cpp ${ARGN})
endif ()
