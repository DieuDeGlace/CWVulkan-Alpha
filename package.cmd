@echo off
setlocal
cd /d "%~dp0"
call "%~dp0build.cmd" || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\package.ps1"
exit /b %errorlevel%
