# Makefile for Hexagonal Terraria Game

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -pthread

# Source files
SOURCES = src/main.cpp src/Game.cpp src/Player.cpp src/World.cpp src/Chunk.cpp \
          src/Camera.cpp src/Menu.cpp src/Utils.cpp src/BlockInteraction.cpp \
          src/MultiplayerClient.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = hexaworld

# Build directory
BUILD_DIR = build

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Link the executable
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)
	@echo "Build complete: $(BUILD_DIR)/$(TARGET)"

# Compile source files
%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJECTS) $(BUILD_DIR)
	@echo "Clean complete"

# Run the game
run: $(TARGET)
	./$(BUILD_DIR)/$(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing SFML dependencies..."
	sudo apt-get update
	sudo apt-get install -y libsfml-dev python3-pip
	@echo "Installing Python dependencies..."
	pip3 install -r src/requirements.txt

# Run multiplayer server
server:
	@echo "Starting multiplayer server..."
	python3 multiplayer_server.py

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build the game (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Build and run the game"
	@echo "  server       - Start the multiplayer server"
	@echo "  install-deps - Install required dependencies"
	@echo "  help         - Show this help message"

.PHONY: all clean run server install-deps help