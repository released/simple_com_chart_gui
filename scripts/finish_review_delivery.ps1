param([Parameter(Mandatory=$true)]$Run,[Parameter(Mandatory=$true)][string]$ProjectRoot,
      [Parameter(Mandatory=$true)][System.Collections.IDictionary]$Evidence,
      [Parameter(Mandatory=$true)][hashtable]$ExpectedHashes)
$ErrorActionPreference='Stop'
$work=$Run.Work
if([IO.Path]::GetFileName($work) -cne 'work' -or !(Test-Path -LiteralPath (Join-Path (Split-Path -Parent $work) '.owner.json'))){throw 'Registered delivery work required'}
$owner=Read-HubTestRun $Run.Root
Assert-HubRunOwner $Run $owner
$fixture=([IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\') -ine $owner.project_root)
if($fixture){
    if($owner.purpose -notlike 'V1-V9 DeliveryCheck *'){throw 'Formal destination does not match source project'}
    [void](Assert-HubPath $work $ProjectRoot)
}
$version=[IO.File]::ReadAllText((Join-Path $ProjectRoot 'VERSION')).Trim()
$cache=[IO.File]::ReadAllText((Join-Path $work 'obj\CMakeCache.txt'))
if($cache -notmatch '(?m)^SCC_ENABLE_RUNTIME_CHECKS:BOOL=OFF\r?$'){throw 'Delivery must exclude runtime checks'}
if(!$fixture){
    $sourceRoot=[regex]::Match($cache,'(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)').Groups[1].Value
    if(!$sourceRoot -or [IO.Path]::GetFullPath($sourceRoot).TrimEnd('\') -ine $owner.project_root){throw 'CMake producer/source mismatch'}
}
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
$dumpbin=Get-ChildItem -LiteralPath (Join-Path $vs 'VC\Tools\MSVC') -Directory | Sort-Object Name -Descending | ForEach-Object {Join-Path $_.FullName 'bin\Hostx64\x64\dumpbin.exe'} | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
$items=New-Object 'Collections.Generic.List[object]'
$header=[IO.File]::ReadAllText((Join-Path $work 'obj\generated\tool_build_identity.h'))
$stamp=[regex]::Match($header,'(?m)^#define TOOL_BUILD_TIMESTAMP_ISO "([^"]+)"').Groups[1].Value
foreach($name in @('SimpleComChart.exe','SimpleComChartCli.exe','lang_en.ini','lang_tw.ini')){
    $source=Join-Path $work ('bin\'+$name);$target=Join-Path $ProjectRoot ('build\'+$name)
    [void](Assert-HubPath $work $source);[void](Assert-HubPath $ProjectRoot $target)
    if(!(Test-Path -LiteralPath $source) -or !(Test-Path -LiteralPath $target)){throw ('Missing candidate/formal: '+$name)}
    if((Get-Item -LiteralPath $target -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse delivery target refused'}
    $old=(Get-FileHash -LiteralPath $target).Hash;$hash=(Get-FileHash -LiteralPath $source).Hash
    if(!$ExpectedHashes.ContainsKey($name) -or $old -cne $ExpectedHashes[$name]){throw ('Formal output changed since the entry snapshot: '+$name)}
    if($name.EndsWith('.exe')){
        if((Get-Item -LiteralPath $source).VersionInfo.FileVersion -cne $version){throw ('Resource version mismatch: '+$name)}
        if([ReviewPrivateDesktop]::VersionString($source,'BuildDateTime') -cne $stamp){throw ('Resource build time mismatch: '+$name)}
        Invoke-ReviewProcess $Run $dumpbin @('/dependents',$source) ($name+'-imports')
        $imports=[IO.File]::ReadAllText((Join-Path $work ($name+'-imports.out.txt')))
        if($imports -match '(?i)(VCRUNTIME|MSVCP\d|MFC\d|UCRTBASE)'){throw ('Unexpected dynamic runtime: '+$name)}
    }elseif($hash -cne (Get-FileHash -LiteralPath (Join-Path $ProjectRoot ('src\i18n\'+$name))).Hash){throw 'Catalog candidate differs from source'}
    $item=[pscustomobject]@{name=$name;source=$source;target=$target;old=$old;hash=$hash;bytes=(Get-Item -LiteralPath $source).Length;backup=(Join-Path $work ('superseded-'+$name));promoted=$false}
    $items.Add($item)
}
try{
    foreach($item in $items){
        if((Get-FileHash -LiteralPath $item.target).Hash -cne $item.old){throw 'Delivery target changed before replacement'}
        [IO.File]::Replace($item.source,$item.target,$item.backup)
        $item.promoted=$true
        if((Get-FileHash -LiteralPath $item.target).Hash -cne $item.hash -or (Get-FileHash -LiteralPath $item.backup).Hash -cne $item.old){throw 'Atomic delivery verification failed'}
    }
}catch{
    foreach($item in @($items.ToArray()|Where-Object promoted)){
        if((Get-FileHash -LiteralPath $item.target).Hash -ceq $item.hash){[IO.File]::Replace($item.backup,$item.target,(Join-Path $work ('rejected-'+$item.name)))}
    }
    throw
}
$Evidence.delivery=@{version=$version;stamp=$stamp;runtimeChecks='OFF';staticRuntime=$true;files=@($items|ForEach-Object {@{name=$_.name;bytes=$_.bytes;sha256=$_.hash;previousSha256=$_.old}})}
# Superseded backups stay in owned work until the collector/retirement verifies them.
Write-Host ('PASS: guarded atomic '+$(if($fixture){'fixture transaction '}else{'delivery '})+$version)
