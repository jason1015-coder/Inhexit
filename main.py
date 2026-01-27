# main.py
import pygame
import math
import sys
import os
import random
import warnings
import pickle
import numpy as np
import time
from blocks import (
    BLOCK_DEFINITIONS,
    COLOR_BY_TYPE,
    HARDNESS_BY_TYPE,
    TRANSPARENT_BY_TYPE,
    SOLID_BY_TYPE,
    COLLECTIBLE_BY_TYPE
)
from item import (
    ITEM_DEFINITIONS,
    ITEM_COLOR_BY_ID,
    ITEM_ICON_COLOR_BY_ID,
    ITEM_NAME_BY_ID,
    DEFAULT_HOTBAR_ITEMS,
    DEFAULT_ITEM_ID
)
from biomes import (
    get_biome_at_position,
    get_biome_block,
    get_surface_height_variation,
    should_spawn_tree
)
from organism import (
    Organism,
    Tree,
    create_organism,
    ORGANISM_DEFINITIONS
)

warnings.filterwarnings("ignore", category=RuntimeWarning, module="importlib._bootstrap")
os.environ['SDL_AUDIODRIVER'] = 'dummy'

pygame.init()
pygame.mixer.init()

# Constants
SCREEN_WIDTH = 1280
SCREEN_HEIGHT = 720
FPS = 60

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
BLUE = (50, 150, 255)
GRAY = (128, 128, 128)
DARK_GRAY = (64, 64, 64)
GREEN = (100, 200, 100)
RED = (200, 100, 100)

# Physics constants
GRAVITY = 0.5
PLAYER_SPEED = 5
JUMP_FORCE = -12
FRICTION = 0.8
MINING_RANGE = 150

# Hexagon constants
HEX_SIZE = 30
HEX_WIDTH = math.sqrt(3) * HEX_SIZE
HEX_HEIGHT = 2 * HEX_SIZE
HEX_V_SPACING = HEX_HEIGHT * 0.75

# Chunk constants
CHUNK_SIZE = 32
CHUNK_WIDTH = CHUNK_SIZE * HEX_WIDTH
CHUNK_HEIGHT = CHUNK_SIZE * HEX_V_SPACING
RENDER_DISTANCE = 4  # Number of chunks to render around player

def pixel_to_hex_center(wx, wy, hex_size=HEX_SIZE):
    q = (math.sqrt(3)/3 * wx + 1/3 * wy) / hex_size
    r = (-math.sqrt(3)/3 * wx + 2/3 * wy) / hex_size

    # Cube rounding
    x = q
    z = r
    y = -x - z

    rx = round(x)
    ry = round(y)
    rz = round(z)

    x_diff = abs(rx - x)
    y_diff = abs(ry - y)
    z_diff = abs(rz - z)

    if x_diff > y_diff and x_diff > z_diff:
        rx = -ry - rz
    elif y_diff > z_diff:
        ry = -rx - rz
    else:
        rz = -rx - ry

    # Convert cube coordinates back to axial
    q = rx
    r = rz
    
    # Calculate pixel center
    center_x = hex_size * (3/2 * q)
    center_y = hex_size * (math.sqrt(3)/2 * q + math.sqrt(3) * r)
    
    # Return column and row in hex grid for chunk lookup
    # For axial coordinates: col = q, row = r
    col = q
    row = r
    
    return center_x, center_y, col, row

class Particle:
    def __init__(self, x, y, color):
        self.x = x
        self.y = y
        self.vel_x = random.uniform(-2, 2)
        self.vel_y = random.uniform(-3, -1)
        self.color = color
        self.lifetime = 30
        self.age = 0
        self.size = random.randint(2, 4)
    
    def update(self):
        self.vel_y += 0.2
        self.x += self.vel_x
        self.y += self.vel_y
        self.age += 1
    
    def draw_offset(self, screen, offset_x, offset_y):
        alpha = 1 - (self.age / self.lifetime)
        if alpha > 0:
            color = tuple(int(c * alpha) for c in self.color[:3])
            pygame.draw.circle(screen, color, (int(self.x - offset_x), int(self.y - offset_y)), self.size)
    
    def is_dead(self):
        return self.age >= self.lifetime

class Hexagon:
    def __init__(self, x, y, size, block_type='dirt'):
        self.x = x
        self.y = y
        self.size = size
        self.block_type = block_type
        
        # Fallback if invalid block type
        if block_type not in COLOR_BY_TYPE:
            block_type = 'dirt'
            
        self.color = COLOR_BY_TYPE[block_type]
        self.active_color = self.color
        self.hovered = False
        self.health = 100
        self.max_health = 100
        self.transparent = TRANSPARENT_BY_TYPE[block_type]
        self.corners = self._calculate_corners()
        self.chunk_x = 0
        self.chunk_y = 0
        
    def _calculate_corners(self):
        corners = []
        for i in range(6):
            angle = math.radians(30 + 60 * i)
            px = self.x + self.size * math.cos(angle)
            py = self.y + self.size * math.sin(angle)
            corners.append((px, py))
        return corners
    
    def check_hover(self, mouse_x, mouse_y, player_x, player_y):
        dx = mouse_x - self.x
        dy = mouse_y - self.y
        distance_sq = dx * dx + dy * dy
        
        pdx = player_x - self.x
        pdy = player_y - self.y
        player_distance_sq = pdx * pdx + pdy * pdy
        in_range = player_distance_sq < MINING_RANGE * MINING_RANGE
        
        hex_radius_sq = (self.size * 0.866) ** 2
        self.hovered = distance_sq < hex_radius_sq and in_range
        
        if self.hovered:
            self.active_color = tuple(min(c + 30, 255) for c in self.color[:3])
        else:
            self.active_color = self.color
    
    def take_damage(self, amount):
        hardness = HARDNESS_BY_TYPE.get(self.block_type, 1.0)
        if hardness <= 0:
            return False
        self.health -= amount / hardness
        return self.health <= 0
    
    def get_top_surface_y(self, x):
        left = self.corners[3]
        top = self.corners[4]
        right = self.corners[5]
        
        if left[0] <= x <= top[0]:
            if left[0] != top[0]:
                t = (x - left[0]) / (top[0] - left[0])
                return left[1] + t * (top[1] - left[1])
            else:
                return left[1]
        elif top[0] <= x <= right[0]:
            if top[0] != right[0]:
                t = (x - top[0]) / (right[0] - top[0])
                return top[1] + t * (right[1] - top[1])
            else:
                return top[1]
        
        return None

class Chunk:
    def __init__(self, chunk_x, chunk_y):
        self.chunk_x = chunk_x
        self.chunk_y = chunk_y
        self.hexagons = {}
        self.modified = False
        self.last_accessed = time.time()
        
    def get_world_position(self):
        world_x = self.chunk_x * CHUNK_WIDTH
        world_y = self.chunk_y * CHUNK_HEIGHT
        return world_x, world_y
    
    def get_hexagon(self, x, y):
        world_x, world_y = self.get_world_position()
        
        # Calculate local row based on vertical spacing
        local_row = int((y - world_y) / HEX_V_SPACING)
        
        # Calculate column based on whether it's an even or odd row
        if local_row % 2 == 0:
            # Even rows: columns start at half-width offset
            local_col = int((x - world_x - HEX_WIDTH/2) / HEX_WIDTH)
        else:
            # Odd rows: columns align with chunk edge
            local_col = int((x - world_x) / HEX_WIDTH)
        
        return self.hexagons.get((local_col, local_row))
    
    def add_hexagon(self, x, y, hexagon):
        world_x, world_y = self.get_world_position()
        
        # Calculate local row
        local_row = int((y - world_y) / HEX_V_SPACING)
        
        # Calculate local column with proper offset for interlocking
        if local_row % 2 == 0:
            # Even rows: columns start at half-width offset
            local_col = int((x - world_x - HEX_WIDTH/2) / HEX_WIDTH)
        else:
            # Odd rows: columns align with chunk edge
            local_col = int((x - world_x) / HEX_WIDTH)
        
        hexagon.chunk_x = self.chunk_x
        hexagon.chunk_y = self.chunk_y
        self.hexagons[(local_col, local_row)] = hexagon
        self.modified = True
        
    def remove_hexagon(self, x, y):
        world_x, world_y = self.get_world_position()
        
        # Calculate local row
        local_row = int((y - world_y) / HEX_V_SPACING)
        
        # Calculate column based on whether it's an even or odd row
        if local_row % 2 == 0:
            # Even rows: columns start at half-width offset
            local_col = int((x - world_x - HEX_WIDTH/2) / HEX_WIDTH)
        else:
            # Odd rows: columns align with chunk edge
            local_col = int((x - world_x) / HEX_WIDTH)
        
        if (local_col, local_row) in self.hexagons:
            del self.hexagons[(local_col, local_row)]
            self.modified = True
            return True
        return False

class World:
    def __init__(self):
        self.chunks = {}
        self.seed = random.randint(0, 1000000)
        self.organisms = {}  # Dictionary to store organisms by position
        
    def get_chunk_coords(self, x, y):
        chunk_x = int(x // CHUNK_WIDTH)
        chunk_y = int(y // CHUNK_HEIGHT)
        return chunk_x, chunk_y
    
    def get_chunk(self, chunk_x, chunk_y):
        if (chunk_x, chunk_y) not in self.chunks:
            self.chunks[(chunk_x, chunk_y)] = Chunk(chunk_x, chunk_y)
            self.generate_chunk(chunk_x, chunk_y)
        return self.chunks[(chunk_x, chunk_y)]
    
    def generate_chunk(self, chunk_x, chunk_y):
        chunk = self.chunks[(chunk_x, chunk_y)]
        world_x, world_y = chunk.get_world_position()
        
        # Base ground level (Y coordinate where ground starts on average)
        base_ground_y = 500
        
        for local_row in range(CHUNK_SIZE):
            for local_col in range(CHUNK_SIZE):
                # Calculate hexagon center position for interlocking pattern
                if local_row % 2 == 0:
                    # Even rows: columns start at half-width offset
                    hex_x = world_x + local_col * HEX_WIDTH + HEX_WIDTH / 2
                else:
                    # Odd rows: columns align with chunk edge
                    hex_x = world_x + local_col * HEX_WIDTH
                
                hex_y = world_y + local_row * HEX_V_SPACING
                
                # --- IMPROVED TERRAIN GENERATION ---
                # Get biome for this position
                biome = get_biome_at_position(hex_x, self.seed)
                
                # Calculate surface height using biome-specific variation
                height_variation = get_surface_height_variation(biome, hex_x, self.seed)
                surface_height = base_ground_y + height_variation
                
                # Only generate hexagons below surface
                if hex_y < surface_height:
                    continue

                # Calculate depth relative to the surface
                depth = int((hex_y - surface_height) / HEX_V_SPACING)
                
                # Get appropriate block type using biome system
                block_type = get_biome_block(biome, depth, hex_x, hex_y, self.seed)
                
                hexagon = Hexagon(hex_x, hex_y, HEX_SIZE, block_type)
                chunk.add_hexagon(hex_x, hex_y, hexagon)
                
                # Try to spawn trees on surface blocks
                if depth == 0 and should_spawn_tree(biome):
                    # Check if there's space for a tree (no blocks above)
                    above_hex_y = hex_y - HEX_V_SPACING
                    above_hex = self.get_hexagon_at(hex_x, above_hex_y)
                    if above_hex is None:
                        # Spawn tree
                        self.add_organism(hex_x, hex_y, "tree")

        # Procedural generation shouldn't mark the chunk as user-modified
        chunk.modified = False
    
    def get_nearby_hexagons(self, center_x, center_y, radius=200):
        nearby = []
        
        min_chunk_x, min_chunk_y = self.get_chunk_coords(center_x - radius, center_y - radius)
        max_chunk_x, max_chunk_y = self.get_chunk_coords(center_x + radius, center_y + radius)
        
        for chunk_x in range(min_chunk_x - 1, max_chunk_x + 2):
            for chunk_y in range(min_chunk_y - 1, max_chunk_y + 2):
                chunk = self.get_chunk(chunk_x, chunk_y)
                chunk.last_accessed = time.time()

                for hexagon in chunk.hexagons.values():
                    if abs(hexagon.x - center_x) <= radius + HEX_SIZE and abs(hexagon.y - center_y) <= radius + HEX_SIZE:
                        nearby.append(hexagon)
        
        return nearby
    
    def add_organism(self, x, y, organism_type):
        """Add an organism to the world at the given position."""
        organism = create_organism(x, y, organism_type)
        self.organisms[(x, y)] = organism
        return organism
    
    def get_nearby_organisms(self, center_x, center_y, radius=200):
        """Get all organisms within a certain radius of a point."""
        nearby = []
        for (ox, oy), organism in self.organisms.items():
            distance = math.sqrt((ox - center_x)**2 + (oy - center_y)**2)
            if distance <= radius:
                nearby.append(organism)
        return nearby
    
    def remove_organism(self, x, y):
        """Remove an organism at the given position."""
        if (x, y) in self.organisms:
            del self.organisms[(x, y)]
            return True
        return False
    
    def get_organism_at(self, x, y, tolerance=30):
        """Get an organism at or near the given position."""
        for (ox, oy), organism in self.organisms.items():
            distance = math.sqrt((ox - x)**2 + (oy - y)**2)
            if distance <= tolerance:
                return organism
        return None
    
    def get_hexagon_at(self, wx, wy):
        # Get snapped hex center position and grid coordinates
        center_x, center_y, col, row = pixel_to_hex_center(wx, wy)
        
        # Convert to chunk coordinates
        chunk_x, chunk_y = self.get_chunk_coords(center_x, center_y)
        
        chunk = self.get_chunk(chunk_x, chunk_y)
        return chunk.get_hexagon(center_x, center_y)
    
    def remove_hexagon_at(self, x, y):
        # First get the snapped position
        center_x, center_y, col, row = pixel_to_hex_center(x, y)
        
        chunk_x, chunk_y = self.get_chunk_coords(center_x, center_y)
        chunk = self.get_chunk(chunk_x, chunk_y)
        return chunk.remove_hexagon(center_x, center_y)
    
    def add_hexagon_at(self, x, y, block_type):
        # First, get the snapped hex position
        center_x, center_y, col, row = pixel_to_hex_center(x, y)
        
        # Check if there's already a hexagon at this position
        existing = self.get_hexagon_at(center_x, center_y)
        if existing is not None:
            return None  # Don't place if already occupied
        
        # Get the chunk for this position
        chunk_x, chunk_y = self.get_chunk_coords(center_x, center_y)
        chunk = self.get_chunk(chunk_x, chunk_y)
        
        # Create and add the hexagon
        hexagon = Hexagon(center_x, center_y, HEX_SIZE, block_type)
        chunk.add_hexagon(center_x, center_y, hexagon)
        return hexagon
    
    def unload_distant_chunks(self, player_x, player_y, max_chunks=50):
        # Sort chunks by last accessed time
        chunk_list = list(self.chunks.items())
        chunk_list.sort(key=lambda x: x[1].last_accessed, reverse=True)
        
        # Keep only the most recently accessed chunks
        if len(chunk_list) > max_chunks:
            for coords, chunk in chunk_list[max_chunks:]:
                del self.chunks[coords]
    
    def save_to_file(self, filename):
        data = {
            'chunks': {},
            'seed': self.seed
        }
        
        for (chunk_x, chunk_y), chunk in self.chunks.items():
            if chunk.modified:
                chunk_data = {
                    'hexagons': {},
                    'modified': chunk.modified
                }
                for (local_x, local_y), hexagon in chunk.hexagons.items():
                    chunk_data['hexagons'][(local_x, local_y)] = {
                        'x': hexagon.x,
                        'y': hexagon.y,
                        'block_type': hexagon.block_type,
                        'health': hexagon.health
                    }
                data['chunks'][(chunk_x, chunk_y)] = chunk_data
        
        with open(filename, 'wb') as f:
            pickle.dump(data, f)
    
    def load_from_file(self, filename):
        try:
            with open(filename, 'rb') as f:
                data = pickle.load(f)
            
            if not isinstance(data, dict) or 'chunks' not in data:
                return False
            
            self.seed = data.get('seed', random.randint(0, 1000000))
            
            for (chunk_x, chunk_y), chunk_data in data['chunks'].items():
                chunk = Chunk(chunk_x, chunk_y)
                chunk.modified = chunk_data.get('modified', True)
                
                for (local_x, local_y), hex_data in chunk_data['hexagons'].items():
                    hexagon = Hexagon(
                        hex_data['x'],
                        hex_data['y'],
                        HEX_SIZE,
                        hex_data['block_type']
                    )
                    hexagon.health = hex_data.get('health', 100)
                    hexagon.chunk_x = chunk_x
                    hexagon.chunk_y = chunk_y
                    chunk.hexagons[(local_x, local_y)] = hexagon
                
                self.chunks[(chunk_x, chunk_y)] = chunk
            
            return True
        except (FileNotFoundError, EOFError, pickle.UnpicklingError):
            return False

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.radius = 15
        self.vel_x = 0
        self.vel_y = 0
        self.on_ground = False
        self.color = BLUE
        # Only include collectible blocks in inventory
        self.inventory = {item_id: 0 for item_id in ITEM_DEFINITIONS.keys()}
        self.selected_slot = 0
        self.flight_mode = False  # Creative mode flight
        self.width = 30  # Player width for collision
        self.height = 50  # Player height for collision
        
    def get_selected_item(self):
        if self.selected_slot < len(DEFAULT_HOTBAR_ITEMS):
            return DEFAULT_HOTBAR_ITEMS[self.selected_slot]
        return DEFAULT_ITEM_ID
    
    def update(self, keys, hexagons):
        if self.flight_mode:
            # Creative mode flight controls
            if keys[pygame.K_a]:
                self.vel_x = -PLAYER_SPEED
            elif keys[pygame.K_d]:
                self.vel_x = PLAYER_SPEED
            else:
                self.vel_x *= 0.9
            
            if keys[pygame.K_w]:
                self.vel_y = -PLAYER_SPEED
            elif keys[pygame.K_s]:
                self.vel_y = PLAYER_SPEED
            else:
                self.vel_y *= 0.9
        else:
            # Survival mode physics
            if keys[pygame.K_a]:
                self.vel_x = -PLAYER_SPEED
            elif keys[pygame.K_d]:
                self.vel_x = PLAYER_SPEED
            else:
                self.vel_x *= FRICTION
            
            self.vel_y += GRAVITY
        
        # Separate X and Y movement for better collision handling
        self.x += self.vel_x
        self.handle_horizontal_collision(hexagons)
        
        self.y += self.vel_y
        self.handle_vertical_collision(hexagons)
        
        # Ensure on_ground state is properly updated
        if not self.flight_mode and not self.on_ground:
            self.on_ground = False
    
    def handle_horizontal_collision(self, hexagons):
        """Handle horizontal collisions with hexagons."""
        if self.flight_mode:
            return
            
        player_left = self.x - self.radius
        player_right = self.x + self.radius
        
        for hexagon in hexagons:
            if hexagon.transparent:
                continue
                
            # Check if player is horizontally colliding with this hexagon
            hex_left = hexagon.x - hexagon.size * 0.866
            hex_right = hexagon.x + hexagon.size * 0.866
            
            if player_right > hex_left and player_left < hex_right:
                # Check vertical overlap
                player_bottom = self.y + self.radius
                player_top = self.y - self.radius
                hex_bottom = hexagon.y + hexagon.size
                hex_top = hexagon.y - hexagon.size
                
                if player_bottom > hex_top and player_top < hex_bottom:
                    # Determine which side we're hitting
                    if self.vel_x > 0:  # Moving right
                        overlap = player_right - hex_left
                        if overlap > 0 and overlap < self.radius + hexagon.size:
                            self.x -= overlap
                            self.vel_x = 0
                    elif self.vel_x < 0:  # Moving left
                        overlap = hex_right - player_left
                        if overlap > 0 and overlap < self.radius + hexagon.size:
                            self.x += overlap
                            self.vel_x = 0
    
    def handle_vertical_collision(self, hexagons):
        """Handle vertical collisions with hexagons."""
        if self.flight_mode:
            return
            
        player_bottom = self.y + self.radius
        player_top = self.y - self.radius
        
        self.on_ground = False
        
        for hexagon in hexagons:
            if hexagon.transparent:
                continue
                
            # Check if player is vertically colliding with this hexagon
            player_left = self.x - self.radius
            player_right = self.x + self.radius
            hex_left = hexagon.x - hexagon.size * 0.866
            hex_right = hexagon.x + hexagon.size * 0.866
            
            if player_right > hex_left and player_left < hex_right:
                # Check vertical collision
                hex_bottom = hexagon.y + hexagon.size * 0.5
                hex_top = hexagon.y - hexagon.size * 0.5
                
                if self.vel_y >= 0:  # Falling down
                    # Check if landing on top of hexagon
                    surface_y = hexagon.get_top_surface_y(self.x)
                    if surface_y is not None:
                        if player_bottom >= surface_y - 5 and player_bottom <= surface_y + 15:
                            self.y = surface_y - self.radius
                            self.vel_y = 0
                            self.on_ground = True
                            return
                else:  # Moving up
                    # Check if hitting bottom of hexagon
                    if player_top < hex_bottom and player_bottom > hex_top:
                        overlap = hex_bottom - player_top
                        if overlap > 0:
                            self.y += overlap
                            self.vel_y = 0
    
    def jump(self):
        if self.on_ground or self.flight_mode:
            self.vel_y = JUMP_FORCE
            if not self.flight_mode:
                self.on_ground = False
    
    def add_to_inventory(self, block_type):
        if block_type in self.inventory:
            self.inventory[block_type] += 1
    
    def remove_from_inventory(self, block_type):
        if self.inventory.get(block_type, 0) > 0:
            self.inventory[block_type] -= 1
            return True
        return False

    def draw_offset(self, screen, offset_x, offset_y):
        screen_x = int(self.x - offset_x)
        screen_y = int(self.y - offset_y)
        pygame.draw.circle(screen, self.color, (screen_x, screen_y), self.radius)
        # Draw eyes
        pygame.draw.circle(screen, WHITE, (screen_x - 5, screen_y - 3), 4)
        pygame.draw.circle(screen, WHITE, (screen_x + 5, screen_y - 3), 4)
        pygame.draw.circle(screen, BLACK, (screen_x - 5, screen_y - 3), 2)
        pygame.draw.circle(screen, BLACK, (screen_x + 5, screen_y - 3), 2)

        # Draw mining range indicator
        if not self.flight_mode:
            pygame.draw.circle(screen, (255, 255, 255, 50), (screen_x, screen_y), MINING_RANGE, 1)

class Button:
    def __init__(self, x, y, width, height, text, color, hover_color, action):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color = color
        self.hover_color = hover_color
        self.action = action
        self.hovered = False
        
    def check_hover(self, mouse_pos):
        self.hovered = self.rect.collidepoint(mouse_pos)
        return self.hovered
    
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1 and self.hovered:
            self.action()
            return True
        return False
    
    def draw(self, screen, font):
        color = self.hover_color if self.hovered else self.color
        pygame.draw.rect(screen, color, self.rect, border_radius=8)
        pygame.draw.rect(screen, WHITE, self.rect, 2, border_radius=8)
        
        text_surface = font.render(self.text, True, WHITE)
        text_rect = text_surface.get_rect(center=self.rect.center)
        screen.blit(text_surface, text_rect)

class Game:
    def __init__(self):
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Tesselbox")
        self.clock = pygame.time.Clock()
        self.state = "MENU"
        self.running = True
        self.player = None
        self.world = None
        self.particles = []
        self.mining = False
        self.camera_x = 0
        self.camera_y = 0
        self.sky_surface = self._create_sky_gradient()
        self.paused = False
        self.game_mode = "survival"  # "survival" or "creative"
        self.render_distance = RENDER_DISTANCE
        self.current_world_name = "world"
        
        # UI Elements
        self.buttons = []
        self._create_menu_buttons()
        self._create_pause_buttons()
        self._get_worlds()
        
    def _create_sky_gradient(self):
        surface = pygame.Surface((SCREEN_WIDTH, SCREEN_HEIGHT))
        for y in range(SCREEN_HEIGHT):
            ratio = y / SCREEN_HEIGHT
            color = (
                int(135 - ratio * 85),
                int(206 - ratio * 106),
                int(235 - ratio * 35)
            )
            pygame.draw.line(surface, color, (0, y), (SCREEN_WIDTH, y))
        return surface

    def _create_menu_buttons(self):
        button_width = 200
        button_height = 50
        center_x = SCREEN_WIDTH // 2 - button_width // 2
        
        # World selection buttons (dynamic)
        self.world_buttons = []
        for i, world_name in enumerate(self.available_worlds[:5]):  # Show up to 5 worlds
            y_pos = 250 + i * 60
            button = Button(center_x, y_pos, button_width, button_height, f"World: {world_name}",
                          GREEN, (120, 220, 120), 
                          lambda wn=world_name: self.start_game("survival", world_name=wn))
            self.world_buttons.append(button)
        
        # Add "New World" button
        if len(self.world_buttons) < 5:
            new_world_y = 250 + len(self.world_buttons) * 60
            new_world_button = Button(center_x, new_world_y, button_width, button_height, "+ New World",
                                    BLUE, (70, 170, 255), self.create_new_world)
            self.world_buttons.append(new_world_button)
        
        self.menu_buttons = [
            Button(center_x, 550, button_width, button_height, "Exit", 
                   RED, (220, 120, 120), lambda: self.quit_game())
        ]
    
    def _create_pause_buttons(self):
        button_width = 200
        button_height = 50
        center_x = SCREEN_WIDTH // 2 - button_width // 2
        
        self.pause_buttons = [
            Button(center_x, 250, button_width, button_height, "Resume", 
                   GREEN, (120, 220, 120), self.resume_game),
            Button(center_x, 320, button_width, button_height, "Save Game", 
                   BLUE, (70, 170, 255), self.save_game),
            Button(center_x, 390, button_width, button_height, "Back to Menu", 
                   RED, (220, 120, 120), self.back_to_menu)
        ]
    
    def _get_worlds(self):
        """Get list of available world save files."""
        self.available_worlds = []
        if os.path.exists("saves"):
            for filename in os.listdir("saves"):
                if filename.endswith(".pkl") and not filename.startswith("player_"):
                    world_name = filename.replace(".pkl", "")
                    self.available_worlds.append(world_name)
        
        if not self.available_worlds:
            self.available_worlds.append("world")
    
    def quit_game(self):
        self.running = False
    
    def create_new_world(self):
        """Create a new world with a unique name."""
        base_name = "world"
        counter = 1
        while True:
            new_name = f"{base_name}_{counter}" if counter > 1 else base_name
            if new_name not in self.available_worlds:
                break
            counter += 1
        
        # Create saves directory if it doesn't exist
        if not os.path.exists("saves"):
            os.makedirs("saves")
        
        # Update available worlds list
        self.available_worlds.append(new_name)
        self._create_menu_buttons()
        self.start_game("survival", world_name=new_name)
    
    def start_game(self, mode, world_name="world"):
        self.game_mode = mode
        self.current_world_name = world_name
        
        # Ensure saves directory exists
        if not os.path.exists("saves"):
            os.makedirs("saves")
        
        # Create new world instance
        self.world = World()
        
        # Try to load existing world
        world_file = os.path.join("saves", f"{world_name}.pkl")
        player_file = os.path.join("saves", f"player_{world_name}.pkl")
        
        if self.world.load_from_file(world_file):
            # Find player spawn point from saved data
            try:
                with open(player_file, 'rb') as f:
                    player_data = pickle.load(f)
                self.player = Player(player_data['x'], player_data['y'])
                self.player.inventory = player_data['inventory']
                self.player.selected_slot = player_data['selected_slot']
            except:
                # If player data fails, spawn at default location
                self._spawn_player()
        else:
            self._spawn_player()
        
        self.player.flight_mode = (mode == "creative")
        self.particles = []
        self.state = "GAME"
    
    def _spawn_player(self):
        # Spawn logic: Find the surface
        spawn_x = SCREEN_WIDTH // 2
        spawn_y = 200  # Default fallback
        
        # Scan downwards from top of screen to find the first block
        # We check a range that covers typical terrain height
        found_ground = False
        for y in range(0, SCREEN_HEIGHT * 2, 20):
            # Check if there is a block at this position
            if self.world.get_hexagon_at(spawn_x, y):
                spawn_y = y - 50  # Spawn slightly above the block
                found_ground = True
                break
        
        if not found_ground:
            # If no ground found (e.g. gap), default to a safe height
            spawn_y = 300

        self.player = Player(spawn_x, spawn_y)
        
        if self.game_mode == "creative":
            for bt in COLOR_BY_TYPE:
                if COLLECTIBLE_BY_TYPE.get(bt, False):
                    self.player.inventory[bt] = 999
    
    def resume_game(self):
        self.paused = False
        self.state = "GAME"
    
    def save_game(self):
        if self.player:
            # Ensure saves directory exists
            if not os.path.exists("saves"):
                os.makedirs("saves")
            
            world_file = os.path.join("saves", f"{self.current_world_name}.pkl")
            player_file = os.path.join("saves", f"player_{self.current_world_name}.pkl")
            
            self.world.save_to_file(world_file)
            player_data = {
                'x': self.player.x,
                'y': self.player.y,
                'inventory': self.player.inventory,
                'selected_slot': self.player.selected_slot
            }
            with open(player_file, 'wb') as f:
                pickle.dump(player_data, f)
    
    def back_to_menu(self):
        """Return to main menu, properly cleaning up game state."""
        try:
            self.save_game()
        except Exception as e:
            print(f"Error saving game: {e}")
        
        # Reset game state
        self.player = None
        self.world = World()
        self.particles = []
        self.camera_x = 0
        self.camera_y = 0
        self.paused = False
        self.state = "MENU"
    
    def handle_menu_input(self, event):
        mouse_pos = pygame.mouse.get_pos()
        
        # Handle world selection buttons
        for button in self.world_buttons:
            button.check_hover(mouse_pos)
            button.handle_event(event)
        
        # Handle menu buttons (Exit)
        for button in self.menu_buttons:
            button.check_hover(mouse_pos)
            button.handle_event(event)
        
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                self.quit_game()
    
    def handle_game_input(self, event):
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                self.player.jump()
            elif event.key == pygame.K_ESCAPE:
                self.paused = not self.paused
                if self.paused:
                    self.state = "PAUSE"
                else:
                    self.state = "GAME"
            elif event.key == pygame.K_1:
                self.player.selected_slot = 0
            elif event.key == pygame.K_2:
                self.player.selected_slot = 1
            elif event.key == pygame.K_3:
                self.player.selected_slot = 2
            elif event.key == pygame.K_4:
                self.player.selected_slot = 3
            elif event.key == pygame.K_5:
                self.player.selected_slot = 4
            elif event.key == pygame.K_6:
                self.player.selected_slot = 5
            elif event.key == pygame.K_7:
                self.player.selected_slot = 6
            elif event.key == pygame.K_8:
                self.player.selected_slot = 7
            elif event.key == pygame.K_9:
                self.player.selected_slot = 8
            elif event.key == pygame.K_EQUALS:  # Increase render distance
                self.render_distance = min(self.render_distance + 1, 10)
            elif event.key == pygame.K_MINUS:  # Decrease render distance
                self.render_distance = max(self.render_distance - 1, 2)
            elif event.key == pygame.K_f and self.game_mode == "creative":
                self.player.flight_mode = not self.player.flight_mode
        
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                self.mining = True
            elif event.button == 3:
                self.place_block()
            elif event.button == 4:  # Mouse wheel up
                self.player.selected_slot = max(0, self.player.selected_slot - 1)
            elif event.button == 5:  # Mouse wheel down
                self.player.selected_slot = min(8, self.player.selected_slot + 1)
        
        if event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1:
                self.mining = False
    
    def handle_pause_input(self, event):
        mouse_pos = pygame.mouse.get_pos()
        for button in self.pause_buttons:
            button.check_hover(mouse_pos)
            button.handle_event(event)
        
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                self.resume_game()
    
    def raycast_to_block(self, start_x, start_y, target_x, target_y, max_distance=MINING_RANGE):
        """Raycast from player position to find the first block in line of sight"""
        dx = target_x - start_x
        dy = target_y - start_y
        distance = math.sqrt(dx * dx + dy * dy)
        
        if distance > max_distance or distance == 0:
            return None, None
        
        # Normalize direction
        dx /= distance
        dy /= distance
        
        # Step along the ray
        step_size = 5
        for d in range(0, int(distance), step_size):
            x = start_x + dx * d
            y = start_y + dy * d
            
            # Check for block at this position
            block = self.world.get_hexagon_at(x, y)
            if block and not block.transparent:
                # Calculate the position for placing a block (just outside the hit block)
                place_x = start_x + dx * (d - step_size)
                place_y = start_y + dy * (d - step_size)
                return block, (place_x, place_y)
        
        return None, None
    
    def mine_block(self):
        mouse_pos = pygame.mouse.get_pos()
        world_mx = mouse_pos[0] + self.camera_x
        world_my = mouse_pos[1] + self.camera_y
        
        # Check for organisms first
        target_organism = self.world.get_organism_at(world_mx, world_my)
        if target_organism:
            # Check if in range
            dx = self.player.x - target_organism.x
            dy = self.player.y - target_organism.y
            distance = math.sqrt(dx*dx + dy*dy)
            
            if distance <= MINING_RANGE:
                # Get selected item type
                selected_item = self.player.get_selected_item()
                item_info = ITEM_DEFINITIONS.get(selected_item, {})
                tool_type = item_info.get("tool_type", "none")
                base_damage = item_info.get("damage", 1)
                
                # Damage organism
                drops = target_organism.take_damage(tool_type, base_damage)
                
                # Create hit particles
                for _ in range(5):
                    particle = Particle(target_organism.x, target_organism.y, (200, 100, 50))
                    self.particles.append(particle)
                
                # Handle drops if organism died
                if not target_organism.alive:
                    self.world.remove_organism(target_organism.x, target_organism.y)
                    for drop_type, amount in drops:
                        for _ in range(amount):
                            self.player.add_to_inventory(drop_type)
                            # Create drop particles
                            for _ in range(3):
                                particle = Particle(target_organism.x, target_organism.y - 20, 
                                                  ITEM_ICON_COLOR_BY_ID.get(drop_type, (100, 100, 100)))
                                self.particles.append(particle)
            return
        
        # Use raycasting for accurate block selection
        target_block, _ = self.raycast_to_block(self.player.x, self.player.y, world_mx, world_my)
        
        if target_block:
            # Get selected item for damage calculation
            selected_item = self.player.get_selected_item()
            item_info = ITEM_DEFINITIONS.get(selected_item, {})
            base_damage = item_info.get("damage", 5)
            
            if target_block.take_damage(base_damage):
                # Create particles
                for _ in range(10):
                    particle = Particle(target_block.x, target_block.y, target_block.color)
                    self.particles.append(particle)
                
                self.player.add_to_inventory(target_block.block_type)
                self.world.remove_hexagon_at(target_block.x, target_block.y)
    
    def place_block(self):
        mouse_pos = pygame.mouse.get_pos()
        world_mx = mouse_pos[0] + self.camera_x
        world_my = mouse_pos[1] + self.camera_y
        
        selected = self.player.get_selected_item()
        
        # Check if selected item is placeable
        item_info = ITEM_DEFINITIONS.get(selected, {})
        item_type = item_info.get("type", "block")
        
        # Handle different item types
        if item_type == "tool":
            # Tools cannot be placed
            return
        
        if item_type == "plantable":
            # Plant sapling
            # Check if there's a solid block below
            place_x, place_y, col, row = pixel_to_hex_center(world_mx, world_my)
            below_x, below_y, _, _ = pixel_to_hex_center(world_mx, world_my + HEX_V_SPACING)
            
            below_block = self.world.get_hexagon_at(below_x, below_y)
            if below_block and not below_block.transparent:
                # Check if there's already an organism here
                if self.world.get_organism_at(place_x, place_y) is None:
                    # In survival: consume from inventory
                    if self.game_mode != "creative" and not self.player.remove_from_inventory(selected):
                        return
                    
                    # For now, saplings don't grow - this is a placeholder for future growth logic
                    # Create particles for planting
                    for _ in range(5):
                        particle = Particle(place_x, place_y, (50, 180, 50))
                        self.particles.append(particle)
            return
        
        # Regular block placement
        # Get snapped hex position
        place_x, place_y, col, row = pixel_to_hex_center(world_mx, world_my)
        
        # Distance checks
        dx = self.player.x - place_x
        dy = self.player.y - place_y
        dist_sq = dx*dx + dy*dy
        
        if dist_sq > MINING_RANGE**2:
            return
        
        if dist_sq < (HEX_SIZE + self.player.radius)**2:
            return
        
        # Check if position is already occupied
        if self.world.get_hexagon_at(place_x, place_y) is not None:
            return
        
        # In survival: consume from inventory
        if self.game_mode != "creative" and not self.player.remove_from_inventory(selected):
            return
        
        # Place the block
        self.world.add_hexagon_at(place_x, place_y, selected)

    def update_game(self):
        keys = pygame.key.get_pressed()
        
        # Update camera - no clamping = can explore left & up
        self.camera_x = self.player.x - SCREEN_WIDTH // 2
        self.camera_y = self.player.y - SCREEN_HEIGHT // 2
        
        # Only process hexagons near player
        nearby_hexagons = self.world.get_nearby_hexagons(self.player.x, self.player.y, radius=500)
        self.player.update(keys, nearby_hexagons)
        
        # Update hover states
        mouse_pos = pygame.mouse.get_pos()
        world_mx = mouse_pos[0] + self.camera_x
        world_my = mouse_pos[1] + self.camera_y
        hover_nearby = self.world.get_nearby_hexagons(world_mx, world_my, radius=HEX_SIZE * 3)
        
        for hexagon in hover_nearby:
            hexagon.check_hover(world_mx, world_my, self.player.x, self.player.y)
        
        if self.mining:
            self.mine_block()
        
        # Update particles
        self.particles = [p for p in self.particles if not (p.update() or p.is_dead())]
        
        # Unload distant chunks
        self.world.unload_distant_chunks(self.player.x, self.player.y)
    
    def draw_menu(self):
        self.screen.fill(BLACK)
        
        title_font = pygame.font.Font(None, 74)
        title = title_font.render("TESSELBOX", True, WHITE)
        title_rect = title.get_rect(center=(SCREEN_WIDTH // 2, 100))
        self.screen.blit(title, title_rect)
        
        subtitle_font = pygame.font.Font(None, 36)
        subtitle = subtitle_font.render("Select a World to Play", True, GRAY)
        subtitle_rect = subtitle.get_rect(center=(SCREEN_WIDTH // 2, 160))
        self.screen.blit(subtitle, subtitle_rect)
        
        mouse_pos = pygame.mouse.get_pos()
        
        # Draw world selection buttons
        for button in self.world_buttons:
            button.check_hover(mouse_pos)
            button.draw(self.screen, pygame.font.Font(None, 32))
        
        # Draw menu buttons (Exit)
        for button in self.menu_buttons:
            button.check_hover(mouse_pos)
            button.draw(self.screen, pygame.font.Font(None, 32))
        
        # Draw controls (simplified)
        font = pygame.font.Font(None, 20)
        controls = [
            "Controls: WASD-Move | Space-Jump | Left Click-Mine | Right Click-Place",
            "1-9/Scroll-Select Item | +/--Render Distance | F-Flight (Creative) | ESC-Pause"
        ]
        
        y_pos = 650
        for control in controls:
            text = font.render(control, True, DARK_GRAY)
            text_rect = text.get_rect(center=(SCREEN_WIDTH // 2, y_pos))
            self.screen.blit(text, text_rect)
            y_pos += 25
    
    def draw_pause_menu(self):
        # Draw semi-transparent overlay
        overlay = pygame.Surface((SCREEN_WIDTH, SCREEN_HEIGHT))
        overlay.set_alpha(128)
        overlay.fill(BLACK)
        self.screen.blit(overlay, (0, 0))
        
        title_font = pygame.font.Font(None, 74)
        title = title_font.render("PAUSED", True, WHITE)
        title_rect = title.get_rect(center=(SCREEN_WIDTH // 2, 150))
        self.screen.blit(title, title_rect)
        
        mouse_pos = pygame.mouse.get_pos()
        for button in self.pause_buttons:
            button.check_hover(mouse_pos)
            button.draw(self.screen, pygame.font.Font(None, 32))
    
    def draw_game(self):
        # Use pre-rendered sky gradient
        self.screen.blit(self.sky_surface, (0, 0))
        
        # Get visible chunks based on render distance
        player_chunk_x, player_chunk_y = self.world.get_chunk_coords(self.player.x, self.player.y)
        
        visible_hexagons = []
        for chunk_x in range(player_chunk_x - self.render_distance, player_chunk_x + self.render_distance + 1):
            for chunk_y in range(player_chunk_y - self.render_distance, player_chunk_y + self.render_distance + 1):
                if (chunk_x, chunk_y) in self.world.chunks:
                    chunk = self.world.chunks[(chunk_x, chunk_y)]
                    # Frustum culling - only add hexagons that are on screen
                    for hexagon in chunk.hexagons.values():
                        screen_x = hexagon.x - self.camera_x
                        screen_y = hexagon.y - self.camera_y
                        
                        if -HEX_SIZE < screen_x < SCREEN_WIDTH + HEX_SIZE and -HEX_SIZE < screen_y < SCREEN_HEIGHT + HEX_SIZE:
                            visible_hexagons.append(hexagon)
        
        # Sort by y position for proper depth rendering
        visible_hexagons.sort(key=lambda h: h.y)
        
        # Draw hexagons
        for hexagon in visible_hexagons:
            offset_corners = [(cx - self.camera_x, cy - self.camera_y) for cx, cy in hexagon.corners]
            pygame.draw.polygon(self.screen, hexagon.active_color, offset_corners)
            pygame.draw.polygon(self.screen, DARK_GRAY, offset_corners, 2)
            
            if hexagon.health < hexagon.max_health:
                health_ratio = hexagon.health / hexagon.max_health
                crack_color = (255, int(255 * health_ratio), int(255 * health_ratio))
                pygame.draw.polygon(self.screen, crack_color, offset_corners, 3)
        
        # Draw visible organisms
        visible_organisms = self.world.get_nearby_organisms(self.player.x, self.player.y, radius=600)
        for organism in visible_organisms:
            screen_x = organism.x - self.camera_x
            screen_y = organism.y - self.camera_y
            if -100 < screen_x < SCREEN_WIDTH + 100 and -100 < screen_y < SCREEN_HEIGHT + 100:
                organism.draw(self.screen, self.camera_x, self.camera_y)
        
        # Draw particles
        for particle in self.particles:
            px = particle.x - self.camera_x
            py = particle.y - self.camera_y
            if -10 < px < SCREEN_WIDTH + 10 and -10 < py < SCREEN_HEIGHT + 10:
                particle.draw_offset(self.screen, self.camera_x, self.camera_y)
        
        self.player.draw_offset(self.screen, self.camera_x, self.camera_y)
        self.draw_ui()
    
    def draw_ui(self):
        # Draw hotbar
        hotbar_width = 400
        hotbar_height = 50
        hotbar_x = SCREEN_WIDTH // 2 - hotbar_width // 2
        hotbar_y = SCREEN_HEIGHT - hotbar_height - 10
        
        block_types_list = list(COLOR_BY_TYPE.keys())
        hotbar_items = DEFAULT_HOTBAR_ITEMS[:9]  # Limit to 9 slots
        slot_width = hotbar_width // 9
        
        for i in range(9):
            slot_x = hotbar_x + i * slot_width
            slot_rect = pygame.Rect(slot_x, hotbar_y, slot_width - 2, hotbar_height)
            
            # Highlight selected slot
            if i == self.player.selected_slot:
                pygame.draw.rect(self.screen, WHITE, slot_rect, 3)
            else:
                pygame.draw.rect(self.screen, GRAY, slot_rect, 1)
            
            if i < len(hotbar_items):
                item_id = hotbar_items[i]
                color = ITEM_ICON_COLOR_BY_ID[item_id]
                
                # Draw item preview (handle possible RGBA colors)
                draw_color = color[:3] if len(color) == 4 else color
                pygame.draw.rect(self.screen, draw_color,
                               (slot_x + 5, hotbar_y + 5, slot_width - 12, hotbar_height - 10))
                
                # Draw count (only in survival mode)
                if self.game_mode == "survival":
                    count = self.player.inventory.get(item_id, 0)
                    font = pygame.font.Font(None, 20)
                    count_text = font.render(str(count), True, WHITE)
                    count_rect = count_text.get_rect(bottomright=(slot_x + slot_width - 5, hotbar_y + hotbar_height - 5))
                    self.screen.blit(count_text, count_rect)
                
                # Draw item name on hover
                mouse_pos = pygame.mouse.get_pos()
                if slot_rect.collidepoint(mouse_pos):
                    font = pygame.font.Font(None, 24)
                    name_text = font.render(ITEM_NAME_BY_ID[item_id], True, WHITE)
                    name_rect = name_text.get_rect(midtop=(slot_x + slot_width // 2, hotbar_y - 5))
                    
                    # Draw tooltip background
                    tooltip_rect = name_rect.inflate(10, 10)
                    pygame.draw.rect(self.screen, (50, 50, 50), tooltip_rect, border_radius=5)
                    pygame.draw.rect(self.screen, WHITE, tooltip_rect, 1, border_radius=5)
                    self.screen.blit(name_text, name_rect)
        
        # Draw mode indicator
        font = pygame.font.Font(None, 24)
        mode_text = f"Mode: {self.game_mode.capitalize()}"
        mode_surface = font.render(mode_text, True, WHITE)
        self.screen.blit(mode_surface, (10, 10))
        
        # Draw render distance
        render_text = f"Render Distance: {self.render_distance}"
        render_surface = font.render(render_text, True, WHITE)
        self.screen.blit(render_surface, (10, 35))
        
        # Draw flight status in creative mode
        if self.game_mode == "creative":
            flight_text = f"Flight: {'ON' if self.player.flight_mode else 'OFF'}"
            flight_surface = font.render(flight_text, True, WHITE)
            self.screen.blit(flight_surface, (10, 60))
    
    def run(self):
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.save_game()
                    self.running = False
                
                if self.state == "MENU":
                    self.handle_menu_input(event)
                elif self.state == "GAME":
                    self.handle_game_input(event)
                elif self.state == "PAUSE":
                    self.handle_pause_input(event)
            
            if self.state == "MENU":
                self.draw_menu()
            elif self.state == "GAME":
                self.update_game()
                self.draw_game()
            elif self.state == "PAUSE":
                self.draw_pause_menu()
            
            pygame.display.flip()
            self.clock.tick(FPS)
        
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    game = Game()
    game.run()
