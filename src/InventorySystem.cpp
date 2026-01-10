/*
 * Inventory System Implementation
 */

#include "InventorySystem.h"
#include <fstream>
#include <cmath>

InventorySystem::InventorySystem() 
    : leftHandSelected(0), rightHandSelected(0), isBackpackOpen(false) {
    // Initialize hand slots (3 left, 3 right)
    float startX = 100.0f;
    float handY = 600.0f;
    
    // Left hand slots (keys 1, 2, 3)
    for (int i = 0; i < LEFT_HAND_SLOTS; i++) {
        slots.push_back(InventorySlot(ItemType::NONE, 0, 
            sf::Vector2f(startX + i * 60.0f, handY), true));
    }
    
    // Right hand slots (keys 4, 5, 6)
    for (int i = 0; i < RIGHT_HAND_SLOTS; i++) {
        slots.push_back(InventorySlot(ItemType::NONE, 0, 
            sf::Vector2f(startX + 300.0f + i * 60.0f, handY), true));
    }
    
    // Backpack slots (30 hidden slots)
    float backpackStartX = 200.0f;
    float backpackStartY = 150.0f;
    int cols = 10;
    
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        int row = i / cols;
        int col = i % cols;
        slots.push_back(InventorySlot(ItemType::NONE, 0, 
            sf::Vector2f(backpackStartX + col * 40.0f, backpackStartY + row * 40.0f), false));
    }
}

int InventorySystem::findEmptySlot(bool preferHandSlots) {
    if (preferHandSlots) {
        // Search hand slots first
        for (int i = 0; i < LEFT_HAND_SLOTS + RIGHT_HAND_SLOTS; i++) {
            if (slots[i].item == ItemType::NONE) {
                return i;
            }
        }
    }
    
    // Search all slots
    for (int i = 0; i < static_cast<int>(slots.size()); i++) {
        if (slots[i].item == ItemType::NONE) {
            return i;
        }
    }
    
    return -1;  // No empty slot found
}

int InventorySystem::findSlotWithItem(ItemType item, bool preferHandSlots) {
    if (preferHandSlots) {
        // Search hand slots first
        for (int i = 0; i < LEFT_HAND_SLOTS + RIGHT_HAND_SLOTS; i++) {
            if (slots[i].item == item && slots[i].quantity < 99) {
                return i;
            }
        }
    }
    
    // Search all slots
    for (int i = 0; i < static_cast<int>(slots.size()); i++) {
        if (slots[i].item == item && slots[i].quantity < 99) {
            return i;
        }
    }
    
    return -1;  // No slot found
}

bool InventorySystem::addItem(ItemType item, int quantity) {
    if (quantity <= 0) return false;
    
    // Try to stack with existing items first (prefer hand slots)
    int existingSlot = findSlotWithItem(item, true);
    while (existingSlot != -1 && quantity > 0) {
        int space = 99 - slots[existingSlot].quantity;
        int addAmount = std::min(space, quantity);
        slots[existingSlot].quantity += addAmount;
        quantity -= addAmount;
        
        if (quantity > 0) {
            existingSlot = findSlotWithItem(item, true);
        }
    }
    
    // If still have items, try to add to empty slots
    while (quantity > 0) {
        int emptySlot = findEmptySlot(true);
        if (emptySlot == -1) {
            return false;  // No space left
        }
        
        int addAmount = std::min(99, quantity);
        slots[emptySlot].item = item;
        slots[emptySlot].quantity = addAmount;
        quantity -= addAmount;
    }
    
    return true;
}

bool InventorySystem::removeItem(ItemType item, int quantity) {
    if (quantity <= 0) return false;
    
    int totalAvailable = getItemCount(item);
    if (totalAvailable < quantity) {
        return false;
    }
    
    int remaining = quantity;
    
    // Remove from slots (iterate backwards)
    for (int i = slots.size() - 1; i >= 0 && remaining > 0; i--) {
        if (slots[i].item == item) {
            int removeAmount = std::min(slots[i].quantity, remaining);
            slots[i].quantity -= removeAmount;
            remaining -= removeAmount;
            
            if (slots[i].quantity <= 0) {
                slots[i].item = ItemType::NONE;
            }
        }
    }
    
    return true;
}

int InventorySystem::getItemCount(ItemType item) const {
    int count = 0;
    for (const auto& slot : slots) {
        if (slot.item == item) {
            count += slot.quantity;
        }
    }
    return count;
}

void InventorySystem::selectLeftHandSlot(int index) {
    if (index >= 0 && index < LEFT_HAND_SLOTS) {
        leftHandSelected = index;
    }
}

void InventorySystem::selectRightHandSlot(int index) {
    if (index >= 0 && index < RIGHT_HAND_SLOTS) {
        rightHandSelected = index;
    }
}

void InventorySystem::toggleBackpack() {
    isBackpackOpen = !isBackpackOpen;
}

bool InventorySystem::moveItem(int fromSlot, int toSlot) {
    if (fromSlot < 0 || fromSlot >= static_cast<int>(slots.size()) ||
        toSlot < 0 || toSlot >= static_cast<int>(slots.size())) {
        return false;
    }
    
    if (fromSlot == toSlot) return false;
    
    // If target slot is empty, just move
    if (slots[toSlot].item == ItemType::NONE) {
        slots[toSlot] = slots[fromSlot];
        slots[fromSlot] = InventorySlot();
        return true;
    }
    
    // If same item type, try to stack
    if (slots[toSlot].item == slots[fromSlot].item) {
        int space = 99 - slots[toSlot].quantity;
        int moveAmount = std::min(space, slots[fromSlot].quantity);
        slots[toSlot].quantity += moveAmount;
        slots[fromSlot].quantity -= moveAmount;
        
        if (slots[fromSlot].quantity <= 0) {
            slots[fromSlot] = InventorySlot();
        }
        return true;
    }
    
    // Different items, swap them
    InventorySlot temp = slots[fromSlot];
    slots[fromSlot] = slots[toSlot];
    slots[toSlot] = temp;
    
    return true;
}

bool InventorySystem::dropItem(int slotIndex, std::vector<InventorySlot>& droppedItems) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size())) {
        return false;
    }
    
    if (slots[slotIndex].item == ItemType::NONE) {
        return false;
    }
    
    // Add to dropped items
    droppedItems.push_back(slots[slotIndex]);
    
    // Clear the slot
    slots[slotIndex] = InventorySlot();
    
    return true;
}

InventorySlot& InventorySystem::getSlot(int index) {
    return slots[index];
}

const InventorySlot& InventorySystem::getSlot(int index) const {
    return slots[index];
}

ItemType InventorySystem::getLeftHandItem() const {
    return slots[leftHandSelected].item;
}

ItemType InventorySystem::getRightHandItem() const {
    return slots[LEFT_HAND_SLOTS + rightHandSelected].item;
}

void InventorySystem::render(sf::RenderWindow& window) const {
    // Render hand slots
    for (int i = 0; i < LEFT_HAND_SLOTS + RIGHT_HAND_SLOTS; i++) {
        const auto& slot = slots[i];
        
        // Slot background
        sf::RectangleShape slotBg(sf::Vector2f(50, 50));
        slotBg.setPosition(slot.position);
        slotBg.setFillColor(sf::Color(50, 50, 50, 200));
        slotBg.setOutlineThickness(2);
        
        // Highlight selected slots
        bool isSelected = false;
        if (i < LEFT_HAND_SLOTS && i == leftHandSelected) {
            slotBg.setOutlineColor(sf::Color::Green);
            isSelected = true;
        } else if (i >= LEFT_HAND_SLOTS && i == LEFT_HAND_SLOTS + rightHandSelected) {
            slotBg.setOutlineColor(sf::Color::Green);
            isSelected = true;
        } else {
            slotBg.setOutlineColor(sf::Color(100, 100, 100));
        }
        
        window.draw(slotBg);
        
        // Render item
        if (slot.item != ItemType::NONE) {
            sf::CircleShape itemShape(15);
            itemShape.setPosition(slot.position.x + 10, slot.position.y + 10);
            
            // Color based on item type
            sf::Color itemColor;
            switch (slot.item) {
                case ItemType::DIRT: itemColor = sf::Color(130, 90, 60); break;
                case ItemType::STONE: itemColor = sf::Color(120, 120, 130); break;
                case ItemType::GRASS: itemColor = sf::Color(90, 170, 70); break;
                case ItemType::WOOD: itemColor = sf::Color(100, 70, 40); break;
                case ItemType::SAND: itemColor = sf::Color(240, 230, 150); break;
                case ItemType::COAL: itemColor = sf::Color(40, 40, 45); break;
                case ItemType::IRON: itemColor = sf::Color(180, 160, 150); break;
                case ItemType::SNOW: itemColor = sf::Color(250, 250, 255); break;
                default: itemColor = sf::Color::White;
            }
            
            itemShape.setFillColor(itemColor);
            window.draw(itemShape);
            
            // Quantity text
            if (slot.quantity > 1) {
                // (Would need font setup for this)
            }
        }
        
        // Key hint
        // (Would need font setup for this)
    }
    
    // Render backpack if open
    if (isBackpackOpen) {
        // Background
        sf::RectangleShape backpackBg(sf::Vector2f(450, 200));
        backpackBg.setPosition(180, 130);
        backpackBg.setFillColor(sf::Color(30, 30, 30, 230));
        backpackBg.setOutlineColor(sf::Color(150, 150, 150));
        backpackBg.setOutlineThickness(2);
        window.draw(backpackBg);
        
        // Backpack slots
        for (int i = LEFT_HAND_SLOTS + RIGHT_HAND_SLOTS; i < static_cast<int>(slots.size()); i++) {
            const auto& slot = slots[i];
            
            sf::RectangleShape slotBg(sf::Vector2f(35, 35));
            slotBg.setPosition(slot.position);
            slotBg.setFillColor(sf::Color(50, 50, 50, 200));
            slotBg.setOutlineColor(sf::Color(80, 80, 80));
            slotBg.setOutlineThickness(1);
            window.draw(slotBg);
            
            if (slot.item != ItemType::NONE) {
                sf::CircleShape itemShape(12);
                itemShape.setPosition(slot.position.x + 6, slot.position.y + 6);
                
                sf::Color itemColor;
                switch (slot.item) {
                    case ItemType::DIRT: itemColor = sf::Color(130, 90, 60); break;
                    case ItemType::STONE: itemColor = sf::Color(120, 120, 130); break;
                    case ItemType::GRASS: itemColor = sf::Color(90, 170, 70); break;
                    case ItemType::WOOD: itemColor = sf::Color(100, 70, 40); break;
                    case ItemType::SAND: itemColor = sf::Color(240, 230, 150); break;
                    case ItemType::COAL: itemColor = sf::Color(40, 40, 45); break;
                    case ItemType::IRON: itemColor = sf::Color(180, 160, 150); break;
                    case ItemType::SNOW: itemColor = sf::Color(250, 250, 255); break;
                    default: itemColor = sf::Color::White;
                }
                
                itemShape.setFillColor(itemColor);
                window.draw(itemShape);
            }
        }
    }
}

void InventorySystem::saveState(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int slotCount = slots.size();
        file.write(reinterpret_cast<const char*>(&slotCount), sizeof(slotCount));
        
        for (const auto& slot : slots) {
            int itemInt = static_cast<int>(slot.item);
            file.write(reinterpret_cast<const char*>(&itemInt), sizeof(itemInt));
            file.write(reinterpret_cast<const char*>(&slot.quantity), sizeof(slot.quantity));
        }
        
        file.write(reinterpret_cast<const char*>(&leftHandSelected), sizeof(leftHandSelected));
        file.write(reinterpret_cast<const char*>(&rightHandSelected), sizeof(rightHandSelected));
        
        file.close();
    }
}

void InventorySystem::loadState(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int slotCount;
        file.read(reinterpret_cast<char*>(&slotCount), sizeof(slotCount));
        
        slots.resize(slotCount);
        
        for (int i = 0; i < slotCount; i++) {
            int itemInt;
            file.read(reinterpret_cast<char*>(&itemInt), sizeof(itemInt));
            file.read(reinterpret_cast<char*>(&slots[i].quantity), sizeof(slots[i].quantity));
            slots[i].item = static_cast<ItemType>(itemInt);
        }
        
        file.read(reinterpret_cast<char*>(&leftHandSelected), sizeof(leftHandSelected));
        file.read(reinterpret_cast<char*>(&rightHandSelected), sizeof(rightHandSelected));
        
        file.close();
    }
}