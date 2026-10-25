# Click at screenshot pixel coordinates (from cap.ps1's full-window image). Rack reads the real cursor, so the cursor
# moves there for ~0.4 s, the button messages are posted to Rack's window only, then the cursor goes back.
param([Parameter(Mandatory)][int]$ProcId, [Parameter(Mandatory)][int]$ImgX, [Parameter(Mandatory)][int]$ImgY, [ValidateSet('Left', 'Right')][string]$Button = 'Left')
Add-Type @"
using System; using System.Runtime.InteropServices;
public class RigClick { [DllImport("user32.dll")] public static extern bool SetProcessDPIAware(); [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r); [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l); [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y); [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
public struct RECT { public int L, T, R, B; } public struct POINT { public int X, Y; } }
"@
[RigClick]::SetProcessDPIAware() | Out-Null
$hwnd = (Get-Process -Id $ProcId).MainWindowHandle
$r = New-Object RigClick+RECT; [RigClick]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$o = New-Object RigClick+POINT; [RigClick]::ClientToScreen($hwnd, [ref]$o) | Out-Null
$cx = $ImgX - ($o.X - $r.L); $cy = $ImgY - ($o.Y - $r.T)
$was = New-Object RigClick+POINT; [RigClick]::GetCursorPos([ref]$was) | Out-Null
$lp = [IntPtr](($cy -shl 16) -bor ($cx -band 0xffff))
$down, $up, $mk = if ($Button -eq 'Left') { 0x0201, 0x0202, 1 } else { 0x0204, 0x0205, 2 }
try {
    [RigClick]::SetCursorPos($o.X + $cx, $o.Y + $cy) | Out-Null
    [RigClick]::PostMessage($hwnd, 0x0200, [IntPtr]0, $lp) | Out-Null; Start-Sleep -Milliseconds 150
    [RigClick]::PostMessage($hwnd, $down, [IntPtr]$mk, $lp) | Out-Null; Start-Sleep -Milliseconds 100
    [RigClick]::PostMessage($hwnd, $up, [IntPtr]0, $lp) | Out-Null; Start-Sleep -Milliseconds 150
} finally { [RigClick]::SetCursorPos($was.X, $was.Y) | Out-Null }
"$Button click at $ImgX,$ImgY"
