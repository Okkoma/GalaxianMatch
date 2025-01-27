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
file (GLOB_RECURSE SOURCE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" ./Source/*.cpp ./Source/*.h)

# Add specific TODO.txt file
#set (SOURCE_FILES "${SOURCE_FILES}" "../../../../TODO.txt")

# Setup target with resource copying
setup_main_executable ()

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)

target_include_directories (${TARGET_NAME} PRIVATE ${SRC_ROOT}/Components/ ${SRC_ROOT}/GraphicEffects/
                                                   ${SRC_ROOT}/Objects/ ${SRC_ROOT}/ObjectsCore/
                                                   ${SRC_ROOT}/States/ ${SRC_ROOT}/Managers/
                                                   ${SRC_ROOT}/Network/ ${SRC_ROOT}/Resources/
                                                   ${SRC_ROOT}/ ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ )
                                                  
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)

if (EMSCRIPTEN)
    target_link_libraries (${TARGET_NAME} datachannel-wasm)
else ()
    target_link_libraries (${TARGET_NAME} datachannel-static)    
endif ()
