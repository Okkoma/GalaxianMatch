cmake_minimum_required(VERSION 3.10.2)

project (SpaceMatch)

set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/app/src/main/cpp/ThirdParty/urho3d)
include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

# Default Options
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	# Release build as default
	set (CMAKE_BUILD_TYPE Release)

	# Default installation folder
  	set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/exe)
	
	# On windows, use DirectX11 backend
	if (WIN32)
	    set (URHO3D_D3D11 TRUE)
	endif ()  
endif()

# Disable libdatachannel media,examples and tests
set (NO_MEDIA ON CACHE STRING "Disable media transport support" FORCE)
set (NO_EXAMPLES ON CACHE STRING "Disable examples" FORCE)
set (NO_TESTS ON CACHE STRING "Disable tests build" FORCE)
mark_as_advanced (FORCE NO_MEDIA NO_EXAMPLES NO_TESTS)

# see Urho3D engine options in cmake/Modules/UrhoCommon.cmake

# Game options
set (SPACEMATCH_WITH_DEMOMODE TRUE CACHE BOOLEAN "Enable Demo Mode : only the 3 first constellations are playable")
set (SPACEMATCH_WITH_TIPS TRUE CACHE BOOLEAN "Enable Keyboard Tips")
set (SPACEMATCH_WITH_ADS FALSE CACHE BOOLEAN "Enable Android Ads")
set (SPACEMATCH_WITH_CINEMATICS TRUE CACHE BOOLEAN "Enable Story Animations")
set (SPACEMATCH_WITH_LOOPTESTS FALSE CACHE BOOLEAN "Enable Tests Loop on Android")

# Dev Game Options
set (SPACEMATCH_WITH_NETWORK FALSE CACHE BOOLEAN "Enable Network via websocket")

if (URHO3D_HOME)
	add_subdirectory (app/src/main/cpp ${ARGN})
endif ()
