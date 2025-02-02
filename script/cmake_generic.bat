::
:: Copyright (c) 2008-2022 the Urho3D project.
::
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
::
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
:: THE SOFTWARE.
::

@echo off

:: Determine source tree and build tree
setlocal
set "SOURCE=%~dp0"
set "SOURCE=%SOURCE:~0,-1%\.."
set "SOURCE=%cd%"
set "BUILD="
if "%~1" == "" goto :continue
set "ARG1=%~1"
if "%ARG1:~0,1%" equ "-" goto :continue
set "BUILD=%~1"
shift
:continue
if "%BUILD%" == "" if exist "%cd%\CMakeCache.txt" (set "BUILD=%cd%") else (echo Usage: %~nx0 \path\to\build-tree [build-options] && exit /B 1)

:: Detect CMake toolchains directory if it is not provided explicitly
if "%TOOLCHAINS%" == "" set "TOOLCHAINS=%SOURCE%\CMake\Toolchains"
if not exist "%TOOLCHAINS%" if exist "%URHO3D_HOME%\share\CMake\Toolchains" set "TOOLCHAINS=%URHO3D_HOME%\share\CMake\Toolchains"

:: Default to native generator and toolchain if none is specified explicitly
set "OPTS="
set "BUILD_OPTS="
set "arch="
:loop
:: Cache the first argument so substring operation can be performed on it
set "ARG1=%~1"
if not "%~1" == "" (
    if "%~1" == "-D" (
        if "%~2" == "MINGW" if "%~3" == "1" set "OPTS=-G "MinGW Makefiles""
        if "%~2" == "URHO3D_64BIT" if "%~3" == "1" set "arch=-A x64"
        if "%~2" == "URHO3D_64BIT" if "%~3" == "0" set "arch=-A Win32"
        set "BUILD_OPTS=%BUILD_OPTS% -D %~2=%~3"
        shift
        shift
        shift
    ) else if "%ARG1:~0,2%" == "-D" (
        :: Handle case where there is no space after "-D" in the options
        set "BUILD_OPTS=%BUILD_OPTS% %~1=%~2"
        shift
        shift
    )
    if "%~1" == "-VS" (
        set "OPTS=-G "Visual Studio %~2" %arch% %TOOLSET%"
        shift
        shift
    )
    if "%~1" == "-G" (
        :: Add quote to MinGW Makefiles flag since Emscripten's emcmake emits them without quotes and this breaks cmake
        if "%~2" == "MinGW Makefiles" (
            set "OPTS=%OPTS% -G "%~2""
        ) else (
            set "OPTS=%OPTS% -G %~2"
        )
        shift
        shift
    )
    goto loop
)
if exist "%BUILD%\CMakeCache.txt" set "OPTS="

:: Create project with the chosen CMake generator and toolchain
cmake -E make_directory "%BUILD%" && cmake -E chdir "%BUILD%" cmake %OPTS% %BUILD_OPTS% "%SOURCE%"
