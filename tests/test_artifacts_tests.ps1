$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $projectRoot 'scripts\test_artifacts.ps1')
$source = Join-Path $projectRoot 'build\SimpleComChart.exe'
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$ini = Join-Path $projectRoot 'build\SimpleComChart.ini'
$iniHash = (Get-FileHash -LiteralPath $ini -Algorithm SHA256).Hash
$key = 'retention-check'
$harness = Join-Path $projectRoot ('build\verify\runs\retention-harness-' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $harness)
$checks = [Collections.Generic.List[string]]::new()
function Check([bool]$Value, [string]$Name) {
    if (-not $Value) { throw $Name }
    $checks.Add("PASS: $Name")
}
function Seed($Run) {
    Copy-Item -LiteralPath $source -Destination (Join-Path $Run.root 'SimpleComChart.exe')
    [IO.File]::WriteAllBytes((Join-Path $Run.root 'synthetic.bin'), [byte[]]::new(1MB))
    [IO.File]::WriteAllText((Join-Path $Run.root 'synthetic.log'), 'synthetic RX only')
    [IO.File]::WriteAllText((Join-Path $Run.root 'smoke-result.txt'), 'PASS: synthetic runner result')
    [IO.File]::WriteAllText((Join-Path $Run.root 'SimpleComChart.ini'), '[UI]')
}
try {
    $run = New-VerificationRun $projectRoot $key $source
    Seed $run
    Complete-VerificationRun $run $true ''
    Check (-not (Test-Path -LiteralPath $run.root)) 'successful run removes all registered scratch files'
    $evidence = Join-Path $run.verify ("evidence\$key\last-pass")
    $unknown = Join-Path $evidence 'unregistered.log'
    [IO.File]::WriteAllText($unknown, 'unknown must stay')
    $run = New-VerificationRun $projectRoot $key $source
    Seed $run
    Complete-VerificationRun $run $true ''
    Check ((Get-Content -LiteralPath $unknown -Raw) -ceq 'unknown must stay') 'unregistered evidence survives replacement'
    Check (@(Get-ChildItem -LiteralPath $evidence -Filter '*.exe').Count -eq 0) 'evidence contains no executable copy'
    Remove-RegisteredTestFiles $evidence @([pscustomobject]@{path='unregistered.log';sha256=(Get-FileHash -LiteralPath $unknown).Hash})
    $modified = Join-Path $evidence 'smoke-result.txt'
    $original = [IO.File]::ReadAllBytes($modified)
    $run = New-VerificationRun $projectRoot $key $source
    Seed $run
    try {
        [IO.File]::WriteAllText($modified, 'modified evidence must stay')
        $blocked = $false
        try { Complete-VerificationRun $run $true '' } catch { $blocked = $_.Exception.Message -like '*modified evidence*' }
        Check ($blocked -and (Get-Content -LiteralPath $modified -Raw) -ceq 'modified evidence must stay') 'modified evidence blocks overwrite and preserves run'
    } finally { [IO.File]::WriteAllBytes($modified, $original) }
    Complete-VerificationRun $run $true ''
    Check (-not (Test-Path -LiteralPath $run.root)) 'owned run can retry after evidence conflict is resolved'

    foreach ($mode in @('failure','cancel')) {
        $run = New-VerificationRun $projectRoot $key $source
        Seed $run
        try { throw [OperationCanceledException]::new("intentional $mode") }
        catch { $failure = $_.Exception.Message }
        finally { Complete-VerificationRun $run $false $failure }
        Check (-not (Test-Path -LiteralPath $run.root)) "$mode finally removes generated binary/log/EXE"
    }
    $run = New-VerificationRun $projectRoot ($key + '-guards') $source
    $outside = Join-Path $harness 'outside.log'
    [IO.File]::WriteAllText($outside, 'protected outside run')
    $blocked = $false
    try { Remove-RegisteredTestFiles $run.root @([pscustomobject]@{path='..\outside.log';sha256=(Get-FileHash -LiteralPath $outside).Hash}) } catch { $blocked = $true }
    Check ($blocked -and (Get-Content -LiteralPath $outside -Raw) -ceq 'protected outside run') 'parent traversal rejected before deletion'
    $junction = Join-Path $run.root 'escape'
    [void](New-Item -ItemType Junction -Path $junction -Value $harness)
    try {
        $blocked = $false
        try { Assert-TestArtifactPath $run.root (Join-Path $junction 'outside.log') } catch { $blocked = $true }
        Check $blocked 'junction traversal rejected'
    } finally { Assert-TestArtifactPath $run.root $run.root; [IO.Directory]::Delete($junction) }
    Complete-VerificationRun $run $true ''
    Remove-RegisteredTestFiles $harness @([pscustomobject]@{path='outside.log';sha256=(Get-FileHash -LiteralPath $outside).Hash.ToLowerInvariant()})
    Check (-not (Test-Path -LiteralPath $outside)) 'equivalent lower-case SHA-256 manifest accepted'

    $helper = Join-Path $projectRoot 'build\obj\cmake-runtime\Release\session_instance_tests.exe'
    $run = New-VerificationRun $projectRoot ($key + '-active') $helper
    $copy = Join-Path $run.root 'owned-helper.exe'
    Copy-Item -LiteralPath $helper -Destination $copy
    $arguments = '--child "' + (Join-Path $run.root 'child.txt') + '" PrivateTest.Retention.' + [guid]::NewGuid().ToString('N') + ' ready 10000'
    $process = Start-Process -FilePath $copy -ArgumentList $arguments -WorkingDirectory $run.root -WindowStyle Hidden -PassThru
    try {
        $blocked = $false
        try { Complete-VerificationRun $run $true '' } catch { $blocked = $_.Exception.Message -like '*still active*' }
        Check ($blocked -and (Test-Path -LiteralPath $copy)) 'active test process blocks artifact deletion'
    } finally {
        if (-not $process.HasExited) {
            if ([IO.Path]::GetFullPath($process.MainModule.FileName) -ne $copy) { throw 'Unexpected helper image' }
            $process.Kill(); [void]$process.WaitForExit(5000)
        }
        $process.Dispose()
    }
    Complete-VerificationRun $run $false 'Intentional private-process guard fixture.'
    Check (-not (Test-Path -LiteralPath $run.root)) 'cleanup completes after private helper exits'
    Check ((Get-FileHash -LiteralPath $source).Hash -ceq $sourceHash) 'formal executable unchanged'
    Check ((Get-FileHash -LiteralPath $ini).Hash -ceq $iniHash) 'personal INI unchanged'
} catch {
    $checks.Add("FAIL: $($_.Exception.Message)")
    throw
} finally {
    $checks | Set-Content -LiteralPath (Join-Path $projectRoot 'build\verify\verify-retention-mechanism-result.txt') -Encoding UTF8
    Assert-TestArtifactPath (Join-Path $projectRoot 'build\verify\runs') $harness
    if (@(Get-ChildItem -LiteralPath $harness -Force).Count -eq 0) { Remove-Item -LiteralPath $harness }
    $checks | Write-Output
}
