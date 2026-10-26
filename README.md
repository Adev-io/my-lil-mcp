# My lil MCP

A VCV Rack 2 module that is a local MCP server. Switch it on and a coding agent (Claude Code, Claude Desktop, Cursor, anything that speaks MCP over HTTP) can read the running patch, add and wire modules, set parameters, undo, bypass, move, save, and listen to the audio with real meters. It only ever listens on 127.0.0.1.

Forked from the Neural Harmonics MCP Server (MIT) and extended: every edit is undoable from Rack's Edit menu or the `vcvrack_undo` tool, requests from other hosts or from web pages are refused, and the LISTEN inputs feed a meter (RMS, peak, crest, DC, clipped samples, BS.1770 loudness).

## Use

1. Add **My lil MCP** to the patch and click SERVER. The latch lights lime while the server runs; BUSY blinks on every request. Port 2600 by default (right-click → Port).
2. Register it once: `claude mcp add --transport http --scope user rack http://127.0.0.1:2600/mcp` (Claude Desktop/Cursor: an HTTP server at that URL).
3. Ask: "list the modules in my patch", "measure the mixer output", "add a VCO and wire it to the filter", "undo that".

To hear what the agent hears, patch audio into LISTEN L/R, or let `vcvrack_measure` patch a module's output in for a second. The mascot's mouth follows whatever LISTEN hears.

## Tools

Patch: `vcvrack_get_status`, `vcvrack_list_modules`, `vcvrack_get_module`, `vcvrack_search_library`, `vcvrack_get_plugin`, `vcvrack_get_rack_layout`, `vcvrack_get_mcp_position`, `vcvrack_get/set_layout_preferences`, `vcvrack_get_sample_rate`.
Edit (all undoable): `vcvrack_add_module`, `vcvrack_delete_module`, `vcvrack_move_module`, `vcvrack_bypass_module`, `vcvrack_get_params`, `vcvrack_set_params`, `vcvrack_add_cable`, `vcvrack_delete_cable`, `vcvrack_get_module_state`, `vcvrack_set_module_state`, `vcvrack_undo`, `vcvrack_redo`, `vcvrack_save_patch`.
Listen: `vcvrack_measure`, `vcvrack_get_voltages`, `vcvrack_find_unpatched`.

`GET /status` is a plain health check. Everything else is JSON-RPC on `POST /mcp`.

Levels are dBFS against Rack's 10 V full scale, so a nominal ±5 V signal reads −6 dBFS peak. Loudness is ungated (whole window), so a window that holds silence reads lower than a gated meter would.

## Build

Needs MSYS2 (mingw64) and the Rack 2 SDK.

```sh
export RACK_DIR=/c/dev/Rack-SDK
make test      # core checks: JSON helpers, request guard, meter (K-weighting against the BS.1770 table)
make install   # builds and copies the plugin into Rack's plugins folder
make dist      # builds the .vcvplugin package into dist/
```

`tests/mcp_smoke.py --port 2601` drives an isolated Rack started by `tools/rig/launch.ps1` through the whole tool set (run the launcher with `pwsh -Command "& tools/rig/launch.ps1 -Patch tools/rig/patches/mcp.vcv -Extra Fundamental"` so the plugin list stays a list).

## Licence

MIT. Original MCP Server module © Neural Harmonics, MIT.
