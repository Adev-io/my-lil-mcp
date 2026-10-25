// plugin.hpp: the plugin instance and its one model.
// Author: Ari Klopfer / A_Development. MIT (forked from Neural Harmonics MCP Server, MIT)
#pragma once
#include <rack.hpp>
#include <vector>

using namespace rack;

namespace rack {
namespace plugin {
extern std::vector<Plugin*> plugins;
}
}

extern Plugin* pluginInstance;
extern Model* modelMcp;
