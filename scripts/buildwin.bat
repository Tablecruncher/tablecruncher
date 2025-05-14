@echo off
setlocal

:: Read version
set /p VERSION=<version.txt

:: Update icon using Resource Hacker
ResourceHacker.exe -open Tablecruncher.exe -save Tablecruncher_patched.exe -action addoverwrite -res ..\assets\windows\app_icon.ico -mask ICONGROUP,MAINICON,

:: Replace original if patching succeeded
if exist Tablecruncher_patched.exe (
    move /Y Tablecruncher_patched.exe Tablecruncher.exe
) else (
    echo Icon patching failed.
    exit /b 1
)

:: Compile Inno Setup with version macro
ISCC.exe /DMyAppVersion=%VERSION% win-setup.iss
