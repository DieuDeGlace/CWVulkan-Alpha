@echo off
rem Configures and builds AlphaVulkan with the Visual Studio 2022 Build Tools.
setlocal
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" set "VSDIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "PATH=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cd /d "%~dp0"
if not defined ALPHAVULKAN_BUILD_DIR (
    set "ALPHAVULKAN_BUILD_DIR=build"
    if exist build-codex\build.ninja set "ALPHAVULKAN_BUILD_DIR=build-codex"
)
if not exist "%ALPHAVULKAN_BUILD_DIR%\build.ninja" "%CMAKE%" -S . -B "%ALPHAVULKAN_BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo || exit /b 1
"%CMAKE%" --build "%ALPHAVULKAN_BUILD_DIR%" %* || exit /b 1
