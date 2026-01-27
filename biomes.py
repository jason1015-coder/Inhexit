# biomes.py
"""
Biome definitions and generation logic for terrain generation.
Provides biome-specific block distributions and characteristics.
"""

from typing import Dict, Tuple, List
import random
import math

# Biome type definitions
BIOME_TYPES = ["plains", "desert", "forest", "mountain"]

# Biome characteristics
BIOME_CHARACTERISTICS = {
    "plains": {
        "name": "Plains",
        "surface_block": "grass",
        "subsurface_block": "dirt",
        "surface_variation": 0.3,
        "tree_density": 0.02,
        "color": (100, 200, 100),
    },
    "desert": {
        "name": "Desert",
        "surface_block": "sand",
        "subsurface_block": "sand",
        "surface_variation": 0.1,
        "tree_density": 0.0,
        "color": (238, 214, 175),
    },
    "forest": {
        "name": "Forest",
        "surface_block": "grass",
        "subsurface_block": "dirt",
        "surface_variation": 0.5,
        "tree_density": 0.08,
        "color": (80, 180, 80),
    },
    "mountain": {
        "name": "Mountain",
        "surface_block": "stone",
        "subsurface_block": "stone",
        "surface_variation": 1.2,
        "tree_density": 0.01,
        "color": (150, 150, 150),
    },
}

def get_biome_at_position(x, seed):
    """
    Determine biome at a given world x position using noise.
    
    Args:
        x: World x coordinate
        seed: Random seed for generation
    
    Returns:
        Biome name string
    """
    biome_noise = math.sin(x * 0.002 + seed)
    
    if biome_noise < -0.5:
        return "desert"
    elif biome_noise > 0.5:
        return "mountain"
    elif abs(biome_noise) < 0.2:
        return "forest"
    else:
        return "plains"

def get_biome_block(biome, depth, world_x, world_y, seed):
    """
    Get the appropriate block type for a given biome and depth.
    
    Args:
        biome: Biome name string
        depth: Depth below surface (in hexagons)
        world_x, world_y: World coordinates
        seed: Random seed
    
    Returns:
        Block type string
    """
    characteristics = BIOME_CHARACTERISTICS[biome]
    
    # Surface layer
    if depth == 0:
        return characteristics["surface_block"]
    
    # Subsurface layers
    if depth <= 3:
        return characteristics["subsurface_block"]
    
    # Deep underground - ore generation
    if depth <= 7:
        r = random.random()
        if r < 0.09 + depth * 0.008:
            return "coal"
        else:
            return "stone"
    elif depth <= 16:
        r = random.random()
        if r < 0.07 + depth * 0.003:
            return "coal"
        elif r < 0.12 + depth * 0.004:
            return "iron"
        else:
            return "stone"
    else:
        r = random.random()
        if r < 0.04:
            return "coal"
        elif r < 0.09:
            return "iron"
        elif r < 0.13:
            return "gold"
        elif r < 0.17 and biome == "mountain":
            return "diamond"
        else:
            return "stone"

def get_surface_height_variation(biome, world_x, seed):
    """
    Get the surface height variation for a biome at a given position.
    
    Args:
        biome: Biome name string
        world_x: World x coordinate
        seed: Random seed
    
    Returns:
        Height offset value
    """
    characteristics = BIOME_CHARACTERISTICS[biome]
    base_variation = characteristics["surface_variation"]
    
    # Multi-frequency noise for natural-looking terrain
    height_variation = (
        100 * math.sin(world_x * 0.005 + seed * 0.01) +
        50 * math.cos(world_x * 0.015 + seed * 0.02) +
        25 * math.sin(world_x * 0.03 + seed * 0.03)
    )
    
    return height_variation * base_variation

def should_spawn_tree(biome):
    """
    Determine if a tree should spawn in this biome.
    
    Args:
        biome: Biome name string
    
    Returns:
        Boolean indicating if trees can spawn
    """
    characteristics = BIOME_CHARACTERISTICS[biome]
    return random.random() < characteristics["tree_density"]