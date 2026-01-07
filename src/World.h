/*
 * World Class - Enhanced with Chunking System
 * Manages the hexagonal grid, terrain generation, and chunk-based rendering
 */

#ifndef WORLD_H
#define WORLD_H

#include "Utils.h"
#include "Chunk.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <queue>
#include <set>

class World {
private:
    // World dimensions in chunks
    int widthChunks;
    int heightChunks;
    
    // Chunk management
    std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> chunks;
    std::queue<ChunkPosition> chunkLoadQueue;
    std::queue<ChunkPosition> chunkUnloadQueue;
    
    // Active chunks (within render distance)
    std::set<ChunkPosition> activeChunks;
    
    // World generation parameters
    float seed;
    
    // Procedural generation helpers
    float noise(int x, int y);
    float smoothNoise(int x, int y);
    float interpolatedNoise(float x, float y);
    float valueNoise(float x, float y);
    float randomValue(int x, int y);
    
    // Chunk management helpers
    ChunkPosition worldToChunk(const HexCoord& coord) const;
    void updateActiveChunks(const sf::Vector2f& playerPos);
    void loadChunk(ChunkPosition pos);
    void unloadChunk(ChunkPosition pos);
    void generateChunkData(Chunk* chunk);
    std::shared_ptr<Chunk> getChunk(int cx, int cy);
    
public:
    World(int width, int height); // Width/height in blocks
    
    // Block management
    BlockType getBlock(const HexCoord& coord) const;
    void setBlock(const HexCoord& coord, BlockType type);
    
    // Terrain generation
    void generateTerrain();
    void generateTrees();
    
    // Rendering with LOD
    void render(sf::RenderWindow& window, const sf::View& view, const sf::Vector2f& playerPos);
    
    // Chunk management
    void updateChunks(const sf::Vector2f& playerPos);
    void update(const sf::Vector2f& playerPos, float deltaTime);
    
    // Getters
    int getWidth() const { return widthChunks * CHUNK_SIZE; }
    int getHeight() const { return heightChunks * CHUNK_HEIGHT; }
    
    // Find ground position at given x coordinate
    float findGroundY(float x) const;
    
    // World save/load
    void saveWorld(const std::string& filename);
    void loadWorld(const std::string& filename);
};

#endif // WORLD_H