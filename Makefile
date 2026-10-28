CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -Iinclude -DSAT_HAS_SFML

SRCDIR = src
INCDIR = include
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/sat_solver

.PHONY: all run SAT sat room clean

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsfml-graphics -lsfml-window -lsfml-system

run: all
	$(TARGET)

SAT:

sat: run

room: run

clean:
	@if exist $(BINDIR) rmdir /S /Q $(BINDIR)