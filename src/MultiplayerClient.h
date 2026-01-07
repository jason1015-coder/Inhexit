/*
 * Multiplayer Client
 * Handles communication with the Python WebSocket server
 */

#ifndef MULTIPLAYER_CLIENT_H
#define MULTIPLAYER_CLIENT_H

#include <SFML/Network.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <atomic>
#include <map>

enum class MessageType {
    INITIAL_STATE,
    PLAYER_JOINED,
    PLAYER_LEFT,
    PLAYER_UPDATE,
    BLOCK_PLACED,
    BLOCK_BROKEN,
    ITEM_DROPPED,
    ITEM_PICKED_UP
};

struct PlayerData {
    std::string id;
    std::string name;
    float x;
    float y;
    float velocityX;
    float velocityY;
    int selectedBlock;
    int colorR, colorG, colorB;
};

struct BlockUpdateData {
    int q;
    int r;
    int blockType;
    std::string playerId;
};

struct ItemData {
    std::string id;
    int q;
    int r;
    int blockType;
    int quantity;
    float velocityX;
    float velocityY;
};

class MultiplayerClient {
private:
    sf::TcpSocket socket;
    std::string serverAddress;
    unsigned short serverPort;
    
    std::atomic<bool> connected;
    std::atomic<bool> running;
    
    std::thread networkThread;
    std::mutex receiveMutex;
    std::mutex sendMutex;
    
    // Message queues
    std::queue<std::string> receiveQueue;
    std::queue<std::string> sendQueue;
    
    // Callbacks
    std::function<void(const PlayerData&)> onPlayerJoined;
    std::function<void(const std::string&)> onPlayerLeft;
    std::function<void(const PlayerData&)> onPlayerUpdate;
    std::function<void(const BlockUpdateData&)> onBlockPlaced;
    std::function<void(const BlockUpdateData&)> onBlockBroken;
    std::function<void(const ItemData&)> onItemDropped;
    std::function<void(const std::string&)> onItemPickedUp;
    
    // Player data
    std::string playerId;
    std::string playerName;
    
    // Network methods
    void networkLoop();
    void processMessages();
    void connectToServer();
    void disconnectFromServer();
    
    // Message parsing
    void parseMessage(const std::string& message);
    std::string extractJsonValue(const std::string& json, const std::string& key);
    
public:
    MultiplayerClient();
    ~MultiplayerClient();
    
    // Connection management
    bool connect(const std::string& address, unsigned short port, const std::string& playerName);
    void disconnect();
    bool isConnected() const { return connected.load(); }
    
    // Update loop (call from main thread)
    void update();
    
    // Send messages
    void sendPlayerJoin(float x, float y);
    void sendPlayerUpdate(float x, float y, float vx, float vy, int selectedBlock);
    void sendBlockPlace(int q, int r, int blockType);
    void sendBlockBreak(int q, int r);
    void sendItemDrop(const std::string& itemId, int q, int r, int blockType, int quantity, float vx, float vy);
    void sendItemPickup(const std::string& itemId);
    
    // Set callbacks
    void setOnPlayerJoined(std::function<void(const PlayerData&)> callback) { onPlayerJoined = callback; }
    void setOnPlayerLeft(std::function<void(const std::string&)> callback) { onPlayerLeft = callback; }
    void setOnPlayerUpdate(std::function<void(const PlayerData&)> callback) { onPlayerUpdate = callback; }
    void setOnBlockPlaced(std::function<void(const BlockUpdateData&)> callback) { onBlockPlaced = callback; }
    void setOnBlockBroken(std::function<void(const BlockUpdateData&)> callback) { onBlockBroken = callback; }
    void setOnItemDropped(std::function<void(const ItemData&)> callback) { onItemDropped = callback; }
    void setOnItemPickedUp(std::function<void(const std::string&)> callback) { onItemPickedUp = callback; }
    
    // Get player data
    std::string getPlayerId() const { return playerId; }
    std::string getPlayerName() const { return playerName; }
    
    // Other players
    std::map<std::string, PlayerData> getOtherPlayers() const;
    
private:
    std::map<std::string, PlayerData> otherPlayers;
    mutable std::mutex playersMutex;
};

#endif // MULTIPLAYER_CLIENT_H