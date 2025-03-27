cmake_minimum_required(VERSION 3.10.2)

# Define target name
set (TARGET_NAME GalaxianMatch)

set (SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set (SRC_DIRS / /Components/ /GraphicEffects/ /Objects/ /ObjectsCore/ /Managers/ /Resources/)

# Check for u3d-galaxian fork
file (READ "${URHO3D_HOME}/README.md" lines)
string (FIND "${lines}" "# U3D-GalaxianMatch" result)
if (NOT ${result} EQUAL -1)
    set (SPACEMATCH_WITH_CUSTOM_URHO TRUE)
endif ()

if (SPACEMATCH_WITH_DEMOMODE)
    message ("-- adding definition ACTIVE_DEMOMODE")
    add_definitions (-DACTIVE_DEMOMODE)
endif ()

if (SPACEMATCH_WITH_TIPS)
    message ("-- adding definition ACTIVE_TIPS")
    add_definitions (-DACTIVE_TIPS)
endif ()

if (SPACEMATCH_WITH_ADS)
    message ("-- adding definition ACTIVE_ADS")
    add_definitions (-DACTIVE_ADS)
endif ()

if (SPACEMATCH_WITH_CINEMATICS)
    message ("-- adding definition ACTIVE_CINEMATICS")
    add_definitions (-DACTIVE_CINEMATICS)
endif ()

if (SPACEMATCH_WITH_3D)
    message ("-- adding definition ACTIVE_3D_MIX")
    add_definitions (-DACTIVE_3D_MIX)
endif ()

if (SPACEMATCH_WITH_CUSTOM_URHO)
    message ("-- adding definition ACTIVE_CUSTOM_URHO")
    add_definitions (-DACTIVE_CUSTOM_URHO)
endif ()

if (SPACEMATCH_WITH_LOOPTESTS)
    message ("-- adding definition ACTIVE_GAMELOOPTESTING")
    add_definitions (-DACTIVE_GAMELOOPTESTING)
endif ()

if (SPACEMATCH_WITH_NETWORK)
    message ("-- adding definition TEST_NETWORK")
    add_definitions (-DTEST_NETWORK)
    list (APPEND SRC_DIRS /Network/)
endif()

if (SPACEMATCH_WITH_CUSTOM_URHO)
    list (APPEND SRC_DIRS /Graphics2D/)
    list (APPEND SRC_DIRS /States/)
else ()
    list (APPEND SOURCE_FILES Source/States/sSplash.cpp Source/States/sMainMenu.cpp Source/States/sLevelMap.cpp
                              Source/States/sOptions.cpp Source/States/sPlay.cpp)
    list (APPEND INC_DIRS ${SRC_ROOT}/Graphics2D/)
    list (APPEND INC_DIRS ${SRC_ROOT}/States/)
endif ()

# Define source files
foreach (DIR ${SRC_DIRS})
    file (GLOB CPP_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" ./Source${DIR}*.cpp)
    list (APPEND SOURCE_FILES ${CPP_FILES})
    list (APPEND INC_DIRS ${SRC_ROOT}${DIR})
endforeach ()

# Setup target with resource copying
setup_main_executable ()

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)
target_include_directories (${TARGET_NAME} PRIVATE ${INC_DIRS})

if (SPACEMATCH_WITH_NETWORK)
    if (WEB)
        add_subdirectory (ThirdParty/libdatachannel-wasm EXCLUDE_FROM_ALL)
        target_link_libraries (${TARGET_NAME} datachannel-wasm)
    else ()
        add_subdirectory (ThirdParty/libdatachannel EXCLUDE_FROM_ALL)
        target_link_libraries (${TARGET_NAME} datachannel-static)    
    endif ()
endif ()

if (SPACEMATCH_WITH_TEST)
    add_subdirectory (ThirdParty/catch2)
    add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Test)
endif ()
