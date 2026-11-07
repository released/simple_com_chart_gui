[CmdletBinding()]
param([switch]$DryRun,[switch]$Yes,[string]$Message='Update project documentation and functionality')
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'pc_tool_publish.ps1')
$root=Split-Path -Parent $PSScriptRoot
$target=@{Name='simple_com_chart_gui';Path=$root;Branch='main';Remote='https://github.com/released/simple_com_chart_gui.git';IdentityFiles=@('README.md','CMakeLists.txt','src/mfc_app.cpp');ReadmeTitle='Simple COM Chart GUI';DeliveryFiles=@('build/SimpleComChart.exe','build/SimpleComChartCli.exe','build/lang_en.ini','build/lang_tw.ini')}
try {
    $plan=New-PcPublishPlan $target
    Write-Host ('Repository: '+$target.Name+' -> '+$target.Remote+' ['+$target.Branch+']')
    Write-Host ('Reviewed paths: '+$plan.Snapshot.Pending.Count)
    foreach($path in $plan.Snapshot.Pending){Write-Host ('  '+$path)}
    if($DryRun){Write-Host 'DRY RUN PASS; no fetch, index, commit or remote changes.';exit 0}
    if(!$Yes -and (Read-Host 'Type PUSH to commit and publish this repository') -cne 'PUSH'){Write-Host 'Canceled; no Git changes.';exit 0}
    Invoke-PcPublication @($plan) $Message
} catch {Write-Error $_;exit 1}
