[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = if ($Configuration -eq 'Release') { Join-Path $projectRoot 'build\SimpleComChart.exe' } else { Join-Path $projectRoot 'build\Debug\SimpleComChart.exe' }
$fixtureRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot "build\verify\$Configuration"))
if (-not $fixtureRoot.StartsWith($projectRoot + '\build\verify\', [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe runtime fixture path.' }
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
$fixtureExe = Join-Path $fixtureRoot 'SimpleComChart.exe'
Copy-Item -LiteralPath $source -Destination $fixtureExe -Force
$report = Join-Path $fixtureRoot 'smoke-result.txt'
if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report -Force }
$process = Start-Process -FilePath $fixtureExe -ArgumentList '--smoke-test' -WorkingDirectory $fixtureRoot -WindowStyle Hidden -PassThru
if (-not $process.WaitForExit(15000)) {
    $process.Kill()
    throw 'MFC startup smoke test timed out (check for an assertion or startup dialog).'
}
if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $report)) { throw "MFC runtime failed. Exit: $($process.ExitCode)" }
Get-Content -LiteralPath $report
if ((Get-Content -LiteralPath $report -Raw) -notmatch '^PASS:') { throw 'Runtime smoke report did not pass.' }
