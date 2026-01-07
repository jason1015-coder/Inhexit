/*
 * Player Implementation - Enhanced with Block Interaction
 */

#include "Player.h"
#include <cmath>

Player::Player(float x, float y) 
    : position(x, y), velocity(0, 0), 
      size(HEX_SIZE * 0.9f, HEX_SIZE * 1.6f),  // About one block size
      isOnGround(false), movingLeft(false), movingRight(false),
      selectedBlock(BlockType::DIRT) {
    
    shape.setRadius(size.x / 2);
    shape.setFillColor(sf::Color(255, 100, 100));
    
    // Initialize block interaction system
    blockInteraction = std::make_shared<BlockInteractionSystem>();
}

void Player::moveLeft() {
    movingLeft = true;
    velocity.x -= MOVE_SPEED * 0.1f;
    if (velocity.x < -MOVE_SPEED) velocity.x = -MOVE_SPEED;
}

void Player::moveRight() {
    movingRight = true;
    velocity.x += MOVE_SPEED * 0.1f;
    if (velocity.x > MOVE_SPEED) velocity.x = MOVE_SPEED;
}

void Player::stopMoving() {
    movingLeft = false;
    movingRight = false;
}

void Player::jump() {
    if (isOnGround) {
        velocity.y = JUMP_FORCE;
        isOnGround = false;
    }
}

HexCoord Player::getHexPosition() const {
    return HexCoord::fromPixel(position.x, position.y + size.y / 2, HEX_SIZE);
}

void Player::update(float deltaTime, World& world) {
    // Apply gravity
    velocity.y += GRAVITY * deltaTime * 60.0f;
    
    // Apply friction
    velocity.x *= FRICTION;
    
    // Update position
    sf::Vector2f newPos = position;
    newPos.x += velocity.x * deltaTime * 60.0f;
    
    // Horizontal collision with slope walking
    const float STEP_HEIGHT = HEX_SIZE * std::sqrt(3.0f);  // Height of one hex
    const float MAX_STEP = STEP_HEIGHT * 1.2f;  // Allow stepping up one block
    
    if (checkCollision(world, newPos)) {
        // Try stepping up (slope walking)
        bool canStepUp = false;
        for (float stepY = STEP_HEIGHT * 0.25f; stepY <= MAX_STEP; stepY += STEP_HEIGHT * 0.25f) {
            sf::Vector2f stepPos = newPos;
            stepPos.y -= stepY;
            
            if (!checkCollision(world, stepPos)) {
                newPos = stepPos;
                canStepUp = true;
                isOnGround = true;
                velocity.y = 0;  // Cancel falling when stepping up
                break;
            }
        }
        
        if (!canStepUp) {
            velocity.x = 0;
            newPos.x = position.x;
        }
    }
    
    // Apply vertical velocity
    newPos.y += velocity.y * deltaTime * 60.0f;
    
    // Vertical collision
    if (checkCollision(world, newPos)) {
        if (velocity.y > 0) {
            isOnGround = true;
            // Snap to surface
            while (checkCollision(world, newPos) && newPos.y > position.y - HEX_SIZE * 2) {
                newPos.y -= 0.5f;
            }
            newPos.y += 0.5f;
        }
        velocity.y = 0;
    } else {
        // Check if player should step down (descending slopes)
        if (isOnGround && velocity.y >= 0) {
            sf::Vector2f stepDownPos = newPos;
            stepDownPos.y += STEP_HEIGHT * 0.5f;  // Check half block down
            
            if (checkCollision(world, stepDownPos)) {
                // There's ground below, step down smoothly
                while (!checkCollision(world, newPos) && newPos.y < stepDownPos.y) {
                    newPos.y += 1.0f;
                }
                newPos.y -= 1.0f;
                isOnGround = true;
                velocity.y = 0;
            } else {
                isOnGround = false;
            }
        } else {
            isOnGround = false;
        }
    }
    
    position = newPos;
    
    // Update shape position
    shape.setPosition(position.x - size.x / 2, position.y - size.y / 2);
    
    // Update block interaction system
    blockInteraction->update(deltaTime, world);
    
    // Check for item pickup
    blockInteraction->pickUpItem(getHexPosition());
}

bool Player::checkCollision(const World& world, const sf::Vector2f& newPos) {
    // Check multiple points around the player for collision
    std::vector<sf::Vector2f> checkPoints = {
        newPos + sf::Vector2f(-size.x / 2, 0),
        newPos + sf::Vector2f(size.x / 2, 0),
        newPos + sf::Vector2f(-size.x / 2, size.y / 2),
        newPos + sf::Vector2f(size.x / 2, size.y / 2),
        newPos + sf::Vector2f(0, size.y / 2),
        newPos + sf::Vector2f(0, -size.y / 2)
    };
    
    for (const auto& point : checkPoints) {
        HexCoord coord = HexCoord::fromPixel(point.x, point.y, HEX_SIZE);
        if (isSolidBlock(world, coord)) {
            return true;
        }
    }
    
    return false;
}

bool Player::isSolidBlock(const World& world, const HexCoord& coord) {
    BlockType type = world.getBlock(coord);
    return type != BlockType::AIR && type != BlockType::WATER;
}

void Player::render(sf::RenderWindow& window) {
    window.draw(shape);
    
    // Render block interaction effects
    blockInteraction->render(window, window.getView());
}

void Player::startMining(const HexCoord& coord, BlockType blockType) {
    blockInteraction->startMining(coord, blockType);
}

void Player::cancelMining() {
    blockInteraction->cancelMining();
}

void Player::placeBlock(const HexCoord& coord) {
    blockInteraction->placeBlock(coord, selectedBlock);
}

void Player::selectBlock(BlockType type) {
    selectedBlock = type;
}