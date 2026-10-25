# Screenshot of a Rack window via PrintWindow (Rack does not have to be in front). Optional crop + nearest-neighbour zoom.
# Saves to Desktop\screenshots by default (Ari's rule: every screenshot lands there).
param([Parameter(Mandatory)][int]$ProcId, [Parameter(Mandatory)][string]$Name, [int]$X = 0, [int]$Y = 0, [int]$W = 0, [int]$H = 0, [int]$Scale = 1)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class RigCap { [DllImport("user32.dll")] public static extern bool SetProcessDPIAware(); [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r); [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
public struct RECT { public int L, T, R, B; } }
"@
[RigCap]::SetProcessDPIAware() | Out-Null
$hwnd = (Get-Process -Id $ProcId).MainWindowHandle
$r = New-Object RigCap+RECT; [RigCap]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc(); [RigCap]::PrintWindow($hwnd, $dc, 2) | Out-Null; $g.ReleaseHdc($dc)
if ($W -gt 0) {
    $c = $bmp.Clone((New-Object System.Drawing.Rectangle $X, $Y, $W, $H), $bmp.PixelFormat)
    $bmp = New-Object System.Drawing.Bitmap ($W * $Scale), ($H * $Scale)
    $g2 = [System.Drawing.Graphics]::FromImage($bmp); $g2.InterpolationMode = 'NearestNeighbor'; $g2.DrawImage($c, 0, 0, $W * $Scale, $H * $Scale)
}
$dir = New-Item -ItemType Directory -Force "$env:USERPROFILE\Desktop\screenshots"
$out = Join-Path $dir.FullName "$Name.png"
$bmp.Save($out); $out
