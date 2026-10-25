# Closes the isolated Rack started by launch.ps1 and deletes its user dir. It refuses any dir that is not an
# mcp-rig-* folder directly under %TEMP%, and warns if a VCV login token was saved (a fresh dir can get logged in).
param([Parameter(Mandatory)][ValidatePattern('\\mcp-rig-\d{8}-\d{6}$')][string]$Dir)
$ErrorActionPreference = 'Stop'
$full = [IO.Path]::GetFullPath($Dir).TrimEnd('\')
$tmp  = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\')
if ([IO.Path]::GetDirectoryName($full) -ne $tmp -or $full -notmatch '\\mcp-rig-\d{8}-\d{6}$') { throw "Refusing: $full" }
if (-not (Test-Path -LiteralPath $full -PathType Container)) { throw "No such dir: $full" }
if ((Get-Item -LiteralPath $full).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse point: $full" }
Get-Process Rack -ErrorAction SilentlyContinue | Where-Object {
    (Get-CimInstance Win32_Process -Filter "ProcessId=$($_.Id)").CommandLine -like "*$full*"
} | ForEach-Object { $_.CloseMainWindow() | Out-Null; $_ | Wait-Process -Timeout 30 }
$s = Get-Content "$full\settings.json" -Raw -ErrorAction SilentlyContinue | ConvertFrom-Json
if ($s.token) { Write-Warning "A VCV login token was saved in $full; deleting it with the dir." }
Remove-Item -LiteralPath $full -Recurse -Force
"deleted $full"
