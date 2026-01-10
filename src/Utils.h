 /*
 * Utility Functions and Constants - Terraria Style
 * Contains helper functions for hexagon math and game constants
 */

#ifndef UTILS_H
#define UTILS_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>

// Game Constants (Tuned for Side-Scrolling Platforming)
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int HEX_SIZE = 24;          // Slightly smaller hexes for a larger "world" feel
const float GRAVITY = 0.6f;       // Stronger gravity for snappy falling
const float JUMP_FORCE = -14.0f;  // Higher jump to clear 2-3 hexes
const float MOVE_SPEED = 6.0f;    // Faster walking
const float FRICTION = 0.82f;     // Slight slide for organic movement

// World Constants
const int WORLD_WIDTH = 500;      // INCREASED: Much wider world (was 200)
const int WORLD_HEIGHT = 300;     // INCREASED: Much deeper world (was 100)

// Block Types (Terraria Essentials)
enum BlockType {
    AIR = 0,
    DIRT,
    STONE,
    GRASS,
    WOOD,
    LEAVES,
    WATER,
    SAND,
    COAL,    // Added for variety
    IRON,    // Added for variety
    SNOW     // Added for winter biome
};

struct HexCoord {
    int q;
    int r;
    
    HexCoord(int q = 0, int r = 0) : q(q), r(r) {}
    
    bool operator==(const HexCoord& other) const {
        return q == other.q && r == other.r;
    }

    // Convert Axial Hex to Pixel coordinates
    sf::Vector2f toPixel(float hexSize) const {
        float x = hexSize * 3.0f / 2.0f * q;
        float y = hexSize * (std::sqrt(3.0f) / 2.0f * q + std::sqrt(3.0f) * r);
        return sf::Vector2f(x, y);
    }
    
    // Convert Pixel coordinates to Axial Hex
    static HexCoord fromPixel(float x, float y, float hexSize) {
        float q = (2.0f / 3.0f * x) / hexSize;
        float r = (-1.0f / 3.0f * x + std::sqrt(3.0f) / 3.0f * y) / hexSize;
        return HexCoord::round(q, r);
    }
    
    // Round fractional hex coordinates to nearest integer hex
    static HexCoord round(float q, float r) {
        float s = -q - r;
        float rq = std::round(q);
        float rr = std::round(r);
        float rs = std::round(s);
        
        float q_diff = std::abs(rq - q);
        float r_diff = std::abs(rr - r);
        float s_diff = std::abs(rs - s);
        
        if (q_diff > r_diff && q_diff > s_diff) {
            rq = -rr - rs;
        } else if (r_diff > s_diff) {
            rr = -rq - rs;
        }
        
        return HexCoord(static_cast<int>(rq), static_cast<int>(rr));
    }
};

// Global Helper Functions
std::vector<sf::Vector2f> getHexagonVertices(float hexSize, const sf::Vector2f& center);
sf::Color getBlockColor(BlockType type);
sf::Color hslToRgb(float h, float s, float l);

// Biome Types
enum class BiomeType {
    NORMAL,
    DESERT
};

// Weather Types
enum class WeatherType {
    CLEAR,
    RAIN,
    SNOW,
    HAIL,
    BLIZZARD,
    CLOUDY
};

// Season Types
enum class Season {
    SPRING,
    SUMMER,
    AUTUMN,
    WINTER
};

#endif // UTILS_H
