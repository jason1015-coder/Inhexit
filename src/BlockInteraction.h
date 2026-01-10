/*
 * Block Interaction System
 * Handles block destruction, placement, and visual feedback
 */

#ifndef BLOCK_INTERACTION_H
#define BLOCK_INTERACTION_H

#include "Utils.h"
#include <map>
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

class World;

// Interaction states
enum class InteractionState {
    NONE,
    MINING,
    PLACING
};

// Block breaking visual effect
struct BlockBreakEffect {
    HexCoord position;
    BlockType blockType;
    float progress; // 0.0 to 1.0
    float lifetime;
    sf::Vector2f velocity;
};

// Dropped item
struct DroppedItem {
    HexCoord position;
    BlockType itemType;
    int quantity;
    sf::Vector2f velocity;
    float lifetime;
    bool onGround;
};

class BlockInteractionSystem {
private:
    std::vector<BlockBreakEffect> breakEffects;
    std::vector<DroppedItem> droppedItems;
    
    // Mining parameters
    float miningSpeed;
    float miningProgress;
    HexCoord currentMiningTarget;
    BlockType currentMiningBlock;
    BlockType selectedBlockType;
    
    // Visual assets
    sf::Texture blockTexture;
    sf::Texture itemTexture;
    
public:
    BlockInteractionSystem();
    
    // Block operations
    bool startMining(const HexCoord& coord, BlockType blockType);
    bool updateMining(float deltaTime);
    void finishMining();
    void cancelMining();
    
    // Block placement
    bool placeBlock(const HexCoord& coord, BlockType type);
    
    // Dropped items
    void dropItem(const HexCoord& coord, BlockType type, int quantity = 1);
    void pickUpItem(const HexCoord& playerPos);
    
    // Update and render
    void update(float deltaTime, World& world);
    void render(sf::RenderWindow& window, const sf::View& view);
    
    // Getters
    float getMiningProgress() const { return miningProgress; }
    bool isMining() const { return miningProgress > 0; }
    HexCoord getMiningTarget() const { return currentMiningTarget; }
    BlockType getSelectedBlockType() const { return selectedBlockType; }
    
    // Setters
    void setSelectedBlockType(BlockType type) { selectedBlockType = type; }
    void stopMining() { cancelMining(); }
    
    // Inventory management
    void addToInventory(BlockType type, int quantity);
    int getInventoryCount(BlockType type) const;
    bool removeFromInventory(BlockType type, int quantity);
    
private:
    std::map<BlockType, int> inventory;
    
    // Helper functions
    void createBreakEffect(const HexCoord& coord, BlockType type);
    void updateBreakEffects(float deltaTime);
    void updateDroppedItems(float deltaTime, World& world);
    void renderBreakEffects(sf::RenderWindow& window, const sf::View& view);
    void renderDroppedItems(sf::RenderWindow& window, const sf::View& view);
};

#endif // BLOCK_INTERACTION_H
