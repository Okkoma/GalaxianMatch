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

# Input variables:
#
# URHO3D_HOME: Urho3D home directory.
# URHO3D_HOME could be a build directory (e.g., URHO3D_BUILD_DIR) that contains subfolders "include" and "lib" if Urho3D is used as an SDK/build library.
# URHO3D_HOME could be the root directory (e.g., URHO3D_ROOT_DIR) that contains all the content of Urho3D.
# URHO3D_ROOT_DIR, URHO3D_SOURCE_DIR, and URHO3D_BUILD_DIR are defined to handle each specific case depending on how Urho3D is used.

# variables for fetch u3d:
#
# URHO3D_FETCH_CONDITION: can be "always", "never", "if_not_found"
# GIT_U3D_REPO: can be an url or a local path
# GIT_U3D_TAG: can be a commit hash, a branch name or a tag
# by default use U3D-community repository

set (DEFAULT_URHO3D_FETCH_CONDITION "if_not_found")
set (DEFAULT_GIT_U3D_REPOSITORY "https://github.com/u3d-community/U3D.git")
set (DEFAULT_GIT_U3D_TAG "master")

# Output variables:
#
# URHO3D_AS_SUBMODULE: Boolean, true if Urho3D is used in the user project as a submodule (i.e., "as third-party, from source").
# URHO3D_ROOT_DIR: The path containing all Urho3D bin/source/CMake folders.
# URHO3D_SOURCE_DIR: The path to the Urho3D source.
# URHO3D_BUILD_DIR: A specific Urho3D build directory (SDK or build tree) that contains the Urho3D include files and library (.a, .lib, .so, or .dll).
#                    If Urho3D is used as a submodule, it will be built in ${CMAKE_BINARY_DIR}/urho3d.
# URHO3D_CMAKE_MODULE: A specific directory containing UrhoCommon.cmake.
#                      This allows using a custom set of CMake files or the latest available with older Urho3D sources.

# TODO: recognize the default compile options for the sdks. these options can be differents between u3d versions.

set (URHO3D_TARGET Urho3D)
string (TOLOWER ${URHO3D_TARGET} URHO3D_TARGET_LOWER)
string (TOUPPER ${CMAKE_PROJECT_NAME} PROJECTNAME)
set (URHO3D_FETCH_DIR ${CMAKE_BINARY_DIR}/_deps/${URHO3D_TARGET_LOWER}-src)
string (TOLOWER "${URHO3D_LIB_TYPE}" URHO3D_LIBTYPE_LOWER)
if (NOT URHO3D_LIBTYPE_LOWER)
    set (URHO3D_LIBTYPE_LOWER "static")
endif ()

# Find the Urho3D root path and source path from a presumed Urho3D subfolder.
function (urho_find_origin dir root source origin)
    unset (${root} PARENT_SCOPE)
    unset (${source} PARENT_SCOPE)
    unset (${origin} PARENT_SCOPE)
    set (currentpath "${dir}")
    while (NOT urhoroot AND currentpath AND NOT previouspath STREQUAL currentpath)
        if (ANDROID AND EXISTS "${currentpath}/android/urho3d-lib/.cxx/${URHO3D_LIBTYPE_LOWER}") # Detect Android build tree
            set (ORIGIN_FOUND "build")
            set (BUILD_STAGING_DIR ${currentpath}/android/urho3d-lib/.cxx/${URHO3D_LIBTYPE_LOWER})
        endif ()
        if (EXISTS "${currentpath}/Source" AND EXISTS "${currentpath}/README.md") # Detect Urho3D source distribution
            set (${root} "${currentpath}" PARENT_SCOPE)
            if (NOT ORIGIN_FOUND)
                set (ORIGIN_FOUND "source")
                set (${source} "${currentpath}/Source" PARENT_SCOPE)
            endif ()
        endif ()
        if (NOT ORIGIN_FOUND AND (EXISTS "${currentpath}/lib" OR EXISTS "${currentpath}/lib64"))
            if (EXISTS "${currentpath}/share/Urho3D/cmake/Modules/UrhoCommon.cmake" OR
                EXISTS "${currentpath}/share/cmake/Modules/UrhoCommon.cmake")     # Detect Urho3D SDK
                set (ORIGIN_FOUND "sdk")
                set (${root} "${currentpath}" PARENT_SCOPE)
            elseif (EXISTS "${currentpath}/include/Urho3D/Urho3DAll.h" AND
                    EXISTS "${currentpath}/CMakeCache.txt")                       # Detect Urho3D build tree
                set (ORIGIN_FOUND "build")
            endif ()
        endif ()
        set (previouspath ${currentpath})
        get_filename_component (currentpath ${previouspath} DIRECTORY)
    endwhile ()
    set (${origin} "${ORIGIN_FOUND}" PARENT_SCOPE)
endfunction ()

function (urho_cmake_module dir module_dir)
    set (dir "${CMAKE_CURRENT_SOURCE_DIR}")
    while (NOT dir STREQUAL "/")
        if (EXISTS "${dir}/cmake/Modules/UrhoCommon.cmake")
            set (${module_dir} "${dir}/cmake/Modules" PARENT_SCOPE)
            break ()
        endif ()
        get_filename_component (dir ${dir} DIRECTORY)
    endwhile ()
endfunction ()

macro (urho_get_num_found_dirs num_dirs)
    unset (${num_dirs})
    if (DEFINED ${PROJECTNAME}_URHO3D_DIRS)
        list (LENGTH ${PROJECTNAME}_URHO3D_DIRS ${num_dirs})
    endif ()
endmacro ()

macro (urho_update_cached_dirs)
    unset (${PROJECTNAME}_URHO3D_DIRS CACHE)
    unset (${PROJECTNAME}_URHO3D_TAGS CACHE)
    set (${PROJECTNAME}_URHO3D_DIRS ${${PROJECTNAME}_URHO3D_DIRS} CACHE INTERNAL "All available Urho3D directories")
    set (${PROJECTNAME}_URHO3D_TAGS ${${PROJECTNAME}_URHO3D_TAGS} CACHE INTERNAL "All available Urho3D tags")
    set (${PROJECTNAME}_URHO3D_SELECT "" CACHE STRING "Urho3D source/build tree selection" FORCE)
    set_property (CACHE ${PROJECTNAME}_URHO3D_SELECT PROPERTY STRINGS ${${PROJECTNAME}_URHO3D_TAGS})
endmacro ()

# Fetch U3D from a git repository
function (urho_fetch_git repo tag)
    if (NOT URHO3D_FETCH_CONDITION)
        set (URHO3D_FETCH_CONDITION ${DEFAULT_URHO3D_FETCH_CONDITION})
    endif ()
    if (URHO3D_FETCH_CONDITION STREQUAL "if_not_found")
        urho_get_num_found_dirs (num_dirs)
        if (num_dirs GREATER 0)
            message (STATUS "	URHO3D_FETCH_CONDITION is set to if_not_found. Skip fetch!")
            return ()
        endif ()
    elseif (NOT condition STREQUAL "always")
        message (STATUS "	URHO3D_FETCH_CONDITION is unknown or is set to never. Skip fetch!")
        return ()
    endif ()
    if (NOT EXISTS "${URHO3D_FETCH_DIR}/README.md")
        if (NOT tag)
            set (tag ${DEFAULT_GIT_U3D_TAG})
        endif ()
        message (STATUS "Fetching U3D from ${repo}:${tag}")
        # CMake “fetchcontent” performs a first configuration step that causes a re-entrance problem with ours modules. We therefore use "git fetch" directly.
        file (MAKE_DIRECTORY "${URHO3D_FETCH_DIR}")
        execute_process (COMMAND git init WORKING_DIRECTORY "${URHO3D_FETCH_DIR}" OUTPUT_QUIET ERROR_QUIET)
        execute_process (COMMAND git fetch --depth=1 "${repo}" "${tag}" WORKING_DIRECTORY "${URHO3D_FETCH_DIR}" OUTPUT_QUIET ERROR_QUIET)
        execute_process (COMMAND git reset --hard FETCH_HEAD WORKING_DIRECTORY "${URHO3D_FETCH_DIR}" OUTPUT_QUIET ERROR_QUIET)
        if (EXISTS "${URHO3D_FETCH_DIR}/README.md")
            message (STATUS "Fetched in directory ${URHO3D_FETCH_DIR}")
            if (${PROJECTNAME}_URHO3D_DIRS)
                list (APPEND ${PROJECTNAME}_URHO3D_DIRS ${URHO3D_FETCH_DIR})
                list (APPEND ${PROJECTNAME}_URHO3D_TAGS "U3D (source_${repo}:${tag}) - ${URHO3D_FETCH_DIR}")
                urho_update_cached_dirs ()
                message (STATUS "	Add ${repo}:${tag} to ${PROJECTNAME}_URHO3D_SELECT drop down list.")
            endif ()
            # as default always select the fetched source
            set (URHO3D_HOME ${URHO3D_FETCH_DIR} PARENT_SCOPE)
            message (STATUS "	Set URHO3D_HOME to ${URHO3D_FETCH_DIR}.")
        else ()
            message ("!! Can't fetch content from this repository.")
        endif ()
    else ()
        message (STATUS "	Already fetched in directory ${URHO3D_FETCH_DIR}.")
    endif ()
endfunction ()

## CHECK PART
# Check cmake folder if exists
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/cmake)
    message ("!! Cannot find the cmake directory !")
    return ()
endif ()
# Android: Check for BUILD_STAGING_DIR and JNI_DIR
if (ANDROID)
    unset (URHO3D_HOME) # unset in this case, because gradle interprets ENV var. and from argument as the same
    unset (URHO3D_HOME CACHE)
    if (BUILD_STAGING_DIR OR JNI_DIR) # FindUrho3D.cmake handles the following case.
        if (NOT URHOCOMMON_INUSE)
            include (${CMAKE_SOURCE_DIR}/cmake/Modules/UrhoCommon.cmake)
        endif ()
        return ()
    endif ()
endif ()
# Reset URHO3D_HOME folder if not exists
if (URHO3D_HOME AND NOT EXISTS ${URHO3D_HOME})
    message ("!! ${URHO3D_HOME} don't exist ... reset URHO3D_HOME !")
    unset (URHO3D_HOME)
endif ()

## DISCOVER/FETCH PART
# Fetch from a specified u3d repo if the fetch condition authorizes it
if (NOT URHO3D_HOME AND GIT_U3D_REPO)
    message (STATUS "Fetch GIT_U3D_REPO ...")
    urho_fetch_git ("${GIT_U3D_REPO}" "${GIT_U3D_TAG}")
endif ()
# Include Discover if available
set (PROJECT_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
if (EXISTS "${PROJECT_CMAKE_DIR}/UrhoDiscover.cmake")
    include (${PROJECT_CMAKE_DIR}/UrhoDiscover.cmake)
endif ()
# Check for results
if (NOT URHO3D_HOME)
    urho_get_num_found_dirs (NUM_DIRS_FOUND)
    if (URHO3D_HOME OR NUM_DIRS_FOUND EQUAL 1) # One result, use it directly
        set (URHO3D_HOME "${${PROJECTNAME}_URHO3D_DIRS}")
        message (STATUS "Use URHO3D_HOME=${URHO3D_HOME} ...")
    endif ()
    # More than one result: let the developer selects manually via cmake-gui.
    if (NUM_DIRS_FOUND GREATER 1)
        message (FATAL_ERROR "NUM_DIRS_FOUND > 1")
        if (NOT ANDROID)
            message (STATUS "Found ${NUM_DIRS_FOUND} Urho3D folders. Please choose one with cmake-gui.")
            return ()
        elseif (DEFINED ENV{URHO3D_HOME})
            get_filename_component (home ${ENV{URHO3D_HOME}}/android DIRECTORY)
            if (EXISTS "${home}/android/urho3d-lib/.cxx/${URHO3D_LIBTYPE_LOWER}") # For Android, we reduce to ENV{URHO3D_HOME} result.
                set (URHO3D_HOME ${home}) 
                set (BUILD_STAGING_DIR ${home}/android/urho3d-lib/.cxx/${URHO3D_LIBTYPE_LOWER})
            endif ()
        endif ()
    elseif (NOT URHO3D_HOME) # Fetch from u3d-community if the fetch condition authorizes it
        message (STATUS "Fetch DEFAULT_GIT_U3D_REPOSITORY ...")
        urho_fetch_git ("${DEFAULT_GIT_U3D_REPOSITORY}" "${DEFAULT_GIT_U3D_TAG}")
    endif ()
endif ()
# Stop here, if u3d directories are not found.
if (NOT URHO3D_HOME)
    message ("!! Could not find Urho3D content. Please set URHO3D_HOME manually and try again.")
    return ()
endif ()
# At this point, URHO3D_HOME is defined.

## FINAL PART: configure variables.
# Set a user-provided CMake module path or use the default path.
if (${PROJECTNAME}_URHO3D_CMAKE_MODULE AND NOT "${${PROJECTNAME}_URHO3D_CMAKE_MODULE}" STREQUAL "${URHO3D_CMAKE_MODULE}")
    set (URHO3D_CMAKE_MODULE "${${PROJECTNAME}_URHO3D_CMAKE_MODULE}")
endif ()
if (NOT URHO3D_CMAKE_MODULE)
    if (EXISTS "${PROJECT_CMAKE_DIR}/Modules/UrhoCommon.cmake")
        set (URHO3D_CMAKE_MODULE ${PROJECT_CMAKE_DIR}/Modules)
    elseif (EXISTS "${URHO3D_ROOT_DIR}/cmake/Modules/UrhoCommon.cmake")
        set (URHO3D_CMAKE_MODULE ${URHO3D_ROOT_DIR}/cmake/Modules)
    endif ()
endif ()
if (URHO3D_CMAKE_MODULE)
    set (URHO3D_CMAKE_MODULE ${URHO3D_CMAKE_MODULE} CACHE INTERNAL STRING)
    set (${PROJECTNAME}_URHO3D_CMAKE_MODULE ${URHO3D_CMAKE_MODULE} CACHE PATH "Path to Urho3D CMake modules for all Urho3D sources" FORCE)
endif ()
list (REMOVE_ITEM CMAKE_MODULE_PATH ${URHO3D_CMAKE_MODULE})
list (PREPEND CMAKE_MODULE_PATH ${URHO3D_CMAKE_MODULE})
# Set project install prefix path.
set (${PROJECTNAME}_INSTALL_PREFIX "" CACHE STRING "${CMAKE_PROJECT_NAME} install prefix added to the global CMake install prefix path.")
# Determine the origin of Urho3D based on URHO3D_HOME.
# Define the following variables: URHO3D_ROOT_DIR, URHO3D_SOURCE_DIR, URHO3D_BUILD_DIR, URHO3D_AS_SUBMODULE, URHO3D_CMAKE_MODULE.
# These variables ensure separation between the Urho3D build and the user project.
urho_find_origin ("${URHO3D_HOME}" URHO3D_ROOT_DIR URHO3D_SOURCE_DIR origin)
if (NOT origin)
    message (FATAL_ERROR "!!! The Urho3D path ${URHO3D_HOME} is invalid !")
endif ()
if (origin STREQUAL "source")
    set (URHO3D_AS_SUBMODULE TRUE CACHE INTERNAL BOOLEAN)
    set (URHO3D_BUILD_DIR ${CMAKE_BINARY_DIR}/${URHO3D_TARGET_LOWER})
    add_subdirectory (${URHO3D_ROOT_DIR} ${URHO3D_BUILD_DIR}) # include Urho3D sources
else ()
    set (URHO3D_AS_SUBMODULE FALSE CACHE INTERNAL BOOLEAN)
    unset (URHO3D_BUILD_DIR)
endif ()
if (NOT URHOCOMMON_INUSE) # include UrhoCommon for the main user project if not already used.
    include (${URHO3D_CMAKE_MODULE}/UrhoCommon.cmake)
endif ()
if (ANDROID AND URHO3D_AS_SUBMODULE AND NOT EXISTS "${CMAKE_SOURCE_DIR}/app/src/main/java/io/urho3d/UrhoActivity.kt") # install java source sets for the engine
    create_symlink(${URHO3D_ROOT_DIR}/android/urho3d-lib/src/main/java/io/urho3d/UrhoActivity.kt ${CMAKE_SOURCE_DIR}/app/src/main/java/io/urho3d/UrhoActivity.kt)
    create_symlink(${URHO3D_ROOT_DIR}/Source/ThirdParty/SDL/android-project/app/src/main/java/org ${CMAKE_SOURCE_DIR}/app/src/main/java/org)
endif ()
