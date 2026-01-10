# Makefile - Modernized & cleaned up (2026 style)

.DEFAULT_GOAL := both

.PHONY: both

both: win linux
	@echo ""
	@echo "Build finished:"
	@ls -lh $(WIN_BIN) $(LINUX_BIN)

TARGET_NAME = game

SRC_DIR = src
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# Separate object files for each platform
LINUX_OBJ = $(notdir $(SOURCES:.cpp=.linux.o))
WIN_OBJ   = $(notdir $(SOURCES:.cpp=.win.o))

# ──────────────── Windows cross-compile settings ────────────────
WIN_CXX       = i686-w64-mingw32-g++           # or x86_64-w64-mingw32-g++ for 64-bit
WIN_CXXFLAGS  = -O2 -Wall -std=c++17 -DSFML_STATIC \
                -I$(SRC_DIR) -I$(SFML_WIN_INC)
WIN_LDFLAGS   = -L$(SFML_WIN_LIB) -static \
                -lsfml-graphics-s -lsfml-window-s -lsfml-network-s -lsfml-system-s \
                -lopengl32 -lwinmm -lgdi32 -lws2_32 -mwindows
WIN_BIN       = $(TARGET_NAME)-win32.exe       # or game.exe

# Adjust these paths!
SFML_WIN_PATH = /home/$(USER)/SFML-3.0.2-windows-mingw-i686
SFML_WIN_INC  = $(SFML_WIN_PATH)/include
SFML_WIN_LIB  = $(SFML_WIN_PATH)/lib

# ──────────────── Linux native settings ────────────────
LINUX_CXXFLAGS = -O2 -Wall -std=c++17 -I$(SRC_DIR)
LINUX_LDFLAGS  = -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system
LINUX_BIN      = $(TARGET_NAME).linux

# AppImage related (very basic — see notes below)
APPIMAGE_NAME  = Game-x86_64.AppImage
APPIMAGETOOL   ?= appimagetool
LINUXDEPLOY    ?= linuxdeploy

.PHONY: all win linux appimage clean

all: linux win

win: $(WIN_BIN)

linux: $(LINUX_BIN)

# Very basic appimage target (needs a lot more work!)
appimage: $(LINUX_BIN) prepare-appdir
	$(APPIMAGETOOL) ./AppDir $(APPIMAGE_NAME)

# You almost always want linuxdeploy instead of raw appimagetool
appimage-modern: $(LINUX_BIN)
	$(LINUXDEPLOY) --executable $(LINUX_BIN) \
	               --appdir AppDir \
	               --desktop-file game.desktop \
	               --icon-file game.png \
	               --output appimage

# Windows build with Windows object files
$(WIN_BIN): $(WIN_OBJ)
	$(WIN_CXX) $^ -o $@ $(WIN_LDFLAGS)

# Linux build with Linux object files
$(LINUX_BIN): $(LINUX_OBJ)
	$(CXX) $^ -o $@ $(LINUX_LDFLAGS)

# Linux object files
%.linux.o: $(SRC_DIR)/%.cpp
	$(CXX) $(LINUX_CXXFLAGS) -c $< -o $@

# Windows object files
%.win.o: $(SRC_DIR)/%.cpp
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.linux.o *.win.o $(LINUX_BIN) $(WIN_BIN) *.AppImage
	rm -rf AppDir

prepare-appdir: $(LINUX_BIN)
	rm -rf AppDir
	mkdir -p AppDir/usr/bin AppDir/usr/lib
	cp $(LINUX_BIN)                AppDir/usr/bin/$(TARGET_NAME)
	# Copy shared libs you actually need (very important!)
	# ldd $(LINUX_BIN) | grep sfml | awk '{print $3}' | xargs -I {} cp {} AppDir/usr/lib/
	# You should also copy plugins, assets, etc...
