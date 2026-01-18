import pygame
import math
import sys
import os
import random
import warnings
import pickle


warnings.filterwarnings("ignore", category=RuntimeWarning, module="importlib._bootstrap")
# Set SDL audio driver to dummy to avoid ALSA errors
os.environ['SDL_AUDIODRIVER'] = 'dummy'

# Initialize pygame
pygame.init()
pygame.mixer.init()

# Constants
SCREEN_WIDTH = 1200
SCREEN_HEIGHT = 800
FPS = 60

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
BLUE = (50, 150, 255)
GRAY = (128, 128, 128)
DARK_GRAY = (64, 64, 64)
GREEN = (100, 200, 100)
RED = (200, 100, 100)
BROWN = (139, 90, 43)
STONE_GRAY = (169, 169, 169)
GOLD = (255, 215, 0)

# Physics constants
GRAVITY = 0.5
PLAYER_SPEED = 5
JUMP_FORCE = -12
FRICTION = 0.8
MINING_RANGE = 100

# Hexagon constants
HEX_SIZE = 30
HEX_WIDTH = math.sqrt(3) * HEX_SIZE
HEX_HEIGHT = 2 * HEX_SIZE
HEX_V_SPACING = HEX_HEIGHT * 0.75

# Block types
BLOCK_TYPES = {
    'dirt': {'color': BROWN, 'name': 'Dirt', 'hardness': 1.0},
    'stone': {'color': STONE_GRAY, 'name': 'Stone', 'hardness': 2.0},
    'gold': {'color': GOLD, 'name': 'Gold Ore', 'hardness': 3.0},
    'grass': {'color': GREEN, 'name': 'Grass', 'hardness': 1.0}
}

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
        self.vel_y += 0.2  # Gravity
        self.x += self.vel_x
        self.y += self.vel_y
        self.age += 1
    
    def draw_offset(self, screen, offset_x, offset_y):
        alpha = 1 - (self.age / self.lifetime)
        if alpha > 0:
            color = tuple(int(c * alpha) for c in self.color)
            pygame.draw.circle(screen, color, (int(self.x - offset_x), int(self.y - offset_y)), self.size)
    
    def is_dead(self):
        return self.age >= self.lifetime

class Hexagon:
    def __init__(self, x, y, size, block_type='dirt'):
        self.x = x
        self.y = y
        self.size = size
        self.block_type = block_type
        self.color = BLOCK_TYPES[block_type]['color']
        self.active_color = self.color
        self.hovered = False
        self.health = 100
        self.max_health = 100
        # Cache corners to save CPU cycles during draw
        self.corners = self._calculate_corners()
        self.dirty = False  # Track if we need to recalculate hover state
        
    def _calculate_corners(self):
        """Calculate the 6 corners of the hexagon (pointy-top orientation)"""
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
        
        # Check if within mining range of player (squared distance to avoid sqrt)
        pdx = player_x - self.x
        pdy = player_y - self.y
        player_distance_sq = pdx * pdx + pdy * pdy
        in_range = player_distance_sq < MINING_RANGE * MINING_RANGE
        
        hex_radius_sq = (self.size * 0.866) ** 2
        self.hovered = distance_sq < hex_radius_sq and in_range
        
        if self.hovered:
            self.active_color = tuple(min(c + 30, 255) for c in self.color)
        else:
            self.active_color = self.color
    
    def take_damage(self, amount):
        """Returns True if block is destroyed"""
        hardness = BLOCK_TYPES[self.block_type]['hardness']
        self.health -= amount / hardness
        return self.health <= 0
    
    def get_top_surface_y(self, x):
        """Get the Y coordinate of the top surface at a given X position"""
        left = self.corners[3]  # 210°
        top = self.corners[4]   # 270°
        right = self.corners[5] # 330°
        
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

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.radius = 15
        self.vel_x = 0
        self.vel_y = 0
        self.on_ground = False
        self.color = BLUE
        self.inventory = {'dirt': 0, 'stone': 0, 'gold': 0, 'grass': 0}
        
    def update(self, keys, hexagons):
        # Handle input
        if keys[pygame.K_a]:
            self.vel_x = -PLAYER_SPEED
        elif keys[pygame.K_d]:
            self.vel_x = PLAYER_SPEED
        else:
            self.vel_x *= FRICTION
            
        # Apply gravity
        self.vel_y += GRAVITY
        
        # Update position
        self.x += self.vel_x
        self.y += self.vel_y

        # World boundaries
        world_width = 5000
        world_height = 50 * HEX_V_SPACING + SCREEN_HEIGHT
        if self.x - self.radius < 0:
            self.x = self.radius
            self.vel_x = 0
        if self.x + self.radius > world_width:
            self.x = world_width - self.radius
            self.vel_x = 0
        if self.y + self.radius > world_height:
            self.y = world_height - self.radius
            self.vel_y = 0
            self.on_ground = True

        # Collision with hexagons - sort by distance to check closest first
        self.on_ground = False
        # Sort hexagons by distance to player
        hexagons_sorted = sorted(hexagons, key=lambda h: (h.x - self.x)**2 + (h.y - self.y)**2)
        for hexagon in hexagons_sorted:
            self.check_collision(hexagon)
    
    def check_collision(self, hexagon):
        """Check collision between player circle and hexagon"""
        dx = self.x - hexagon.x
        dy = self.y - hexagon.y
        distance_sq = dx * dx + dy * dy
        
        # Quick check using squared distance
        max_distance_sq = (self.radius + hexagon.size * 1.5) ** 2
        if distance_sq > max_distance_sq:
            return
        
        # Check top surface collision (landing on top)
        player_bottom = self.y + self.radius
        surface_y = hexagon.get_top_surface_y(self.x)
        
        if surface_y is not None:
            # More lenient collision detection
            if self.vel_y >= 0 and player_bottom >= surface_y - 10 and player_bottom <= surface_y + 15:
                self.y = surface_y - self.radius
                self.vel_y = 0
                self.on_ground = True
                return
        
        # Fallback: Check if player is inside hexagon bounds at all
        # This prevents sinking by catching any overlap
        if distance_sq < (self.radius + hexagon.size * 0.9) ** 2:
            # If we're above the hexagon center, push up
            if self.y < hexagon.y:
                overlap = (self.radius + hexagon.size * 0.9) - math.sqrt(distance_sq)
                if overlap > 0:
                    # Push player up
                    self.y -= overlap
                    self.vel_y = 0
                    self.on_ground = True
                    return
            
            # Side collision
            if abs(dy) < hexagon.size * 0.7:
                if dx > 0:
                    self.x = hexagon.x + hexagon.size * 0.95 + self.radius
                else:
                    self.x = hexagon.x - hexagon.size * 0.95 - self.radius
                self.vel_x = 0
    
    def jump(self):
        if self.on_ground:
            self.vel_y = JUMP_FORCE
            self.on_ground = False
    
    def add_to_inventory(self, block_type):
        self.inventory[block_type] += 1
    
    def remove_from_inventory(self, block_type):
        if self.inventory[block_type] > 0:
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
        pygame.draw.circle(screen, (255, 255, 255, 50), (screen_x, screen_y), MINING_RANGE, 1)

class Game:
    def __init__(self):
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Inhexit")
        self.clock = pygame.time.Clock()
        self.state = "MENU"
        self.running = True
        self.player = None
        self.hex_map = {}
        self.particles = []
        self.selected_block = 'dirt'
        self.mining = False
        self.camera_x = 0
        self.camera_y = 0
        # Cache sky gradient
        self.sky_surface = self._create_sky_gradient()
        
    def _create_sky_gradient(self):
        """Pre-render sky gradient for better performance"""
        surface = pygame.Surface((SCREEN_WIDTH, SCREEN_HEIGHT))
        for y in range(SCREEN_HEIGHT):
            # Create a nice sky gradient from light blue to darker blue
            ratio = y / SCREEN_HEIGHT
            color = (
                int(135 - ratio * 85),    # Red: 135 -> 50
                int(206 - ratio * 106),   # Green: 206 -> 100
                int(235 - ratio * 35)     # Blue: 235 -> 200
            )
            pygame.draw.line(surface, color, (0, y), (SCREEN_WIDTH, y))
        return surface

    def get_grid_coords(self, x, y):
        """Convert world coordinates to spatial hash grid coordinates"""
        grid_x = int(x // HEX_WIDTH)
        grid_y = int(y // HEX_V_SPACING)
        return (grid_x, grid_y)

    def generate_world(self):
        """Generate a world using the spatial hash map"""
        self.hex_map = {}

        ground_y = SCREEN_HEIGHT - 200
        rows = 2000
        cols = int(1000 / HEX_WIDTH) + 2

        for row in range(rows):
            for col in range(cols):
                x_offset = (HEX_WIDTH / 2) if row % 2 == 1 else 0
                x = col * HEX_WIDTH + x_offset - HEX_WIDTH / 2
                y = ground_y + row * HEX_V_SPACING

                if row == 0:
                    block_type = 'grass'
                elif row < 3:
                    block_type = 'dirt'
                elif row < 6:
                    block_type = 'stone'
                else:
                    block_type = 'gold' if random.random() < 0.2 else 'stone'

                hexagon = Hexagon(x, y, HEX_SIZE, block_type)
                self.hex_map[(col, row)] = hexagon

    def save_world(self):
        data = {
            'hex_map': self.hex_map,
            'player_x': self.player.x if self.player else 0,
            'player_y': self.player.y if self.player else 0,
            'inventory': self.player.inventory if self.player else {},
            'selected_block': self.selected_block
        }
        with open('world.pkl', 'wb') as f:
            pickle.dump(data, f)

    def load_world(self):
        try:
            with open('world.pkl', 'rb') as f:
                data = pickle.load(f)
            if not isinstance(data, dict) or 'hex_map' not in data:
                return None
            self.hex_map = data['hex_map']
            self.selected_block = data.get('selected_block', 'dirt')
            return data
        except (FileNotFoundError, EOFError, pickle.UnpicklingError):
            return None

    def start_game(self):
        data = self.load_world()
        if data:
            self.player = Player(data['player_x'], data['player_y'])
            self.player.inventory = data['inventory']
            self.selected_block = data['selected_block']
        else:
            self.generate_world()
            # Spawn player above the ground
            spawn_y = SCREEN_HEIGHT - 200 - 100  # 100 pixels above ground
            self.player = Player(SCREEN_WIDTH // 2, spawn_y)
        self.particles = []
        self.state = "GAME"
    
    def handle_menu_input(self, event):
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                self.start_game()
            elif event.key == pygame.K_ESCAPE:
                self.running = False
    
    def handle_game_input(self, event):
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                self.player.jump()
            elif event.key == pygame.K_ESCAPE:
                self.save_world()
                self.state = "MENU"
            elif event.key == pygame.K_1:
                self.selected_block = 'dirt'
            elif event.key == pygame.K_2:
                self.selected_block = 'stone'
            elif event.key == pygame.K_3:
                self.selected_block = 'gold'
            elif event.key == pygame.K_4:
                self.selected_block = 'grass'
        
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                self.mining = True
            elif event.button == 3:
                self.place_block()
        
        if event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1:
                self.mining = False
    
    def mine_block(self):
        mouse_pos = pygame.mouse.get_pos()
        world_mx = mouse_pos[0] + self.camera_x
        world_my = mouse_pos[1] + self.camera_y
        
        nearby = self.get_nearby_hexagons(world_mx, world_my, radius=HEX_SIZE * 2)
        
        for hexagon in nearby:
            if hexagon.hovered:
                if hexagon.take_damage(5):
                    for _ in range(10):
                        particle = Particle(hexagon.x, hexagon.y, hexagon.color)
                        self.particles.append(particle)
                    
                    self.player.add_to_inventory(hexagon.block_type)
                    coords_to_remove = None
                    for coords, h in self.hex_map.items():
                        if h == hexagon:
                            coords_to_remove = coords
                            break
                    if coords_to_remove:
                        del self.hex_map[coords_to_remove]
                break
    
    def place_block(self):
        mouse_pos = pygame.mouse.get_pos()
        mx, my = mouse_pos
        world_mx = mx + self.camera_x
        world_my = my + self.camera_y
        
        # Check if within range (use squared distance)
        dx = self.player.x - world_mx
        dy = self.player.y - world_my
        if dx * dx + dy * dy > MINING_RANGE * MINING_RANGE:
            return
        
        if not self.player.remove_from_inventory(self.selected_block):
            return
        
        nearby = self.get_nearby_hexagons(world_mx, world_my, radius=HEX_SIZE)
        if nearby:
            self.player.add_to_inventory(self.selected_block)
            return
        
        # Check overlap with player (squared distance)
        dx = self.player.x - world_mx
        dy = self.player.y - world_my
        min_dist_sq = (HEX_SIZE + self.player.radius) ** 2
        if dx * dx + dy * dy < min_dist_sq:
            self.player.add_to_inventory(self.selected_block)
            return
        
        new_hex = Hexagon(world_mx, world_my, HEX_SIZE, self.selected_block)
        self.hex_map[f"custom_{random.randint(0, 1000000)}"] = new_hex
    
    def get_nearby_hexagons(self, center_x, center_y, radius=200):
        """Fast lookup using spatial hash grid"""
        nearby = []
        
        min_gx, min_gy = self.get_grid_coords(center_x - radius, center_y - radius)
        max_gx, max_gy = self.get_grid_coords(center_x + radius, center_y + radius)
        
        # Expand search range to ensure we don't miss hexagons
        for gy in range(min_gy - 1, max_gy + 2):
            for gx in range(min_gx - 1, max_gx + 2):
                key = (gx, gy)
                if key in self.hex_map:
                    h = self.hex_map[key]
                    # Double-check actual distance
                    if abs(h.x - center_x) <= radius + HEX_SIZE and abs(h.y - center_y) <= radius + HEX_SIZE:
                        nearby.append(h)
        
        for key, h in self.hex_map.items():
            if isinstance(key, str) and key.startswith("custom_"):
                if abs(h.x - center_x) <= radius + HEX_SIZE and abs(h.y - center_y) <= radius + HEX_SIZE:
                    nearby.append(h)
                    
        return nearby

    def update_game(self):
        keys = pygame.key.get_pressed()
        
        # Update camera
        self.camera_x = self.player.x - SCREEN_WIDTH // 2
        self.camera_y = self.player.y - SCREEN_HEIGHT // 2
        
        world_width = 5000
        world_height = 50 * HEX_V_SPACING + SCREEN_HEIGHT
        self.camera_x = max(0, min(self.camera_x, world_width - SCREEN_WIDTH))
        self.camera_y = max(0, min(self.camera_y, world_height - SCREEN_HEIGHT))

        # Only process hexagons near the player for physics (increased radius significantly)
        nearby_hexagons = self.get_nearby_hexagons(self.player.x, self.player.y, radius=500)
        self.player.update(keys, nearby_hexagons)

        # Update hover states for blocks near mouse
        mouse_pos = pygame.mouse.get_pos()
        world_mx = mouse_pos[0] + self.camera_x
        world_my = mouse_pos[1] + self.camera_y
        hover_nearby = self.get_nearby_hexagons(world_mx, world_my, radius=HEX_SIZE * 2)
        player_screen_x = self.player.x - self.camera_x
        player_screen_y = self.player.y - self.camera_y
        for hexagon in hover_nearby:
            hexagon.check_hover(world_mx, world_my, self.player.x, self.player.y)

        if self.mining:
            self.mine_block()

        # Update particles (batch removal)
        self.particles = [p for p in self.particles if not (p.update() or p.is_dead())]
    
    def draw_menu(self):
        self.screen.fill(BLACK)
        
        title_font = pygame.font.Font(None, 74)
        title = title_font.render("INHEXIT", True, WHITE)
        title_rect = title.get_rect(center=(SCREEN_WIDTH // 2, SCREEN_HEIGHT // 3))
        self.screen.blit(title, title_rect)
        
        font = pygame.font.Font(None, 36)
        instructions = [
            "Press SPACE to Start",
            "",
            "Controls:",
            "A / D - Move Left / Right",
            "SPACE - Jump",
            "Left Click - Mine Blocks",
            "Right Click - Place Blocks",
            "1/2/3/4 - Select Block Type",
            "ESC - Back to Menu"
        ]
        
        y_offset = SCREEN_HEIGHT // 2
        for instruction in instructions:
            text = font.render(instruction, True, WHITE)
            text_rect = text.get_rect(center=(SCREEN_WIDTH // 2, y_offset))
            self.screen.blit(text, text_rect)
            y_offset += 35
    
    def draw_game(self):
        # Use pre-rendered sky gradient
        self.screen.blit(self.sky_surface, (0, 0))

        # Draw hexagons
        viewport_center_x = self.camera_x + SCREEN_WIDTH // 2
        viewport_center_y = self.camera_y + SCREEN_HEIGHT // 2
        visible_hexagons = self.get_nearby_hexagons(viewport_center_x, viewport_center_y, radius=SCREEN_WIDTH // 2 + HEX_SIZE * 2)

        for hexagon in visible_hexagons:
            offset_corners = [(cx - self.camera_x, cy - self.camera_y) for cx, cy in hexagon.corners]
            pygame.draw.polygon(self.screen, hexagon.active_color, offset_corners)
            pygame.draw.polygon(self.screen, DARK_GRAY, offset_corners, 2)
            
            if hexagon.health < hexagon.max_health:
                health_ratio = hexagon.health / hexagon.max_health
                crack_color = (255, int(255 * health_ratio), int(255 * health_ratio))
                pygame.draw.polygon(self.screen, crack_color, offset_corners, 3)

        # Draw particles with culling
        for particle in self.particles:
            px = particle.x - self.camera_x
            py = particle.y - self.camera_y
            if -10 < px < SCREEN_WIDTH + 10 and -10 < py < SCREEN_HEIGHT + 10:
                particle.draw_offset(self.screen, self.camera_x, self.camera_y)

        self.player.draw_offset(self.screen, self.camera_x, self.camera_y)
        self.draw_arrow()
        self.draw_ui()
    
    def draw_arrow(self):
        """Draw an arrow from player to mouse cursor"""
        mouse_pos = pygame.mouse.get_pos()
        mx, my = mouse_pos
        world_mx = mx + self.camera_x
        world_my = my + self.camera_y
        px, py = self.player.x, self.player.y

        dx = world_mx - px
        dy = world_my - py
        distance_sq = dx * dx + dy * dy

        if distance_sq > MINING_RANGE * MINING_RANGE:
            return

        distance = math.sqrt(distance_sq)
        if distance == 0:
            return

        dx_norm = dx / distance
        dy_norm = dy / distance

        arrow_length = min(distance - self.player.radius - 5, MINING_RANGE - self.player.radius - 5)
        if arrow_length < 10:
            return

        start_x = px + dx_norm * (self.player.radius + 5)
        start_y = py + dy_norm * (self.player.radius + 5)
        end_x = start_x + dx_norm * arrow_length
        end_y = start_y + dy_norm * arrow_length

        screen_start_x = start_x - self.camera_x
        screen_start_y = start_y - self.camera_y
        screen_end_x = end_x - self.camera_x
        screen_end_y = end_y - self.camera_y

        arrow_color = (255, 255, 0)
        pygame.draw.line(self.screen, arrow_color, (screen_start_x, screen_start_y), (screen_end_x, screen_end_y), 3)

        arrowhead_size = 10
        angle = math.atan2(dy, dx)
        arrowhead_x1 = end_x - arrowhead_size * math.cos(angle - math.pi / 6)
        arrowhead_y1 = end_y - arrowhead_size * math.sin(angle - math.pi / 6)
        arrowhead_x2 = end_x - arrowhead_size * math.cos(angle + math.pi / 6)
        arrowhead_y2 = end_y - arrowhead_size * math.sin(angle + math.pi / 6)

        screen_arrowhead_points = [
            (end_x - self.camera_x, end_y - self.camera_y),
            (arrowhead_x1 - self.camera_x, arrowhead_y1 - self.camera_y),
            (arrowhead_x2 - self.camera_x, arrowhead_y2 - self.camera_y)
        ]
        pygame.draw.polygon(self.screen, arrow_color, screen_arrowhead_points)
    
    def draw_ui(self):
        font = pygame.font.Font(None, 28)
        
        y_pos = 10
        inventory_title = font.render("Inventory:", True, WHITE)
        self.screen.blit(inventory_title, (10, y_pos))
        y_pos += 30
        
        for i, (block_type, count) in enumerate(self.player.inventory.items(), 1):
            color = BLOCK_TYPES[block_type]['color']
            selected = " <--" if block_type == self.selected_block else ""
            text = font.render(f"{i}. {BLOCK_TYPES[block_type]['name']}: {count}{selected}", True, WHITE)
            
            pygame.draw.rect(self.screen, color, (10, y_pos, 20, 20))
            pygame.draw.rect(self.screen, WHITE, (10, y_pos, 20, 20), 1)
            
            self.screen.blit(text, (35, y_pos))
            y_pos += 25
    
    def run(self):
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.save_world()
                    self.running = False

                if self.state == "MENU":
                    self.handle_menu_input(event)
                elif self.state == "GAME":
                    self.handle_game_input(event)

            if self.state == "MENU":
                self.draw_menu()
            elif self.state == "GAME":
                self.update_game()
                self.draw_game()

            pygame.display.flip()
            self.clock.tick(FPS)

        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    game = Game()
    game.run()