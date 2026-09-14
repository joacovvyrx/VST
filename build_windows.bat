@echo off
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake no esta en PATH. Instala CMake y Visual Studio 2022 con Desktop development with C++.
  exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target VoxChoir_VST3
if errorlevel 1 exit /b 1

echo.
echo Build terminado.
echo VST3: build\VoxChoir_artefacts\Release\VST3\VoxChoir.vst3
endlocal
