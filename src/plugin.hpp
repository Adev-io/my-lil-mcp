// plugin.hpp: the plugin instance and its one model.
// Author: Ari Klopfer / A_Development. MIT (forked from Neural Harmonics MCP Server, MIT)
#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelMcp;

// The panel colours as lights (tools/panel.py uses the same two).
struct LimeLight : GrayModuleLightWidget { LimeLight() { addBaseColor(nvgRGB(0xc8, 0xff, 0x3a)); } };
struct MagentaLight : GrayModuleLightWidget { MagentaLight() { addBaseColor(nvgRGB(0xff, 0x3d, 0x9a)); } };
