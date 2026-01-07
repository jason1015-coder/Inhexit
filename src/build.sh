#!/bin/bash

# Hexagon World Build Script
# This script compiles the game with SFML

echo "=== Hexagon World Build Script ==="
echo ""

# Check if SFML is installed
echo "Checking for SFML installation..."
if ! pkg-config --exists sfml-all; then
    echo "ERROR: SFML not found!"
    echo "Please install SFML first:"
    echo "  Ubuntu/Debian: sudo apt-get install libsfml-dev"
    echo "  Fedora: sudo dnf install SFML-devel"
    echo "  Arch: sudo pacman -S sfml"
    exit 1
fi

echo "SFML found! Proceeding with compilation..."
echo ""

# Create build directory if it doesn't exist
mkdir -p build

# Compile the game
echo "Compiling Hexagon World..."
g++ -std=c++17 -o build/hexagon_world \
    main.cpp \
    Game.cpp \
    World.cpp \
    Chunk.cpp \
    Player.cpp \
    Camera.cpp \
    Menu.cpp \
    BlockInteraction.cpp \
    MultiplayerClient.cpp \
    Utils.cpp \
    $(pkg-config --cflags --libs sfml-graphics sfml-window sfml-system) \
    -Wall -Wextra

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Executable created: build/hexagon_world"
    echo ""
    echo "To run the game:"
    echo "  cd build"
    echo "  ./hexagon_world"
    echo ""
    echo "Or from src directory:"
    echo "  ./build/hexagon_world"
else
    echo ""
    echo "❌ Build failed!"
    echo "Please check the error messages above."
    exit 1
fi