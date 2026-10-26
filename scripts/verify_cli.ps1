[CmdletBinding()]
param([ValidateSet('Release','Debug')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'test_artifacts.ps1')
$source=Join-Path $projectRoot $(if($Configuration -eq 'Release'){'build\SimpleComChartCli.exe'}else{'build\Debug\SimpleComChartCli.exe'})
$version=(Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim()
if((Get-Item -LiteralPath $source).VersionInfo.FileVersion -ne $version){throw 'CLI version is stale.'}
$run=New-VerificationRun $projectRoot "$Configuration-cli" $source
$copy=Join-Path $run.root 'RenamedFiniteHelper.exe'
$checks=[Collections.Generic.List[string]]::new()
$passed=$false
$failure='CLI checks interrupted.'
$previousTestTemp=$env:TEMP
$previousTestTmp=$env:TMP
function Check([bool]$value,[string]$name){if(-not $value){throw $name};$checks.Add("PASS: $name")}
function Invoke-OwnedCli([string]$key,[string[]]$arguments){
    $quoted=@($arguments|ForEach-Object {'"'+$_+'"'})
    # Own the Process from before Start: Windows PowerShell Start-Process may
    # expose null ExitCode for a very short-lived redirected console child.
    $start=[Diagnostics.ProcessStartInfo]::new()
    $start.FileName=$copy
    $start.Arguments=$quoted -join ' '
    $start.WorkingDirectory=$run.root
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $start.RedirectStandardOutput=$true
    $start.RedirectStandardError=$true
    $process=[Diagnostics.Process]::new()
    $process.StartInfo=$start
    if(-not $process.Start()){throw 'Cannot start owned CLI.'}
    try {
        if(-not $process.WaitForExit(5000)){throw 'Owned CLI timed out.'}
        return [pscustomobject]@{exit=$process.ExitCode;output=$process.StandardOutput.ReadToEnd();error=$process.StandardError.ReadToEnd()}
    } finally {
        if(-not $process.HasExited){if([IO.Path]::GetFullPath($process.MainModule.FileName) -ne $copy){throw 'Unexpected CLI image; preserved.'};$process.Kill();if(-not $process.WaitForExit(5000)){throw 'Owned CLI did not exit.'}}
        $process.Dispose()
    }
}
try {
    $env:TEMP=$run.root
    $env:TMP=$run.root
    Copy-Item -LiteralPath $source -Destination $copy
    Check ((Get-FileHash -LiteralPath $copy).Hash -ceq $run.source_sha256) 'isolated renamed CLI matches formal bytes'
    $help=Invoke-OwnedCli 'help' @('--help')
    Check ($help.exit -eq 0 -and $help.output.Contains($version) -and $help.output.Contains('no reconnect')) 'formal CLI help/version/finite limits'
    $fixture=Join-Path $run.root 'private.sccsteps'
    $cells=@('Fixture','Read','TEXT','Q','0','0','0','0','100','1','0','4F4B','0','1000')
    $row='"'+($cells -join '","')+'"'
    [IO.File]::WriteAllText($fixture,"SCC_STEPS_V1`r`n$row`r`n",[Text.UTF8Encoding]::new($false))
    $validated=Invoke-OwnedCli 'validate' @('validate','--script',$fixture)
    Check ($validated.exit -eq 0 -and $validated.output.Contains('VALID: 1') -and $validated.output.Contains('no port opened')) 'formal CLI validates private script without hardware'
    $blocked=Invoke-OwnedCli 'missing-confirm' @('run','--script',$fixture,'--port','TEST_NOT_COM','--baud','115200','--report',(Join-Path $run.root 'never.csv'))
    Check ($blocked.exit -eq 2 -and $blocked.error.Contains('--confirm-tx') -and -not(Test-Path -LiteralPath (Join-Path $run.root 'never.csv'))) 'missing explicit TX confirmation fails before hardware/report creation'
    $created=$false
    $guard=[Threading.Mutex]::new($true,'Local\Released.SimpleComChart.UARTChart.v1.Owner',[ref]$created)
    try {
        if($created){
            # Even a guard regression cannot open hardware: scanner names always
            # start with COM; this test target cannot match an enumerated device.
            $busy=Invoke-OwnedCli 'guard' @('run','--script',$fixture,'--port','TEST_NOT_COM','--baud','115200','--report',(Join-Path $run.root 'blocked.csv'),'--confirm-tx')
            Check ($busy.exit -eq 2 -and $busy.error.Contains('already owns this session') -and -not(Test-Path -LiteralPath (Join-Path $run.root 'blocked.csv'))) 'renamed CLI shares GUI owner guard and fails closed'
        } else {$checks.Add('NOT RUN: guard-owned case; preserve existing normal owner.')}
    } finally {if($created){$guard.ReleaseMutex()};$guard.Dispose()}
    Check (-not(Test-Path -LiteralPath (Join-Path $run.root 'SimpleComChart.ini'))) 'CLI never creates portable personal settings'
    Check ((Get-FileHash -LiteralPath $source).Hash -ceq $run.source_sha256) 'formal CLI preserved'
    $passed=$true
    $failure=''
} catch {$failure=$_.Exception.Message;$checks.Add("FAIL: $failure");throw}
finally {
    $env:TEMP=$previousTestTemp
    $env:TMP=$previousTestTmp
    $checks|Set-Content -LiteralPath (Join-Path $run.root 'cli-result.txt') -Encoding UTF8
    Complete-VerificationRun $run $passed $failure
    $checks|Write-Output
}
