/*
 * Chunk Implementation - Fixed for SFML 3.0.2
 */

#include "Chunk.h"
#include <cmath>

Chunk::Chunk(ChunkPosition pos) : position(pos) {
    blocks.resize(CHUNK_SIZE * CHUNK_HEIGHT, BlockType::AIR);
    modified.resize(CHUNK_SIZE * CHUNK_HEIGHT, false);
    currentLOD = LODLevel::HIGH;
    isActive = true;
    needsUpdate = true;
    
    // Pre-generate low detail shapes
    generateLowDetailMesh();
}

BlockType Chunk::getBlock(int localX, int localY) const {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_HEIGHT) {
        return BlockType::AIR;
    }
    return blocks[localY * CHUNK_SIZE + localX];
}

void Chunk::setBlock(int localX, int localY, BlockType type) {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_HEIGHT) {
        return;
    }
    
    int index = localY * CHUNK_SIZE + localX;
    if (blocks[index] != type) {
        blocks[index] = type;
        modified[index] = true;
        needsUpdate = true;
    }
}

void Chunk::updateLOD(float distanceToPlayer) {
    // Determine LOD level based on distance
    LODLevel newLOD;
    
    if (distanceToPlayer < 500.0f) {
        newLOD = LODLevel::HIGH;
    } else if (distanceToPlayer < 1200.0f) {
        newLOD = LODLevel::MEDIUM;
    } else {
        newLOD = LODLevel::LOW;
    }
    
    if (newLOD != currentLOD) {
        currentLOD = newLOD;
        needsUpdate = true;
    }
}

void Chunk::render(sf::RenderWindow& window, const sf::View& view) {
    if (!isActive || currentLOD == LODLevel::LOW) {
        return;
    }
    
    sf::ConvexShape hexShape(6);
    hexShape.setOutlineThickness(0.0f);  // Remove outline for perfect fit
    hexShape.setOutlineColor(sf::Color(0, 0, 0, 30));
    
    // Calculate hex dimensions for culling
    const float hexWidth = HEX_SIZE * 2.0f;
    const float hexHeight = HEX_SIZE * std::sqrt(3.0f);
    
    // SFML 2.5 compatible: Use .left/.top/.width/.height
    sf::FloatRect viewBounds;
    viewBounds.left = view.getCenter().x - view.getSize().x / 2.0f;
    viewBounds.top = view.getCenter().y - view.getSize().y / 2.0f;
    viewBounds.width = view.getSize().x;
    viewBounds.height = view.getSize().y;
    
    for (int q = 0; q < CHUNK_SIZE; q++) {
        for (int r = 0; r < CHUNK_HEIGHT; r++) {
            BlockType type = blocks[r * CHUNK_SIZE + q];
            if (type == BlockType::AIR) continue;
            
            // Calculate global hex axial coords for this block
            int globalQ = q + position.chunkX * CHUNK_SIZE;
            int globalR = r + position.chunkY * CHUNK_HEIGHT;
            
            // Use the HexCoord toPixel method for perfect alignment
            sf::Vector2f center = HexCoord(globalQ, globalR).toPixel(HEX_SIZE);
            
            // Culling check
            if (center.x + hexWidth < viewBounds.left || 
                center.x - hexWidth > viewBounds.left + viewBounds.width ||
                center.y + hexHeight < viewBounds.top ||
                center.y - hexHeight > viewBounds.top + viewBounds.height) {
                continue;
            }
            
            // Get hexagon vertices using the exact same method everywhere
            float size = currentLOD == LODLevel::MEDIUM ? HEX_SIZE * 0.95f : HEX_SIZE;
            std::vector<sf::Vector2f> vertices = getHexagonVertices(size, center);
            
            // Set hexagon shape
            for (int i = 0; i < 6; ++i) {
                hexShape.setPoint(i, vertices[i]);
            }
            
            hexShape.setFillColor(getBlockColor(type));
            window.draw(hexShape);
        }
    }
    
    needsUpdate = false;
}

void Chunk::renderLOD(sf::RenderWindow& window, const sf::View& /*view*/) {
    if (currentLOD != LODLevel::LOW || !isActive) {
        return;
    }
    
    // Render low-detail representation
    float worldX = position.chunkX * CHUNK_SIZE * HEX_SIZE * 1.5f;
    float worldY = position.chunkY * CHUNK_HEIGHT * HEX_SIZE * std::sqrt(3.0f);
    
    // Create a simplified mesh representing the chunk
    sf::RectangleShape chunkRect;
    chunkRect.setSize({CHUNK_SIZE * HEX_SIZE * 1.5f, CHUNK_HEIGHT * HEX_SIZE * std::sqrt(3.0f)});
    
    // SFML 3 Fix: setPosition now requires a Vector2 or initializer list
    chunkRect.setPosition({worldX, worldY});
    
    // Calculate dominant block type for chunk color
    int blockCounts[10] = {0};
    for (const auto& block : blocks) {
        if (block != BlockType::AIR) {
            blockCounts[static_cast<int>(block)]++;
        }
    }
    
    int maxCount = 0;
    BlockType dominant = BlockType::AIR;
    for (int i = 0; i < 10; i++) {
        if (blockCounts[i] > maxCount) {
            maxCount = blockCounts[i];
            dominant = static_cast<BlockType>(i);
        }
    }
    
    if (dominant != BlockType::AIR) {
        chunkRect.setFillColor(getBlockColor(dominant));
        window.draw(chunkRect);
    }
}

void Chunk::generateLowDetailMesh() {
    // Pre-compute simplified shapes for low LOD rendering
    lowDetailShapes.clear();
}

void Chunk::rebuildMesh() {
    needsUpdate = true;
}

std::vector<uint8_t> Chunk::serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize chunk position
    data.push_back((position.chunkX >> 24) & 0xFF);
    data.push_back((position.chunkX >> 16) & 0xFF);
    data.push_back((position.chunkX >> 8) & 0xFF);
    data.push_back(position.chunkX & 0xFF);
    
    data.push_back((position.chunkY >> 24) & 0xFF);
    data.push_back((position.chunkY >> 16) & 0xFF);
    data.push_back((position.chunkY >> 8) & 0xFF);
    data.push_back(position.chunkY & 0xFF);
    
    // Serialize blocks
    for (const auto& block : blocks) {
        data.push_back(static_cast<uint8_t>(block));
    }
    
    return data;
}

void Chunk::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return;
    
    // Deserialize position
    position.chunkX = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    position.chunkY = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    
    // Deserialize blocks
    size_t blockDataSize = data.size() - 8;
    blocks.resize(blockDataSize);
    for (size_t i = 0; i < blockDataSize && i < blocks.size(); i++) {
        blocks[i] = static_cast<BlockType>(data[8 + i]);
    }
    
    needsUpdate = true;
}
