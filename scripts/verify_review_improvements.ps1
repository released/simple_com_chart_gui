[CmdletBinding()]
param([ValidateSet('Settings','Baseline','Verify','Identity','DeliveryCheck','Delivery')][string]$Phase='Verify',
      [ValidateSet('Release','Debug')][string]$Configuration='Release', [string]$ReuseCompletedBuild='', [string]$PreparedStamp='')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$hub='D:\SourceCode\python\#READ_ME_FIRST#'
. (Join-Path $hub 'scripts\invoke_hub_test.ps1')
$script:reviewEvidence=[ordered]@{phase=$Phase;configuration=$Configuration;commands=@();hashes=@();results=@()}
$script:reviewEvidence.version=[IO.File]::ReadAllText((Join-Path $projectRoot 'VERSION')).Trim()
$script:reviewProcesses=New-Object 'Collections.Generic.List[object]'
$script:formalBaseline=@{}
if($Phase -eq 'Delivery'){foreach($name in @('SimpleComChart.exe','SimpleComChartCli.exe','lang_en.ini','lang_tw.ini')){$script:formalBaseline[$name]=(Get-FileHash -LiteralPath (Join-Path $projectRoot ('build\'+$name))).Hash}}
$prior=@{}
foreach($key in @('TEMP','TMP','SCC_TEST_WORK','PYTHONDONTWRITEBYTECODE','QT_QPA_PLATFORM','MSBUILDDISABLENODEREUSE')){$prior[$key]=[Environment]::GetEnvironmentVariable($key,'Process')}
function Invoke-ReviewProcess($run,[string]$file,[string[]]$arguments,[string]$name,[int]$timeout=180,[switch]$Private,[switch]$Startup,[int]$ExpectedExit=0) {
    $out=Join-Path $run.Work ($name+'.out.txt');$err=Join-Path $run.Work ($name+'.err.txt')
    $quoted=@($arguments|ForEach-Object { '"'+$_+'"' })
    $desktop=[IntPtr]::Zero
    if($Private){
        $native=[ReviewPrivateDesktop]::Start($file,($quoted -join ' '),$run.Work)
        $desktop=$native.Desktop;$p=Get-Process -Id $native.Pid
        [IO.File]::WriteAllText($out,'Private non-input desktop process; never switched input desktop.')
        [IO.File]::WriteAllText($err,'')
    }else{
        $options=@{FilePath=$file;WorkingDirectory=$run.Work;WindowStyle='Hidden';PassThru=$true;RedirectStandardOutput=$out;RedirectStandardError=$err}
        if($quoted.Count){$options.ArgumentList=$quoted}
        $p=Start-Process @options
    }
    $null=$p.Handle
    $born=$p.StartTime.ToUniversalTime();$ownedEntry=@{process=$p;born=$born;desktop=$desktop};$script:reviewProcesses.Add($ownedEntry)
    if(!$p.HasExited){Register-HubTestProcess -Run $run -ProcessId $p.Id}
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
$result=Invoke-HubOwnedTest -ProjectRoot $projectRoot -Purpose ('V1-V9 '+$Phase+' '+$Configuration) -MaxBytes 3GB -Action {
    param($run)
    $temp=Join-Path $run.Work 'temp';[void][IO.Directory]::CreateDirectory($temp)
    $env:TEMP=$temp;$env:TMP=$temp;$env:SCC_TEST_WORK=$run.Work;$env:PYTHONDONTWRITEBYTECODE='1';$env:QT_QPA_PLATFORM='offscreen';$env:MSBUILDDISABLENODEREUSE='1'
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
    $cmake=Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if(!(Test-Path -LiteralPath $cmake)){$cmake=(Get-Command cmake.exe -ErrorAction Stop|Select-Object -First 1).Source}
    if($Phase -eq 'Identity'){. (Join-Path $projectRoot 'tests\verify_build_identity.ps1') -Run $run -ProjectRoot $projectRoot -Cmake $cmake;return}
    if($Phase -eq 'DeliveryCheck'){. (Join-Path $projectRoot 'tests\verify_delivery_transaction.ps1') -Run $run -ProjectRoot $projectRoot;return}
    $obj=Join-Path $run.Work 'obj'
    $checks=if($Phase -eq 'Verify'){'ON'}else{'OFF'}
    $args=@('-S',$projectRoot,'-B',$obj,'-G','Visual Studio 17 2022','-A','x64',('-DSCC_ENABLE_RUNTIME_CHECKS='+$checks),('-DSCC_VERIFICATION_ROOT='+$run.Work))
    if($PreparedStamp){$args+=('-DSCC_BUILD_STAMP='+$PreparedStamp)}
    $override=Join-Path $run.Work 'isolated-build.cmake'
    $compilerJobs=[Math]::Max(1,[Math]::Min(4,[int][Math]::Floor([Environment]::ProcessorCount/4)))
    $buildOverride='set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP'+$compilerJobs+'")'+[Environment]::NewLine
    if($Phase -eq 'Delivery'){
        $override=Join-Path $run.Work 'delivery-output.cmake';$bin=(Join-Path $run.Work 'bin').Replace('\','/')
        $buildOverride+='cmake_language(DEFER CALL set_target_properties simple_com_chart_gui_mfc simple_com_chart_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "'+$bin+'")'
    }
    [IO.File]::WriteAllText($override,$buildOverride)
    $args+=('-DCMAKE_PROJECT_INCLUDE='+$override)
    if(!$ReuseCompletedBuild){Invoke-ReviewProcess $run $cmake $args 'configure'}
    $targets=if($Phase -eq 'Settings'){@('ini_settings_tests')}elseif($Phase -eq 'Baseline'){@('ini_settings_tests','message_view_tests')}elseif($Phase -eq 'Verify' -and $Configuration -eq 'Debug'){@('ini_settings_tests','message_view_tests','simple_com_chart_gui_mfc')}elseif($Phase -eq 'Verify'){@('ini_settings_tests','serial_chunk_tests','message_view_tests','simple_com_chart_gui_mfc','uart_workbench_tests','uart_lab_tests')}else{@('simple_com_chart_gui_mfc','simple_com_chart_cli')}
    $args=@('--build',$obj,'--config',$Configuration,'--target')+$targets+@('--parallel','2','--','/nr:false')
    if($Configuration -eq 'Debug'){$args+='/p:DebugInformationFormat=OldStyle'}
    if($ReuseCompletedBuild){
        if($Phase -ne 'Verify'){throw 'Reuse is limited to affected verification artifacts'}
        $record=Read-HubTestRun $ReuseCompletedBuild
        if($record.project_root -ine $projectRoot -or $record.purpose -cne ('V1-V9 Verify '+$Configuration)){throw 'Reused producer/source identity mismatch'}
        $waitUntil=[DateTime]::UtcNow.AddMinutes(10)
        while(@(@($record.owner)+@($record.processes)|Where-Object {Test-HubProcessAlive $_}).Count){
            if([DateTime]::UtcNow -gt $waitUntil){throw 'Reused producer is still running'}
            Start-Sleep -Milliseconds 500
        }
        $oldWork=Join-Path $ReuseCompletedBuild 'work'
        $cache=[IO.File]::ReadAllText((Join-Path $oldWork 'obj\CMakeCache.txt'))
        if($cache -notmatch '(?m)^SCC_ENABLE_RUNTIME_CHECKS:BOOL=ON\r?$'){throw 'Reused GUI must be check-enabled'}
        [void][IO.Directory]::CreateDirectory((Join-Path $obj $Configuration))
        foreach($name in $targets|Where-Object {$_ -ne 'simple_com_chart_gui_mfc'}){
            $old=Join-Path $oldWork ('obj\'+$Configuration+'\'+$name+'.exe');$copy=Join-Path $obj ($Configuration+'\'+$name+'.exe')
            Copy-Item -LiteralPath $old -Destination $copy
            if((Get-FileHash -LiteralPath $old).Hash -cne (Get-FileHash -LiteralPath $copy).Hash){throw 'Reused test copy hash mismatch'}
        }
        $bin=Join-Path $run.Work ('runtime-bin\'+$Configuration);[void][IO.Directory]::CreateDirectory($bin)
        $old=Join-Path $oldWork ('runtime-bin\'+$Configuration+'\SimpleComChart.exe');$copy=Join-Path $bin 'SimpleComChart.exe'
        if((Get-Item -LiteralPath $old).VersionInfo.FileVersion -cne [IO.File]::ReadAllText((Join-Path $projectRoot 'VERSION')).Trim()){throw 'Reused GUI version mismatch'}
        Copy-Item -LiteralPath $old -Destination $copy
        if((Get-FileHash -LiteralPath $old).Hash -cne (Get-FileHash -LiteralPath $copy).Hash){throw 'Reused GUI copy hash mismatch'}
        foreach($name in @('lang_en.ini','lang_tw.ini')){Copy-Item -LiteralPath (Join-Path $projectRoot ('src\i18n\'+$name)) -Destination (Join-Path $bin $name)}
        $script:reviewEvidence.reusedBuild=$ReuseCompletedBuild
    }else{Invoke-ReviewProcess $run $cmake $args 'build' 1800}
    if($Phase -ne 'Delivery'){
        $testBin=Join-Path $obj $Configuration
        if($Phase -ne 'Baseline'){Invoke-ReviewProcess $run (Join-Path $testBin 'ini_settings_tests.exe') @() 'ini-settings'}
        if($Phase -eq 'Settings'){return}
        Invoke-ReviewProcess $run (Join-Path $testBin 'message_view_tests.exe') @('--paused-clear') 'paused-clear'
        if($Phase -eq 'Verify'){Invoke-ReviewProcess $run (Join-Path $testBin 'message_view_tests.exe') @('--format-cache') 'format-cache'}
        Invoke-ReviewProcess $run (Join-Path $testBin 'message_view_tests.exe') @('--scroll-stability') 'scroll-stability'
        if($Configuration -eq 'Release'){Invoke-ReviewProcess $run (Join-Path $testBin 'message_view_tests.exe') @('--refresh-benchmark') 'refresh-benchmark'}
        $python=(Get-Command python.exe|Select-Object -First 1).Source
        if($Configuration -eq 'Release'){Invoke-ReviewProcess $run $python @('-B',(Join-Path $projectRoot 'tests\test_serial_receive.py')) 'python-receive'}
    }
    if($Phase -eq 'Verify'){
        $testBin=Join-Path $obj $Configuration
        if($Configuration -eq 'Release'){
            Invoke-ReviewProcess $run (Join-Path $testBin 'serial_chunk_tests.exe') @() 'serial-chunks'
            Invoke-ReviewProcess $run (Join-Path $testBin 'uart_workbench_tests.exe') @((Join-Path $run.Work 'workbench-fixtures')) 'workbench'
            Invoke-ReviewProcess $run (Join-Path $testBin 'uart_lab_tests.exe') @('--finite-runner') 'finite-runner'
        }
        $gui=Join-Path $run.Work ('runtime-bin\'+$Configuration+'\SimpleComChart.exe')
        $seed=Join-Path (Split-Path -Parent $gui) 'SimpleComChart.ini'
        [IO.File]::WriteAllText($seed,"[UI]`r`nLanguage=en`r`n",[Text.Encoding]::Unicode)
        . (Join-Path $projectRoot 'tests\private_desktop_process.ps1')
        Invoke-ReviewProcess $run $gui @('--smoke-test=settings') 'native-settings' 180 -Private
        $report=Join-Path (Split-Path -Parent $gui) 'settings-result.txt'
        $script:reviewEvidence.results+=@{name='native-settings-report';lines=@([IO.File]::ReadAllLines($report))}
        if([IO.File]::ReadAllText($report) -match 'FAIL:'){throw 'Native settings report failed'}
        if($Configuration -eq 'Release'){. (Join-Path $projectRoot 'tests\verify_build_identity.ps1') -Run $run -ProjectRoot $projectRoot -Cmake $cmake}
    }
    if($Phase -eq 'Delivery'){
        # Formal publication remains a separate guarded step; retain candidates only through this callback.
        Invoke-ReviewProcess $run (Join-Path $run.Work 'bin\SimpleComChartCli.exe') @('--help') 'cli-help'
        Invoke-ReviewProcess $run (Join-Path $run.Work 'bin\SimpleComChart.exe') @('--smoke-test') 'production-reject-smoke' -ExpectedExit 2
        Invoke-ReviewProcess $run (Join-Path $run.Work 'bin\SimpleComChart.exe') @('--demo') 'production-reject-demo' -ExpectedExit 2
        . (Join-Path $projectRoot 'tests\private_desktop_process.ps1')
        $guard=$null
        if([Threading.Mutex]::TryOpenExisting('Local\Released.SimpleComChart.UARTChart.v1.Owner',[ref]$guard)){
            $guard.Dispose();$script:reviewEvidence.results+=@{name='production-startup';lines=@('NOT RUN: existing production owner preserved')}
        }else{
            [IO.File]::WriteAllText((Join-Path $run.Work 'bin\SimpleComChart.ini'),"[UI]`r`nLanguage=en`r`nStartupTab=Messages`r`n",[Text.Encoding]::Unicode)
            Invoke-ReviewProcess $run (Join-Path $run.Work 'bin\SimpleComChart.exe') @() 'production-startup' 30 -Private -Startup
        }
        . (Join-Path $PSScriptRoot 'finish_review_delivery.ps1') -Run $run -ProjectRoot $projectRoot -Evidence $script:reviewEvidence -ExpectedHashes $script:formalBaseline
    }
} -Teardown {
    param($run)
    foreach($owned in $script:reviewProcesses){$p=$owned.process;if(!$p.HasExited){$live=Get-Process -Id $p.Id -ErrorAction SilentlyContinue;if($live -and $live.StartTime.ToUniversalTime() -eq $owned.born){if(!$p.WaitForExit(15000)){Stop-Process -Id $p.Id -ErrorAction Stop;$p.WaitForExit()}}};if($owned.desktop -ne [IntPtr]::Zero){if(![ReviewPrivateDesktop]::CloseDesktop($owned.desktop)){throw 'Owned private desktop remains open'};$owned.desktop=[IntPtr]::Zero}}
    foreach($key in $prior.Keys){[Environment]::SetEnvironmentVariable($key,$prior[$key],'Process')}
} -CollectArtifacts {
    param($run)
    # All of work was allocated empty to the declared configure/build/fake-test producers above.
    $files=@(Get-ChildItem -LiteralPath $run.Work -Recurse -File -Force)
    foreach($file in $files){if($file.Extension -eq '.exe'){$script:reviewEvidence.hashes+=@{path=$file.FullName.Substring($run.Work.Length+1);bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}}}
    $script:reviewEvidence.generatedBytes=if($files.Count){($files|Measure-Object Length -Sum).Sum}else{0}
    Register-HubArtifactTree -Run $run -Producer ('V1-V9 isolated CMake/compiler/fake tests '+$Phase)
} -Evidence {
    param($run,$outcome)
    $script:reviewEvidence.run=$run.Root;$script:reviewEvidence.outcome=$outcome
    $path=Join-Path $projectRoot ('docs\verification\review-'+$Phase.ToLower()+'-'+$Configuration.ToLower()+'.json')
    [IO.File]::WriteAllText($path,($script:reviewEvidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
}
$result|Select-Object Success,Outcome,RunRoot,CleanupRemoved,Errors|ConvertTo-Json -Depth 5
if(!$result.Success){throw 'Selected review verification or cleanup failed.'}
