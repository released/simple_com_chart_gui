[CmdletBinding()]
param([ValidateSet('Core','UI','Background','Registration','Runtime','Delivery')][string]$Phase='Core',
      [ValidateSet('Release','Debug')][string]$Configuration='Release', [string]$PreparedStamp='', [switch]$Interactive, [ValidateSet('CR1-CR5','ChannelOverlay')][string]$ScopeName='CR1-CR5')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$hub=Join-Path (Split-Path -Parent $projectRoot) '#READ_ME_FIRST#'
. (Join-Path $hub 'scripts\invoke_hub_test.ps1')
$script:reviewEvidence=[ordered]@{scope=$ScopeName;phase=$Phase;configuration=$Configuration;version=[IO.File]::ReadAllText((Join-Path $projectRoot 'VERSION')).Trim();commands=@();hashes=@();results=@()}
$script:reviewProcesses=New-Object 'Collections.Generic.List[object]'
$script:formalBaseline=@{}
foreach($name in @('SimpleComChart.exe','SimpleComChartCli.exe','lang_en.ini','lang_tw.ini')){$script:formalBaseline[$name]=(Get-FileHash -LiteralPath (Join-Path $projectRoot ('build\'+$name))).Hash}
$prior=@{}
foreach($key in @('TEMP','TMP','SCC_TEST_WORK','PYTHONDONTWRITEBYTECODE','QT_QPA_PLATFORM','MSBUILDDISABLENODEREUSE')){$prior[$key]=[Environment]::GetEnvironmentVariable($key,'Process')}
function Invoke-ReviewProcess($run,[string]$file,[string[]]$arguments,[string]$name,[int]$timeout=180,[switch]$Private,[switch]$Startup,[int]$ExpectedExit=0) {
    $out=Join-Path $run.Work ($name+'.out.txt');$err=Join-Path $run.Work ($name+'.err.txt')
    $quoted=@($arguments|ForEach-Object { '"'+$_+'"' })
    $desktop=[IntPtr]::Zero
    if($Private){
        $native=[ReviewPrivateDesktop]::Start($file,($quoted -join ' '),$run.Work)
        $desktop=$native.Desktop;$p=$native.Process
        [IO.File]::WriteAllText($out,'Private non-input desktop process; never switched input desktop.')
        [IO.File]::WriteAllText($err,'')
    }else{
        $options=@{FilePath=$file;WorkingDirectory=$run.Work;WindowStyle='Hidden';PassThru=$true;RedirectStandardOutput=$out;RedirectStandardError=$err}
        if($quoted.Count){$options.ArgumentList=$quoted}
        $p=Start-Process @options
    }
    $null=$p.Handle
    $born=$p.StartTime.ToUniversalTime();$ownedEntry=@{process=$p;born=$born;desktop=$desktop};$script:reviewProcesses.Add($ownedEntry)
    Register-ReviewProcess $run $p
    $until=[DateTime]::UtcNow.AddSeconds($timeout)
    $startupSeen=$false;$startupClosed=$false
    while(!$p.WaitForExit(200)) {
        if($Startup -and !$startupClosed){
            $window=[ReviewPrivateDesktop]::FindMain($desktop,$p.Id,(Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim())
            if($window -ne [IntPtr]::Zero -and [ReviewPrivateDesktop]::IsZoomed($window) -and [ReviewPrivateDesktop]::HasBuildLabel($window,(Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim())){
                $startupSeen=$true
                $startupClosed=[ReviewPrivateDesktop]::CloseOwned($window,$p.Id)
            }
        }
        # Parent PID alone is not ownership: require the same live parent/start identity.
        $parents=@($script:reviewProcesses.ToArray())
        foreach($parent in $parents) {
            $owner=$parent.process
            if($owner.HasExited){continue}
            $live=Get-Process -Id $owner.Id -ErrorAction SilentlyContinue
            if(!$live -or $live.StartTime.ToUniversalTime() -ne $parent.born){continue}
            foreach($child in @(Get-CimInstance Win32_Process -Filter ('ParentProcessId='+$owner.Id))) {
                if(!$child -or !$child.CreationDate){continue}
                if($child.CreationDate.ToUniversalTime().AddMilliseconds(1) -lt $parent.born){continue}
                $cp=Get-Process -Id $child.ProcessId -ErrorAction SilentlyContinue
                if(!$cp -or !$cp.StartTime){continue}
                if(!$cp -or [Math]::Abs(($cp.StartTime.ToUniversalTime()-$child.CreationDate.ToUniversalTime()).TotalMilliseconds) -gt 1){continue}
                $cb=$cp.StartTime.ToUniversalTime()
                $null=$cp.Handle
                if(@($script:reviewProcesses|Where-Object {$_.process.Id -eq $cp.Id -and $_.born -eq $cb}).Count){continue}
                if(!$owner.HasExited -and !$cp.HasExited){
                    $script:reviewProcesses.Add(@{process=$cp;born=$cb;desktop=[IntPtr]::Zero})
                    try{Register-HubTestProcess -Run $run -ProcessId $cp.Id}catch{if(!$cp.HasExited){throw}}
                }
            }
        }
        if([DateTime]::UtcNow -gt $until){throw ('Owned process timeout: '+$name)}
    }
    $p.WaitForExit();$code=$p.ExitCode
    if($desktop -ne [IntPtr]::Zero -and ![ReviewPrivateDesktop]::CloseDesktop($desktop)){throw 'Private desktop did not close'}
    $ownedEntry.desktop=[IntPtr]::Zero
    if($Startup -and !$startupSeen){throw 'Production GUI exited before startup observation'}
    $script:reviewEvidence.commands+=@{name=$name;file=$file;arguments=$arguments;exit=$code}
    $text=[IO.File]::ReadAllText($out)+[IO.File]::ReadAllText($err)
    $script:reviewEvidence.results+=@{name=$name;lines=@($text -split '\r?\n'|Where-Object {$_ -match 'BENCH |PASS:|FAIL:|Ran |^OK$|checks=|Prepared |Error|error |FAILED'})}
    if($code -ne $ExpectedExit){throw ($name+' exited '+$code+': '+($text -split '\r?\n'|Select-Object -Last 18|Out-String))}
    if($Startup){$script:reviewEvidence.results+=@{name=$name;lines=@('PASS: actual production GUI startup, maximized/build label and graceful close on private non-input desktop')}}
    Write-Host ('PASS process '+$name)
}

$result=Invoke-HubOwnedTest -ProjectRoot $projectRoot -Purpose ($ScopeName+' '+$Phase+' '+$Configuration) -MaxBytes 3GB -Action {
    param($run)
    if($Phase -eq 'Delivery' -and $ScopeName -eq 'ChannelOverlay') {
        foreach($config in @('release','debug')) {
            $path=Join-Path $projectRoot ('docs\verification\channel-overlay-background-'+$config+'.json')
            $core=[IO.File]::ReadAllText($path)|ConvertFrom-Json
            if($core.version -cne $script:reviewEvidence.version -or $core.outcome -cne 'PASS' -or (Get-Item -LiteralPath (Join-Path $projectRoot 'src\channel_panel.cpp')).LastWriteTimeUtc -gt (Get-Item -LiteralPath $path).LastWriteTimeUtc){throw 'Matching in-process native background evidence is required before delivery'}
        }
    }
    $temp=Join-Path $run.Work 'temp';[void][IO.Directory]::CreateDirectory($temp)
    $env:TEMP=$temp;$env:TMP=$temp;$env:SCC_TEST_WORK=$run.Work;$env:PYTHONDONTWRITEBYTECODE='1';$env:QT_QPA_PLATFORM='offscreen';$env:MSBUILDDISABLENODEREUSE='1'
    . (Join-Path $projectRoot 'tests\private_desktop_process.ps1')
    if($Phase -eq 'Registration') {
        . (Join-Path $projectRoot 'tests\private_process_registration.ps1') -Run $run
        $script:reviewEvidence.results+=@{name='registration';lines=@('PASS: actual exit-registration race','PASS: other errors remain fatal','PASS: normal registration')}
        return
    }
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
    $cmake=Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if(!(Test-Path -LiteralPath $cmake)){$cmake=(Get-Command cmake.exe|Select-Object -First 1).Source}
    $obj=Join-Path $run.Work 'obj';$checks=if($Phase -eq 'Runtime'){'ON'}else{'OFF'}
    $override=Join-Path $run.Work 'owned-output.cmake';$bin=(Join-Path $run.Work 'bin').Replace('\','/')
    $contents='set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP4")'+[Environment]::NewLine
    if($Phase -eq 'Delivery'){$contents+='cmake_language(DEFER CALL set_target_properties simple_com_chart_gui_mfc simple_com_chart_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "'+$bin+'")'}
    [IO.File]::WriteAllText($override,$contents)
    $args=@('-S',$projectRoot,'-B',$obj,'-G','Visual Studio 17 2022','-A','x64',('-DSCC_ENABLE_RUNTIME_CHECKS='+$checks),('-DSCC_VERIFICATION_ROOT='+$run.Work),('-DCMAKE_PROJECT_INCLUDE='+$override))
    if($PreparedStamp){$args+=('-DSCC_BUILD_STAMP='+$PreparedStamp)}
    Invoke-ReviewProcess $run $cmake $args 'configure'
    $targets=if($Phase -eq 'Background'){@('message_background_tests')}elseif($Phase -eq 'UI' -or ($Phase -eq 'Core' -and $Configuration -eq 'Debug')){@('message_view_tests')}elseif($Phase -eq 'Core'){@('ini_settings_tests','uart_advanced_tests','uart_receive_tests','message_view_tests')}elseif($Phase -eq 'Runtime'){@('simple_com_chart_gui_mfc')}else{@('simple_com_chart_gui_mfc','simple_com_chart_cli')}
    $args=@('--build',$obj,'--config',$Configuration,'--target')+$targets+@('--parallel','2','--','/nr:false')
    if($Configuration -eq 'Debug'){$args+='/p:DebugInformationFormat=OldStyle'}
    Invoke-ReviewProcess $run $cmake $args 'build' 1800
    if($Phase -in @('Core','UI')) {
        $bin=Join-Path $obj $Configuration
        if($Phase -eq 'Core' -and $Configuration -eq 'Release') {
            Invoke-ReviewProcess $run (Join-Path $bin 'ini_settings_tests.exe') @() 'ini-literals'
            Invoke-ReviewProcess $run (Join-Path $bin 'uart_advanced_tests.exe') @($run.Work,'--profile-ini') 'profile-literals'
            Invoke-ReviewProcess $run (Join-Path $bin 'uart_receive_tests.exe') @($run.Work,'--recording-status') 'recording-state'
            $python=(Get-Command python.exe|Select-Object -First 1).Source
            Invoke-ReviewProcess $run $python @('-B',(Join-Path $projectRoot 'tests\test_serial_receive.py')) 'python-receive'
        }
        Invoke-ReviewProcess $run (Join-Path $bin 'message_view_tests.exe') @('--recording-status') 'message-recording-state'
        if($Phase -eq 'UI'){Invoke-ReviewProcess $run (Join-Path $bin 'message_view_tests.exe') @('--scroll-stability') 'message-scroll-stability'}
    } elseif($Phase -eq 'Background') {
        $test=Join-Path $obj ($Configuration+'\message_background_tests.exe')
        Invoke-ReviewProcess $run $test @('--channel-panel') 'channel-background'
        Invoke-ReviewProcess $run $test @() 'message-background'
    } elseif($Phase -eq 'Runtime') {
        $source=Join-Path $run.Work ('runtime-bin\'+$Configuration+'\SimpleComChart.exe')
        $pending=@(@('settings','en',''),@('message-log','tw','--font125'),@('message-log','en',''))
        $attempt=0
        while($pending.Count) {
          $attempt++
          try {
           foreach($item in @($pending)) {
            $name=$item[0]+'-'+$item[1];$folder=Join-Path $run.Work ($name+'-attempt'+$attempt);[void][IO.Directory]::CreateDirectory($folder)
            foreach($leaf in @('SimpleComChart.exe','lang_en.ini','lang_tw.ini')){Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $source) $leaf) -Destination (Join-Path $folder $leaf)}
            [IO.File]::WriteAllText((Join-Path $folder 'SimpleComChart.ini'),("[UI]`r`nLanguage="+$item[1]+"`r`n"),[Text.Encoding]::Unicode)
            $args=@('--smoke-test='+$item[0]);if($item[2]){$args+=$item[2]}
            try {Invoke-ReviewProcess $run (Join-Path $folder 'SimpleComChart.exe') $args ($name+'-attempt'+$attempt) 180 -Private}
            finally {
                foreach($text in @(Get-ChildItem -LiteralPath $folder -File -Filter '*-result.txt')) {
                    $reportLines=[IO.File]::ReadAllLines($text.FullName)
                    $script:reviewEvidence.results+=@{name=$name+'-attempt'+$attempt+'-'+$text.Name;lines=$reportLines}
                    $reportLines|Where-Object {$_ -match '^FAIL:|^BOUNDS:'}|ForEach-Object {Write-Host $_}
                }
            }
            $report=Join-Path $folder ($item[0]+'-result.txt');$lines=[IO.File]::ReadAllLines($report)
            $script:reviewEvidence.results+=@{name=$name+'-report';lines=$lines}
            if($lines -match '^FAIL:'){throw ('Native report failed: '+$name)}

            $pending=@($pending|Where-Object {($_[0]+'-'+$_[1]) -ne $name})
           }
          }catch{
            if(!$Interactive){throw}
            Write-Host ('FAILED CHECK: '+$_.Exception.Message)
            Write-Host ('OWNED RUN: '+$run.Root)
            Write-Host 'Enter retry after correcting the affected source, or stop to finalize this failed run.'
            if([Console]::ReadLine() -cne 'retry'){throw}
            Invoke-ReviewProcess $run $cmake @('--build',$obj,'--config',$Configuration,'--target','simple_com_chart_gui_mfc','--parallel','2','--','/nr:false') ('rebuild-'+$attempt) 1800
          }
        }

    } else {
        Invoke-ReviewProcess $run (Join-Path $run.Work 'bin\SimpleComChartCli.exe') @('--help') 'cli-help'
        $powershell=(Get-Command powershell.exe|Select-Object -First 1).Source
        $candidatePassed=$false
        while(!$candidatePassed) {
            try {
        Invoke-ReviewProcess $run $powershell @('-NoProfile','-ExecutionPolicy','Bypass','-File',(Join-Path $projectRoot 'scripts\verify_delivery.ps1'),'-CandidateExe',(Join-Path $run.Work 'bin\SimpleComChart.exe'),'-UiLanguage','tw') 'candidate-delivery' 180
                $candidatePassed=$true
            } catch {
                if(!$Interactive){throw}
                Write-Host ('Candidate verifier failed: '+$_.Exception.Message)
                Write-Host ('OWNED RUN: '+$run.Root)
                Write-Host 'Enter retry after correcting the verifier, or stop to finalize this failed run.'
                if([Console]::ReadLine() -cne 'retry'){throw}
            }
        }
        . (Join-Path $PSScriptRoot 'finish_review_delivery.ps1') -Run $run -ProjectRoot $projectRoot -Evidence $script:reviewEvidence -ExpectedHashes $script:formalBaseline
    }
} -Teardown {
    param($run)
    $errors=New-Object 'Collections.Generic.List[string]'
    foreach($owned in $script:reviewProcesses) {
        try {
            $p=$owned.process
            if(!$p.HasExited){$live=Get-Process -Id $p.Id -ErrorAction SilentlyContinue;if(!$live -or $live.StartTime.ToUniversalTime() -ne $owned.born){throw 'Owned process identity changed'};if(!$p.WaitForExit(15000)){Stop-Process -Id $p.Id -ErrorAction Stop;$p.WaitForExit()}}
            if($owned.desktop -ne [IntPtr]::Zero){if(![ReviewPrivateDesktop]::CloseDesktop($owned.desktop)){throw 'Owned private desktop remains open'};$owned.desktop=[IntPtr]::Zero}
        }catch{$errors.Add($_.Exception.Message)}
    }
    foreach($key in $prior.Keys){[Environment]::SetEnvironmentVariable($key,$prior[$key],'Process')}
    if($errors.Count){throw ($errors -join '; ')}
} -CollectArtifacts {
    param($run)
    # Fresh work is exclusively produced by the declared CMake/compiler and fake-test processes.
    $files=@(Get-ChildItem -LiteralPath $run.Work -Recurse -File -Force)
    foreach($file in $files){if($file.Extension -eq '.exe'){$script:reviewEvidence.hashes+=@{path=$file.FullName.Substring($run.Work.Length+1);bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}}}
    $script:reviewEvidence.generatedBytes=if($files.Count){($files|Measure-Object Length -Sum).Sum}else{0}
    Register-HubArtifactTree -Run $run -Producer ('CR1-CR5 CMake/compiler and focused fake tests '+$Phase)
} -Evidence {
    param($run,$outcome)
    $script:reviewEvidence.run=$run.Root;$script:reviewEvidence.outcome=$outcome
    [IO.File]::WriteAllText((Join-Path $projectRoot ('docs\verification\'+$(if($ScopeName -eq 'ChannelOverlay'){'channel-overlay'}else{'contracts'})+'-'+$Phase.ToLower()+'-'+$Configuration.ToLower()+'.json')),($script:reviewEvidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
}
$result|Select-Object Success,Outcome,RunRoot,CleanupRemoved,Errors|ConvertTo-Json -Depth 5
foreach($owned in $script:reviewProcesses){$owned.process.Dispose()}
if(!$result.Success){throw 'Focused contract verification or cleanup failed'}
