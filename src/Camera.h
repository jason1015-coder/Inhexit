/*
 * Camera Class
 * Handles camera movement and following the player
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <SFML/Graphics.hpp>
#include "Player.h"

class Camera {
private:
    sf::View view;
    float smoothFactor;
    
public:
    Camera(float width, float height);
    
    // Camera update
    void update(const Player& player);
    
    // Get the view
    const sf::View& getView() const { return view; }
    
    // Camera controls
    void zoom(float factor);
    void setPosition(const sf::Vector2f& position);
};

#endif // CAMERA_H