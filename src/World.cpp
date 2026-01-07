/*
 * World Implementation - Enhanced with Chunking System
 */

#include "World.h"
#include <cmath>
#include <fstream>
#include <iostream>

World::World(int width, int height) {
    // Calculate number of chunks needed
    widthChunks = (width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    heightChunks = (height + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;
    
    seed = static_cast<float>(rand()) / RAND_MAX * 10000.0f;
    
    std::cout << "World initialized: " << widthChunks << "x" << heightChunks << " chunks" << std::endl;
    std::cout << "Total world size: " << getWidth() << "x" << getHeight() << " blocks" << std::endl;
    
    generateTerrain();
}

float World::valueNoise(float x, float y) {
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    
    float xf = x - xi;
    float yf = y - yi;
    
    // Get random values at grid points
    float v00 = randomValue(xi, yi);
    float v10 = randomValue(xi + 1, yi);
    float v01 = randomValue(xi, yi + 1);
    float v11 = randomValue(xi + 1, yi + 1);
    
    // Bilinear interpolation
    float i1 = v00 * (1.0f - xf) + v10 * xf;
    float i2 = v01 * (1.0f - xf) + v11 * xf;
    
    return i1 * (1.0f - yf) + i2 * yf;
}

float World::randomValue(int x, int y) {
    // Simple deterministic hash based on seed
    int n = x + y * 57 + static_cast<int>(seed);
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

void World::generateTerrain() {
    // Initially, we generate the starting chunks around (0,0)
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            getChunk(x, y); // This forces generation
        }
    }
}

void World::generateChunkData(Chunk* chunk) {
    ChunkPosition pos = chunk->getPosition();
    
    // Calculate world coordinates for this chunk
    int startX = pos.chunkX * CHUNK_SIZE;
    int startY = pos.chunkY * CHUNK_HEIGHT;

    for (int q = 0; q < CHUNK_SIZE; q++) {
        for (int r = 0; r < CHUNK_HEIGHT; r++) {
            int worldQ = startX + q;
            int worldR = startY + r;

            // FIX: Use world coordinates for noise to ensure continuity
            float groundLevel = valueNoise(worldQ * 0.05f + seed, 0) * 15.0f;
            groundLevel += valueNoise(worldQ * 0.1f + seed, seed) * 5.0f;

            // FIX: Base level is absolute world R coordinate
            int baseLevel = 30 + static_cast<int>(groundLevel);

            BlockType blockType = BlockType::AIR;

            if (worldR > baseLevel + 5) {
                blockType = BlockType::STONE;
                // Add ore veins
                float oreChance = valueNoise(worldQ * 0.2f + seed, worldR * 0.2f + seed);
                if (oreChance > 0.88f) blockType = BlockType::COAL;
                else if (oreChance > 0.92f) blockType = BlockType::IRON;
            } else if (worldR > baseLevel) {
                blockType = BlockType::DIRT;
            } else if (worldR == baseLevel) {
                blockType = BlockType::GRASS;
            }

            chunk->setBlock(q, r, blockType);
        }
    }
}

std::shared_ptr<Chunk> World::getChunk(int cx, int cy) {
    ChunkPosition pos(cx, cy);
    if (chunks.find(pos) == chunks.end()) {
        auto newChunk = std::make_shared<Chunk>(pos);
        generateChunkData(newChunk.get());
        chunks[pos] = newChunk;
        return newChunk;
    }
    return chunks[pos];
}

void World::setBlock(const HexCoord& coord, BlockType type) {
    // Convert hex coord to chunk/local coord
    int cx = static_cast<int>(std::floor(static_cast<float>(coord.q) / CHUNK_SIZE));
    int cy = static_cast<int>(std::floor(static_cast<float>(coord.r) / CHUNK_HEIGHT));
    
    int lq = coord.q % CHUNK_SIZE;
    int lr = coord.r % CHUNK_HEIGHT;
    
    // Handle negative modulo
    if (lq < 0) lq += CHUNK_SIZE;
    if (lr < 0) lr += CHUNK_HEIGHT;
    
    getChunk(cx, cy)->setBlock(lq, lr, type);
}

BlockType World::getBlock(const HexCoord& coord) const {
    int cx = static_cast<int>(std::floor(static_cast<float>(coord.q) / CHUNK_SIZE));
    int cy = static_cast<int>(std::floor(static_cast<float>(coord.r) / CHUNK_HEIGHT));
    
    int lq = coord.q % CHUNK_SIZE;
    int lr = coord.r % CHUNK_HEIGHT;
    
    if (lq < 0) lq += CHUNK_SIZE;
    if (lr < 0) lr += CHUNK_HEIGHT;
    
    auto it = chunks.find(ChunkPosition(cx, cy));
    if (it != chunks.end()) {
        return it->second->getBlock(lq, lr);
    }
    return BlockType::AIR;
}

float World::findGroundY(float x) const {
    // Convert pixel x to hex q
    int q = static_cast<int>(std::round(x / (HEX_SIZE * 1.5f)));
    
    // Scan down from a high point
    for (int r = 0; r < 200; r++) {
        if (getBlock(HexCoord(q, r)) != BlockType::AIR) {
            return r * HEX_SIZE * std::sqrt(3.0f);
        }
    }
    return 0.0f;
}

void World::saveWorld(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    
    file.write(reinterpret_cast<char*>(&widthChunks), sizeof(widthChunks));
    file.write(reinterpret_cast<char*>(&heightChunks), sizeof(heightChunks));
    file.write(reinterpret_cast<char*>(&seed), sizeof(seed));
    
    uint32_t chunkCount = chunks.size();
    file.write(reinterpret_cast<char*>(&chunkCount), sizeof(chunkCount));
    
    for (auto const& [pos, chunk] : chunks) {
        std::vector<uint8_t> chunkData = chunk->serialize();
        uint32_t dataSize = chunkData.size();
        file.write(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        file.write(reinterpret_cast<const char*>(chunkData.data()), dataSize);
    }
    file.close();
}

void World::loadWorld(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    
    file.read(reinterpret_cast<char*>(&widthChunks), sizeof(widthChunks));
    file.read(reinterpret_cast<char*>(&heightChunks), sizeof(heightChunks));
    file.read(reinterpret_cast<char*>(&seed), sizeof(seed));
    
    uint32_t chunkCount;
    file.read(reinterpret_cast<char*>(&chunkCount), sizeof(chunkCount));
    
    chunks.clear();
    for (uint32_t i = 0; i < chunkCount; i++) {
        uint32_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        std::vector<uint8_t> chunkData(dataSize);
        file.read(reinterpret_cast<char*>(chunkData.data()), dataSize);
        
        auto newChunk = std::make_shared<Chunk>(ChunkPosition(0,0));
        newChunk->deserialize(chunkData);
        chunks[newChunk->getPosition()] = newChunk;
    }
    file.close();
}

void World::update(const sf::Vector2f& playerPos, float /*deltaTime*/) {
    // Update active chunks based on player position
    updateChunks(playerPos);
}

void World::render(sf::RenderWindow& window, const sf::View& view, const sf::Vector2f& /*playerPos*/) {
    // Simple render implementation - just render active chunks
    for (const auto& pos : activeChunks) {
        if (chunks.find(pos) != chunks.end()) {
            chunks[pos]->render(window, view);
        }
    }
}

void World::updateChunks(const sf::Vector2f& playerPos) {
    // Update active chunks based on player position
    // This is a stub implementation
    updateActiveChunks(playerPos);
}

void World::generateTrees() {
    // Stub implementation for tree generation
}

void World::updateActiveChunks(const sf::Vector2f& playerPos) {
    // Stub: Mark chunks around player as active
    int playerChunkX = static_cast<int>(playerPos.x / (CHUNK_SIZE * HEX_SIZE));
    int playerChunkY = static_cast<int>(playerPos.y / (CHUNK_HEIGHT * HEX_SIZE));
    
    activeChunks.clear();
    for (int x = playerChunkX - 2; x <= playerChunkX + 2; x++) {
        for (int y = playerChunkY - 2; y <= playerChunkY + 2; y++) {
            activeChunks.insert(ChunkPosition(x, y));
        }
    }
}

void World::loadChunk(ChunkPosition /*pos*/) {
    // Stub: Chunk is loaded automatically by getChunk
}

void World::unloadChunk(ChunkPosition pos) {
    // Stub: Remove chunk from memory
    chunks.erase(pos);
}

ChunkPosition World::worldToChunk(const HexCoord& coord) const {
    int cx = static_cast<int>(std::floor(static_cast<float>(coord.q) / CHUNK_SIZE));
    int cy = static_cast<int>(std::floor(static_cast<float>(coord.r) / CHUNK_HEIGHT));
    return ChunkPosition(cx, cy);
}
