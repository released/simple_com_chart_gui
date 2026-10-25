[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Join-Path $projectRoot $(if ($Configuration -eq 'Release') { 'build\SimpleComChart.exe' } else { 'build\Debug\SimpleComChart.exe' })
$sourceHash = (Get-FileHash -LiteralPath $source).Hash
$personalIni = [IO.Path]::ChangeExtension($source, '.ini')
$iniHash = if (Test-Path -LiteralPath $personalIni) { (Get-FileHash -LiteralPath $personalIni).Hash } else { $null }
$version = (Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim()
if ((Get-Item -LiteralPath $source).VersionInfo.FileVersion -ne $version) { throw 'Delivery EXE version is stale.' }

# Do not activate/close a user's existing owner when testing the actual delivery
# binary. A race still fails safely: only this script's own child PID is closed.
$guard = $null
if ([Threading.Mutex]::TryOpenExisting('Local\Released.SimpleComChart.UARTChart.v1.Owner', [ref]$guard)) {
    $guard.Dispose()
    throw 'A normal user session exists. Preserve it; retry this test when the session is idle/closed.'
}

if (-not ('ChartDeliveryProbe' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ChartDeliveryProbe {
    private delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumChildWindows(IntPtr owner, EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] private static extern IntPtr GetDlgItem(IntPtr window,int id);
    [DllImport("user32.dll", EntryPoint="GetWindowLongW")] private static extern int GetWindowStyle(IntPtr window, int index);
    [DllImport("user32.dll")] private static extern bool PostMessage(IntPtr window, uint message, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] private static extern IntPtr SendMessageTimeout(IntPtr window, uint message, IntPtr w, IntPtr l, uint flags, uint timeout, out IntPtr result);
    public static IntPtr FindMain(int processId, string version) {
        IntPtr result=IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr unused) {
            uint owner; GetWindowThreadProcessId(window,out owner);
            if(owner!=(uint)processId) return true;
            StringBuilder text=new StringBuilder(256); GetWindowText(window,text,text.Capacity);
            if(!text.ToString().StartsWith("Simple COM Chart "+version+" | ",StringComparison.Ordinal)) return true;
            result=window; return false;
        },IntPtr.Zero);
        return result;
    }
    private static int Read(IntPtr window,uint message) {
        IntPtr value;
        if(SendMessageTimeout(window,message,IntPtr.Zero,IntPtr.Zero,2,200,out value)==IntPtr.Zero) return -1;
        return value.ToInt32();
    }
    public static int[] Sample(IntPtr main) {
        int[] sample=new int[] {-1,-1,0,0};
        EnumChildWindows(main,delegate(IntPtr window,IntPtr unused) {
            StringBuilder name=new StringBuilder(64); GetClassName(window,name,name.Capacity);
            if(name.ToString()=="SysTabControl32") {sample[0]=Read(window,0x1304);sample[1]=Read(window,0x130b);}
            if(name.ToString()=="SysListView32") sample[2]=Math.Max(sample[2],Read(window,0x1004));
            if(name.ToString()=="Edit" && IsWindowVisible(window) && (GetWindowStyle(window,-16)&0x800)!=0)
                sample[3]=Math.Max(sample[3],Read(window,0x000e));
            return true;
        },IntPtr.Zero);
        return sample;
    }
    public static bool CloseOwned(IntPtr window,int processId) {
        uint owner; GetWindowThreadProcessId(window,out owner);
        return owner==(uint)processId && PostMessage(window,0x0010,IntPtr.Zero,IntPtr.Zero);
    }
    public static bool CommandOwned(IntPtr window,int processId,int command) {
        uint owner; GetWindowThreadProcessId(window,out owner);
        return owner==(uint)processId && PostMessage(window,0x0111,new IntPtr(command),IntPtr.Zero);
    }
    public static IntPtr FindRecordingOptions(int processId) {
        IntPtr result=IntPtr.Zero;
        EnumWindows(delegate(IntPtr window,IntPtr unused) {
            uint owner;GetWindowThreadProcessId(window,out owner);
            StringBuilder text=new StringBuilder(128);GetWindowText(window,text,text.Capacity);
            if(owner!=(uint)processId || text.ToString()!="Recording options") return true;
            result=window;return false;
        },IntPtr.Zero);
        return result;
    }
    public static bool RecordingControls(IntPtr window,int sizeId,int dailyId) {
        return Read(GetDlgItem(window,sizeId),0x000e)>0 && Read(GetDlgItem(window,dailyId),0x00f0)==1;
    }
}
'@
}

$resourceText = Get-Content -LiteralPath (Join-Path $projectRoot 'src\resource.h') -Raw
function Get-ResourceId([string]$Name) {
    $match = [regex]::Match($resourceText, "(?m)^#define\s+$Name\s+(\d+)")
    if (-not $match.Success) { throw "Missing resource ID: $Name" }
    return [int]$match.Groups[1].Value
}

$fixtureRoot = Join-Path $projectRoot ("build\verify\delivery-$Configuration-" + [guid]::NewGuid().ToString('N'))
function Assert-OwnedCopyPath([string]$Path) {
    $absolute = [IO.Path]::GetFullPath($Path)
    $verifyRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build\verify'))
    if (-not $absolute.StartsWith($verifyRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture escaped verification root.' }
    $current = $absolute
    while ($current) {
        if ((Test-Path -LiteralPath $current) -and ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Fixture reparse point rejected.' }
        $parent = [IO.Directory]::GetParent($current)
        if ($null -eq $parent) { break }
        $current = $parent.FullName
    }
}
Assert-OwnedCopyPath $fixtureRoot
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$results = [Collections.Generic.List[string]]::new()
$ownedCopies = [Collections.Generic.List[string]]::new()
try {
    foreach ($choice in @(@('Messages',0), @('Chart',1), @('4Charts',2))) {
        $fixture = Join-Path $fixtureRoot $choice[0]
        New-Item -ItemType Directory -Path $fixture | Out-Null
        $exe = Join-Path $fixture 'SimpleComChart.exe'
        Copy-Item -LiteralPath $source -Destination $exe
        $ownedCopies.Add($exe)
        if ((Get-FileHash -LiteralPath $exe).Hash -ne $sourceHash) { throw 'Delivery copy hash mismatch.' }
        [IO.File]::WriteAllText((Join-Path $fixture 'SimpleComChart.ini'), "[UI]`r`nStartupTab=$($choice[0])`r`n", [Text.Encoding]::Unicode)
        $owned = Start-Process -FilePath $exe -WorkingDirectory $fixture -WindowStyle Hidden -PassThru
        $quietUntil = [DateTime]::UtcNow.AddMilliseconds(1600)
        $frame = [IntPtr]::Zero
        try {
            $deadline = [DateTime]::UtcNow.AddSeconds(15)
            $ready = $false
            while ([DateTime]::UtcNow -lt $deadline) {
                $owned.Refresh()
                if ($owned.HasExited) { throw "Delivery exited before startup: $($owned.ExitCode)" }
                $frame = [ChartDeliveryProbe]::FindMain($owned.Id, $version)
                if ($frame -ne [IntPtr]::Zero) {
                    $sample = [ChartDeliveryProbe]::Sample($frame)
                    if ($sample[2] -gt 0 -or $sample[3] -gt 0) { throw 'Unexpected synthetic data in an idle delivery session.' }
                    if ($sample[0] -eq 3 -and $sample[1] -eq $choice[1] -and [DateTime]::UtcNow -ge $quietUntil) { $ready = $true; break }
                }
                Start-Sleep -Milliseconds 100
            }
            if (-not $ready) { throw "Delivery did not restore the empty $($choice[0]) page." }
            if ($choice[0] -eq 'Messages') {
                if (-not [ChartDeliveryProbe]::CommandOwned($frame, $owned.Id, (Get-ResourceId 'ID_UART_RECORD_OPTIONS'))) { throw 'Cannot open recording options.' }
                $dialog = [IntPtr]::Zero
                $deadline = [DateTime]::UtcNow.AddSeconds(5)
                while ([DateTime]::UtcNow -lt $deadline) {
                    $dialog = [ChartDeliveryProbe]::FindRecordingOptions($owned.Id)
                    if ($dialog -ne [IntPtr]::Zero -and [ChartDeliveryProbe]::RecordingControls($dialog,
                        (Get-ResourceId 'IDC_RECORD_MIB'), (Get-ResourceId 'IDC_RECORD_DAILY'))) { break }
                    Start-Sleep -Milliseconds 50
                }
                if ($dialog -eq [IntPtr]::Zero -or [ChartDeliveryProbe]::IsWindowEnabled($frame) -or
                    -not [ChartDeliveryProbe]::RecordingControls($dialog, (Get-ResourceId 'IDC_RECORD_MIB'), (Get-ResourceId 'IDC_RECORD_DAILY'))) { throw 'Recording dialog/standard controls did not initialize.' }
                if (-not [ChartDeliveryProbe]::CloseOwned($dialog, $owned.Id)) { throw 'Cannot cancel recording options.' }
                $deadline = [DateTime]::UtcNow.AddSeconds(5)
                while (-not [ChartDeliveryProbe]::IsWindowEnabled($frame) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 50 }
                if (-not [ChartDeliveryProbe]::IsWindowEnabled($frame)) { throw 'Recording dialog did not release its modal owner.' }
                $results.Add('PASS: actual delivery recording-options modal/edit/checkbox/cancel without starting a recording')
            }
            if (-not [ChartDeliveryProbe]::CloseOwned($frame, $owned.Id) -or -not $owned.WaitForExit(10000)) { throw 'Delivery normal close failed.' }
            if ($owned.ExitCode -ne 0) { throw "Delivery close exit code: $($owned.ExitCode)" }
            $results.Add("PASS: $($choice[0]) startup/three tabs/no synthetic rows or messages/normal close, exact delivery hash $sourceHash")
        } finally {
            $owned.Refresh()
            if (-not $owned.HasExited) {
                # Only a verified test-owned copy is eligible for timeout cleanup.
                if ([IO.Path]::GetFullPath($owned.MainModule.FileName) -ne $exe) { throw 'Unexpected child image; not terminating it.' }
                $owned.Kill(); $owned.WaitForExit(5000) | Out-Null
            }
            $owned.Dispose()
        }
    }
    $rejectExe = Join-Path $fixtureRoot 'Chart\SimpleComChart.exe'
    foreach ($blockedFlag in @('--demo', '--smoke-test')) {
        $reject = Start-Process -FilePath $rejectExe -ArgumentList $blockedFlag -WorkingDirectory (Split-Path -Parent $rejectExe) -WindowStyle Hidden -PassThru
        try {
            if (-not $reject.WaitForExit(5000) -or $reject.ExitCode -ne 2) { throw "Delivery did not reject $blockedFlag." }
            $results.Add("PASS: production $blockedFlag exits 2 without generating data or running self-tests")
        } finally {
            $reject.Refresh()
            if (-not $reject.HasExited -and [IO.Path]::GetFullPath($reject.MainModule.FileName) -eq $rejectExe) { $reject.Kill(); $reject.WaitForExit(5000) | Out-Null }
            $reject.Dispose()
        }
    }
    if ((Get-FileHash -LiteralPath $source).Hash -ne $sourceHash) { throw 'Canonical EXE changed during verification.' }
    $afterIni = if (Test-Path -LiteralPath $personalIni) { (Get-FileHash -LiteralPath $personalIni).Hash } else { $null }
    if ($afterIni -ne $iniHash) { throw 'Personal INI changed during verification.' }
    $results.Add('PASS: canonical EXE/personal INI unchanged; no real COM open/TX or copied private settings')
} catch {
    $results.Add("FAIL: $($_.Exception.Message)")
    throw
} finally {
    $cleanupFailed = $false
    $removedCopyBytes = 0L
    foreach ($copy in $ownedCopies) {
        try {
            Assert-OwnedCopyPath $copy
            if (-not (Test-Path -LiteralPath $copy -PathType Leaf)) { continue }
            if ((Get-FileHash -LiteralPath $copy).Hash -ne $sourceHash -or (Get-FileHash -LiteralPath $source).Hash -ne $sourceHash) { throw 'Changed copy/source retained for investigation.' }
            if (Get-CimInstance Win32_Process -Filter "Name='SimpleComChart.exe'" | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.Equals($copy, [StringComparison]::OrdinalIgnoreCase) }) { throw 'Copy is still running; preserved.' }
            $bytes = (Get-Item -LiteralPath $copy).Length
            Remove-Item -LiteralPath $copy
            if (Test-Path -LiteralPath $copy) { throw 'Exact-copy removal failed.' }
            $removedCopyBytes += $bytes
        } catch {
            $cleanupFailed = $true
            $results.Add("FAIL: test-copy cleanup: $copy : $($_.Exception.Message)")
        }
    }
    if (-not $cleanupFailed) { $results.Add("PASS: removed $removedCopyBytes bytes of test-owned EXE copies; canonical binary/hash and fixture INI/results retained") }
    [IO.File]::WriteAllLines((Join-Path $fixtureRoot 'delivery-result.txt'), $results, [Text.Encoding]::UTF8)
    $results | Write-Output
    Write-Host "Evidence retained: $fixtureRoot"
    if ($cleanupFailed) { throw 'Test-copy cleanup incomplete; see retained result.' }
}
