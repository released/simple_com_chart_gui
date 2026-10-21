param()

function Test-Admin {
    $current = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($current)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    Write-Host "[INFO] Elevation required. Relaunching as Administrator..." -ForegroundColor Yellow
    $args = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    Start-Process -FilePath "powershell.exe" -ArgumentList $args -Verb RunAs
    exit 0
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Host "[ERROR] vswhere not found. Please install Visual Studio Installer." -ForegroundColor Red
    exit 1
}

$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installPath) {
    Write-Host "[ERROR] Visual Studio installation not found." -ForegroundColor Red
    exit 1
}

$installer = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vs_installer.exe"
if (-not (Test-Path $installer)) {
    Write-Host "[ERROR] Visual Studio Installer not found." -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Using VS install: $installPath"

$components = @(
    "Microsoft.VisualStudio.Workload.NativeDesktop",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
    "Microsoft.VisualStudio.Component.Windows10SDK.19041",
    "Microsoft.VisualStudio.Component.VC.CMake.Project"
)

$argList = @(
    "modify",
    "--installPath", "`"$installPath`"",
    "--includeRecommended",
    "--passive",
    "--norestart"
)

foreach ($c in $components) {
    $argList += @("--add", $c)
}

Write-Host "[INFO] Installing C++ build components..." -ForegroundColor Cyan
Write-Host "[INFO] This may take several minutes."
Write-Host "[INFO] If the installer UI opens, keep it running until it finishes."

$argString = ($argList -join " ")
$proc = Start-Process -FilePath $installer -ArgumentList $argString -Wait -PassThru

if ($proc.ExitCode -eq 0) {
    Write-Host "[OK] Components installed." -ForegroundColor Green
} else {
    Write-Host "[ERROR] Installer failed with exit code $($proc.ExitCode)." -ForegroundColor Red
    if ($proc.ExitCode -eq 5007) {
        Write-Host "[HINT] Please run this script as Administrator from the start." -ForegroundColor Yellow
    } elseif ($proc.ExitCode -eq 1001) {
        Write-Host "[HINT] The installer was canceled or closed." -ForegroundColor Yellow
    }
    Write-Host "[INFO] Trying interactive mode..." -ForegroundColor Yellow
    $argList = @(
        "modify",
        "--installPath", "`"$installPath`"",
        "--includeRecommended",
        "--norestart"
    )
    foreach ($c in $components) { $argList += @("--add", $c) }
    $argString = ($argList -join " ")
    Start-Process -FilePath $installer -ArgumentList $argString
    Write-Host "[INFO] Interactive installer launched. Complete it, then re-run build." -ForegroundColor Yellow
    exit $proc.ExitCode
}
