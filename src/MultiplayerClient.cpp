/*
 * Multiplayer Client Implementation
 * Simplified TCP-based client for C++ to Python communication
 */

#include "MultiplayerClient.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>

MultiplayerClient::MultiplayerClient() 
    : serverAddress("127.0.0.1"), serverPort(8765),
      connected(false), running(false) {
    
    // Generate unique player ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    playerId = "player_" + std::to_string(dis(gen));
}

MultiplayerClient::~MultiplayerClient() {
    disconnect();
}

bool MultiplayerClient::connect(const std::string& address, unsigned short port, const std::string& name) {
    serverAddress = address;
    serverPort = port;
    playerName = name;
    
    std::cout << "Connecting to " << address << ":" << port << " as " << name << std::endl;
    
    // Attempt to connect
    sf::Socket::Status status = socket.connect(serverAddress, serverPort);
    
    if (status != sf::Socket::Done) {
        std::cerr << "Failed to connect to server" << std::endl;
        return false;
    }
    
    connected = true;
    running = true;
    
    // Start network thread
    networkThread = std::thread(&MultiplayerClient::networkLoop, this);
    
    std::cout << "Connected to multiplayer server" << std::endl;
    return true;
}

void MultiplayerClient::disconnect() {
    if (!connected.load()) {
        return;
    }
    
    running = false;
    connected = false;
    
    if (networkThread.joinable()) {
        networkThread.join();
    }
    
    socket.disconnect();
    std::cout << "Disconnected from server" << std::endl;
}

void MultiplayerClient::networkLoop() {
    char buffer[4096];
    std::size_t received;
    
    while (running.load() && connected.load()) {
        // Receive data
        sf::Socket::Status status = socket.receive(buffer, sizeof(buffer), received);
        
        if (status == sf::Socket::Done) {
            std::string message(buffer, received);
            
            std::lock_guard<std::mutex> lock(receiveMutex);
            receiveQueue.push(message);
        } else if (status == sf::Socket::Disconnected) {
            std::cout << "Server disconnected" << std::endl;
            connected = false;
            break;
        }
    }
}

void MultiplayerClient::update() {
    // Process received messages
    {
        std::lock_guard<std::mutex> lock(receiveMutex);
        while (!receiveQueue.empty()) {
            std::string message = receiveQueue.front();
            receiveQueue.pop();
            parseMessage(message);
        }
    }
    
    // Send queued messages
    {
        std::lock_guard<std::mutex> lock(sendMutex);
        while (!sendQueue.empty()) {
            std::string message = sendQueue.front();
            sendQueue.pop();
            
            socket.send(message.c_str(), message.size());
        }
    }
}

void MultiplayerClient::parseMessage(const std::string& message) {
    // Simple JSON parsing (simplified for this example)
    // In production, use a proper JSON library like nlohmann/json
    
    if (message.find("\"type\":\"player_joined\"") != std::string::npos) {
        if (onPlayerJoined) {
            PlayerData player;
            // Parse player data (simplified)
            player.id = extractJsonValue(message, "\"id\"");
            player.name = extractJsonValue(message, "\"name\"");
            // Parse other fields...
            onPlayerJoined(player);
        }
    }
    else if (message.find("\"type\":\"player_left\"") != std::string::npos) {
        if (onPlayerLeft) {
            std::string playerId = extractJsonValue(message, "\"player_id\"");
            onPlayerLeft(playerId);
        }
    }
    else if (message.find("\"type\":\"player_update\"") != std::string::npos) {
        if (onPlayerUpdate) {
            PlayerData player;
            player.id = extractJsonValue(message, "\"player_id\"");
            // Parse position data...
            onPlayerUpdate(player);
        }
    }
    else if (message.find("\"type\":\"block_placed\"") != std::string::npos) {
        if (onBlockPlaced) {
            BlockUpdateData update;
            // Parse block data...
            onBlockPlaced(update);
        }
    }
    else if (message.find("\"type\":\"block_broken\"") != std::string::npos) {
        if (onBlockBroken) {
            BlockUpdateData update;
            // Parse block data...
            onBlockBroken(update);
        }
    }
}

std::string MultiplayerClient::extractJsonValue(const std::string& json, const std::string& key) {
    size_t keyPos = json.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    size_t valueStart = json.find(":", keyPos);
    if (valueStart == std::string::npos) {
        return "";
    }
    valueStart++;
    
    // Skip whitespace
    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    if (json[valueStart] == '"') {
        valueStart++;
        size_t valueEnd = json.find("\"", valueStart);
        if (valueEnd == std::string::npos) {
            return "";
        }
        return json.substr(valueStart, valueEnd - valueStart);
    } else if (json[valueStart] == '{' || json[valueStart] == '[') {
        // Complex value - not handling in simplified version
        return "";
    } else {
        // Number or boolean
        size_t valueEnd = json.find(",", valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = json.find("}", valueStart);
        }
        if (valueEnd == std::string::npos) {
            return "";
        }
        std::string value = json.substr(valueStart, valueEnd - valueStart);
        // Trim whitespace
        size_t start = value.find_first_not_of(" \t\n\r");
        size_t end = value.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }
        return value.substr(start, end - start + 1);
    }
}

void MultiplayerClient::sendPlayerJoin(float x, float y) {
    std::string message = R"({"type":"player_join","player_id":")" + playerId + R"(","player_name":")" + playerName + R"(","x":)" + std::to_string(x) + R"(,"y":)" + std::to_string(y) + R"(,"color":[255,100,100]})";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

void MultiplayerClient::sendPlayerUpdate(float x, float y, float vx, float vy, int selectedBlock) {
    std::string message = R"({"type":"player_update","player_id":")" + playerId + R"(","x":)" + std::to_string(x) + R"(,"y":)" + std::to_string(y) + R"(,"velocity_x":)" + std::to_string(vx) + R"(,"velocity_y":)" + std::to_string(vy) + R"(,"selected_block":)" + std::to_string(selectedBlock) + "}";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

void MultiplayerClient::sendBlockPlace(int q, int r, int blockType) {
    std::string message = R"({"type":"block_place","q":)" + std::to_string(q) + R"(,"r":)" + std::to_string(r) + R"(,"block_type":)" + std::to_string(blockType) + R"(,"player_id":")" + playerId + R"("})";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

void MultiplayerClient::sendBlockBreak(int q, int r) {
    std::string message = R"({"type":"block_break","q":)" + std::to_string(q) + R"(,"r":)" + std::to_string(r) + R"(,"player_id":")" + playerId + R"("})";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

void MultiplayerClient::sendItemDrop(const std::string& itemId, int q, int r, int blockType, int quantity, float vx, float vy) {
    std::string message = R"({"type":"item_drop","item_id":")" + itemId + R"(","q":)" + std::to_string(q) + R"(,"r":)" + std::to_string(r) + R"(,"block_type":)" + std::to_string(blockType) + R"(,"quantity":)" + std::to_string(quantity) + R"(,"velocity_x":)" + std::to_string(vx) + R"(,"velocity_y":)" + std::to_string(vy) + "}";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

void MultiplayerClient::sendItemPickup(const std::string& itemId) {
    std::string message = R"({"type":"item_pickup","item_id":")" + itemId + R"("})";
    
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(message);
}

std::map<std::string, PlayerData> MultiplayerClient::getOtherPlayers() const {
    std::lock_guard<std::mutex> lock(playersMutex);
    return otherPlayers;
}