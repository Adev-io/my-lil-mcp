#include "plugin.hpp"

Plugin* pluginInstance;

extern "C" void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelMcp);
}
