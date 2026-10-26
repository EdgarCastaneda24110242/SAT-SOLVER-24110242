CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -DSAT_HAS_SFML

BINDIR = bin

SOURCES = $(wildcard *.cpp)
OBJECTS = $(patsubst %.cpp,$(BINDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/sat_solver

.PHONY: all run SAT sat room clean

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsfml-graphics -lsfml-window -lsfml-system

run: all
	$(TARGET)

SAT:

sat: run

room: run

clean:
	rm -rf $(BINDIR)