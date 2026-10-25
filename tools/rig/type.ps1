# Types text then Enter into Rack's focused text field (e.g. the value field a right-click on a knob opens).
param([Parameter(Mandatory)][int]$ProcId, [Parameter(Mandatory)][string]$Text)
Add-Type @"
using System; using System.Runtime.InteropServices;
public class RigType { [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l); }
"@
$hwnd = (Get-Process -Id $ProcId).MainWindowHandle
Start-Sleep -Milliseconds 300
foreach ($ch in $Text.ToCharArray()) { [RigType]::PostMessage($hwnd, 0x0102, [IntPtr][int]$ch, [IntPtr]1) | Out-Null; Start-Sleep -Milliseconds 60 }
[RigType]::PostMessage($hwnd, 0x0100, [IntPtr]0x0D, [IntPtr]0x001C0001) | Out-Null; Start-Sleep -Milliseconds 60
[RigType]::PostMessage($hwnd, 0x0101, [IntPtr]0x0D, [IntPtr]0xC01C0001) | Out-Null
"typed '$Text' + Enter"
