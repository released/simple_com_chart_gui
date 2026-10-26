[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release', [switch]$RuntimeChecks,
    [string]$TestRegex = '^telemetry_core$', [string[]]$ExtraTargets = @())

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer / vswhere not found.' }
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
if (-not $vsRoot) { throw 'Install Visual Studio C++ desktop tools with MFC (x64).' }
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
$buildPath = Join-Path $projectRoot $(if ($RuntimeChecks) { 'build\obj\cmake-runtime' } else { 'build\obj\cmake' })
$checks = if ($RuntimeChecks) { 'ON' } else { 'OFF' }
& $cmake -S $projectRoot -B $buildPath -G 'Visual Studio 17 2022' -A x64 "-DSCC_ENABLE_RUNTIME_CHECKS=$checks" -DSCC_OPTIMIZE_SIZE=ON
if ($LASTEXITCODE) { throw 'CMake configure failed.' }
if ($RuntimeChecks) {
    & $cmake --build $buildPath --config $Configuration --target simple_com_chart_gui_mfc --parallel
    if ($LASTEXITCODE) { throw 'Runtime verification build failed.' }
    Write-Host "PASS: $Configuration x64 verification EXE: $projectRoot\build\verify\runtime-bin\$Configuration\SimpleComChart.exe"
    return
}
$targets = @('simple_com_chart_gui_mfc', 'simple_com_chart_cli', 'telemetry_tests') + $ExtraTargets | Select-Object -Unique
& $cmake --build $buildPath --config $Configuration --target @targets --parallel
if ($LASTEXITCODE) { throw 'C++ build failed.' }
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
$delivery = Join-Path $projectRoot $(if ($Configuration -eq 'Release') { 'build\SimpleComChart.exe' } else { 'build\Debug\SimpleComChart.exe' })
. (Join-Path $PSScriptRoot 'test_artifacts.ps1')
$testRun = New-VerificationRun $projectRoot "$Configuration-core-tests" $delivery
$previousTestTemp = $env:TEMP
$previousTestTmp = $env:TMP
$testPassed = $false
$testFailure = 'Native core tests interrupted.'
try {
    $env:TEMP = $testRun.root
    $env:TMP = $testRun.root
    & $ctest --test-dir $buildPath -C $Configuration -R $TestRegex --output-on-failure --no-tests=error --timeout 120
    if ($LASTEXITCODE) { throw 'Selected native regression tests failed.' }
    $testPassed = $true
    $testFailure = ''
} catch {
    $testFailure = $_.Exception.Message
    throw
} finally {
    $env:TEMP = $previousTestTemp
    $env:TMP = $previousTestTmp
    Complete-VerificationRun $testRun $testPassed $testFailure
}
Write-Host "PASS: $Configuration x64 delivery EXE: $delivery"
