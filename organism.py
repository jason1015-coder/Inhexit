# organism.py
"""
Organism definitions and logic for living entities in the game.
Trees, plants, and other living objects that can be broken and harvested.
"""

from typing import Dict, Tuple, List, Optional
import random
import math

# Organism types
ORGANISM_TYPES = ["tree"]

# Organism definitions
ORGANISM_DEFINITIONS = {
    "tree": {
        "name": "Tree",
        "health": 100,
        "hitbox_radius": 30,
        "drop_items": ["wood", "sapling"],
        "drop_chances": {"wood": 1.0, "sapling": 0.3},
        "drop_amounts": {"wood": [3, 6], "sapling": [0, 1]},
        "structure": {
            "type": "stacked_circles",
            "components": [
                {"y_offset": 20, "radius": 15, "color": (101, 67, 33)},  # Trunk bottom
                {"y_offset": 5, "radius": 12, "color": (101, 67, 33)},   # Trunk top
                {"y_offset": -15, "radius": 25, "color": (34, 139, 34)},  # Leaves
            ]
        },
        "axe_multiplier": 5.0,  # Axes do 5x damage
        "other_multiplier": 0.2,  # Other weapons do 20% damage
    },
}

class Organism:
    """Base class for all organisms in the game."""
    
    def __init__(self, x, y, organism_type):
        self.x = x
        self.y = y
        self.organism_type = organism_type
        self.definition = ORGANISM_DEFINITIONS[organism_type]
        self.health = self.definition["health"]
        self.max_health = self.definition["health"]
        self.alive = True
        
    def get_damage(self, item_type, base_damage):
        """
        Calculate damage based on the item type used.
        
        Args:
            item_type: Type of item used to attack
            base_damage: Base damage of the attack
        
        Returns:
            Actual damage to deal
        """
        if item_type == "axe":
            return base_damage * self.definition.get("axe_multiplier", 1.0)
        else:
            return base_damage * self.definition.get("other_multiplier", 1.0)
    
    def take_damage(self, item_type, base_damage):
        """
        Apply damage to the organism.
        
        Args:
            item_type: Type of item used to attack
            base_damage: Base damage of the attack
        
        Returns:
            List of (item_type, amount) tuples if organism dies, empty list otherwise
        """
        damage = self.get_damage(item_type, base_damage)
        self.health -= damage
        
        if self.health <= 0:
            self.alive = False
            return self.get_drops()
        return []
    
    def get_drops(self):
        """
        Get items dropped when organism dies.
        
        Returns:
            List of (item_type, amount) tuples
        """
        drops = []
        drop_items = self.definition["drop_items"]
        drop_chances = self.definition["drop_chances"]
        drop_amounts = self.definition["drop_amounts"]
        
        for item_type in drop_items:
            if random.random() < drop_chances[item_type]:
                amount_range = drop_amounts[item_type]
                amount = random.randint(amount_range[0], amount_range[1])
                if amount > 0:
                    drops.append((item_type, amount))
        
        return drops
    
    def draw(self, screen, offset_x, offset_y):
        """
        Draw the organism on screen.
        
        Args:
            screen: Pygame screen surface
            offset_x: Camera x offset
            offset_y: Camera y offset
        """
        structure = self.definition["structure"]
        
        if structure["type"] == "stacked_circles":
            for component in structure["components"]:
                y_offset = component["y_offset"]
                radius = component["radius"]
                color = component["color"]
                
                screen_x = int(self.x - offset_x)
                screen_y = int(self.y + y_offset - offset_y)
                
                pygame.draw.circle(screen, color, (screen_x, screen_y), radius)
                # Add outline for better visibility
                pygame.draw.circle(screen, (0, 0, 0), (screen_x, screen_y), radius, 2)
    
    def get_hitbox(self):
        """
        Get the hitbox for collision detection.
        
        Returns:
            Tuple of (x, y, radius)
        """
        return (self.x, self.y, self.definition["hitbox_radius"])

# Need to import pygame for drawing
import pygame

class Tree(Organism):
    """Specific implementation of a tree organism."""
    
    def __init__(self, x, y):
        super().__init__(x, y, "tree")
    
    def draw(self, screen, offset_x, offset_y):
        """Draw the tree with its distinctive stacked circle structure."""
        # Draw trunk (two brown circles)
        trunk_bottom_color = (101, 67, 33)  # Dark brown
        trunk_top_color = (139, 90, 43)    # Lighter brown
        leaves_color = (34, 139, 34)       # Forest green
        
        screen_x = int(self.x - offset_x)
        screen_y = int(self.y - offset_y)
        
        # Draw trunk bottom (larger brown circle)
        pygame.draw.circle(screen, trunk_bottom_color, (screen_x, screen_y + 20), 15)
        pygame.draw.circle(screen, (0, 0, 0), (screen_x, screen_y + 20), 15, 2)
        
        # Draw trunk top (smaller brown circle)
        pygame.draw.circle(screen, trunk_top_color, (screen_x, screen_y + 5), 12)
        pygame.draw.circle(screen, (0, 0, 0), (screen_x, screen_y + 5), 12, 2)
        
        # Draw leaves (larger green circle on top)
        pygame.draw.circle(screen, leaves_color, (screen_x, screen_y - 15), 25)
        pygame.draw.circle(screen, (0, 0, 0), (screen_x, screen_y - 15), 25, 2)
        
        # Draw health bar if damaged
        if self.health < self.max_health:
            bar_width = 40
            bar_height = 5
            bar_x = screen_x - bar_width // 2
            bar_y = screen_y - 45
            
            # Background bar
            pygame.draw.rect(screen, (100, 100, 100), (bar_x, bar_y, bar_width, bar_height))
            # Health bar
            health_ratio = self.health / self.max_health
            health_width = int(bar_width * health_ratio)
            health_color = (int(255 * (1 - health_ratio)), int(255 * health_ratio), 0)
            pygame.draw.rect(screen, health_color, (bar_x, bar_y, health_width, bar_height))

def create_organism(x, y, organism_type):
    """
    Factory function to create organisms.
    
    Args:
        x: World x coordinate
        y: World y coordinate
        organism_type: Type of organism to create
    
    Returns:
        Organism instance
    """
    if organism_type == "tree":
        return Tree(x, y)
    else:
        return Organism(x, y, organism_type)