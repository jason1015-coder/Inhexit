# items.py
"""
Central place for all holdable/placeable items in the game.
Mostly mirrors blocks right now, but allows future separation 
(tools, consumables, placeable decorations, etc.).
"""

from typing import Dict, Tuple, List, Optional

# ───────────────────────────────────────────────
#  Item definitions — what the player can hold/select/place
# ───────────────────────────────────────────────

ITEM_DEFINITIONS = {
    # Placeable blocks
    "dirt": {
        "name":        "Dirt",
        "color":       (139, 90, 43),
        "icon_color":  (139, 90, 43),  # usually same as block, but can differ
        "hotbar_order": 1,
        "type":        "block",
    },
    "grass": {
        "name":        "Grass",
        "color":       (100, 200, 100),
        "icon_color":  (100, 200, 100),
        "hotbar_order": 2,
        "type":        "block",
    },
    "stone": {
        "name":        "Stone",
        "color":       (169, 169, 169),
        "icon_color":  (169, 169, 169),
        "hotbar_order": 3,
        "type":        "block",
    },
    "sand": {
        "name":        "Sand",
        "color":       (238, 214, 175),
        "icon_color":  (238, 214, 175),
        "hotbar_order": 4,
        "type":        "block",
    },
    "coal": {
        "name":        "Coal",
        "color":       (40, 40, 40),
        "icon_color":  (60, 60, 60),
        "hotbar_order": 5,
        "type":        "block",
    },
    "iron": {
        "name":        "Iron Ore",          # or "Iron Ore" — your choice
        "color":       (229, 194, 159),
        "icon_color":  (200, 160, 120),
        "hotbar_order": 6,
        "type":        "block",
    },
    "gold": {
        "name":        "Gold Ore",
        "color":       (255, 215, 0),
        "icon_color":  (240, 190, 50),
        "hotbar_order": 7,
        "type":        "block",
    },
    "diamond": {
        "name":        "Diamond Ore",
        "color":       (0, 220, 255),
        "icon_color":  (0, 200, 240),
        "hotbar_order": 8,
        "type":        "block",
    },
    "wood": {
        "name":        "Wood Planks",
        "color":       (101, 67, 33),
        "icon_color":  (140, 90, 50),
        "hotbar_order": 9,
        "type":        "block",
    },
    "sapling": {
        "name":        "Sapling",
        "color":       (34, 139, 34),
        "icon_color":  (50, 180, 50),
        "hotbar_order": 10,
        "type":        "plantable",
    },
    # Tools
    "axe": {
        "name":        "Stone Axe",
        "color":       (100, 100, 100),
        "icon_color":  (120, 120, 120),
        "hotbar_order": 11,
        "type":        "tool",
        "damage":      5,
        "tool_type":   "axe",
    },
    # You can add more later: torch, ladder, workbench, etc.
}

# ───────────────────────────────────────────────
#  Quick lookup tables (generated at import time)
# ───────────────────────────────────────────────

ITEM_COLOR_BY_ID: Dict[str, Tuple[int, ...]] = {
    k: v["color"] for k, v in ITEM_DEFINITIONS.items()
}

ITEM_ICON_COLOR_BY_ID: Dict[str, Tuple[int, ...]] = {
    k: v.get("icon_color", v["color"]) for k, v in ITEM_DEFINITIONS.items()
}

ITEM_NAME_BY_ID: Dict[str, str] = {
    k: v["name"] for k, v in ITEM_DEFINITIONS.items()
}

# Sorted list for hotbar / selection (lowest hotbar_order first)
DEFAULT_HOTBAR_ITEMS: List[str] = sorted(
    ITEM_DEFINITIONS.keys(),
    key=lambda k: ITEM_DEFINITIONS[k].get("hotbar_order", 999)
)

# Optional: fallback item when something goes wrong
DEFAULT_ITEM_ID = "dirt"
