set (CMAKE_PROJECT_VERSION 1.0.0)

if (WEB)
    add_subdirectory (libdatachannel-wasm EXCLUDE_FROM_ALL)
else ()
    add_subdirectory (libdatachannel EXCLUDE_FROM_ALL)
endif ()

if (SPACEMATCH_WITH_TEST)
    add_subdirectory (catch2)
endif ()