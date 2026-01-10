/*
 * Player Class - Enhanced with Inventory and Block Interaction
 * Handles player movement, physics, collision detection, and block interaction
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include "Utils.h"
#include "World.h"
#include "BlockInteraction.h"

class Player {
private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f size;
    sf::CircleShape shape;
    
    bool isOnGround;
    bool movingLeft;
    bool movingRight;
    
    // Selected block for placement
    BlockType selectedBlock;
    
    // Block interaction
    std::shared_ptr<BlockInteractionSystem> blockInteraction;
    
    // Spawn position for respawn
    sf::Vector2f spawnPosition;
    
    // Player skin color
    sf::Color playerColor;
    
    // Collision detection helper
    bool checkCollision(const World& world, const sf::Vector2f& newPos);
    bool isSolidBlock(const World& world, const HexCoord& coord);
    
    // Helper to get player's hex position
    HexCoord getHexPosition() const;
    
public:
    Player(float x, float y);
    
    // Movement methods
    void moveLeft();
    void moveRight();
    void stopMoving();
    void jump();
    
    // Physics update
    void update(float deltaTime, World& world);
    
    // Rendering
    void render(sf::RenderWindow& window);
    
    // Block interaction
    void startMining(const HexCoord& coord, BlockType blockType);
    void cancelMining();
    void placeBlock(const HexCoord& coord);
    void selectBlock(BlockType type);
    BlockType getSelectedBlock() const { return selectedBlock; }
    
    // Getters
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    bool getIsOnGround() const { return isOnGround; }
    sf::Color getPlayerColor() const { return playerColor; }
    sf::Vector2f getSpawnPosition() const { return spawnPosition; }
    
    // Setters
    void setPosition(float x, float y) { position.x = x; position.y = y; }
    void setPlayerColor(const sf::Color& color) { playerColor = color; shape.setFillColor(color); }
    void setSpawnPosition(float x, float y) { spawnPosition = sf::Vector2f(x, y); }
    
    // Respawn
    void respawn();
    
    // Inventory access
    std::shared_ptr<BlockInteractionSystem> getBlockInteraction() { return blockInteraction; }
};

#endif // PLAYER_H