[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release', [switch]$Font125, [switch]$Font150,
    [ValidateSet('All', 'Serial', 'Startup', 'Uart', 'UartDisplay', 'UartOperations', 'Advanced', 'Charts', 'ChartTime', 'MessageLog', 'Optimization', 'Soak', 'Workbench', 'AnalysisExpansion', 'Lab', 'Session', 'Language', 'Workflow')][string]$Scope = 'All',
    [ValidateSet('en','tw')][string]$UiLanguage='tw',
    [ValidateSet('Messages', 'Chart', '4Charts')][string]$StartupTab = 'Chart',
    [ValidateRange(60,86400)][int]$SoakSeconds=60, [switch]$WaterfallSoak, [switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$StartupTab = @{Messages = 'Messages'; Chart = 'Chart'; '4Charts' = '4Charts'}[$StartupTab]
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'test_artifacts.ps1')
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -RuntimeChecks }
$source = Join-Path $projectRoot "build\verify\runtime-bin\$Configuration\SimpleComChart.exe"
if ((Get-Item -LiteralPath $source).VersionInfo.FileVersion -ne (Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim()) { throw 'Verification binary version is stale; build it first.' }
$fixtureName = if ($Font125) { "$Configuration-font125" } else { $Configuration }
if($Font150) { $fixtureName = "$Configuration-font150" }
if ($Scope -eq 'Serial') { $fixtureName += '-serial' }
if ($Scope -eq 'Uart') { $fixtureName += '-uart' }
if ($Scope -eq 'UartDisplay') { $fixtureName += '-uart-display' }
if ($Scope -eq 'UartOperations') { $fixtureName += '-uart-operations' }
if ($Scope -eq 'Advanced') { $fixtureName += '-advanced' }
if ($Scope -eq 'Charts') { $fixtureName += '-charts' }
if ($Scope -eq 'ChartTime') { $fixtureName += "-chart-time-$UiLanguage" }
if ($Scope -eq 'MessageLog') { $fixtureName += "-message-log-$UiLanguage" }
if ($Scope -eq 'Optimization') { $fixtureName += '-optimization' }
if ($Scope -eq 'Soak') { $fixtureName += '-soak' }
if($WaterfallSoak){if($Scope -ne 'Soak'){throw 'WaterfallSoak requires Scope Soak'};$fixtureName+='-waterfall'}
if ($Scope -eq 'Workbench') { $fixtureName += '-workbench' }
if ($Scope -eq 'AnalysisExpansion') { $fixtureName += '-analysis-expansion' }
if ($Scope -eq 'Lab') { $fixtureName += '-lab' }
if ($Scope -eq 'Session') { $fixtureName += '-session' }
if ($Scope -eq 'Language') { $fixtureName += "-language-$UiLanguage" }
if ($Scope -eq 'Workflow') { $fixtureName += "-workflow-$UiLanguage" }
if ($Scope -eq 'Startup') { $fixtureName += "-startup-$StartupTab" }
$run = New-VerificationRun $projectRoot $fixtureName $source
$fixtureRoot = $run.root
$fixtureExe = Join-Path $fixtureRoot 'SimpleComChart.exe'
$passed = $false
$failure = 'Runner interrupted or cancelled before completion.'
$process = $null
$previousTestTemp = $env:TEMP
$previousTestTmp = $env:TMP
try {
$env:TEMP = $fixtureRoot
$env:TMP = $fixtureRoot
Copy-Item -LiteralPath $source -Destination $fixtureExe
foreach($catalog in @('lang_en.ini','lang_tw.ini')) { Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $source) $catalog) -Destination (Join-Path $fixtureRoot $catalog) }
if ((Get-FileHash -LiteralPath $fixtureExe -Algorithm SHA256).Hash -cne $run.source_sha256) { throw 'Verification copy hash mismatch.' }
$report = Join-Path $fixtureRoot 'smoke-result.txt'
if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report -Force }
$testArguments = if ($Scope -eq 'Serial') { '--smoke-test=serial' } else { '--smoke-test' }
if ($Scope -eq 'Uart' -or $Scope -eq 'UartDisplay' -or $Scope -eq 'UartOperations') {
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'), "[UI]`r`nStartupTab=Messages`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = '--smoke-test=uart'
    if ($Scope -eq 'UartDisplay') { $testArguments = '--smoke-test=uart-display' }
    if ($Scope -eq 'UartOperations') { $testArguments = '--smoke-test=uart-operations' }
}
if ($Scope -eq 'Startup') {
    # This is an isolated, test-owned INI; never copy or edit the personal delivery INI.
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),
        "[UI]`r`nStartupTab=$StartupTab`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = "--smoke-test=startup --expected-startup=$StartupTab"
}
if ($Scope -eq 'Advanced') {
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'), "[UI]`r`nStartupTab=Messages`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = '--smoke-test=advanced'
}
if ($Scope -eq 'Charts') {
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'), "[UI]`r`nStartupTab=Chart`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = '--smoke-test=charts'
}
if ($Font125) { $testArguments += ' --font125' }
if ($Scope -eq 'Optimization') {
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'), "[UI]`r`nStartupTab=Chart`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = '--smoke-test=optimization'
    if($Font125) { $testArguments += ' --font125' }
}
if ($Font150) { $testArguments += ' --font150' }
if ($Scope -eq 'Workbench') { [IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),"[UI]`r`nStartupTab=Chart`r`n[DualCOM]`r`nSplitPercent=50`r`n",[Text.Encoding]::Unicode);$testArguments = '--smoke-test=workbench';if($Font125){$testArguments+=' --font125'};if($Font150){$testArguments+=' --font150'} }
if ($Scope -eq 'AnalysisExpansion') { [IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),"[UI]`r`nStartupTab=Chart`r`n",[Text.Encoding]::Unicode); $testArguments='--smoke-test=analysis-expansion';if($Font125){$testArguments+=' --font125'} }
if ($Scope -eq 'Lab') { [IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),"[UI]`r`nStartupTab=Chart`r`n",[Text.Encoding]::Unicode); $testArguments='--smoke-test=lab';if($Font125){$testArguments+=' --font125'} }
if ($Scope -eq 'Session') { [IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'),"[UI]`r`nStartupTab=Chart`r`n",[Text.Encoding]::Unicode); $testArguments='--smoke-test=session';if($Font125){$testArguments+=' --font125'} }
if ($Scope -eq 'Soak') {
    [System.IO.File]::WriteAllText((Join-Path $fixtureRoot 'SimpleComChart.ini'), "[UI]`r`nStartupTab=Messages`r`n", [System.Text.Encoding]::Unicode)
    $testArguments = "--smoke-test=soak --soak-seconds=$SoakSeconds"
    if($WaterfallSoak){$testArguments+=' --waterfall-soak'}
    # Keep measured receive/UI work separate from compilation and other private
    # GUI fixtures. Never stop another builder or a normal user session.
    $idleDeadline = [DateTime]::UtcNow.AddMinutes(5)
    $quietSamples = 0
    $waitingReported = $false
    while ($quietSamples -lt 3) {
        $processSnapshot = @(Get-CimInstance Win32_Process)
        $buildPids = @($processSnapshot | Where-Object Name -eq 'MSBuild.exe' | Select-Object -ExpandProperty ProcessId)
        # Reused MSBuild nodes can outlive their root build. An orphan node is
        # idle infrastructure; preserve it without blocking/terminating it.
        $busy = @($processSnapshot | Where-Object {
            $_.Name -in @('cl.exe','link.exe','cmake.exe') -or
            ($_.Name -eq 'MSBuild.exe' -and ($_.CommandLine -notmatch '[/-]nodemode:' -or $_.ParentProcessId -in $buildPids)) -or
            ($_.ExecutablePath -and $_.ExecutablePath.StartsWith((Join-Path $projectRoot 'build\verify') + '\', [StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -like '*--smoke-test*')
        })
        if ($busy.Count) {
            $quietSamples = 0
            if (-not $waitingReported) { Write-Host 'Waiting for builds/private GUI fixtures to finish before receive-performance measurement.'; $waitingReported = $true }
        } else { $quietSamples++ }
        if ([DateTime]::UtcNow -gt $idleDeadline) { throw 'Performance check not started: build/private-fixture activity did not settle. No performance PASS claimed.' }
        if ($quietSamples -lt 3) { Start-Sleep -Milliseconds 1000 }
    }
}
$seedPath=Join-Path $fixtureRoot 'SimpleComChart.ini'
$seed=if(Test-Path -LiteralPath $seedPath){[IO.File]::ReadAllText($seedPath)}else{''}
$language=if($Scope -in @('Language','Workflow','ChartTime','MessageLog')){$UiLanguage}else{'en'}
if($seed -match '(?m)^\[UI\]\r?$'){$seed=$seed -replace '(?m)^\[UI\]\r?$',("[UI]`r`nLanguage=$language")}else{$seed+="`r`n[UI]`r`nLanguage=$language`r`n"}
if($Scope -eq 'Language'){$testArguments='--smoke-test=language';if($Font125){$testArguments+=' --font125'}}
if($Scope -eq 'Workflow'){$testArguments='--smoke-test=workflow';if($Font125){$testArguments+=' --font125'}}
if($Scope -eq 'ChartTime'){$testArguments='--smoke-test=chart-time';if($Font125){$testArguments+=' --font125'}}
if($Scope -eq 'MessageLog'){$testArguments='--smoke-test=message-log';if($Font125){$testArguments+=' --font125'}}
[IO.File]::WriteAllText($seedPath,$seed,[Text.Encoding]::Unicode)
$process = Start-Process -FilePath $fixtureExe -ArgumentList $testArguments -WorkingDirectory $fixtureRoot -WindowStyle Hidden -PassThru
# Three-page repaint/screenshot and settings/raw-message regressions need a bounded 30 s budget.
$timeoutMs = if($Scope -eq 'Soak') { ($SoakSeconds+120)*1000 } elseif($Scope -in @('Session','Workflow','MessageLog','Optimization')) { 90000 } else { 30000 }
$deadline = [DateTime]::UtcNow.AddMilliseconds($timeoutMs)
while (-not $process.WaitForExit(1000)) { if ([DateTime]::UtcNow -gt $deadline) { throw 'MFC startup smoke test timed out (check for an assertion or startup dialog).' } }
if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $report)) { throw "MFC runtime failed. Exit: $($process.ExitCode)" }
Get-Content -LiteralPath $report
if ((Get-Content -LiteralPath $report -Raw) -notmatch '^PASS:') { throw 'Runtime smoke report did not pass.' }
$passed = $true
$failure = ''
} catch {
    $failure = $_.Exception.Message
    throw
} finally {
    $env:TEMP = $previousTestTemp
    $env:TMP = $previousTestTmp
    if ($process) {
        if (-not $process.HasExited) {
            if ([IO.Path]::GetFullPath($process.MainModule.FileName) -ne $fixtureExe) { throw 'Unexpected test image; refusing termination/cleanup.' }
            $process.Kill()
            if (-not $process.WaitForExit(5000)) { throw 'Owned test worker did not exit; files preserved.' }
        }
        $process.Dispose()
    }
    Complete-VerificationRun $run $passed $failure
}
