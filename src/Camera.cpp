/*
 * Camera Implementation
 */

#include "Camera.h"

Camera::Camera(float width, float height) {
    view.setSize(width, height);
    view.setCenter(width / 2, height / 2);
    smoothFactor = 0.15f;  // Smooth following factor
}

void Camera::update(const Player& player) {
    sf::Vector2f playerPos = player.getPosition();
    sf::Vector2f currentCenter = view.getCenter();
    
    // Side-scrolling camera (Terraria style):
    // Follow player horizontally smoothly
    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (playerPos.x - currentCenter.x) * smoothFactor;
    
    // Follow player vertically but keep camera slightly above player for better view
    // This creates a more natural side-scrolling feel
    float targetY = playerPos.y - view.getSize().y * 0.25f;  // Keep player in upper portion of screen
    newCenter.y = currentCenter.y + (targetY - currentCenter.y) * smoothFactor * 0.5f;
    
    view.setCenter(newCenter);
}

void Camera::zoom(float factor) {
    view.zoom(factor);
}

void Camera::setPosition(const sf::Vector2f& position) {
    view.setCenter(position);
}