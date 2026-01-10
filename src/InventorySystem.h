/*
 * Inventory System
 * Handles backpack slots, hand slots, and item management
 */

#ifndef INVENTORY_SYSTEM_H
#define INVENTORY_SYSTEM_H

#include "Utils.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

// Item types (extends BlockType for usability)
enum class ItemType {
    NONE = 0,
    DIRT = static_cast<int>(BlockType::DIRT),
    STONE = static_cast<int>(BlockType::STONE),
    GRASS = static_cast<int>(BlockType::GRASS),
    WOOD = static_cast<int>(BlockType::WOOD),
    LEAVES = static_cast<int>(BlockType::LEAVES),
    SAND = static_cast<int>(BlockType::SAND),
    COAL = static_cast<int>(BlockType::COAL),
    IRON = static_cast<int>(BlockType::IRON),
    SNOW = static_cast<int>(BlockType::SNOW)
};

// Inventory slot
struct InventorySlot {
    ItemType item;
    int quantity;
    sf::Vector2f position;  // For rendering
    bool isHandSlot;        // True for hand slots, false for backpack
    
    InventorySlot() : item(ItemType::NONE), quantity(0), position(0, 0), isHandSlot(false) {}
    InventorySlot(ItemType it, int qty, sf::Vector2f pos, bool hand) 
        : item(it), quantity(qty), position(pos), isHandSlot(hand) {}
};

class InventorySystem {
private:
    static const int LEFT_HAND_SLOTS = 3;
    static const int RIGHT_HAND_SLOTS = 3;
    static const int BACKPACK_SLOTS = 30;
    
    std::vector<InventorySlot> slots;
    
    // Currently selected hand slots
    int leftHandSelected;  // 0-2 (keys 1, 2, 3)
    int rightHandSelected; // 0-2 (keys 4, 5, 6)
    
    // UI state
    bool isBackpackOpen;
    
    // Helper functions
    int findEmptySlot(bool preferHandSlots = false);
    int findSlotWithItem(ItemType item, bool preferHandSlots = false);
    sf::Vector2f calculateSlotPosition(int index, bool isBackpack, const sf::Vector2f& basePos);
    
public:
    InventorySystem();
    
    // Slot management
    bool addItem(ItemType item, int quantity = 1);
    bool removeItem(ItemType item, int quantity = 1);
    int getItemCount(ItemType item) const;
    
    // Slot selection
    void selectLeftHandSlot(int index);
    void selectRightHandSlot(int index);
    void toggleBackpack();
    
    // Drag and drop
    bool moveItem(int fromSlot, int toSlot);
    bool dropItem(int slotIndex, std::vector<InventorySlot>& droppedItems);
    
    // Getters
    InventorySlot& getSlot(int index);
    const InventorySlot& getSlot(int index) const;
    int getLeftHandSelected() const { return leftHandSelected; }
    int getRightHandSelected() const { return rightHandSelected; }
    bool isBackpackOpenUI() const { return isBackpackOpen; }
    int getSlotCount() const { return slots.size(); }
    
    // Get currently selected items
    ItemType getLeftHandItem() const;
    ItemType getRightHandItem() const;
    
    // Rendering
    void render(sf::RenderWindow& window) const;
    
    // Save/Load
    void saveState(const std::string& filename) const;
    void loadState(const std::string& filename);
};

#endif // INVENTORY_SYSTEM_H