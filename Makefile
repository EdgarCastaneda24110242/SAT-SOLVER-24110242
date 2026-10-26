CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
TARGET = sat_solver
SOURCE = main.cpp

.PHONY: all build run clean

all: build

build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)

run: build
ifeq ($(OS),Windows_NT)
	cmd /c start "SAT Solver" $(TARGET).exe
else
	./$(TARGET)
endif

clean:
	-del /Q $(TARGET).exe 2>NUL || rm -f $(TARGET)