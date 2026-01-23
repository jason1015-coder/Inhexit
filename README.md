# Tesselbox

**Tesselbox** is an enhanced, hexagon-based sandbox exploration game built with Python and Pygame. It features a procedurally generated world with multiple biomes, mining mechanics, and a chunk-based rendering system for optimized performance.

---

## 🚀 Features

* **Hexagonal World**: A unique grid system using $30$-pixel hexagons for a distinct visual style.
* **Procedural Generation**: Uses noise-based generation to create plains, deserts, mountains, and forests.
* **Game Modes**: 
    * **Survival**: Mine resources like coal, iron, gold, and diamonds to build your inventory.
    * **Creative**: Unlimited resources and a flight mode for free building.
* **Resource System**: Includes various block types such as Grass, Dirt, Stone, Sand, Glass, and various Ores.
* **Physics & Raycasting**: Features gravity-based player movement and accurate raycasting for block mining and placement.
* **Save/Load**: Automatically saves world and player data using Python's `pickle` module.

---

## 🛠️ Installation

1.  **Clone the repository** (or copy the files into a folder).
2.  **Install Dependencies**:
    ```bash
    pip install pygame numpy
    ```
3.  **Run the Game**:
    ```bash
    python main.py
    ```

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
