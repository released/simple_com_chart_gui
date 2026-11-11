param([Parameter(Mandatory=$true)]$Run,[Parameter(Mandatory=$true)][string]$ProjectRoot,[Parameter(Mandatory=$true)][string]$Cmake)
$ErrorActionPreference='Stop'
$fixture=Join-Path $Run.Work 'i'
$null=New-Item -ItemType Directory -Path (Join-Path $fixture 'src'),(Join-Path $fixture 'scripts')
$version=Join-Path $fixture 'VERSION';[IO.File]::WriteAllText($version,'1.0.0.001')
$helper=Join-Path $fixture 'scripts\prepare_tool_build_identity.ps1'
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'scripts\prepare_tool_build_identity.ps1') -Destination $helper
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'src\build_version.h.in') -Destination (Join-Path $fixture 'src\build_version.h.in')
# Exercise the actual identity configuration, with only its independently supplied source inputs replaced.
$project=[IO.File]::ReadAllText((Join-Path $ProjectRoot 'CMakeLists.txt'))
$prefix=$project.Substring(0,$project.IndexOf('# MFC:'))
$tail=@'
add_executable(identity_probe probe.cpp probe.rc)
target_include_directories(identity_probe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
'@
[IO.File]::WriteAllText((Join-Path $fixture 'CMakeLists.txt'),$prefix+$tail)
[IO.File]::WriteAllText((Join-Path $fixture 'probe.cpp'),@'
#include "tool_build_identity.h"
#include <iostream>
int main(){std::cout<<TOOL_BUILD_VERSION_TEXT<<"|"<<TOOL_BUILD_TIMESTAMP_ISO;}
'@)
[IO.File]::WriteAllText((Join-Path $fixture 'probe.rc'),@'
#include "tool_build_identity.h"
#include <windows.h>
1 VERSIONINFO
FILEVERSION TOOL_BUILD_VERSION_NUMERIC
PRODUCTVERSION TOOL_BUILD_VERSION_NUMERIC
BEGIN
 BLOCK "StringFileInfo"
 BEGIN
  BLOCK "040904b0"
  BEGIN
   VALUE "FileVersion", TOOL_BUILD_VERSION_TEXT
  END
 END
END
'@)
$obj=Join-Path $Run.Work 'ib'
Invoke-ReviewProcess $Run $Cmake @('-S',$fixture,'-B',$obj,'-G','Visual Studio 17 2022','-A','x64') 'identity-configure'
$header=Join-Path $obj 'generated\tool_build_identity.h'
$initial=[IO.File]::ReadAllText($header)
Invoke-ReviewProcess $Run $Cmake @('--build',$obj,'--config','Release','--','/nr:false') 'identity-first-build'
$exe=Join-Path $obj 'Release\identity_probe.exe'
Invoke-ReviewProcess $Run $exe @() 'identity-first'
Invoke-ReviewProcess $Run $Cmake @('-S',$fixture,'-B',$obj) 'identity-reconfigure'
if([IO.File]::ReadAllText($header) -cne $initial){throw 'Ordinary reconfigure changed prepared identity'}
[IO.File]::WriteAllText($version,'1.0.0.002')
Invoke-ReviewProcess $Run $Cmake @('--build',$obj,'--config','Release','--','/nr:false') 'identity-version-incremental'
Invoke-ReviewProcess $Run $exe @() 'identity-version-consumer'
if([IO.File]::ReadAllText((Join-Path $Run.Work 'identity-version-consumer.out.txt')) -notmatch '^1\.0\.0\.002\|'){throw 'Direct build did not regenerate VERSION consumer'}
if((Get-Item -LiteralPath $exe).VersionInfo.FileVersion -ne '1.0.0.002'){throw 'Resource consumer has stale version'}
$prepared=[IO.File]::ReadAllText($header)
[IO.File]::AppendAllText($helper,"`r`n# Synthetic dependency edit.`r`n")
Invoke-ReviewProcess $Run $Cmake @('--build',$obj,'--config','Release','--','/nr:false') 'identity-helper-incremental'
if([IO.File]::ReadAllText($header) -cne $prepared){throw 'Helper dependency regeneration changed the reused timestamp'}
if([IO.File]::ReadAllText((Join-Path $Run.Work 'identity-helper-incremental.out.txt')) -notmatch 'Prepared v1.0.0.002'){throw 'Helper edit failed to trigger configuration'}
Invoke-ReviewProcess $Run 'powershell.exe' @('-NoProfile','-ExecutionPolicy','Bypass','-File',$helper,'-VersionFile',$version,'-OutputHeader',$header,'-NewPreparation','-Stamp','2026-11-11T12:34:56+08:00') 'identity-explicit-preparation'
Invoke-ReviewProcess $Run $Cmake @('--build',$obj,'--config','Release','--','/nr:false') 'identity-prepared-build'
Invoke-ReviewProcess $Run $exe @() 'identity-new-consumer'
if([IO.File]::ReadAllText((Join-Path $Run.Work 'identity-new-consumer.out.txt')) -cne '1.0.0.002|2026-11-11T12:34:56+08:00'){throw 'Consumer did not use the new immutable preparation'}
$script:reviewEvidence.results+=@{name='build-identity';lines=@('PASS: ordinary configure reuses timestamp','PASS: direct build tracks VERSION and helper','PASS: C++ and RC consumers agree','PASS: explicit preparation changes timestamp without another counter')}
