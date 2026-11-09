@echo off
rem Builds the 32-bit Alpha extractor mod (build\alpha-extractor\AlphaExtractor.dll).
rem Copy the DLL into Alpha's Mods folder to use it; see tools\alpha-extractor\README.md.
setlocal
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" set "VSDIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
call "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
cd /d "%~dp0"
set "OUT=..\..\build\alpha-extractor"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /O2 /MT /EHa /W4 /DUNICODE /D_UNICODE /LD AlphaExtractor.cpp /Fo"%OUT%\\" /Fe"%OUT%\AlphaExtractor.dll" /link user32.lib || exit /b 1
