#!/usr/bin/env bash
set -euo pipefail

# Hexagon Sandbox Game Build Script
# - Builds native Linux binary
# - Packages an AppImage
# - Cross-compiles a Windows .exe (requires MinGW + SFML for MinGW)

PROJECT_NAME="hexagon_sandbox"
APP_NAME="Hexagon Sandbox Game"
BUILD_DIR="build"
DIST_DIR="dist"
SRC_DIR="src"
TOOLS_DIR="tools"

# Clean previous build artifacts: binaries, AppImages, .exe and object files
rm -rf "$BUILD_DIR" "$DIST_DIR"
rm -f "$SRC_DIR"/*.o

# Detect version (fallback to 0.1.0)
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  VERSION=$(git describe --tags --always --dirty 2>/dev/null || echo "0.1.0")
else
  VERSION="0.1.0"
fi

log() { echo -e "[+] $*"; }
err() { echo -e "[!] $*" >&2; }
die() { err "$*"; exit 1; }

mkdir -p "$BUILD_DIR" "$DIST_DIR" "$TOOLS_DIR"

# ------------------------------------------------------------
# Build Linux binary
# ------------------------------------------------------------
log "Checking for SFML (native) via pkg-config..."
if ! pkg-config --exists sfml-all; then
  cat >&2 <<EOF
ERROR: SFML for Linux not found by pkg-config.
Install SFML dev packages first, e.g.:
  Ubuntu/Debian: sudo apt-get install libsfml-dev
  Arch:          sudo pacman -S sfml
  Fedora:        sudo dnf install SFML-devel
EOF
  exit 1
fi

CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -O2 -pipe -fno-plt -DNDEBUG $(pkg-config --cflags sfml-all)"
LDFLAGS_NATIVE="$(pkg-config --libs sfml-all)"

log "Compiling (Linux) sources in $SRC_DIR -> $BUILD_DIR/*.o"
rm -f "$BUILD_DIR"/*.o
OBJ_FILES=()
shopt -s nullglob
for src_file in "$SRC_DIR"/*.cpp; do
  obj_file="$BUILD_DIR/$(basename "${src_file%.cpp}").o"
  "$CXX" $CXXFLAGS -c "$src_file" -o "$obj_file"
  OBJ_FILES+=("$obj_file")
  log "Compiled $(basename "$src_file")"
done
shopt -u nullglob

if [ ${#OBJ_FILES[@]} -eq 0 ]; then
  die "No source files found in $SRC_DIR"
fi

log "Linking Linux binary -> $BUILD_DIR/$PROJECT_NAME"
"$CXX" -std=c++17 "${OBJ_FILES[@]}" -o "$BUILD_DIR/$PROJECT_NAME" $LDFLAGS_NATIVE
chmod +x "$BUILD_DIR/$PROJECT_NAME"

# ------------------------------------------------------------
# Package AppImage
# ------------------------------------------------------------
package_appimage() {
  local appdir="$DIST_DIR/AppDir"
  log "Preparing AppDir at $appdir"
  rm -rf "$appdir"

  mkdir -p \
    "$appdir/usr/bin" \
    "$appdir/usr/lib" \
    "$appdir/usr/share/applications" \
    "$appdir/usr/share/icons/hicolor/256x256/apps"

  # Copy binary
  install -Dm755 "$BUILD_DIR/$PROJECT_NAME" "$appdir/usr/bin/$PROJECT_NAME"

  # Desktop file
  cat > "$appdir/$PROJECT_NAME.desktop" <<EOF
[Desktop Entry]
Name=$APP_NAME
Exec=$PROJECT_NAME
Icon=$PROJECT_NAME
Type=Application
Categories=Game;
Terminal=false
EOF

  # AppRun launcher (simple)
  cat > "$appdir/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
exec "$HERE/usr/bin/hexagon_sandbox" "$@"
EOF
  chmod +x "$appdir/AppRun"

  # Try to copy SFML runtime libs into AppDir/usr/lib to improve portability
  # Only copy a curated set to avoid bundling system libs
  log "Collecting SFML libraries into AppDir/usr/lib"
  while IFS= read -r line; do
    set +e
    so_path=$(echo "$line" | awk '{print $(NF-1)}')
    set -e
    [ -f "$so_path" ] || continue
    base=$(basename "$so_path")
    case "$base" in
      libsfml*|libopenal*.so*|libFLAC*.so*|libvorbis*.so*|libogg*.so*|libfreetype*.so*)
        cp -n "$so_path" "$appdir/usr/lib/" || true
        ;;
    esac
  done < <(ldd "$BUILD_DIR/$PROJECT_NAME" || true)

  # Icon: use provided icon if exists
  if [ -f assets/icon.png ]; then
    install -Dm644 assets/icon.png "$appdir/usr/share/icons/hicolor/256x256/apps/$PROJECT_NAME.png"
  else
    log "No assets/icon.png found; AppImage will use a generic icon"
    # Create a minimal 1x1 PNG placeholder if 'convert' exists
    if command -v convert >/dev/null 2>&1; then
      convert -size 256x256 xc:#3a86ff "$appdir/usr/share/icons/hicolor/256x256/apps/$PROJECT_NAME.png" || true
    fi
  fi

  # Also place icon at AppDir root so appimagetool finds hexagon_sandbox.png
  if [ -f "$appdir/usr/share/icons/hicolor/256x256/apps/$PROJECT_NAME.png" ]; then
    cp "$appdir/usr/share/icons/hicolor/256x256/apps/$PROJECT_NAME.png" "$appdir/$PROJECT_NAME.png" || true
  fi

  # Ensure desktop file references the icon name without extension
  mv "$appdir/$PROJECT_NAME.desktop" "$appdir/$PROJECT_NAME.desktop.tmp"
  sed 's|Icon=.*|Icon='$PROJECT_NAME'|g' "$appdir/$PROJECT_NAME.desktop.tmp" > "$appdir/$PROJECT_NAME.desktop"
  rm -f "$appdir/$PROJECT_NAME.desktop.tmp"

  # Acquire appimagetool
  local appimagetool_bin="appimagetool"
  if ! command -v appimagetool >/dev/null 2>&1; then
    appimagetool_bin="$TOOLS_DIR/appimagetool-x86_64.AppImage"
    if [ ! -x "$appimagetool_bin" ]; then
      log "Downloading appimagetool..."
      curl -L -o "$appimagetool_bin" https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
      chmod +x "$appimagetool_bin"
    fi
  fi

  local out="$DIST_DIR/${PROJECT_NAME}-${VERSION}-x86_64.AppImage"
  log "Building AppImage -> $out"
  "$appimagetool_bin" "$appdir" "$out"
  chmod +x "$out" || true
  log "AppImage created: $out"
}

# ------------------------------------------------------------
# Cross-compile Windows .exe using MinGW
# ------------------------------------------------------------
build_windows_exe() {
  local triples=("x86_64-w64-mingw32" "i686-w64-mingw32")
  local TRIPLE=""
  for t in "${triples[@]}"; do
    if command -v "${t}-g++" >/dev/null 2>&1; then
      TRIPLE="$t"; break
    fi
  done
  if [ -z "$TRIPLE" ]; then
    cat >&2 <<EOF
WARNING: MinGW cross-compiler not found.
Install it to enable .exe build, e.g.:
  Ubuntu/Debian: sudo apt-get install mingw-w64
  Arch:          sudo pacman -S mingw-w64-gcc
  Fedora:        sudo dnf install mingw64-gcc mingw32-gcc
Skipping Windows .exe build.
EOF
    return 0
  fi

  log "Using MinGW toolchain: $TRIPLE"
  local CXXW="$TRIPLE-g++"

  # Determine SFML flags for MinGW
  local HAVE_PKGCFG=0
  if command -v "$TRIPLE-pkg-config" >/dev/null 2>&1 && "$TRIPLE-pkg-config" --exists sfml-all 2>/dev/null; then
    HAVE_PKGCFG=1
  fi

  local CXXFLAGS_W="-std=c++17 -O2 -pipe -DNDEBUG"
  local LDFLAGS_W="-static-libstdc++ -static-libgcc"
  if [ $HAVE_PKGCFG -eq 1 ]; then
    CXXFLAGS_W+=" $($TRIPLE-pkg-config --cflags sfml-all)"
    LDFLAGS_W+=" $($TRIPLE-pkg-config --libs sfml-all)"
  else
    # Fallback: use SFML_MINGW_DIR env var pointing to the MinGW-built SFML root
    # Expected layout: $SFML_MINGW_DIR/include and $SFML_MINGW_DIR/lib
    local SFML_MINGW_DIR=${SFML_MINGW_DIR:-}
    if [ -z "$SFML_MINGW_DIR" ]; then
      cat >&2 <<EOF
ERROR: Unable to locate SFML for MinGW.
Either:
  - Install MinGW SFML and its pkg-config files so that '$TRIPLE-pkg-config --exists sfml-all' works, or
  - Set SFML_MINGW_DIR to the root of your MinGW SFML installation (with include/ and lib/ inside).
Skipping Windows .exe build.
EOF
      return 0
    fi
    CXXFLAGS_W+=" -I\"$SFML_MINGW_DIR/include\""
    LDFLAGS_W+=" -L\"$SFML_MINGW_DIR/lib\" -lsfml-graphics -lsfml-window -lsfml-network -lsfml-audio -lsfml-system -lws2_32 -lopengl32 -lwinmm -lgdi32 -luser32 -lkernel32"
  fi

  log "Compiling (Windows) sources"
  rm -f "$BUILD_DIR"/*.owin
  local OBJW=()
  shopt -s nullglob
  for src_file in "$SRC_DIR"/*.cpp; do
    obj_file="$BUILD_DIR/$(basename "${src_file%.cpp}").owin"
    "$CXXW" $CXXFLAGS_W -c "$src_file" -o "$obj_file"
    OBJW+=("$obj_file")
    log "Compiled (Win) $(basename "$src_file")"
  done
  shopt -u nullglob

  if [ ${#OBJW[@]} -eq 0 ]; then
    err "No source files for Windows build"
    return 0
  fi

  local OUT_EXE="$DIST_DIR/${PROJECT_NAME}.exe"
  log "Linking Windows .exe -> $OUT_EXE"
  "$CXXW" "${OBJW[@]}" -o "$OUT_EXE" $LDFLAGS_W || {
    cat >&2 <<EOF
ERROR: Linking Windows .exe failed. Ensure MinGW SFML libraries are installed and linkable.
EOF
    return 0
  }
  chmod +x "$OUT_EXE" || true
  log "Windows .exe created: $OUT_EXE"
}

# ------------------------------------------------------------
# Run steps
# ------------------------------------------------------------
log "Linux build completed: $BUILD_DIR/$PROJECT_NAME"
package_appimage
build_windows_exe

# Final cleanup: remove all object files (both native and Windows)
log "Removing intermediate object files"
rm -f "$SRC_DIR"/*.o "$BUILD_DIR"/*.o "$BUILD_DIR"/*.owin || true

log "All done. Artifacts:"
log "- Linux binary:        $BUILD_DIR/$PROJECT_NAME"
log "- AppImage:            $DIST_DIR/${PROJECT_NAME}-${VERSION}-x86_64.AppImage (if appimagetool succeeded)"
log "- Windows executable:  $DIST_DIR/${PROJECT_NAME}.exe (if MinGW + SFML were available)"
