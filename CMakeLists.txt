cmake_minimum_required(VERSION 3.10.2)

project (SpaceMatch VERSION 1.038)

set (APP_SRC app)
#set (APP_SRC appads)

# clangd helper
#set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set (URHO3D_SEARCH_ROOT ${CMAKE_SOURCE_DIR}/../../U3D/U3D-okkoman-git)

#set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../../U3D/U3D-okkoman-git)
#set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D-GalaxianMatch)
set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../U3D-GalaxianMatch)

#set (GIT_U3D_REPO "/media/DATA/Dev/Projets/U3D-GalaxianMatch")
set (GIT_U3D_REPO "https://github.com/Okkoma/U3D-GalaxianMatch.git")

include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

# For Android, Options are in app/build.gradle.kts
if (NOT ANDROID)
    # Default Options
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	    # Default installation folder
      	set_property (CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/exe)
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
    option (SPACEMATCH_WITH_DEMOMODE "Enable Demo Mode : only the 3 first constellations are playable" TRUE)
    option (SPACEMATCH_WITH_TIPS "Enable Keyboard Tips" TRUE)
    option (SPACEMATCH_WITH_ADS "Enable Android Ads" FALSE)
    option (SPACEMATCH_WITH_CINEMATICS "Enable Story Animations" TRUE)
    option (SPACEMATCH_WITH_3D "Enable some 3d features" TRUE)
    # Dev Game Options
    option (SPACEMATCH_WITH_NETWORK "Enable Network via websocket" TRUE)
    # Dev Test
    option (SPACEMATCH_WITH_TEST "Enable Test" FALSE)
    option (SPACEMATCH_WITH_LOOPTESTS "Enable Tests Loop on Android" FALSE)

    if (SPACEMATCH_WITH_TEST)
        include (CTest)
        enable_testing ()
    endif ()
endif ()

message (STATUS "Project configuration:")
message (STATUS "   Host      ${CMAKE_HOST_SYSTEM_NAME}")
message (STATUS "   Platform  ${CMAKE_SYSTEM_NAME}")

if (URHO3D_HOME)
    message (STATUS "   Build     ${CMAKE_BUILD_TYPE}")
    message (STATUS "   Library   ${URHO3D_LIB_TYPE}")
    add_subdirectory (${APP_SRC}/src/main/cpp ${ARGN})

    # Install
    if (NOT ANDROID)
        # install stuff is included in UrhoCommon module
        if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) # Default installation folder	
            set_property (CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/exe)
        endif ()
        install (FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
        install (FILES ${CMAKE_SOURCE_DIR}/bin/Data/EngineConfig/default.xml DESTINATION bin RENAME engine_config.xml)

        # CPack
        set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Galaxian Match - a space-themed match3 game")
        set (CPACK_PACKAGE_VENDOR "https://github.com/Okkoma")
        set (CPACK_PACKAGE_CONTACT "okkoman0xx0@gmail.com")
        set (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
        set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
        set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
        set (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
        set (CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
        set (CPACK_PACKAGE_FILE_NAME "GalaxianMatch-${CMAKE_SYSTEM_NAME}-${CPACK_PACKAGE_VERSION}")
        set (CPACK_GENERATOR ZIP)
        include (CPack)
    endif ()
else ()
    message ("!! GalaxianMatch - wait to add source because URHO3D_HOME is undefined here !")
endif ()
