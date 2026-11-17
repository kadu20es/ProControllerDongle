@echo off
REM Copie este arquivo para tools\env.cmd (ignorado pelo git) e ajuste os caminhos da sua maquina.
REM Obrigatorio:
set "PICO_SDK_PATH=C:\pico\pico-sdk"
REM Opcionais (se ja estiverem no PATH, pode apagar):
set "ARM_GCC_BIN=C:\pico\arm-gnu-toolchain\bin"
set "NINJA_DIR=C:\pico\ninja"
set "PICOTOOL_DIR=C:\pico\picotool"
REM set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
