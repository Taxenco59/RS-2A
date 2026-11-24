@echo off
REM Compila RS-2A (VST3, Windows x64). Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo para el escritorio con C++"), CMake y Git.
cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :error
cmake --build build --config Release --target RS2A_VST3 --parallel || goto :error
echo.
echo Listo: build\RS2A_artefacts\Release\VST3\RS-2A.vst3
echo Para instalarlo, copia esa carpeta a  C:\Program Files\Common Files\VST3\  (ejecutando como administrador)
exit /b 0
:error
echo ERROR en la compilacion.
exit /b 1
