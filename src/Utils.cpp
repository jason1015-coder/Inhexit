/*
 * Utility Functions Implementation
 * Handles the visual colors and math for the hexagons
 */

#include "Utils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<sf::Vector2f> getHexagonVertices(float hexSize, const sf::Vector2f& center) {
    std::vector<sf::Vector2f> vertices;
    for (int i = 0; i < 6; i++) {
        // Offset by -30 degrees for flat-top orientation
        float angle_deg = 60.0f * i - 30.0f;
        float angle_rad = M_PI / 180.0f * angle_deg;
        vertices.push_back(sf::Vector2f(
            center.x + hexSize * cos(angle_rad),
            center.y + hexSize * sin(angle_rad)
        ));
    }
    return vertices;
}

sf::Color getBlockColor(BlockType type) {
    switch (type) {
        case AIR:    return sf::Color(0, 0, 0, 0);       // Transparent
        case GRASS:  return sf::Color(90, 170, 70);      // Grass Green
        case DIRT:   return sf::Color(130, 90, 60);      // Dirt Brown
        case STONE:  return sf::Color(120, 120, 130);    // Stone Gray
        case WOOD:   return sf::Color(100, 70, 40);      // Wood
        case SAND:   return sf::Color(240, 230, 150);    // Light Desert Yellow
        case WATER:  return sf::Color(50, 120, 200, 180); // Blueish water
        case LEAVES: return sf::Color(40, 130, 40);      // Leaf Green
        case COAL:   return sf::Color(40, 40, 45);       // Dark Coal
        case IRON:   return sf::Color(180, 160, 150);    // Metallic Tan
        default:     return sf::Color::Magenta;          // Error color
    }
}

sf::Color hslToRgb(float h, float s, float l) {
    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    
    float r, g, b;
    
    if (h < 60.0f) {
        r = c; g = x; b = 0;
    } else if (h < 120.0f) {
        r = x; g = c; b = 0;
    } else if (h < 180.0f) {
        r = 0; g = c; b = x;
    } else if (h < 240.0f) {
        r = 0; g = x; b = c;
    } else if (h < 300.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return sf::Color(
        static_cast<sf::Uint8>((r + m) * 255),
        static_cast<sf::Uint8>((g + m) * 255),
        static_cast<sf::Uint8>((b + m) * 255)
    );
}
