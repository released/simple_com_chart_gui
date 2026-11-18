param($Run)
$actualRegistration=(Get-Command Register-HubTestProcess).ScriptBlock
$powershell=(Get-Command powershell.exe -CommandType Application|Select-Object -First 1).Source
function New-RegistrationChild {
    $native=[ReviewPrivateDesktop]::Start($powershell,'-NoProfile -Command "Start-Sleep -Milliseconds 1500; exit 2"',$Run.Work)
    $p=$native.Process
    $script:reviewProcesses.Add(@{process=$p;born=$p.StartTime.ToUniversalTime();desktop=$native.Desktop})
    & $actualRegistration -Run $Run -ProcessId $p.Id
    return $p
}
$script:raceCalls=0
try {
    $script:raceChild=New-RegistrationChild
    function Register-HubTestProcess($Run,[int]$ProcessId) {
        $script:raceCalls++
        if(!$script:raceChild.WaitForExit(5000)){throw 'Synthetic child did not finish'}
        & $actualRegistration -Run $Run -ProcessId $ProcessId
    }
    Register-ReviewProcess $Run $script:raceChild
    if($script:raceCalls -ne 1 -or $script:raceChild.ExitCode -ne 2){throw 'Exit-during-registration path was not exercised'}
    Write-Host 'PASS: actual child exits between liveness check and registration without losing its exit result'
    $script:raceChild=New-RegistrationChild
    function Register-HubTestProcess($Run,[int]$ProcessId) {
        if(!$script:raceChild.WaitForExit(5000)){throw 'Synthetic child did not finish'}
        throw 'Injected ownership error'
    }
    $rejected=$false
    try{Register-ReviewProcess $Run $script:raceChild}catch{if($_.Exception.Message -ceq 'Injected ownership error'){$rejected=$true}else{throw}}
    if(!$rejected){throw 'Ownership error was swallowed'}
    Write-Host 'PASS: unrelated ownership/registry errors remain fatal after child exit'
} finally {
    Set-Item Function:\Register-HubTestProcess $actualRegistration
}
$live=New-RegistrationChild
Register-ReviewProcess $Run $live
if(!$live.WaitForExit(5000) -or $live.ExitCode -ne 2){throw 'Live registration child failed'}
Write-Host 'PASS: normal live registration retains owned PID/start identity and expected exit'
