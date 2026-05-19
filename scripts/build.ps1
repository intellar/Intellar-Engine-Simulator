# Build Intellar-Engine-Simulator (CMake + SDL2 FetchContent)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$cmake = "C:\Program Files\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) { $cmake = "cmake" }

Set-Location $root
& $cmake -B build -DCMAKE_BUILD_TYPE=Release
& $cmake --build build --config Release
Write-Host ""
Write-Host "Run from repo root:"
Write-Host "  .\build\Release\engine_sim_demo.exe"
Write-Host ""
