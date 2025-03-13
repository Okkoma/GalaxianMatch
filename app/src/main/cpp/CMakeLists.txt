cmake_minimum_required(VERSION 3.10.2)

# Define target name
set (TARGET_NAME GalaxianMatch)

if (SPACEMATCH_WITH_DEMOMODE)
    message ("-- adding definition ACTIVE_DEMOMODE")
    add_definitions (-DACTIVE_DEMOMODE)
endif()

if (SPACEMATCH_WITH_TIPS)
    message ("-- adding definition ACTIVE_TIPS")
    add_definitions (-DACTIVE_TIPS)
endif()

if (SPACEMATCH_WITH_ADS)
    message ("-- adding definition ACTIVE_ADS")
    add_definitions (-DACTIVE_ADS)
endif()

if (SPACEMATCH_WITH_CINEMATICS)
    message ("-- adding definition ACTIVE_CINEMATICS")
    add_definitions (-DACTIVE_CINEMATICS)
endif()

if (SPACEMATCH_WITH_LOOPTESTS)
    message ("-- adding definition ACTIVE_GAMELOOPTESTING")
    add_definitions (-DACTIVE_GAMELOOPTESTING)
endif()

if (SPACEMATCH_WITH_NETWORK)
    message ("-- adding definition TEST_NETWORK")
    add_definitions (-DTEST_NETWORK)
endif()

# Define source files
set (SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set (SRC_DIRS / /Components/ /GraphicEffects/ /Graphics2D/ /Objects/ /ObjectsCore/ /States/ /Managers/ /Resources/)
              
if (SPACEMATCH_WITH_NETWORK)
    list (APPEND SRC_DIRS /Network/)
    list (APPEND INC_DIRS ${SRC_ROOT}/Network/)
endif ()

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
