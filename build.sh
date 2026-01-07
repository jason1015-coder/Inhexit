#!/bin/bash

# Hexagon Sandbox Game Build Script
# This script compiles the 2.5D hexagon-based sandbox game

echo "==================================="
echo "Hexagon Sandbox Game - Build Script"
echo "==================================="
echo ""

# Check if SFML is installed
echo "Checking for SFML installation..."
if ! pkg-config --exists sfml-all; then
    echo "ERROR: SFML is not installed!"
    echo "Please install SFML first:"
    echo "  Ubuntu/Debian: sudo apt-get install libsfml-dev"
    echo "  Arch: sudo pacman -S sfml"
    echo "  Fedora: sudo dnf install SFML-devel"
    exit 1
fi

echo "SFML found! Proceeding with compilation..."
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# Clean old object files
echo "Cleaning old object files..."
rm -f build/*.o

# Compile the game
echo "Compiling the game..."
echo ""

# Compile source files into individual object files
OBJ_FILES=""
for src_file in src/*.cpp; do
    if [ ! -f "$src_file" ]; then
        continue
    fi
    obj_file="build/$(basename "${src_file%.cpp}").o"
    
    # Added -Wall for better debugging and fixed the g++ command
    g++ -std=c++17 -Wall -Wextra -c "$src_file" -o "$obj_file"
    
    if [ $? -ne 0 ]; then
        echo ""
        echo "ERROR: Compilation failed for $src_file!"
        exit 1
    fi
    echo "Compiled $src_file to $obj_file"
    OBJ_FILES="$OBJ_FILES $obj_file"
done

if [ -z "$OBJ_FILES" ]; then
    echo ""
    echo "ERROR: No source files found in src/ folder!"
    exit 1
fi

# Link the executable
echo ""
echo "Linking executable..."

# Fixed the multi-line command using backslashes
g++ -std=c++17 $OBJ_FILES -o build/hexagon_sandbox \
    -lsfml-graphics -lsfml-window -lsfml-network -lsfml-audio -lsfml-system

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Linking failed!"
    exit 1
fi

# Clean up object files after successful build
echo "Cleaning up object files..."
rm -f build/*.o

echo ""
echo "==================================="
echo "Build completed successfully!"
echo "==================================="
echo ""
echo "Executable location: build/hexagon_sandbox"
echo ""
echo "To run the game:"
echo "  ./build/hexagon_sandbox"
echo ""
