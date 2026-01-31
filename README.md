# TesselBox

A 2D sandbox adventure game inspired by *Terraria*, but built on a **hexagonal grid**.

Explore worlds, mine resources, build structures, craft items, fight enemies, and survive — all in beautiful hex tiles.

![Game screenshot placeholder](https://via.placeholder.com/800x450.png?text=TesselBox+Gameplay+Screenshot+Coming+Soon)  
*(Screenshots will be added once gameplay is more complete)*

## Migration Notice

**Sorry!**  
The game is currently migrating from a pure-Python prototype to a **Golang + Python hybrid** structure.  
This refactor improves performance (Go for core engine/world/physics) while keeping Python for game logic, scripting, and easier modding.  

Expect some features to be temporarily broken or incomplete during this phase.  
Thanks for your patience — progress is ongoing!

## Current Status

- Early development / heavy refactoring
- ~87% Python, ~13% Rust (as of Jan 2026)
- Basic hex world gen and rendering exist in parts
- No full gameplay loop yet — focus is on engine foundation

## Installation

### Via Releases (Recommended once binaries are ready)

1. Go to the **[Releases page](https://github.com/tesselstudio/TesselBox-game/releases)**.
2. Select the latest release (e.g. "Testing_stage1" or higher).
3. Under **Assets**, download the file for your platform:
   - **Windows**: `TesselBox.exe` (or similar)
   - **Linux**: `TesselBox.AppImage` → run with `./TesselBox.AppImage` (make executable first: `chmod +x TesselBox.AppImage`)
   - **macOS**: `TesselBox.app.zip` → unzip and open the .app
4. Launch the file — no installation needed!

**Current status (January 30, 2026):**  
Releases exist (2 tags/pre-releases), but **no binary assets are attached yet**.  
We're setting up automated builds to attach .exe / AppImage / .app files soon.

### Temporary: Download Latest Build from GitHub Actions (Nightly-ish Builds)

While official release binaries are pending:

1. Visit the **[Actions tab](https://github.com/tesselstudio/TesselBox-game/actions)**.
2. Find the most recent **successful** workflow run (green checkmark, e.g. "Build Executables" or similar).
3. Scroll to the bottom → **Artifacts** section.
4. Download the zip for your OS (e.g. `tesselbox-windows-exe`, `tesselbox-linux-appimage`, `tesselbox-macos-app`).
5. Unzip and run the executable.

This gives you the freshest compiled version directly from recent commits.

### Development / Run from Source

If you want to contribute, test bleeding-edge code, or modify the game:

**Prerequisites**
- Git
- Python 3.11+
- Rust stable toolchain [](https://rustup.rs)

**Steps**
```bash
git clone https://github.com/tesselstudio/TesselBox-game.git
cd TesselBox-game

# Python side (legacy / high-level logic)
cd "python files"                  # adjust folder name if different
python -m venv venv
source venv/bin/activate           # Windows: venv\Scripts\activate
pip install pygame                 # add other deps from requirements.txt when created
python main.py                     # replace with actual entry point file

# Rust side (core engine – still WIP integration)
cd ../rust_engine                  # adjust if folder name differs
cargo build --release
# Run Rust binary (once bridged): cargo run --release
