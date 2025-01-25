#
# Copyright (c) 2008-2017 the Urho3D project.
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
# Define target name
set (TARGET_NAME Urho3D)
# Prevent ICE from old MinGW versions, workaround from the GCC bugzilla https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61189
if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1 AND NOT URHO3D_64BIT)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpreferred-stack-boundary=2")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mpreferred-stack-boundary=2")
endif ()
check_include_files (stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
  add_definitions (-DHAVE_STDINT_H)
endif ()

# Save keep the preprocessor macros (for using the Urho3D library) for later use in generating Urho3D.pc file
get_directory_property (URHO3D_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
# Define preprocessor macros (for building the Urho3D library)
add_definitions (-DURHO3D_IS_BUILDING)
if (ANDROID AND DEFINED ENV{ASSET_DIR_INDICATOR})
    add_definitions (-DASSET_DIR_INDICATOR=\"$ENV{ASSET_DIR_INDICATOR}\")
endif ()
if ($ENV{COVERITY_SCAN_BRANCH})
    # This macro is used when scanning Urho3D library via Coverity-Scan
    # TODO: The Coverity-Scan modelling is not yet working properly (anyone interested in static analyzer is welcome to give it another try)
    add_definitions (-DCOVERITY_SCAN_MODEL)
endif ()
if (TARGET GLEW)
    # These macros are required because Urho3D (OpenGL) headers are exposed to GLEW headers
    add_definitions (-DGLEW_STATIC -DGLEW_NO_GLU)
endif ()
if (HAVE_SINCOSF)
    add_definitions (-DHAVE_SINCOSF)
elseif (HAVE___SINCOSF)
    add_definitions (-DHAVE___SINCOSF)
endif ()
# Enable/disable SIMD instruction set for STB image
if (NEON)
    if (XCODE)
        if (IOS)
            set (SDK iphoneos)
        elseif (TVOS)
            set (SDK appletvos)
        endif ()
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CFLAGS)")
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CPLUSPLUSFLAGS)")
    else ()
        add_definitions (-DSTBI_NEON)
    endif ()
elseif (MSVC AND NOT URHO3D_SSE)
    add_definitions (-DSTBI_NO_SIMD)
elseif (MINGW AND NOT URHO3D_64BIT AND URHO3D_SSE)
    add_definitions (-DSTBI_MINGW_ENABLE_SSE2)
endif ()

if (ODBC_VERSION AND NOT ODBC_VERSION VERSION_LESS 3)
    add_definitions (-DODBC_3_OR_LATER)
endif ()
if (WIN32)
    set (CMAKE_DEBUG_POSTFIX _d)
    if (NOT MINGW OR HAVE_RTL_OSVERSIONINFOW)
        add_definitions (-DHAVE_RTL_OSVERSIONINFOW)
    endif ()
endif ()
# Define source files
foreach (DIR Urho2D WebP)
    string (TOUPPER URHO3D_${DIR} OPT)
    if (NOT ${OPT})
        list (APPEND EXCLUDED_SOURCE_DIRS ${DIR})
    endif ()
endforeach ()
if (URHO3D_DATABASE)
    if (URHO3D_DATABASE_ODBC)
        list (APPEND EXCLUDED_SOURCE_DIRS Database/SQLite)
    else ()
        list (APPEND EXCLUDED_SOURCE_DIRS Database/ODBC)
    endif ()
else ()
    list (APPEND EXCLUDED_SOURCE_DIRS Database)
endif ()
if (URHO3D_OPENGL)
    # Exclude the opposite source directory
    list (APPEND EXCLUDED_SOURCE_DIRS Graphics/Direct3D9 Graphics/Direct3D11)
else ()
    list (APPEND EXCLUDED_SOURCE_DIRS Graphics/OpenGL)
    if (URHO3D_D3D11)
        list (APPEND EXCLUDED_SOURCE_DIRS Graphics/Direct3D9)
    else ()
        list (APPEND EXCLUDED_SOURCE_DIRS Graphics/Direct3D11)
    endif ()
endif ()
if (APPLE AND NOT ARM)
    set (GLOB_OBJC_PATTERN *.m)     # Should only pick up MacFileWatcher.m for macOS platform at the moment
endif ()
string (REPLACE ";" "/[^;]+;" EXCLUDE_PATTERNS "${EXCLUDED_SOURCE_DIRS};")
define_source_files (EXCLUDE_PATTERNS ${EXCLUDE_PATTERNS} GLOB_CPP_PATTERNS *.cpp ${GLOB_OBJC_PATTERN} RECURSE GROUP)
# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P cmake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${URHO3D_ROOT_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()
set (URHO3D_DEPS ${STATIC_LIBRARY_TARGETS})

set_source_files_properties (${SYMBOLIC_SOURCES} PROPERTIES SYMBOLIC TRUE)
add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/librevision.h
    COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new -P cmake/Modules/GetUrhoRevision.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new ${CMAKE_CURRENT_BINARY_DIR}/librevision.h
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new
    DEPENDS ${URHO3D_DEPS} ${URHO3D_ROOT_DIR}/cmake/Modules/GetUrhoRevision.cmake
    WORKING_DIRECTORY ${URHO3D_ROOT_DIR}
    COMMENT "")     # Suppress the comment as the header file may or may not be re-generated
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)

# Aggregate all source files
list (APPEND SOURCE_FILES ${GEN_CPP_FILES} ${LUA_GEN_CPP_FILES} librevision.h Urho3D.h ${SYMBOLIC_SOURCES})
# Define dependency libs
# Add include directories to find the precompiled header, export header, and installed headers from third-party libs
set (INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${THIRD_PARTY_INCLUDE_DIR})

# Workaround for GCC 5.4 and above when building a SHARED lib type for Linux platform to fix the undefined symbol "__cpu_model" issue (see #1519)
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.3.1 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND URHO3D_LIB_TYPE STREQUAL SHARED)  # 5.3.1 was the last known good version
    list (APPEND LIBS gcc)
endif ()
# Setup library output path
set_output_directories (${URHO3D_BUILD_DIR}/lib ARCHIVE LIBRARY)
# Setup target
if (IOS OR TVOS)
    set (EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
endif ()
setup_library (${URHO3D_LIB_TYPE} ${EXCLUDE_FROM_ALL})
# Generate platform specific export header file (can only be called after the target has been added)
if (MSVC)
    set (APPENDIX "\n#pragma warning(disable: 4251)\n#pragma warning(disable: 4275)\n\n#if _MSC_VER < 1900\n#define strtoll _strtoi64\n#define strtoull _strtoui64\n#endif\n")
endif ()
if (URHO3D_CLANG_TOOLS)
    set (ANNOTATE_NONSCRIPTABLE "__attribute__((annotate(\"nonscriptable\")))")
endif ()
set (APPENDIX "${APPENDIX}\n#define NONSCRIPTABLE ${ANNOTATE_NONSCRIPTABLE}\n\n")
foreach (DEFINE URHO3D_STATIC_DEFINE URHO3D_VULKAN URHO3D_OPENGL URHO3D_D3D11 URHO3D_SSE URHO3D_DATABASE_ODBC URHO3D_DATABASE_SQLITE URHO3D_TESTING CLANG_PRE_STANDARD)
    if (${DEFINE})
		# TO SKIP WARNING WITH THE DEFINE STATIC
		set (APPENDIX "${APPENDIX}#ifndef ${DEFINE}\n#define ${DEFINE}\n#endif\n")
    endif ()
endforeach()
if (URHO3D_VULKAN AND URHO3D_VOLK)
	set (APPENDIX "${APPENDIX}#ifndef URHO3D_VOLK\n#define URHO3D_VOLK\n#endif\n")
endif ()
if (URHO3D_FORCE_AS_MAX_PORTABILITY)
    set (APPENDIX "${APPENDIX}#define AS_MAX_PORTABILITY\n")
endif ()
generate_export_header (${TARGET_NAME} EXPORT_MACRO_NAME URHO3D_API EXPORT_FILE_NAME Urho3D.h.new CUSTOM_CONTENT_FROM_VARIABLE APPENDIX)
file (READ Precompiled.h COPYRIGHT LIMIT 1128)
file (READ ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h.new EXPORT_HEADER)
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h.new ${COPYRIGHT}${EXPORT_HEADER})
execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h.new ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h)
file (REMOVE ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h.new)
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h)
# Precompile the header after the export header file has been generated
enable_pch (Precompiled.h)
if (NOT ANDROID AND NOT WEB)
    file (READ .soversion SOVERSION)
    string (STRIP ${SOVERSION} SOVERSION)
    string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED ${SOVERSION})
    if (MATCHED)
        set_target_properties (${TARGET_NAME} PROPERTIES VERSION ${MATCHED} SOVERSION ${CMAKE_MATCH_1})
    else ()
        message (FATAL_ERROR "The .soversion file is corrupted. It should contain a version number with this format major(0xFFFF).minor(0xFF).patch-level(0xFF). e.g.: 0.1.2")
    endif ()
else ()
    set_target_properties (${TARGET_NAME} PROPERTIES NO_SONAME TRUE)
endif ()
# TODO: to use 'xcframework' as Apple discourages the use of 'lipo' to create universal binary combining libs/frameworks from different SDK
# As of Xcode 12 the 'iphonesimulator' SDK produces binary for both 'x86_64' and 'arm64' archs
# This has caused problem with 'lipo' to combine the already FAT lib from 'iphonesimulator' SDK with 'iphoneos' SDK containing 'arm64' arch
# Temporary workaround by not generating the universal binary when the issue is detected
if (IOS)
    # Add a custom target to build Mach-O universal binary consisting of both iPhoneOS and iPhoneSimulator SDKs
    add_custom_target (${TARGET_NAME}_universal ALL
        COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both iphoneos and iphonesimulator SDKs; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |egrep -cq x86_64; then sdk=iphoneos; else sdk=iphonesimulator; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.$$sdk} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.$$sdk 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'; then echo 'Skip on Xcode 12 due to arch conflict' && mv $<TARGET_FILE:${TARGET_NAME}>{.ori,} && rm $<TARGET_FILE:${TARGET_NAME}>.$$sdk; else lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.$$sdk} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.$$sdk}; fi; fi; fi
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${URHO3D_BUILD_DIR}
        COMMENT "Creating Mach-O universal binary library consisting of both iPhoneOS and iPhoneSimulator SDks")
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
elseif (TVOS)
    # Add a custom target to build Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator SDKs
    add_custom_target (${TARGET_NAME}_universal ALL
        COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator SDKs; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |grep -cq x86_64; then sdk=appletvos; else sdk=appletvsimulator; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.$$sdk} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.$$sdk 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'; then echo 'Skip on Xcode 12 due to arch conflict' && mv $<TARGET_FILE:${TARGET_NAME}>{.ori,} && rm $<TARGET_FILE:${TARGET_NAME}>.$$sdk; else lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.$$sdk} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.$$sdk}; fi; fi; fi
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${URHO3D_BUILD_DIR}
        COMMENT "Creating Mach-O universal binary library consisting of both AppleTVOS and AppleTVSimulator SDKs")
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
    # TODO: Temporary workaround as CMake does not recognize AppleTV platform yet
    add_dependencies (${TARGET_NAME} APPLETV_POST_CMAKE_FIX)
elseif (EMSCRIPTEN)
    install (TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
else ()
    install (TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${DEST_RUNTIME_DIR} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
endif ()
if (WIN32 AND URHO3D_LIB_TYPE STREQUAL SHARED)
    set (URHO3D_DLL_INSTALLED TRUE)
endif ()
# TODO: This seems out of place
if (MINGW)
    set (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    if (URHO3D_LIB_TYPE STREQUAL SHARED)
        set_target_properties (${TARGET_NAME} PROPERTIES PREFIX "")
endif ()
endif ()
# Setup dependency libraries to be linked or archived as a whole inside Urho3D library for SHARED or STATIC lib type, respectively
if (APPLE)
    set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES -force_load)
elseif (MSVC)
    foreach (THIRD_PARTY_LIB ${STATIC_LIBRARY_TARGETS})
        if (NOT ${THIRD_PARTY_LIB} MATCHES boost|rapidjson|STB)  # Exclude header-only "fake" static libs
            # Must be specified for each library https://github.com/urho3d/Urho3D/issues/2760
            set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_OPTIONS /WHOLEARCHIVE:${THIRD_PARTY_LIB})  # Require VS 2015 Update 2 onward
        endif ()
    endforeach ()
    set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_OPTIONS /FORCE)
else ()
    set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES -Wl,--whole-archive)
endif ()
foreach (TARGET ${STATIC_LIBRARY_TARGETS})
    set (ARCHIVE $<TARGET_FILE:${TARGET}>)
    if (NOT ${TARGET} MATCHES boost|rapidjson|STB|vma|volk)  # Exclude header-only "fake" static lib targets
        set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES ${ARCHIVE})      # Should work for all compiler toolchains while building shared library
        if (URHO3D_LIB_TYPE STREQUAL STATIC)
            if (XCODE OR MSVC)  # Only works in Xcode and MSVC while building static library
                set_property (TARGET ${TARGET_NAME} APPEND PROPERTY STATIC_LIBRARY_OPTIONS ${ARCHIVE})
            else ()
                list (APPEND ARCHIVES ${ARCHIVE})   # Accumulate the target locations for archiving later
            endif ()
        endif ()
    endif ()
endforeach ()
if (NOT XCODE AND NOT MSVC)
    if (NOT APPLE)
        set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES -Wl,--no-whole-archive)  # GCC/Clang-specific to pair up with whole-archive linker option
    endif ()
    if (URHO3D_LIB_TYPE STREQUAL STATIC)
        if (APPLE)
            # Apple libtool takes archives directly as input
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND mv $<TARGET_FILE:${TARGET_NAME}>{,.engine}
                COMMAND libtool -static $<TARGET_FILE:${TARGET_NAME}>.engine ${ARCHIVES} -o $<TARGET_FILE:${TARGET_NAME}>
                COMMAND rm $<TARGET_FILE:${TARGET_NAME}>.engine
                COMMENT "Merging all archives into a single static library using libtool")
        elseif (EMSCRIPTEN OR CMAKE_BINARY_DIR MATCHES "[ +*;,]")
            # Do it the hard way by first extracting the object (bitcode) files and appending them to final archive:
            # a) For Emscripten build as Emscripten emar (llvm-ar) neither takes archives directly as input nor supports MRI-style script
            # b) When the build tree path contains special characters for MRI scripts (space, '+', '*', ';', ',') as escaping is not supported
            get_filename_component (AR ${CMAKE_AR} NAME_WE)
            if (CMAKE_HOST_WIN32)
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives
                    COMMAND ${CMAKE_COMMAND} -E echo "Merging all archives into a single static library using ${AR}")
                foreach (ARCHIVE ${ARCHIVES})
                    # Merge per library because Window cmd shell does not handle long argument list
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory archives)
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_AR} x ${ARCHIVE}
                        COMMAND ${CMAKE_COMMAND} -E echo_append "\\\"${CMAKE_AR}\\\"" q \\\"$<TARGET_FILE:${TARGET_NAME}>\\\" " " >merge.bat
                        COMMAND for /F %%i in ('dir /B *.o*') do ${CMAKE_COMMAND} -E echo_append %%i" " >>merge.bat
                        COMMAND merge.bat WORKING_DIRECTORY archives)
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove_directory archives)
                endforeach ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:${TARGET_NAME}>)
            else ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives
                    COMMAND ${CMAKE_COMMAND} -E make_directory archives COMMENT "Merging all archives into a single static library using ${AR}")
                foreach (ARCHIVE ${ARCHIVES})
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_AR} x ${ARCHIVE} WORKING_DIRECTORY archives)
                endforeach ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_AR} q $<TARGET_FILE:${TARGET_NAME}> `ls -1` WORKING_DIRECTORY archives)
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:${TARGET_NAME}>
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives)
            endif ()
        else ()
            # GCC ar does not take archives directly as input like Apple libtool, however, it can be scripted to do so
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.engine
                COMMAND echo CREATE $<TARGET_FILE:${TARGET_NAME}> >script.ar
                COMMAND echo ADDLIB $<TARGET_FILE:${TARGET_NAME}>.engine >>script.ar)
            foreach (ARCHIVE ${ARCHIVES})
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND echo ADDLIB ${ARCHIVE} >>script.ar)
            endforeach ()
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND echo SAVE >>script.ar
                COMMAND echo END >>script.ar
                COMMAND ${CMAKE_AR} -M <script.ar
                COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${TARGET_NAME}>.engine script.ar
                COMMENT "Merging all archives into a single static library using ar")
        endif ()
    endif ()
endif ()
# Use PIC on platforms that support it (shared library type has this property set to true by default, so we only have to deal with those static ones that the shared library links against)
if (NOT URHO3D_LIB_TYPE STREQUAL STATIC)
    set_target_properties (${STATIC_LIBRARY_TARGETS} PROPERTIES POSITION_INDEPENDENT_CODE true)
endif ()
# Install headers for using the Urho3D library
install_header_files (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h DESTINATION ${DEST_INCLUDE_DIR} ACCUMULATE GENERATED_HEADERS)
install_header_files (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h ACCUMULATE URHO_HEADERS)    # Note: the trailing slash is significant
# Generate the include-all-headers header file even though we do not encourage Urho3D library users to use it
list (SORT URHO_HEADERS)
list (REVERSE URHO_HEADERS)
set (OPTIONAL_SUBS Database Urho2D)
foreach (SUB ${OPTIONAL_SUBS})
    if (URHO_HEADERS MATCHES "(include/Urho3D/${SUB}[^;]+)")
        list (FIND URHO_HEADERS ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT URHO_HEADERS ${FOUND_INDEX} "#endif")
    endif ()
endforeach ()
list (REVERSE URHO_HEADERS)
foreach (SUB ${OPTIONAL_SUBS})
    if (URHO_HEADERS MATCHES "(include/Urho3D/${SUB}[^;]+)")
        string (TOUPPER ${SUB} SUB)
        list (FIND URHO_HEADERS ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT URHO_HEADERS ${FOUND_INDEX} "#if URHO3D_${SUB}")
    endif ()
endforeach ()
string (REGEX REPLACE "include/[^;]+(DebugNew|Direct3D|ODBC|OpenGL|Precompiled|SQLite|ToluaUtils|Urho3D|librevision)[^;]+;" "" URHO_HEADERS "${URHO_HEADERS};")
string (REGEX REPLACE "include/([^;]+)" "#include <\\1>" URHO_HEADERS "${GENERATED_HEADERS};${URHO_HEADERS}")
string (REPLACE ";" \n URHO_HEADERS "${URHO_HEADERS}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3DAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h)
install_header_files (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h ${CMAKE_CURRENT_BINARY_DIR}/librevision.h DESTINATION ${DEST_INCLUDE_DIR})
# Generate platform specific pkg-config file for the benefit of Urho3D library users via SDK without CMake
if (ABSOLUTE_PATH_LIBS)
    string (REPLACE ";" "\" \"" URHO3D_ABS_PATH_LIBS "\"${ABSOLUTE_PATH_LIBS}\"")   # Note: need to always "stringify" a variable in list context for replace to work correctly, besides the list could be empty
    string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" URHO3D_ABS_PATH_LIBS "${URHO3D_ABS_PATH_LIBS}")
endif ()
if (MSVC)
    set (DASH /)
    set (LIB_DIR "/LIBPATH:\"\${libdir}\"")
    string (REPLACE ";" ".lib\" \"" URHO3D_LIBS "\"@LIB_NAME@\" \"${LIBS}.lib\"")
    set (PC_SUFFIX .msvc)
else ()
    set (LIB_NAME ${TARGET_NAME})
    if (MINGW)
        if (CMAKE_BUILD_TYPE STREQUAL Debug)
            set (LIB_NAME ${LIB_NAME}_d)
        endif ()
        if (URHO3D_LIB_TYPE STREQUAL SHARED)
            set (LIB_NAME ${LIB_NAME}.dll)
        endif ()
    endif ()
    set (DASH -)
    set (LIB_DIR "-L\"\${libdir}\"")
    string (REPLACE ";" " -l" URHO3D_LIBS "-l${LIB_NAME};${LIBS}")
endif ()
string (REPLACE ";" " ${DASH}D" URHO3D_COMPILE_DEFINITIONS "${DASH}D${URHO3D_COMPILE_DEFINITIONS}")
get_directory_property (GLOBAL_INCLUDE_DIRS DIRECTORY ${URHO3D_ROOT_DIR} INCLUDE_DIRECTORIES)
if (GLOBAL_INCLUDE_DIRS)
    string (REPLACE ";" "\" ${DASH}I\"" GLOBAL_INCLUDE_DIRS "${DASH}I\"${GLOBAL_INCLUDE_DIRS}\"")
    string (REPLACE "${SYSROOT}" "" GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS})
endif ()
set (ENGINE_INCLUDE_DIRS "${DASH}I\"\${includedir}\" ${DASH}I\"\${includedir}/Urho3D/ThirdParty\"")

string (REGEX REPLACE " -include \"[^\"]+\"" "" CLEANED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_CXX_FLAGS "${CLEANED_CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3D.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.pc${PC_SUFFIX} @ONLY)
if (MSVC)
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -DLIB_NAME=$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}> -P ${URHO3D_ROOT_DIR}/cmake/Modules/AdjustPkgConfigForMSVC.cmake)
endif ()
if (ANDROID)
    # After migration to Gradle build system we do not support Android cross-compiling build via pkgconfig anymore, however, this file is still being provided as reference only
    set (RENAME RENAME Urho3D-${ANDROID_ABI}.pc)
endif ()
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.pc DESTINATION ${DEST_PKGCONFIG_DIR} ${RENAME})
# Define post build steps
if (URHO3D_LIB_TYPE STREQUAL SHARED AND ARM)
    # Strip the output shared library for embedded ARM platforms (we don't do this for Android anymore, assuming Android plugin will take care of it)
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>
        COMMENT "Stripping Urho3D shared library")
endif ()
if (URHO3D_SCP_TO_TARGET)
    # Ensure SCP is the last command
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND scp $<TARGET_FILE:${TARGET_NAME}> ${URHO3D_SCP_TO_TARGET} || exit 0
        COMMENT "Scp-ing Urho3D library to target system")
endif ()

