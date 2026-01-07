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
#include "MultiplayerClient.h"

class Game {
private:
    sf::RenderWindow window;
    Menu menu;
    World* world;
    Player* player;
    Camera* camera;
    MultiplayerClient* multiplayer;
    
    int width;
    int height;
    
    // World dimensions - Increased for larger maps
    static constexpr int WORLD_WIDTH = 2000;  // Doubled from 1000
    static constexpr int WORLD_HEIGHT = 200;  // Increased from 120
    
    bool running;
    bool multiplayerMode;
    
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
    void setupMultiplayerCallbacks();
    void updateMultiplayer();
    
    // Helper method
    HexCoord pixelToHex(sf::Vector2f pixel);
    
public:
    Game(int width, int height, const std::string& title);
    ~Game();
    void run();
    bool isRunning() const { return running; }
    void enableMultiplayer(bool enabled) { multiplayerMode = enabled; }
};

#endif // GAME_H