cmake_minimum_required(VERSION 3.10.2)

project (SpaceMatch)

include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

if (URHO3D_HOME)
	add_subdirectory (app/src/main/cpp ${ARGN})
endif ()
