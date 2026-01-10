/*
 * Chunk System
 * Divides world into chunks for efficient memory management and streaming
 */

#ifndef CHUNK_H
#define CHUNK_H

#include "Utils.h"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <vector>
#include <memory>

// Chunk dimensions
const int CHUNK_SIZE = 32; // 32x32 hex blocks per chunk
const int CHUNK_HEIGHT = 64; // 64 blocks tall per chunk

// LOD Levels
enum class LODLevel {
    HIGH,    // Full detail, close to player
    MEDIUM,  // Reduced detail, medium distance
    LOW      // Minimal detail, far away
};

struct ChunkPosition {
    int chunkX;
    int chunkY;
    
    ChunkPosition(int x = 0, int y = 0) : chunkX(x), chunkY(y) {}
    
    bool operator==(const ChunkPosition& other) const {
        return chunkX == other.chunkX && chunkY == other.chunkY;
    }
    
    bool operator<(const ChunkPosition& other) const {
        if (chunkX != other.chunkX) return chunkX < other.chunkX;
        return chunkY < other.chunkY;
    }
};

class Chunk {
private:
    ChunkPosition position;
    std::vector<BlockType> blocks;
    std::vector<bool> modified; // Track modified blocks for saving
    
    LODLevel currentLOD;
    bool isActive;
    bool needsUpdate;
    
    // Pre-computed hex vertices for LOD levels
    std::vector<sf::ConvexShape> lowDetailShapes;
    
public:
    Chunk(ChunkPosition pos);
    
    // Block operations
    BlockType getBlock(int localX, int localY) const;
    void setBlock(int localX, int localY, BlockType type);
    
    // Chunk management
    void updateLOD(float distanceToPlayer);
    void markForUpdate() { needsUpdate = true; }
    bool isNeedsUpdate() const { return needsUpdate; }
    
    // Rendering
    void render(sf::RenderWindow& window, const sf::View& view);
    void renderLOD(sf::RenderWindow& window, const sf::View& view);
    
    // Getters
    ChunkPosition getPosition() const { return position; }
    LODLevel getLOD() const { return currentLOD; }
    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);
    
    // Optimization
    void generateLowDetailMesh();
    void rebuildMesh();
};

// Hash function for ChunkPosition
namespace std {
    template<>
    struct hash<ChunkPosition> {
        size_t operator()(const ChunkPosition& pos) const {
            return ((uint64_t)pos.chunkX << 32) | (uint32_t)pos.chunkY;
        }
    };
}

#endif // CHUNK_H
