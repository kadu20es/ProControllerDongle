@echo off
REM Compila o firmware do Pro Controller Dongle.
REM   tools\build.cmd            -> debug (com porta serial de log), pasta firmware\build
REM   tools\build.cmd release    -> final (sem serial; descritor igual ao do controle original), pasta firmware\build-release
REM   tools\build.cmd [release] clean   -> limpa a pasta de build antes
REM
REM Ferramentas: veja docs\COMPILAR.md. Caminhos locais ficam em tools\env.cmd (copie de tools\env.example.cmd).
setlocal
if exist "%~dp0env.cmd" call "%~dp0env.cmd"

REM compilador C/C++ do PC (so para o pioasm do SDK): MSVC via vswhere, se ainda nao estiver no PATH
where cl >nul 2>nul
if not errorlevel 1 goto have_cl
if defined VCVARS goto use_vcvars
call :find_vcvars
:use_vcvars
if defined VCVARS call "%VCVARS%" >nul
:have_cl
if defined NINJA_DIR set "PATH=%PATH%;%NINJA_DIR%"
if defined ARM_GCC_BIN set "PATH=%PATH%;%ARM_GCC_BIN%"
if defined PICOTOOL_DIR (
  set "PATH=%PATH%;%PICOTOOL_DIR%"
  set "picotool_DIR=%PICOTOOL_DIR%"
)
if not defined PICO_SDK_PATH (
  echo ERRO: defina PICO_SDK_PATH ^(Pico SDK 2.3.1^) em tools\env.cmd ou no ambiente.
  exit /b 1
)

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
goto :eof

:find_vcvars
set "VSINSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%VSINSTALLER%\vswhere.exe" goto :eof
pushd "%VSINSTALLER%"
for /f "usebackq delims=" %%i in (`.\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
popd
if defined VSDIR set "VCVARS=%VSDIR%\VC\Auxiliary\Build\vcvars64.bat"
goto :eof
