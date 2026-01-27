# Tesselbox

**Tesselbox** is an enhanced, hexagon-based sandbox exploration game built with Python and Pygame. It features a procedurally generated world with multiple biomes, mining mechanics, and a chunk-based rendering system for optimized performance.

## why in hexagons?

everyone uses squares, accross the market of sandbox game, being hexagons is more fun, and less boring

## ❇️recent update 

fixed the map showing error

## 🛠️ Installation

🛠️ Build Requirements

You must run the build command on the operating system you are targeting (e.g., use Windows to make the .exe).

    #Install Dependencies: Open your terminal or command prompt and run:
    Bash

    pip install pygame numpy
    pip install pyinstaller

    #Verify File Structure: Ensure all three files are in the same directory:

        #main.py (The entry point)

        #blocks.py

        #item.py

📦 Create the Executable

Run the following command in your terminal. This command works for both Windows PowerShell/CMD and Linux Terminal

    pyinstaller --onefile --windowed --name Tesselbox main.py

---

## 🎮 Controls

| Action | Control |
| :--- | :--- |
| **Move** | `W`, `A`, `S`, `D` |
| **Jump** | `Space` |
| **Mine Block** | `Left Click` |
| **Place Block** | `Right Click` |
| **Select Block** | `1-9` or `Mouse Wheel` |
| **Toggle Flight** | `F` (Creative Mode only) |
| **Render Distance** | `+` / `-` keys |
| **Pause/Menu** | `Esc` |

---

## 📄 License

This work is licensed under a **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** License.

* **Attribution**: You must give appropriate credit to the original creator.
* **Non-Commercial**: You may not use this material for commercial purposes.
* **Share-Alike**: If you remix or build upon this work, you must distribute your contributions under the same license.

To view a copy of this license, visit [http://creativecommons.org/licenses/by-nc-sa/4.0/](http://creativecommons.org/licenses/by-nc-sa/4.0/).
