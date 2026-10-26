# Starts an isolated VCV Rack: a fresh user dir under %TEMP% holding only the dist build of this plugin (plus any
# extra plugins named, copied from Ari's Rack), with tips turned off. Ari's own Rack data is never touched.
# Usage: pwsh -File tools/rig/launch.ps1 -Patch tools/rig/patches/one-module.vcv [-Extra Fundamental]
param([Parameter(Mandatory)][string]$Patch, [string[]]$Extra = @())
$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot)
$dist = Join-Path $repo 'dist\MyLilMcp'
if (-not (Test-Path "$dist\plugin.dll")) { throw "No build at $dist. Run make dist first." }
if (Get-Process Rack -ErrorAction SilentlyContinue) { throw "A Rack is already running (Rack Free allows one instance). Close it first." }
$patchPath = (Resolve-Path $Patch).Path
foreach ($x in $Extra) { if (-not (Test-Path "$env:LOCALAPPDATA\Rack2\plugins-win-x64\$x")) { throw "No plugin $x in Ari's Rack" } }
$u = Join-Path $env:TEMP ("mcp-rig-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory "$u\plugins-win-x64" | Out-Null
Copy-Item $dist "$u\plugins-win-x64\" -Recurse
foreach ($x in $Extra) { Copy-Item "$env:LOCALAPPDATA\Rack2\plugins-win-x64\$x" "$u\plugins-win-x64\" -Recurse }
'{ "showTipsOnLaunch": false, "tipIndex": -1 }' | Set-Content -Encoding utf8NoBOM "$u\settings.json"
$p = Start-Process 'C:\Program Files\VCV\Rack2Free\Rack.exe' -ArgumentList '-u', $u, $patchPath -PassThru
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Milliseconds 500
    if ($p.HasExited -or (Get-Process -Id $p.Id -ErrorAction SilentlyContinue).MainWindowHandle -ne 0) { break }
}
if ($p.HasExited) { throw "Rack exited (code $($p.ExitCode)). See $u\log.txt" }
$h = (Get-Process -Id $p.Id -ErrorAction SilentlyContinue).MainWindowHandle   # an IntPtr: compare with 0, -not is always false
if ($null -eq $h -or $h -eq 0) { throw "No Rack window after 30 s (pid $($p.Id)); dir $u kept" }
Start-Sleep 3
"pid=$($p.Id)"
"dir=$u"
