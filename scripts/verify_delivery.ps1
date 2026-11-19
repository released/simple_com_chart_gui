[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release', [ValidateSet('tw','en')][string]$UiLanguage='tw', [string]$CandidateExe='')

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Join-Path $projectRoot $(if ($Configuration -eq 'Release') { 'build\SimpleComChart.exe' } else { 'build\Debug\SimpleComChart.exe' })
if($CandidateExe){$source=[IO.Path]::GetFullPath($CandidateExe)}
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

function Initialize-DeliveryProbes {
if (-not ('ChartDeliveryProbe' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ChartDeliveryProbe {
    const int MessagesPage=0, ChartPage=1, FourChartsPage=2;
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int left,top,right,bottom; }
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr window,out Rect rect);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window,out Rect rect);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr window,IntPtr after,int x,int y,int width,int height,uint flags);
    [DllImport("user32.dll")] static extern bool RedrawWindow(IntPtr window,IntPtr rect,IntPtr region,uint flags);
    [DllImport("user32.dll",EntryPoint="GetClassLongPtrW")] static extern IntPtr ClassValue(IntPtr window,int key);
    [DllImport("user32.dll")] static extern IntPtr GetSysColorBrush(int index);
    private delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumDesktopWindows(IntPtr desktop, EnumProc callback, IntPtr parameter);
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
    public static IntPtr FindRecordingOptions(IntPtr desktop, int processId) {
        IntPtr result=IntPtr.Zero;
        EnumDesktopWindows(desktop,delegate(IntPtr window,IntPtr unused) {
            uint owner;GetWindowThreadProcessId(window,out owner);
            StringBuilder text=new StringBuilder(128);GetWindowText(window,text,text.Capacity);
            if(owner!=(uint)processId || (text.ToString()!="Recording options" && text.ToString()!="\u9304\u88fd\u9078\u9805")) return true;
            result=window;return false;
        },IntPtr.Zero);
        return result;
    }
    public static bool RecordingControls(IntPtr window,int sizeId,int dailyId) {
        return Read(GetDlgItem(window,sizeId),0x000e)>0 && Read(GetDlgItem(window,dailyId),0x00f0)==1;
    }
    public static bool LanguageControls(IntPtr main,bool chinese) {
        StringBuilder text=new StringBuilder(128);GetWindowText(GetDlgItem(main,102),text,text.Capacity);
        return text.ToString()==(chinese?"\u6383\u63cf":"Scan");
    }
    static IntPtr ChildClass(IntPtr main,string name) {
        IntPtr found=IntPtr.Zero;
        EnumChildWindows(main,delegate(IntPtr w,IntPtr unused){StringBuilder s=new StringBuilder(80);GetClassName(w,s,s.Capacity);if(s.ToString()==name){found=w;return false;}return true;},IntPtr.Zero);
        return found;
    }
    public static bool SelectPage(IntPtr main,int pid,int page) {
        uint owner;GetWindowThreadProcessId(main,out owner);if(owner!=(uint)pid)return false;
        IntPtr tab=ChildClass(main,"SysTabControl32"),result;
        // Let the owned native tab produce notifications inside its own process.
        // Button tabs activate the focused item with Space; never send an NMHDR
        // pointer from this controller process or use global desktop input.
        if(tab==IntPtr.Zero || SendMessageTimeout(tab,0x1330,new IntPtr(page),IntPtr.Zero,0x22,2000,out result)==IntPtr.Zero)return false;
        if((GetWindowStyle(tab,-16)&0x100)!=0) {
            if(SendMessageTimeout(tab,0x0100,new IntPtr(32),new IntPtr(0x00390001),0x22,2000,out result)==IntPtr.Zero)return false;
            if(SendMessageTimeout(tab,0x0101,new IntPtr(32),new IntPtr(unchecked((int)0xc0390001)),0x22,2000,out result)==IntPtr.Zero)return false;
        }
        RedrawWindow(main,IntPtr.Zero,IntPtr.Zero,0x185);
        return Read(tab,0x130b)==page;
    }
    public static bool PageState(IntPtr main,int page) {
        IntPtr channels=ChildClass(main,"ChannelPanelWnd"),plot=ChildClass(main,"PlotViewWnd"),messages=ChildClass(main,"TelemetryMessages"),four=ChildClass(main,"TelemetryDataPage");
        if(channels==IntPtr.Zero || plot==IntPtr.Zero || messages==IntPtr.Zero || four==IntPtr.Zero)return false;
        if(IsWindowVisible(channels)!=(page==ChartPage) || IsWindowVisible(plot)!=(page==ChartPage) || IsWindowVisible(messages)!=(page==MessagesPage) || IsWindowVisible(four)!=(page==FourChartsPage))return false;
        if(page!=ChartPage)return true;
        Rect area;if(!GetClientRect(channels,out area) || area.right<10 || area.bottom<90)return false;
        IntPtr brush=ClassValue(channels,-10);
        return (GetWindowStyle(channels,-16)&0x02000000)!=0 && (brush==new IntPtr(16)||brush==GetSysColorBrush(15));
    }
    public static bool ResizeOwned(IntPtr main,int pid,int width,int height) {
        uint owner;GetWindowThreadProcessId(main,out owner);return owner==(uint)pid&&SetWindowPos(main,IntPtr.Zero,0,0,width,height,0x16);
    }
    public static string PageDiagnostic(IntPtr main) {
        IntPtr panel=ChildClass(main,"ChannelPanelWnd");
        return "selected="+Read(ChildClass(main,"SysTabControl32"),0x130b)+" channels="+IsWindowVisible(panel)+" messages="+IsWindowVisible(ChildClass(main,"TelemetryMessages"))+" plot="+IsWindowVisible(ChildClass(main,"PlotViewWnd"))+" four="+IsWindowVisible(ChildClass(main,"TelemetryDataPage"))+" style="+GetWindowStyle(panel,-16).ToString("X")+" brush="+ClassValue(panel,-10).ToInt64().ToString("X");
    }
}
'@
}

}

# Keep PowerShell registration functions in the caller's script scope as well as
# loading the process-wide CLR probe types.
. (Join-Path $projectRoot 'tests\private_desktop_process.ps1')

$resourceText = Get-Content -LiteralPath (Join-Path $projectRoot 'src\resource.h') -Raw
function Get-ResourceId([string]$Name) {
    $match = [regex]::Match($resourceText, "(?m)^#define\s+$Name\s+(\d+)")
    if (-not $match.Success) { throw "Missing resource ID: $Name" }
    return [int]$match.Groups[1].Value
}


. (Join-Path (Split-Path -Parent $projectRoot) '#READ_ME_FIRST#\scripts\invoke_hub_test.ps1')
$script:deliveryResults=New-Object 'Collections.Generic.List[string]'
$script:deliveryProcesses=New-Object 'Collections.Generic.List[object]'
$script:deliveryBytes=0L
$deliveryTemp=@{TEMP=$env:TEMP;TMP=$env:TMP}
function Start-DeliveryProcess($run,[string]$exe,[string]$arguments,[string]$cwd) {
    $native=[ReviewPrivateDesktop]::Start($exe,$arguments,$cwd)
    $p=$native.Process
    $script:deliveryProcesses.Add($native)
    Register-ReviewProcess $run $p
    return $native
}
$result=Invoke-HubOwnedTest -ProjectRoot $projectRoot -Purpose ('Formal delivery '+$Configuration+' '+$UiLanguage) -MaxBytes 128MB -Action {
    param($run)
    $temp=Join-Path $run.Work 'temp';[void][IO.Directory]::CreateDirectory($temp);$env:TEMP=$temp;$env:TMP=$temp
    Initialize-DeliveryProbes
    foreach($choice in @(@('Messages',0),@('Chart',1),@('4Charts',2))) {
        $fixture=Join-Path $run.Work $choice[0];[void][IO.Directory]::CreateDirectory($fixture)
        $exe=Join-Path $fixture 'SimpleComChart.exe';Copy-Item -LiteralPath $source -Destination $exe
        if((Get-FileHash -LiteralPath $exe).Hash -cne $sourceHash){throw 'Delivery copy hash mismatch'}
        foreach($name in @('lang_en.ini','lang_tw.ini')) {
            $catalog=Join-Path (Split-Path -Parent $source) $name;$copy=Join-Path $fixture $name
            Copy-Item -LiteralPath $catalog -Destination $copy
            if((Get-FileHash -LiteralPath $copy).Hash -cne (Get-FileHash -LiteralPath $catalog).Hash){throw 'Catalog copy hash mismatch'}
        }
        $languageLine=if($UiLanguage -eq 'en'){"Language=en`r`n"}else{''}
        [IO.File]::WriteAllText((Join-Path $fixture 'SimpleComChart.ini'),"[UI]`r`n${languageLine}StartupTab=$($choice[0])`r`n",[Text.Encoding]::Unicode)
        $native=Start-DeliveryProcess $run $exe '' $fixture;$owned=$native.Process
        $quietUntil=[DateTime]::UtcNow.AddMilliseconds(1600);$deadline=[DateTime]::UtcNow.AddSeconds(15);$frame=[IntPtr]::Zero;$ready=$false
        while([DateTime]::UtcNow -lt $deadline) {
            if($owned.HasExited){throw ('Delivery exited before startup: '+$owned.ExitCode)}
            $frame=[ReviewPrivateDesktop]::FindMain($native.Desktop,$owned.Id,$version)
            if($frame -ne [IntPtr]::Zero) {
                $sample=[ChartDeliveryProbe]::Sample($frame)
                if($sample[2] -gt 0 -or $sample[3] -gt 0){throw 'Unexpected synthetic data in an idle delivery session'}
                if($sample[0] -eq 3 -and $sample[1] -eq $choice[1] -and [DateTime]::UtcNow -ge $quietUntil){$ready=$true;break}
            }
            Start-Sleep -Milliseconds 100
        }
        if(!$ready){throw ('Delivery did not restore the empty '+$choice[0]+' page')}
        if([ReviewPrivateDesktop]::FindMain($native.Desktop,0,$version) -ne [IntPtr]::Zero -or [ReviewPrivateDesktop]::FindMain($native.Desktop,$owned.Id,'invalid-version') -ne [IntPtr]::Zero){throw 'Window identity accepted an incorrect PID/version'}
        if(![ReviewPrivateDesktop]::HasBuildLabel($frame,$version)){throw 'Prepared build label missing'}
        if(![ChartDeliveryProbe]::LanguageControls($frame,($UiLanguage -eq 'tw'))){throw 'Formal delivery language/default preference did not match'}
        if($choice[0] -eq 'Messages') {
            if(![ChartDeliveryProbe]::CommandOwned($frame,$owned.Id,(Get-ResourceId 'ID_UART_RECORD_OPTIONS'))){throw 'Cannot open recording options'}
            $dialog=[IntPtr]::Zero;$deadline=[DateTime]::UtcNow.AddSeconds(5)
            while([DateTime]::UtcNow -lt $deadline) {
                $dialog=[ChartDeliveryProbe]::FindRecordingOptions($native.Desktop,$owned.Id)
                if($dialog -ne [IntPtr]::Zero -and [ChartDeliveryProbe]::RecordingControls($dialog,(Get-ResourceId 'IDC_RECORD_MIB'),(Get-ResourceId 'IDC_RECORD_DAILY'))){break}
                Start-Sleep -Milliseconds 50
            }
            if($dialog -eq [IntPtr]::Zero -or [ChartDeliveryProbe]::IsWindowEnabled($frame) -or ![ChartDeliveryProbe]::RecordingControls($dialog,(Get-ResourceId 'IDC_RECORD_MIB'),(Get-ResourceId 'IDC_RECORD_DAILY'))){throw 'Recording dialog/standard controls did not initialize'}
            if(![ChartDeliveryProbe]::CloseOwned($dialog,$owned.Id)){throw 'Cannot cancel recording options'}
            $deadline=[DateTime]::UtcNow.AddSeconds(5)
            while(![ChartDeliveryProbe]::IsWindowEnabled($frame) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 50}
            if(![ChartDeliveryProbe]::IsWindowEnabled($frame)){throw 'Recording dialog did not release its modal owner'}
            $script:deliveryResults.Add('PASS: actual binary recording-options modal/edit/checkbox/cancel without starting recording')
            $original=New-Object ChartDeliveryProbe+Rect
            if(![ChartDeliveryProbe]::GetWindowRect($frame,[ref]$original)){throw 'Cannot retain owned window bounds'}
            foreach($size in @(@(800,780),@(1200,840))) {
                if(![ChartDeliveryProbe]::ResizeOwned($frame,$owned.Id,$size[0],$size[1])){throw 'Owned resize failed'}
                foreach($page in @(@('Chart',1),@('4Charts',2),@('Messages',0),@('Chart',1),@('Messages',0))) {
                    if(![ChartDeliveryProbe]::SelectPage($frame,$owned.Id,$page[1])){throw ('Native tab selection failed: '+$page[0]+' '+[ChartDeliveryProbe]::PageDiagnostic($frame))}
                    if(![ChartDeliveryProbe]::PageState($frame,$page[1])){throw ('Page visibility/background ownership failed: '+$page[0]+' '+[ChartDeliveryProbe]::PageDiagnostic($frame))}
                }
            }
            if(![ChartDeliveryProbe]::ResizeOwned($frame,$owned.Id,$original.right-$original.left,$original.bottom-$original.top)){throw 'Cannot restore owned window bounds'}
            $script:deliveryResults.Add('PASS: actual Messages/Chart/4Charts tab cycles and resize; exclusive page visibility and deployed system background/child clipping')
        }
        if(![ReviewPrivateDesktop]::CloseOwned($frame,$owned.Id) -or !$owned.WaitForExit(10000) -or $owned.ExitCode -ne 0){throw 'Delivery normal close failed'}
        $script:deliveryResults.Add('PASS: '+$choice[0]+' startup/three tabs/no synthetic data/normal close; wrong PID/version rejected; hash '+$sourceHash)
    }
    foreach($flag in @('--demo','--smoke-test')) {
        $native=Start-DeliveryProcess $run $exe $flag $fixture;$reject=$native.Process
        if(!$reject.WaitForExit(5000) -or $reject.ExitCode -ne 2){throw ('Delivery did not reject '+$flag)}
        $script:deliveryResults.Add('PASS: production '+$flag+' exits 2')
    }
    if((Get-FileHash -LiteralPath $source).Hash -cne $sourceHash){throw 'Selected source EXE changed during verification'}
    $afterIni=if(Test-Path -LiteralPath $personalIni){(Get-FileHash -LiteralPath $personalIni).Hash}else{$null}
    if($afterIni -ne $iniHash){throw 'Source personal INI changed during verification'}
    $script:deliveryResults.Add('PASS: source EXE/personal INI unchanged; private non-input desktops; no real COM/TX')
} -Teardown {
    param($run)
    $errors=New-Object 'Collections.Generic.List[string]'
    foreach($native in $script:deliveryProcesses) {
        try {
            $p=$native.Process
            if(!$p.HasExited){$p.Kill();if(!$p.WaitForExit(5000)){throw 'Owned test process did not stop'}}
            if(![ReviewPrivateDesktop]::CloseDesktop($native.Desktop)){throw 'Owned private desktop did not close'}
        }catch{$errors.Add($_.Exception.Message)}
    }
    $env:TEMP=$deliveryTemp.TEMP;$env:TMP=$deliveryTemp.TMP
    if($errors.Count){throw ($errors -join '; ')}
} -CollectArtifacts {
    param($run)
    # This fresh work belongs exclusively to the copied EXE/catalog/INI/test producers above.
    $script:deliveryBytes=(Get-ChildItem -LiteralPath $run.Work -Recurse -File|Measure-Object Length -Sum).Sum
    Register-HubArtifactTree -Run $run -Producer 'Hash-matched isolated formal delivery checks'
} -Evidence {
    param($run,$outcome)
    $evidence=@{outcome=$outcome;version=$version;source=$source;sha256=$sourceHash;candidate=[bool]$CandidateExe;language=$UiLanguage;configuration=$Configuration;generatedBytes=$script:deliveryBytes;results=$script:deliveryResults.ToArray()}
    [IO.File]::WriteAllText((Join-Path $projectRoot 'docs\verification\delivery-latest.json'),($evidence|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
}
$script:deliveryResults|Write-Output
$result|Select-Object Success,Outcome,CleanupRemoved,Errors|ConvertTo-Json -Depth 5
foreach($native in $script:deliveryProcesses){$native.Process.Dispose()}
if(!$result.Success){throw 'Formal delivery verification or owned cleanup failed'}
