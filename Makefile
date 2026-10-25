# My lil MCP. Build in MSYS2 mingw64 with RACK_DIR pointing at the Rack 2 SDK (see README).
RACK_DIR ?= ../..
FLAGS += -Icore -Isrc/lib
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res LICENSE
include $(RACK_DIR)/arch.mk
ifdef ARCH_WIN
  FLAGS += -D_WIN32_WINNT=0x0A00
  LDFLAGS += -lws2_32
endif
include $(RACK_DIR)/plugin.mk
# cpp-httplib and the server are C++17; Rack's compile.mk asks for C++11.
CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS)) -std=c++17

test:
	$(MAKE) -C tests
.PHONY: test
