CXX ?= g++
CXXFLAGS += -std=c++17 -Wall -Wextra -pedantic

SRC_DIR := src
BUILD_DIR := build
ALL_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
GUI_SOURCES := $(SRC_DIR)/GuiApp.cpp $(SRC_DIR)/SoundManager.cpp $(SRC_DIR)/TextInput.cpp $(SRC_DIR)/UITheme.cpp

ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    MKDIR_CMD := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
    RM_CMD := if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
    NULL_DEVICE := NUL
else
    UNAME_S := $(shell uname -s)
    EXE_EXT :=
    MKDIR_CMD := mkdir -p "$(BUILD_DIR)"
    RM_CMD := rm -rf "$(BUILD_DIR)"
    NULL_DEVICE := /dev/null
endif

ifeq ($(OS),Windows_NT)
    PKG_CONFIG := $(shell where pkg-config 2>$(NULL_DEVICE))
    BREW_SFML_PREFIX :=
else
    PKG_CONFIG := $(shell command -v pkg-config 2>$(NULL_DEVICE))
    BREW_SFML_PREFIX := $(shell brew --prefix sfml 2>$(NULL_DEVICE))
endif

ifneq ($(strip $(PKG_CONFIG)),)
    SFML_CFLAGS := $(shell pkg-config --cflags sfml-graphics sfml-window sfml-system sfml-audio 2>$(NULL_DEVICE))
    SFML_LIBS := $(shell pkg-config --libs sfml-graphics sfml-window sfml-system sfml-audio 2>$(NULL_DEVICE))
endif
ifeq ($(strip $(SFML_LIBS)),)
    ifneq ($(strip $(BREW_SFML_PREFIX)),)
        SFML_CFLAGS := -I$(BREW_SFML_PREFIX)/include
        SFML_LIBS := -L$(BREW_SFML_PREFIX)/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
    else
        SFML_LIBS := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
    endif
endif

ifeq ($(OS),Windows_NT)
    PROBE_MKDIR := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
else
    PROBE_MKDIR := mkdir -p "$(BUILD_DIR)"
endif
$(shell $(PROBE_MKDIR))
SFML_PROBE_FILE := $(BUILD_DIR)/sfml_probe.cpp
$(file >$(SFML_PROBE_FILE),#include <SFML/Graphics.hpp>)
$(file >>$(SFML_PROBE_FILE),#include <SFML/Audio.hpp>)
$(file >>$(SFML_PROBE_FILE),int main() { return 0; })
SFML_PROBE := $(shell $(CXX) -std=c++17 $(SFML_CFLAGS) -x c++ "$(SFML_PROBE_FILE)" -fsyntax-only >$(NULL_DEVICE) 2>&1 && echo yes)

ifeq ($(SFML_PROBE),yes)
    CPPFLAGS += -Iinclude -DSAT_HAS_SFML
    SOURCES := $(ALL_SOURCES)
    SFML_MESSAGE := SFML encontrada: se compilara la interfaz grafica.
else
    CPPFLAGS += -Iinclude
    SOURCES := $(filter-out $(GUI_SOURCES),$(ALL_SOURCES))
    SFML_CFLAGS :=
    SFML_LIBS :=
    SFML_MESSAGE := SFML no encontrada: se compilara la version de consola. Instala SFML segun README.md para activar la ventana.
endif

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET := $(BUILD_DIR)/sat_solver$(EXE_EXT)

.PHONY: all run SAT sat room clean

all: $(TARGET)
	@echo "$(SFML_MESSAGE)"

$(BUILD_DIR):
	@$(MKDIR_CMD)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SFML_CFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SFML_LIBS)

run: all
	./$(TARGET)

SAT:

sat: run

room: run

clean:
	@$(RM_CMD)

-include $(OBJECTS:.o=.d)
