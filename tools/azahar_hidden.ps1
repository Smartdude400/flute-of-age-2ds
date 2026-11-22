# Starts the portable Azahar (tools/bin/azahar, own "user" config with the RPC server enabled) on a
# separate, never-shown Windows desktop, so test runs do not pop up a window. Prints the process id.
# usage: powershell -File azahar_hidden.ps1 -Rom <path.3dsx> -Emu <azahar.exe>
param([Parameter(Mandatory = $true)][string]$Rom,
      [string]$Emu = "$PSScriptRoot\..\..\tools\bin\azahar\azahar.exe")

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class HiddenDesk {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct STARTUPINFO {
        public int cb; public string lpReserved; public string lpDesktop; public string lpTitle;
        public int dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
        public short wShowWindow, cbReserved2; public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION { public IntPtr hProcess, hThread; public int dwProcessId, dwThreadId; }
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr CreateDesktop(string name, IntPtr dev, IntPtr mode, int flags, uint access, IntPtr sa);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool CreateProcess(string app, string cmd, IntPtr pa, IntPtr ta, bool inherit, uint flags,
        IntPtr env, string dir, ref STARTUPINFO si, out PROCESS_INFORMATION pi);
    public static int Start(string desk, string exe, string args, string dir) {
        IntPtr h = CreateDesktop(desk, IntPtr.Zero, IntPtr.Zero, 0, 0x10000000u /* GENERIC_ALL */, IntPtr.Zero);
        if (h == IntPtr.Zero) throw new Exception("CreateDesktop failed " + Marshal.GetLastWin32Error());
        STARTUPINFO si = new STARTUPINFO();
        si.cb = Marshal.SizeOf(si);
        si.lpDesktop = "WinSta0\\" + desk;
        PROCESS_INFORMATION pi;
        if (!CreateProcess(exe, "\"" + exe + "\" " + args, IntPtr.Zero, IntPtr.Zero, false, 0, IntPtr.Zero, dir, ref si, out pi))
            throw new Exception("CreateProcess failed " + Marshal.GetLastWin32Error());
        return pi.dwProcessId;
    }
}
"@

$exe = (Resolve-Path $Emu).Path
$romPath = (Resolve-Path $Rom).Path
$procId = [HiddenDesk]::Start("foa_test_desktop", $exe, "`"$romPath`"", (Split-Path $exe))
Write-Output $procId
