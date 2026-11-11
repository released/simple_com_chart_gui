param([Parameter(Mandatory=$true)]$Run,[Parameter(Mandatory=$true)][string]$ProjectRoot)
$actualProject=$ProjectRoot
. (Join-Path $actualProject 'tests\private_desktop_process.ps1')
$fakeProject=Join-Path $Run.Work 'p'
foreach($part in @('p\build','p\src\i18n','bin','obj\generated')){[void][IO.Directory]::CreateDirectory((Join-Path $Run.Work $part))}
$baseline=@{};$actualHashes=@{}
foreach($name in @('SimpleComChart.exe','SimpleComChartCli.exe','lang_en.ini','lang_tw.ini')){
    $source=Join-Path $actualProject ('build\'+$name)
    $actualHashes[$name]=(Get-FileHash -LiteralPath $source).Hash
    Copy-Item -LiteralPath $source -Destination (Join-Path $fakeProject ('build\'+$name))
    $baseline[$name]=$actualHashes[$name]
    $candidate=if($name.EndsWith('.exe')){$source}else{Join-Path $actualProject ('src\i18n\'+$name)}
    Copy-Item -LiteralPath $candidate -Destination (Join-Path $Run.Work ('bin\'+$name))
    if(!$name.EndsWith('.exe')){Copy-Item -LiteralPath $candidate -Destination (Join-Path $fakeProject ('src\i18n\'+$name))}
}
$gui=Join-Path $Run.Work 'bin\SimpleComChart.exe'
$version=(Get-Item -LiteralPath $gui).VersionInfo.FileVersion
$stamp=[ReviewPrivateDesktop]::VersionString($gui,'BuildDateTime')
[IO.File]::WriteAllText((Join-Path $fakeProject 'VERSION'),$version)
[IO.File]::WriteAllText((Join-Path $Run.Work 'obj\CMakeCache.txt'),"SCC_ENABLE_RUNTIME_CHECKS:BOOL=OFF`r`n")
[IO.File]::WriteAllText((Join-Path $Run.Work 'obj\generated\tool_build_identity.h'),('#define TOOL_BUILD_TIMESTAMP_ISO "'+$stamp+'"'))
$helper=Join-Path $actualProject 'scripts\finish_review_delivery.ps1'
. $helper -Run $Run -ProjectRoot $fakeProject -Evidence $script:reviewEvidence -ExpectedHashes $baseline
$after=@{}
foreach($name in $baseline.Keys){$after[$name]=(Get-FileHash -LiteralPath (Join-Path $fakeProject ('build\'+$name))).Hash}
$script:reviewEvidence.results+=@{name='synthetic-delivery';lines=@('PASS: actual shared finisher/scoped process calls, resource/import checks and four-file atomic transaction on isolated copies')}
foreach($name in $baseline.Keys){Copy-Item -LiteralPath (Join-Path $fakeProject ('build\'+$name)) -Destination (Join-Path $Run.Work ('bin\'+$name))}
$overlay=[IO.File]::Open((Join-Path $Run.Work 'bin\SimpleComChart.exe'),[IO.FileMode]::Append,[IO.FileAccess]::Write,[IO.FileShare]::None)
try{$bytes=[Text.Encoding]::ASCII.GetBytes('SCC-distinct-rollback-fixture');$overlay.Write($bytes,0,$bytes.Length)}finally{$overlay.Dispose()}
if((Get-FileHash -LiteralPath (Join-Path $Run.Work 'bin\SimpleComChart.exe')).Hash -ceq $after['SimpleComChart.exe']){throw 'Rollback fixture must contain a distinct first candidate'}
$lock=[IO.File]::Open((Join-Path $fakeProject 'build\SimpleComChartCli.exe'),[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
$denied=$false
try{. $helper -Run $Run -ProjectRoot $fakeProject -Evidence $script:reviewEvidence -ExpectedHashes $after}catch{$denied=$true}finally{$lock.Dispose()}
if(!$denied){throw 'Synthetic second-file replacement denial was not observed'}
foreach($name in $after.Keys){if((Get-FileHash -LiteralPath (Join-Path $fakeProject ('build\'+$name))).Hash -cne $after[$name]){throw 'Synthetic rollback did not preserve the whole original'}}
foreach($name in $actualHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $actualProject ('build\'+$name))).Hash -cne $actualHashes[$name]){throw 'Actual formal output changed during synthetic verification'}}
$script:reviewEvidence.results+=@{name='synthetic-rollback';lines=@('PASS: denied second replacement rolls back the first; actual project outputs unchanged')}
