[CmdletBinding(SupportsShouldProcess)]
param()
$ErrorActionPreference = 'Stop'
if ($PSCmdlet.ShouldProcess('known compiler files and Python caches', 'Clean')) {
    & (Join-Path $PSScriptRoot 'clean_python.ps1') -PreflightOnly
    & (Join-Path $PSScriptRoot 'clean_intermediates.ps1')
    & (Join-Path $PSScriptRoot 'clean_python.ps1')
}
