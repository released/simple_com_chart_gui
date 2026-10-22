[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release', [switch]$Font125,
    [ValidateSet('All', 'Serial', 'Startup')][string]$Scope = 'All',
    [ValidateSet('Messages', 'Chart', '4Charts')][string]$StartupTab = 'Chart')

$ErrorActionPreference = 'Stop'
$StartupTab = @{Messages = 'Messages'; Chart = 'Chart'; '4Charts' = '4Charts'}[$StartupTab]
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = if ($Configuration -eq 'Release') { Join-Path $projectRoot 'build\SimpleComChart.exe' } else { Join-Path $projectRoot 'build\Debug\SimpleComChart.exe' }
$fixtureName = if ($Font125) { "$Configuration-font125" } else { $Configuration }
if ($Scope -eq 'Serial') { $fixtureName += '-serial' }
if ($Scope -eq 'Startup') { $fixtureName += "-startup-$StartupTab" }
$fixtureRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot "build\verify\$fixtureName"))
if (-not $fixtureRoot.StartsWith($projectRoot + '\build\verify\', [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe runtime fixture path.' }
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
$fixtureExe = Join-Path $fixtureRoot 'SimpleComChart.exe'
Copy-Item -LiteralPath $source -Destination $fixtureExe -Force
$report = Join-Path $fixtureRoot 'smoke-result.txt'
if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report -Force }
$testArguments = if ($Scope -eq 'Serial') { '--smoke-test=serial' } else { '--smoke-test' }
if ($Scope -eq 'Startup') {
    # This is an isolated, test-owned INI; never copy or edit the personal delivery INI.
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),
        "[UI]`r`nStartupTab=$StartupTab`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = "--smoke-test=startup --expected-startup=$StartupTab"
}
if ($Font125) { $testArguments += ' --font125' }
$process = Start-Process -FilePath $fixtureExe -ArgumentList $testArguments -WorkingDirectory $fixtureRoot -WindowStyle Hidden -PassThru
# Three-page repaint/screenshot and settings/raw-message regressions need a bounded 30 s budget.
if (-not $process.WaitForExit(30000)) {
    $process.Kill()
    throw 'MFC startup smoke test timed out (check for an assertion or startup dialog).'
}
if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $report)) { throw "MFC runtime failed. Exit: $($process.ExitCode)" }
Get-Content -LiteralPath $report
if ((Get-Content -LiteralPath $report -Raw) -notmatch '^PASS:') { throw 'Runtime smoke report did not pass.' }
