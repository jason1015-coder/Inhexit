# Hexagon World - Compilation Instructions

## Prerequisites

### Required Libraries
- **SFML 2.5+** (Simple and Fast Multimedia Library)
- **C++17 compatible compiler** (g++ 7+, clang++ 5+, or MSVC 2017+)

### Installing SFML

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libsfml-dev
```

#### Fedora
```bash
sudo dnf install SFML-devel
```

#### Arch Linux
```bash
sudo pacman -S sfml
```

#### macOS
```bash
brew install sfml
```

#### Windows
1. Download SFML from https://www.sfml-dev.org/download.php
2. Extract to a location of your choice
3. Configure your IDE to link against SFML libraries

## Compilation Methods

### Method 1: Using the Build Script (Recommended)

1. Navigate to the src directory:
```bash
cd src
```

2. Make the build script executable (Linux/macOS only):
```bash
chmod +x build.sh
```

3. Run the build script:
```bash
./build.sh
```

4. Run the game:
```bash
cd build
./hexagon_world
```

### Method 2: Manual Compilation with g++

#### Linux/macOS
```bash
cd src
g++ -std=c++17 -o hexagon_world \
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

./hexagon_world
```

#### Windows (MinGW)
```bash
cd src
g++ -std=c++17 -o hexagon_world.exe ^
    main.cpp ^
    Game.cpp ^
    World.cpp ^
    Chunk.cpp ^
    Player.cpp ^
    Camera.cpp ^
    Menu.cpp ^
    BlockInteraction.cpp ^
    MultiplayerClient.cpp ^
    Utils.cpp ^
    -IC:\SFML\include ^
    -LC:\SFML\lib ^
    -lsfml-graphics-s -lsfml-window-s -lsfml-system-s ^
    -lopengl32 -lwinmm -lgdi32

hexagon_world.exe
```

### Method 3: Using CMake

Create a `CMakeLists.txt` file in the src directory:

```cmake
cmake_minimum_required(VERSION 3.10)
project(HexagonWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add executable
add_executable(hexagon_world
    main.cpp
    Game.cpp
    World.cpp
    Chunk.cpp
    Player.cpp
    Camera.cpp
    Menu.cpp
    BlockInteraction.cpp
    MultiplayerClient.cpp
    Utils.cpp
)

# Link SFML libraries
target_link_libraries(hexagon_world
    sfml-graphics
    sfml-window
    sfml-system
)
```

Then build:
```bash
mkdir build
cd build
cmake ..
make
./hexagon_world
```

## Troubleshooting

### SFML Not Found
**Error:** `SFML not found` or `sfml/Graphics.hpp: No such file`

**Solution:** Install SFML using the appropriate package manager for your system (see Prerequisites section).

### Font Loading Warnings
**Warning:** `WARNING: Failed to load any font, using default system font`

**Solution:** This is not critical - the game will use SFML's default font. To fix:
```bash
# Install system fonts
sudo apt-get install fonts-dejavu-core fonts-liberation fonts-freefont-ttf
```

### Compilation Errors
**Error:** Various C++ compilation errors

**Solution:** Ensure you have a C++17 compatible compiler:
```bash
# Check g++ version
g++ --version

# Install/update g++ (Ubuntu/Debian)
sudo apt-get install g++
```

### Linking Errors
**Error:** `undefined reference to sf::...`

**Solution:** Ensure SFML libraries are properly linked. Check that you're including all required components:
```bash
$(pkg-config --cflags --libs sfml-graphics sfml-window sfml-system)
```

## Running the Game

### Basic Execution
```bash
./hexagon_world
```

### Command Line Options
Currently, the game doesn't support command line options, but you can modify `main.cpp` to add window size customization.

### Configuration
Game settings can be modified in the following files:
- `Utils.h` - Game constants (gravity, movement speed, world size)
- `Game.h` - Window dimensions and world size
- `Menu.cpp` - Menu colors and settings

## Multiplayer Server

To run the multiplayer server (requires Python 3.6+):

```bash
# Install dependencies
pip install -r requirements.txt

# Run the server
python multiplayer_server.py
```

The server runs on `127.0.0.1:8765` by default.

## Performance Tips

1. **Reduce Chunk Size**: Modify `CHUNK_SIZE` and `CHUNK_HEIGHT` in `Chunk.h`
2. **Lower Render Distance**: Adjust LOD thresholds in `Chunk.cpp`
3. **Disable Debug Output**: Comment out debug `std::cout` statements in production
4. **Optimize Compilation**: Use `-O2` or `-O3` optimization flags:
   ```bash
   g++ -std=c++17 -O2 -o hexagon_world ...
   ```

## IDE Setup

### Visual Studio Code
1. Install C/C++ extension
2. Install CMake Tools extension (optional)
3. Configure `.vscode/c_cpp_properties.json` to include SFML headers

### CLion
1. Open the project folder
2. Use CMakeLists.txt (provided above)
3. Build and run using CLion's interface

### Visual Studio
1. Create a new Empty C++ project
2. Add all .cpp and .h files
3. Configure project properties to include SFML directories
4. Link SFML libraries

## Additional Resources

- [SFML Official Documentation](https://www.sfml-dev.org/learn.php)
- [SFML Tutorials](https://www.sfml-dev.org/tutorials/)
- [C++ Reference](https://en.cppreference.com/)

## Support

If you encounter issues not covered here:
1. Check the `FIXES_AND_ENHANCEMENTS.md` for known fixes
2. Review the debug output in the console
3. Ensure all prerequisites are properly installed
4. Verify file permissions on Linux/macOS