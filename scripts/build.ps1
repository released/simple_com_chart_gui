[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer / vswhere not found.' }
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
if (-not $vsRoot) { throw 'Install Visual Studio C++ desktop tools with MFC (x64).' }
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
$buildPath = Join-Path $projectRoot 'build\obj\cmake'
& $cmake -S $projectRoot -B $buildPath -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE) { throw 'CMake configure failed.' }
& $cmake --build $buildPath --config $Configuration --parallel
if ($LASTEXITCODE) { throw 'C++ build failed.' }
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
& $ctest --test-dir $buildPath -C $Configuration --output-on-failure
if ($LASTEXITCODE) { throw 'Native telemetry regression tests failed.' }
Write-Host "PASS: $Configuration x64. Release EXE: $projectRoot\build\SimpleComChart.exe"
