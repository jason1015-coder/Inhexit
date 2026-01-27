# Contributing to Tesselbox

First off, thank you for considering contributing to Tesselbox! It’s people like you who make open-source projects such as this a great tool for everyone.

## 🛠️ Getting Started

1.  **Fork the repository** and clone it locally.
2.  **Set up your environment**:
    ```bash
    python -m venv venv
    source venv/bin/activate  # On Windows: venv\Scripts\activate
    pip install -r requirements.txt
    ```
3.  **Run the game** to ensure everything is working:
    ```bash
    python main.py
    ```

## 📐 The Hexagonal Grid System

Tesselbox uses a flat-top hexagonal coordinate system. If you are adding new world generation features or physics, keep these constants in mind:

* **Hex Size ($30$px)**: The distance from the center to any corner.
* **Vertical Spacing**: Set to `HEX_HEIGHT * 0.75` to allow hexagons to "nest" into each other.
* **Raycasting**: We use a custom step-based raycast in `main.py` to detect block collisions for mining and placement.

## 📂 Project Structure

* `main.py`: Contains the core `Game` loop, `Player` physics, and `World` chunking logic.
* `blocks.py`: The "Data Source." Add new block types here by updating the `BLOCK_DEFINITIONS` dictionary.

## 📝 How to Contribute

### Adding New Blocks
You can contribute easily by adding new block types to `blocks.py`. Ensure you define:
1.  **Hardness**: Affects how long it takes to mine.
2.  **Transparency**: Determines if the block blocks light or allows the player to see through it.
3.  **Solid**: Determines if the player can walk through it.

### Reporting Bugs/ New Feature Request
* Use the GitHub Issues tab.
* Describe the bug/feature and provide steps to reproduce it.
* Mention your Operating System and Python version.(for a bug)

### Pull Requests
1.  Create a new branch for your feature (`git checkout -b feature/AmazingFeature`).
2.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
3.  Push to the branch (`git push origin feature/AmazingFeature`).
4.  Open a Pull Request.

## ⚖️ License
By contributing, you agree that your contributions will be licensed under the project's **CC BY-NC-SA 4.0** License.
