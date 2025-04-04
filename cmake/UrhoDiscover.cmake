#
# Copyright (c) 2022-2025 the U3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Find all Urho3D sources or libraries (SDKs or build trees) available on the device.
# Only use this in a user project CMake file.

# Input variables:
#
# URHO3D_SEARCH_PATH  : The root path where Urho3D libraries will be searched.
#                       If the list is empty, the current directory will be added.
# FORCEDISCOVER : Force the search

# Added cached variables:
#
# ${PROJECTNAME}_URHO3D_DIRS : List of all discovered Urho3D source or build paths.
# ${PROJECTNAME}_URHO3D_TAGS : A UI-friendly list of URHO3D_DIRS.
# ${PROJECTNAME}_URHO3D_SELECT : The selected Urho3D directory.

# Output variable:
#
# URHO3D_HOME

# Find the cmakefile cache in the current dir or parent dir (that's the case when building as submodule at a first time)
function (get_cmakecachefile dir cachefile)
    unset (${cachefile} PARENT_SCOPE)
    if (EXISTS ${dir}/CMakeCache.txt)
        set (${cachefile} "${dir}/CMakeCache.txt" PARENT_SCOPE)
    else ()
        get_filename_component (dir ${dir} DIRECTORY)
        if (EXISTS ${dir}/CMakeCache.txt)
            set (${cachefile} "${dir}/CMakeCache.txt" PARENT_SCOPE)
        endif ()
    endif ()
endfunction ()

# Get target system name from a build tree.
function (get_build_system dir system)
    get_cmakecachefile (${dir} CacheFile)
    if (EXISTS ${CacheFile})
        file (STRINGS ${CacheFile} HeaderStrings)
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            if (${VAR} MATCHES "^ARM_PREFIX:STRING")
                set (target "ARM")
            elseif (${VAR} MATCHES "^MINGW_PREFIX:STRING")
                set (target "Windows")
            elseif (${VAR} MATCHES "^EMSCRIPTEN_SYSROOT:")
                set (target "Emscripten")
            elseif (${VAR} MATCHES "^IOS:")
                set (target "iOS")
            elseif (${VAR} MATCHES "^TVOS:")
                set (target "tvOS")
            else ()
                string (REGEX MATCH "^RPI_ABI:STRING=(.*)$" target ${VAR})
                if (target)
                    set (target "RPI_${CMAKE_MATCH_1}")
                else ()
                    string (REGEX MATCH "^CMAKE_ANDROID_ARCH_ABI:UNINITIALIZED=(.*)$" target ${VAR})
                    if (target)
                        set (target "Android_${CMAKE_MATCH_1}")
                    endif ()
                endif ()
            endif ()
            if (target)
                break ()
            endif ()
        endforeach ()
    elseif (ANDROID)
        set (target "Android")
    endif ()
    if (NOT target) # by default, use host system name
        set (target ${CMAKE_HOST_SYSTEM_NAME})
    endif ()
    set (${system} ${target} PARENT_SCOPE)
endfunction ()

# Get main build compile options from a build tree.
function (get_build_options dir archtype libtype buildtype)
    get_cmakecachefile (${dir} CacheFile)
    if (EXISTS ${CacheFile})
        file (STRINGS ${CacheFile} HeaderStrings)
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            if (NOT atype)
                string (REGEX MATCH "^CHECK_CPU_ARCHITECTURE_(.*):INTERNAL=1$" atype ${VAR})
                if (atype)
                    set (atype "${CMAKE_MATCH_1}")
                    continue ()
                endif ()
            endif ()
            if (NOT ltype)
                string (REGEX MATCH "^URHO3D_LIB_TYPE:STRING=(.*)$" ltype ${VAR})
                if (ltype)
                set (ltype "${CMAKE_MATCH_1}")
                    continue ()
                endif ()
            endif ()
            if (NOT btype)
                string (REGEX MATCH "^CMAKE_BUILD_TYPE:STRING=(.*)$" btype ${VAR})
                if (btype)
                    set (btype "${CMAKE_MATCH_1}")
                    continue ()
                endif ()
            endif ()
        endforeach ()
        if (atype MATCHES "64")
            set (${archtype} "64Bit" PARENT_SCOPE)
        else ()
            set (${archtype} "32Bit" PARENT_SCOPE)
        endif ()
        set (${libtype} "${ltype}" PARENT_SCOPE)
        set (${buildtype} "${btype}" PARENT_SCOPE)
    else ()
        set (${archtype} "*" PARENT_SCOPE)
        if (URHO3D_LIB_TYPE)
            set (${libtype} "${URHO3D_LIB_TYPE}" PARENT_SCOPE)
        else ()
            set (${libtype} "STATIC" PARENT_SCOPE)
        endif ()
        set (${buildtype} "*" PARENT_SCOPE)
    endif ()
endfunction ()

# Launches a search process starting at "search_path" to find the filename "filename" inside a sub-folder "dirname"
#   (excluding all sub-folders listed in "excludepaths" from the search for Unix-like systems only)
function (urho_find_process search_path filename dirname excludepaths results errors)
    message (DEBUG "	Searching for Urho3D directories in path = ${search_path}")
    if (MSVC)
        string(REPLACE "/" "\\" dirname "${dirname}")
        execute_process (
            COMMAND powershell -Command "Get-ChildItem -Path '${search_path}' -Recurse -Filter '${filename}' | 
                                            Where-Object { $_.DirectoryName -like \"*${dirname}*\" } | 
                                            Select-Object FullName"
            ERROR_VARIABLE ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE RESULTS
        )
    else ()
        set (excludedirs "")
        foreach (dir ${excludepaths})
            set (excludedirs "${excludedirs} *${dir}*")
        endforeach ()
        string(STRIP "${excludedirs}" excludedirs)
        execute_process (
            COMMAND find ${search_path} -name ${filename} ! -path ${excludedirs}
            COMMAND grep /${dirname}/${filename}$
            ERROR_VARIABLE ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE RESULTS
        )
    endif ()
    # add new results to previous results in parent_scope
    set (${errors} ${${errors}} ${ERROR} PARENT_SCOPE)
    set (${results} ${${results}} ${RESULTS} PARENT_SCOPE)
endfunction ()

# Search in ${PROJECTNAME}_URHO3D_SEARCH_PATH for all occurrences of Source/Urho3D/CMakeLists.txt
macro (urho_find_sources_dirs)
    set (excludepaths /. /android/ /bin*/ /build*/ /cmake/ /CMake/ /Docs/ /include/ /gradle/ /script/ /source/ /SourceAssets /tools/ /website/ /CMakeFiles/ /generated/ /lib/)
    urho_find_process(${${PROJECTNAME}_URHO3D_SEARCH_PATH} CMakeLists.txt Source/Urho3D "${excludepaths}" HEADERS FIND_ERRORS)
    if (DEFINED ENV{URHO3D_HOME} AND (NOT(SEARCH_FIRST_IN_PROJECT_FOLDER AND HEADERS) OR FORCEDISCOVER))
        urho_find_process($ENV{URHO3D_HOME} CMakeLists.txt Source/Urho3D "${excludepaths}" HEADERS FIND_ERRORS)
    endif ()
    if (FIND_ERRORS)
        message (FATAL_ERROR "!!! urho_find_sources_dirs ERROR: ${FIND_ERRORS}")
    endif ()
    set (NUM_SOURCE_DIRS 0)
    if (HEADERS)
        string (REPLACE "\n" ";" HEADERS_LIST "${HEADERS}")
        foreach (HEADER ${HEADERS_LIST})
            if (HEADER MATCHES "CMakeLists.txt$")
                unset (DIR)
                file (TO_CMAKE_PATH "${HEADER}" DIR)
                string (REPLACE "/Source/Urho3D/CMakeLists.txt" "" DIR ${DIR})
                list (FIND ${PROJECTNAME}_URHO3D_DIRS "${DIR}" index)
                if (index EQUAL -1)
                    list (APPEND ${PROJECTNAME}_URHO3D_DIRS ${DIR})
                    math (EXPR NUM_SOURCE_DIRS "${NUM_SOURCE_DIRS} + 1")
                    message (STATUS "	source: ${DIR}")
                endif ()
            endif ()
        endforeach ()
    endif ()
    message (DEBUG "	num sources = ${NUM_SOURCE_DIRS}")
endmacro ()

# Search in ${PROJECTNAME}_URHO3D_SEARCH_PATH for all occurrences of include/Urho3D/Urho3D.h
macro (urho_find_builds_dirs)
    set (excludepaths /. /bin*/ /build*/ /cmake/ /CMake/ /Docs/ /gradle/ /script/ /Source*/ /source/ /SourceAssets/ /tools/ /website/ /CMakeFiles/ /generated/ /lib/)
    urho_find_process(${${PROJECTNAME}_URHO3D_SEARCH_PATH} Urho3D.h include/Urho3D "${excludepaths}" HEADERS FIND_ERRORS)
    if (DEFINED ENV{URHO3D_HOME})
        urho_find_process($ENV{URHO3D_HOME} Urho3D.h include/Urho3D "${excludepaths}" HEADERS FIND_ERRORS)
    endif ()
    if (FIND_ERRORS)
        message (FATAL_ERROR "!!! urho_find_builds_dirs ERROR: ${FIND_ERRORS}")
    endif ()
    set (NUM_BUILD_DIRS 0)
    if (HEADERS)
        string (REPLACE "\n" ";" HEADERS_LIST "${HEADERS}")
        foreach (HEADER ${HEADERS_LIST})
            if (HEADER MATCHES "Urho3D.h$")
                file (TO_CMAKE_PATH "${HEADER}" DIR)
                string (REPLACE "/include/Urho3D/Urho3D.h" "" DIR ${DIR})
                # exclude duplicates from the list
                list (FIND ${PROJECTNAME}_URHO3D_DIRS "${DIR}" index)
                if (NOT index EQUAL -1)
                    continue ()
                endif ()
                get_build_system (${DIR} system)
                # always exclude libs that don't match the target system
                if (NOT system MATCHES "${CMAKE_SYSTEM_NAME}" OR
                    (ANDROID AND NOT system MATCHES "${CMAKE_ANDROID_ARCH_ABI}"))
                    message (DEBUG "	exclude library for ${system} in ${DIR}")
                    continue ()
                endif ()
                file (GLOB LIB_FILES LIST_DIRECTORIES FALSE 
                        "${DIR}/lib*/*Urho3D.a" "${DIR}/lib*/Urho3D/*Urho3D.a"
                        "${DIR}/lib*/*Urho3D.lib" "${DIR}/lib*/Urho3D/*Urho3D.lib" 
                        "${DIR}/lib*/*Urho3D.so" "${DIR}/lib*/Urho3D/*Urho3D.so" 
                        "${DIR}/lib*/*Urho3D.dll*" "${DIR}/lib*/Urho3D/*Urho3D.dll*" 
                        "${DIR}/lib*/*Urho3D.dylib" "${DIR}/lib*/Urho3D/*Urho3D.dylib")
                if (LIB_FILES)
                    message (STATUS "	library: found in ${DIR}")
                    list (APPEND ${PROJECTNAME}_URHO3D_DIRS ${DIR})
                    math (EXPR NUM_BUILD_DIRS "${NUM_BUILD_DIRS} + 1")
                    unset (LIB_FILES)
                endif ()
            endif ()
        endforeach ()
    endif ()
    message (DEBUG "	num builds = ${NUM_SOURCE_DIRS}")
endmacro ()

# Generate the tag list for ${PROJECTNAME}_URHO3D_SELECT
function (urho_generate_tags_list taglist)
    unset (${taglist} PARENT_SCOPE)
    foreach (home ${${PROJECTNAME}_URHO3D_DIRS})
        if (home)
			unset (desc)
            urho_find_origin ("${home}" root src origin)
            if (NOT origin)
                message (DEBUG "!! can't find an origin for the detected home=${home} !")
                continue ()
            endif ()
            get_filename_component (shortpath ${root} NAME)
            if (origin STREQUAL "sdk")
                file (STRINGS ${home}/include/Urho3D/Urho3D.h HeaderStrings)
                set (libtype "SHARED")
                # TODO: For the main Urho3D branch, URHO3D_STATIC_DEFINE is only set if MSVC
                foreach (VAR ${HeaderStrings})
                    if (VAR STREQUAL "#define URHO3D_STATIC_DEFINE")
                        set (libtype "STATIC")
                        break ()
                    endif ()
                endforeach ()
                set (desc "_${libtype}")
            elseif (origin STREQUAL "build")
                get_build_system (${home} system)
                get_build_options (${home} archtype libtype buildtype)
                set (desc "_${system}_${archtype}_${libtype}_${buildtype}")
            endif ()
            list (APPEND tags "${shortpath} (${origin}${desc}) - ${home}")
        endif ()
    endforeach ()
    if (tags)
        set (${taglist} ${tags} PARENT_SCOPE)
    endif ()
endfunction ()

# Remove all cached variables that don't match the prefix
# Also, remove global variables with the same name as cached variables.
# Always exclude CMAKE_vars
macro (unset_cache_variables_without prefix)
    get_cmake_property(cachedVariables CACHE_VARIABLES)
    string (TOUPPER ${prefix} prefix)
    foreach (variable ${cachedVariables})
        string (TOUPPER ${variable} uppervariable)
        if (NOT "${uppervariable}" MATCHES "^${prefix}" AND NOT "${uppervariable}" MATCHES "^CMAKE")
            unset ("${variable}" CACHE)
            unset ("${variable}")
        endif ()
    endforeach ()
endmacro ()


set (${PROJECTNAME}_URHO3D_SEARCH_PATH "" CACHE PATH "Root path for searching Urho3D")
if (NOT ${PROJECTNAME}_URHO3D_SEARCH_PATH)
    if (URHO3D_SEARCH_PATH)
        set (${PROJECTNAME}_URHO3D_SEARCH_PATH "${URHO3D_SEARCH_PATH}" CACHE PATH "Root path for searching Urho3D" FORCE)
        unset (URHO3D_SEARCH_PATH)
        unset (SEARCH_FIRST_IN_PROJECT_FOLDER)
    else ()
        set (${PROJECTNAME}_URHO3D_SEARCH_PATH "${CMAKE_SOURCE_DIR}" CACHE PATH "Root path for searching Urho3D" FORCE)
        set (SEARCH_FIRST_IN_PROJECT_FOLDER TRUE)
    endif ()
endif ()

# Conditions to launch the search of Urho3D directories
if ((NOT URHO3D_HOME AND NOT ${PROJECTNAME}_URHO3D_DIRS) OR FORCEDISCOVER OR
    (DEFINED ${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST AND NOT "${${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST}" STREQUAL "${${PROJECTNAME}_URHO3D_SEARCH_PATH}"))
    set (${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST ${${PROJECTNAME}_URHO3D_SEARCH_PATH} CACHE INTERNAL STRING)
    set (SEARCH_URHO3D_ENABLE TRUE)
    unset (FORCEDISCOVER CACHE)
else ()
    set (SEARCH_URHO3D_ENABLE FALSE)
endif ()

if (SEARCH_URHO3D_ENABLE)
    message (STATUS "URHO3D_DISCOVER ... search (this may take some time) ...")
    unset (${PROJECTNAME}_URHO3D_DIRS)
    unset (${PROJECTNAME}_URHO3D_DIRS CACHE)
    unset (${PROJECTNAME}_URHO3D_TAGS)
    unset (${PROJECTNAME}_URHO3D_TAGS CACHE)
    urho_find_sources_dirs ()
    if (NOT (SEARCH_FIRST_IN_PROJECT_FOLDER AND HEADERS) OR FORCEDISCOVER)
        urho_find_builds_dirs ()
    endif ()
    if (${PROJECTNAME}_URHO3D_DIRS)
        list (SORT ${PROJECTNAME}_URHO3D_DIRS)
        urho_generate_tags_list (${PROJECTNAME}_URHO3D_TAGS)
    endif ()
    set (num_tags 0)
    if (${PROJECTNAME}_URHO3D_TAGS)
        list (LENGTH ${PROJECTNAME}_URHO3D_TAGS num_tags)
    endif ()
    message (STATUS "	Found ${num_tags} Urho3D directories!")
    urho_update_cached_dirs()
endif ()

# Update the Urho3D directory selection
if (NOT "${${PROJECTNAME}_URHO3D_SELECT_LAST}" STREQUAL "${${PROJECTNAME}_URHO3D_SELECT}")
    set (${PROJECTNAME}_URHO3D_SELECT_LAST ${${PROJECTNAME}_URHO3D_SELECT} CACHE INTERNAL STRING)
    unset_cache_variables_without ("${PROJECTNAME}")
    if (CMAKE_CROSSCOMPILING AND CMAKE_TOOLCHAIN_FILE)
        # Include the toolchain to be sure to have well-defined cross-compile variables
        include ("${CMAKE_TOOLCHAIN_FILE}")
    endif ()
    unset (URHO3D_ROOT_DIR)
    unset (URHO3D_SOURCE_DIR)
    unset (URHO3D_HOME)
    unset (URHO3D_HOME CACHE)
endif ()

# Set Urho3D home directory based on the selected folder
if (${PROJECTNAME}_URHO3D_SELECT AND NOT URHO3D_HOME_LAST)
    set (URHO3D_HOME_LAST ${URHO3D_HOME})
    list (FIND ${PROJECTNAME}_URHO3D_TAGS "${${PROJECTNAME}_URHO3D_SELECT}" index)
    if (NOT index EQUAL -1)
        list (GET ${PROJECTNAME}_URHO3D_DIRS ${index} home)
        if (EXISTS ${home})
            set (URHO3D_HOME ${home} CACHE PATH "Urho3D source, build tree, or SDK directory" FORCE)
            set (${PROJECTNAME}_URHO3D_DIR_SELECT ${home} CACHE INTERNAL STRING)
        endif ()
    endif ()
endif ()

# Align the selection with URHO3D_HOME if it exists
if (URHO3D_HOME AND NOT "${${PROJECTNAME}_URHO3D_DIR_SELECT}" STREQUAL "${URHO3D_HOME}")
    list (FIND ${PROJECTNAME}_URHO3D_DIRS "${URHO3D_HOME}" index)
    if (NOT index EQUAL -1)
        set (${PROJECTNAME}_URHO3D_DIR_SELECT ${URHO3D_HOME} CACHE INTERNAL STRING)
        list (GET ${PROJECTNAME}_URHO3D_TAGS ${index} tag)
        set (${PROJECTNAME}_URHO3D_SELECT "${tag}" CACHE STRING "Urho3D source/build tree selection" FORCE)
    endif ()
endif ()
