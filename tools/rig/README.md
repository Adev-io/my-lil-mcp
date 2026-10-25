# Rack rig (Windows, dev only)

Drives an isolated VCV Rack 2 Free to check the module by hand-free clicks. Never writes to %LOCALAPPDATA%\Rack2
(`-Extra` only copies plugins out of it). Run the commands from the repo root.

1. `make dist` (see the main README's Build section), then `pwsh -File tools/rig/launch.ps1 -Patch tools/rig/patches/mcp.vcv -Extra Fundamental` → prints `pid=` and `dir=`. The patch enables the server on port 2601, so it never collides with a live Rack on 2600.
2. `pwsh -File tools/rig/cap.ps1 -ProcId <pid> -Name <name>` → full-window PNG in Desktop\screenshots. Read pixel positions off it.
3. `click.ps1 -ProcId <pid> -ImgX <x> -ImgY <y> [-Button Right]`, `type.ps1 -ProcId <pid> -Text 5` (Enter is sent too).
4. Undo/Redo: click Rack's **Edit** menu (posted Ctrl+Z does not work: GLFW reads the real modifier keys).
5. `stop.ps1 -Dir <dir>` closes that Rack and deletes the dir.
click.ps1 moves the real cursor for ~0.4 s: don't run it while someone is using the mouse. Read click coordinates from a full-window cap.ps1 shot, not a crop.
Never trigger a native dialog (e.g. pasting junk): it blocks the window and the scripts cannot close it.
