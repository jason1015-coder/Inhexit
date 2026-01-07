/*
 * Block Interaction System Implementation
 * Handles block destruction, placement, and visual feedback
 */

#include "BlockInteraction.h"
#include "World.h"
#include <iostream>
#include <cmath>

BlockInteractionSystem::BlockInteractionSystem() 
    : miningSpeed(1.0f), miningProgress(0.0f), 
      currentMiningBlock(BlockType::AIR), selectedBlockType(BlockType::DIRT) {
}
    
bool BlockInteractionSystem::startMining(const HexCoord& coord, BlockType blockType) {
    currentMiningTarget = coord;
    currentMiningBlock = blockType;
    miningProgress = 0.0f;
    return true;
}

bool BlockInteractionSystem::updateMining(float deltaTime) {
    if (currentMiningBlock == BlockType::AIR) {
        return false;
    }
    
    // Increase mining progress
    miningProgress += miningSpeed * deltaTime;
    
    // Mining takes 1 second
    if (miningProgress >= 1.0f) {
        finishMining();
        return true;
    }
    
    return false;
}

void BlockInteractionSystem::finishMining() {
    if (currentMiningBlock != BlockType::AIR) {
        dropItem(currentMiningTarget, currentMiningBlock, 1);
    }
    cancelMining();
}

void BlockInteractionSystem::cancelMining() {
    currentMiningBlock = BlockType::AIR;
    miningProgress = 0.0f;
}

bool BlockInteractionSystem::placeBlock(const HexCoord& /*coord*/, BlockType type) {
    if (removeFromInventory(type, 1)) {
        // Block placement would happen here
        return true;
    }
    return false;
}

void BlockInteractionSystem::dropItem(const HexCoord& coord, BlockType type, int quantity) {
    DroppedItem item;
    item.position = coord;
    item.itemType = type;
    item.quantity = quantity;
    item.velocity = sf::Vector2f(0.0f, 0.0f);
    item.onGround = false;
    item.lifetime = 5.0f; // 5 seconds
    
    droppedItems.push_back(item);
}

void BlockInteractionSystem::pickUpItem(const HexCoord& playerPos) {
    // Simple implementation: pick up items near player
    for (auto it = droppedItems.begin(); it != droppedItems.end(); ) {
        int dq = it->position.q - playerPos.q;
        int dr = it->position.r - playerPos.r;
        int distance = dq * dq + dr * dr;
        
        if (distance < 4) {  // Within 2 hex tiles
            addToInventory(it->itemType, it->quantity);
            it = droppedItems.erase(it);
        } else {
            ++it;
        }
    }
}

void BlockInteractionSystem::update(float deltaTime, World& /*world*/) {
    // Update mining
    updateMining(deltaTime);
    
    // Update dropped items
    for (auto it = droppedItems.begin(); it != droppedItems.end(); ) {
        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f) {
            it = droppedItems.erase(it);
        } else {
            ++it;
        }
    }
}

void BlockInteractionSystem::render(sf::RenderWindow& /*window*/, const sf::View& /*view*/) {
    // Render dropped items - simple implementation
    // In a real game, we'd render items at their hex positions
}

void BlockInteractionSystem::addToInventory(BlockType type, int quantity) {
    inventory[type] += quantity;
}

int BlockInteractionSystem::getInventoryCount(BlockType type) const {
    auto it = inventory.find(type);
    if (it != inventory.end()) {
        return it->second;
    }
    return 0;
}

bool BlockInteractionSystem::removeFromInventory(BlockType type, int quantity) {
    if (getInventoryCount(type) >= quantity) {
        inventory[type] -= quantity;
        return true;
    }
    return false;
}

void BlockInteractionSystem::createBreakEffect(const HexCoord& /*coord*/, BlockType /*type*/) {
    // Could add visual effects here
}