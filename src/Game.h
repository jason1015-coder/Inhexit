/*
 * Game Class - Enhanced with Multiplayer Support
 * Main game controller that manages all game systems
 */

#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <map>
#include "Player.h"
#include "World.h"
#include "Camera.h"
#include "Menu.h"
#include "TimeSystem.h"
#include "WeatherSystem.h"
#include "InventorySystem.h"
// #include "MultiplayerClient.h"

class Game {
private:
    sf::RenderWindow window;
    Menu menu;
    World* world;
    Player* player;
    Camera* camera;
    // MultiplayerClient* multiplayer;
    
    int width;
    int height;
    
    // World dimensions - Increased for larger maps
    static constexpr int WORLD_WIDTH = 2000;  // Doubled from 1000
    static constexpr int WORLD_HEIGHT = 200;  // Increased from 120
    
    bool running;
    bool multiplayerMode;
    
    // Time and weather systems
    TimeSystem* timeSystem;
    WeatherSystem* weatherSystem;
    InventorySystem* inventorySystem;
    
    // Menu state management
    MenuState currentState;
    MenuState previousMenuState;
    
    // Multiplayer - other players
    std::map<int, Player*> otherPlayers;
    
    void initialize();
    void cleanup();
    void handleInput();
    void update(float deltaTime);
    void render();
    
    // Multiplayer methods
    // void setupMultiplayerCallbacks();
    // void updateMultiplayer();
    
    // Helper method
    HexCoord pixelToHex(sf::Vector2f pixel);
    
    // Save/Load
    void saveGame(const std::string& filename);
    void loadGame(const std::string& filename);
    
public:
    Game(int width, int height, const std::string& title);
    ~Game();
    void run();
    bool isRunning() const { return running; }
    void enableMultiplayer(bool enabled) { multiplayerMode = enabled; }
};

#endif // GAME_H