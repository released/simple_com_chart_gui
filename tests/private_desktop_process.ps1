if('ReviewPrivateDesktop' -as [type]){return}
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class ReviewPrivateDesktop {
 [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] public struct SI {
  public int cb; public string reserved, desktop, title; public int x,y,w,h,cx,cy,fill,flags; public short show,cb2; public IntPtr r2,input,output,error;
 }
 [StructLayout(LayoutKind.Sequential)] public struct PI {public IntPtr process,thread;public int pid,tid;}
 public class Owned {public IntPtr Desktop;public int Pid;}
 delegate bool EnumProc(IntPtr window,IntPtr data);
 [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr CreateDesktop(string n,IntPtr d,IntPtr m,int f,uint a,IntPtr s);
 [DllImport("user32.dll",SetLastError=true)] public static extern bool CloseDesktop(IntPtr d);
 [DllImport("user32.dll")] static extern bool EnumDesktopWindows(IntPtr desktop,EnumProc callback,IntPtr data);
 [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr owner,EnumProc callback,IntPtr data);
 [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr w,out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr w,StringBuilder text,int count);
 [DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr w);
 [DllImport("user32.dll")] static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool CreateProcess(string app,string command,IntPtr pa,IntPtr ta,bool inherit,uint flags,IntPtr env,string cwd,ref SI si,out PI pi);
 [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern int GetFileVersionInfoSize(string path,out int unused);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern bool GetFileVersionInfo(string path,int unused,int length,IntPtr buffer);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern bool VerQueryValue(IntPtr buffer,string key,out IntPtr value,out int length);
 public static string VersionString(string path,string key) {
  int unused;int size=GetFileVersionInfoSize(path,out unused);if(size<=0||size>1048576)throw new Exception("Invalid version resource size");
  byte[] bytes=new byte[size];GCHandle pin=GCHandle.Alloc(bytes,GCHandleType.Pinned);
  try{IntPtr buffer=pin.AddrOfPinnedObject();if(!GetFileVersionInfo(path,0,size,buffer))throw new Exception("Version resource read failed");IntPtr value;int length;
   if(!VerQueryValue(buffer,"\\StringFileInfo\\040904b0\\"+key,out value,out length)||length<1)throw new Exception("Missing version resource key");
   return Marshal.PtrToStringUni(value,length-1);
  }finally{pin.Free();}
 }
 public static Owned Start(string exe,string args,string cwd) {
  string name="SccReview_"+Guid.NewGuid().ToString("N");IntPtr d=CreateDesktop(name,IntPtr.Zero,IntPtr.Zero,0,0xff,IntPtr.Zero);
  if(d==IntPtr.Zero)throw new System.ComponentModel.Win32Exception();
  SI si=new SI();si.cb=Marshal.SizeOf(si);si.desktop=name;PI pi;
  if(!CreateProcess(exe,"\""+exe+"\" "+args,IntPtr.Zero,IntPtr.Zero,false,0,IntPtr.Zero,cwd,ref si,out pi)){CloseDesktop(d);throw new System.ComponentModel.Win32Exception();}
  CloseHandle(pi.thread);CloseHandle(pi.process);return new Owned{Desktop=d,Pid=pi.pid};
 }
 public static IntPtr FindMain(IntPtr desktop,int pid,string version) {
  IntPtr found=IntPtr.Zero;
  EnumDesktopWindows(desktop,delegate(IntPtr w,IntPtr unused){uint id;GetWindowThreadProcessId(w,out id);if(id!=(uint)pid)return true;StringBuilder s=new StringBuilder(256);GetWindowText(w,s,s.Capacity);if(s.ToString().StartsWith("Simple COM Chart v"+version,StringComparison.Ordinal)){found=w;return false;}return true;},IntPtr.Zero);
  return found;
 }
 public static bool HasBuildLabel(IntPtr main,string version) {
  bool found=false;EnumChildWindows(main,delegate(IntPtr w,IntPtr unused){StringBuilder s=new StringBuilder(256);GetWindowText(w,s,s.Capacity);if(s.ToString().Contains("v"+version+" | BUILD "))found=true;return true;},IntPtr.Zero);return found;
 }
 public static bool CloseOwned(IntPtr w,int pid) {uint id;GetWindowThreadProcessId(w,out id);return id==(uint)pid&&PostMessage(w,0x10,IntPtr.Zero,IntPtr.Zero);}
}
'@
