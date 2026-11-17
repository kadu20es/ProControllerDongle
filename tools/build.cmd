@echo off
REM Compila o firmware.
REM   tools\build.cmd            -> debug (com porta serial de log), pasta firmware\build
REM   tools\build.cmd release    -> final (sem CDC, descritor igual ao original), pasta firmware\build-release
REM   tools\build.cmd clean|release clean  -> limpa antes
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set PATH=%PATH%;%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;E:\Dev\pico-toolchain\arm-gnu\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin;E:\Dev\pico-toolchain\picotool\picotool
set PICO_SDK_PATH=E:\Dev\pico-toolchain\pico-sdk
set picotool_DIR=E:\Dev\pico-toolchain\picotool\picotool
cd /d "%~dp0..\firmware"
set BUILD=build
set OPTS=-DPROCON_RELEASE=OFF
set CLEAN=
:args
if "%1"=="" goto run
if /i "%1"=="release" (set BUILD=build-release& set OPTS=-DPROCON_RELEASE=ON)
if /i "%1"=="clean" set CLEAN=1
shift
goto args
:run
if defined CLEAN rmdir /s /q %BUILD% 2>nul
cmake -S . -B %BUILD% -G Ninja -DCMAKE_BUILD_TYPE=Release %OPTS% || exit /b 1
cmake --build %BUILD% || exit /b 1
dir %BUILD%\*.uf2
