/*
 * Game Implementation - Enhanced with Multiplayer Support
 */

#include "Game.h"
#include <iostream>
#include <cmath>

Game::Game(int width, int height, const std::string& title) 
    : width(width), height(height), multiplayerMode(false), 
      currentState(MenuState::MAIN_MENU), previousMenuState(MenuState::MAIN_MENU) {
    window.create(sf::VideoMode(width, height), title, sf::Style::Close);
    window.setFramerateLimit(60);
    running = true;
    
    world = nullptr;
    player = nullptr;
    camera = nullptr;
    multiplayer = nullptr;
    
    initialize();
}

Game::~Game() {
    cleanup();
}

void Game::initialize() {
    world = new World(WORLD_WIDTH, WORLD_HEIGHT);
    // Determine a safe spawn position above ground near center of the window
    float spawnX = static_cast<float>(width) / 2.0f;
    float groundY = world->findGroundY(spawnX);
    float spawnY;
    if (groundY > 0.0f) {
        spawnY = groundY - HEX_SIZE * 3.0f; // Spawn 3 hexes above ground
    } else {
        spawnY = (30.0f - 5.0f) * HEX_SIZE * std::sqrt(3.0f);
    }

    player = new Player(spawnX, spawnY);
    camera = new Camera(static_cast<float>(this->width), static_cast<float>(this->height));
    multiplayer = new MultiplayerClient();
    
    setupMultiplayerCallbacks();
    std::cout << "Game Initialized. Spawn at: " << spawnX << "," << spawnY << std::endl;
}

void Game::handleInput() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
            window.close();
        }

        if (currentState == MenuState::GAME) {
            // ESC to pause
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                previousMenuState = currentState;
                currentState = MenuState::PAUSE_MENU;
            }
            
            // Handle key presses for player control and block selection
            if (event.type == sf::Event::KeyPressed) {
                // Movement
                if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) {
                    player->moveLeft();
                } else if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) {
                    player->moveRight();
                } else if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::Up) {
                    player->jump();
                }

                // Number keys for block selection (1-9)
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int blockIndex = event.key.code - sf::Keyboard::Num1; // 0-8
                    auto interaction = player->getBlockInteraction();
                    if (interaction) {
                        BlockType selectedType = static_cast<BlockType>(blockIndex);
                        interaction->setSelectedBlockType(selectedType);
                    }
                }
            }

            // Handle key releases to stop horizontal movement
            if (event.type == sf::Event::KeyReleased) {
                if ((event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) && !sf::Keyboard::isKeyPressed(sf::Keyboard::D) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    player->stopMoving();
                }
                if ((event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) && !sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    player->stopMoving();
                }
            }

            // Mouse interaction
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                HexCoord clickedHex = pixelToHex(worldPos);
                
                auto interaction = player->getBlockInteraction();
                if (interaction) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        // Place block at clicked location
                        BlockType selectedType = interaction->getSelectedBlockType();
                        interaction->placeBlock(clickedHex, selectedType);
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        // Start mining block at clicked location
                        BlockType targetBlockType = world->getBlock(clickedHex);
                        interaction->startMining(clickedHex, targetBlockType);
                    }
                }
            }
            
            // Mouse button release - stop mining
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Right) {
                    auto interaction = player->getBlockInteraction();
                    if (interaction) {
                        interaction->stopMining();
                    }
                }
            }
        } else {
            // Handle Menu input
            MenuState newState = currentState;
            menu.handleInput(window, event, newState);
            
            // Update current state from menu
            if (newState != currentState) {
                previousMenuState = currentState;
                currentState = newState;
            }
            
            // If returning from pause menu
            if (previousMenuState == MenuState::GAME && currentState == MenuState::GAME) {
                previousMenuState = MenuState::MAIN_MENU;
            }
        }
    }
}

void Game::update(float deltaTime) {
    if (currentState == MenuState::GAME) {
        player->update(deltaTime, *world);
        camera->update(*player);
        world->update(player->getPosition(), deltaTime);
        
        // Update block interaction system
        auto interaction = player->getBlockInteraction();
        if (interaction) {
            interaction->update(deltaTime, *world);
        }
        
        // Update multiplayer if enabled
        if (multiplayerMode) {
            updateMultiplayer();
        }
    }
}

void Game::render() {
    window.clear(sf::Color(135, 206, 235)); // Sky blue

    if (currentState == MenuState::GAME || currentState == MenuState::PAUSE_MENU) {
        // Set camera view for world rendering
        window.setView(camera->getView());
        
        // Render world
        world->render(window, camera->getView(), player->getPosition());
        
        // Render local player
        player->render(window);
        
        // Render other players (multiplayer)
        for (auto const& [id, p] : otherPlayers) {
            if (p) {
                p->render(window);
            }
        }
        
        // Reset to default view for UI rendering
        window.setView(window.getDefaultView());
        
        // Debug Info - Player position
        sf::Text posText;
        posText.setFont(menu.getFont());
        posText.setString("Pos: (" + 
                         std::to_string(static_cast<int>(player->getPosition().x)) + ", " + 
                         std::to_string(static_cast<int>(player->getPosition().y)) + ")");
        posText.setPosition(10, 10);
        posText.setCharacterSize(18);
        posText.setFillColor(sf::Color::White);
        window.draw(posText);
        
        // Show selected block type
        auto interaction = player->getBlockInteraction();
        if (interaction) {
            sf::Text blockText;
            blockText.setFont(menu.getFont());
            blockText.setString("Selected: " + std::to_string(static_cast<int>(interaction->getSelectedBlockType())));
            blockText.setPosition(10, 35);
            blockText.setCharacterSize(18);
            blockText.setFillColor(sf::Color::White);
            window.draw(blockText);
        }

        // Render pause menu overlay if paused
        if (currentState == MenuState::PAUSE_MENU) {
            menu.render(window);
        }
    } else {
        // Render menu screens
        window.setView(window.getDefaultView());
        menu.render(window);
    }
    
    window.display();
}

void Game::run() {
    sf::Clock clock;
    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        handleInput();
        update(deltaTime);
        render();
    }
}

void Game::cleanup() {
    delete world;
    delete player;
    delete camera;
    delete multiplayer;
    
    // Clean up other players
    for (auto& pair : otherPlayers) {
        delete pair.second;
    }
    otherPlayers.clear();
}

void Game::setupMultiplayerCallbacks() {
    // Setup callbacks for multiplayer events
    // Example implementation (adjust based on your MultiplayerClient API):
    /*
    multiplayer->onPlayerJoined([this](int playerId, float x, float y) {
        if (otherPlayers.find(playerId) == otherPlayers.end()) {
            otherPlayers[playerId] = new Player(x, y);
        }
    });
    
    multiplayer->onPlayerLeft([this](int playerId) {
        auto it = otherPlayers.find(playerId);
        if (it != otherPlayers.end()) {
            delete it->second;
            otherPlayers.erase(it);
        }
    });
    
    multiplayer->onPlayerUpdate([this](int playerId, float x, float y) {
        auto it = otherPlayers.find(playerId);
        if (it != otherPlayers.end()) {
            it->second->setPosition(x, y);
        }
    });
    */
}

void Game::updateMultiplayer() {
    if (multiplayerMode && multiplayer && multiplayer->isConnected()) {
        multiplayer->update();
        
        // Send player position
        sf::Vector2f pos = player->getPosition();
        sf::Vector2f vel = player->getSize(); // Placeholder for velocity
        multiplayer->sendPlayerUpdate(pos.x, pos.y, vel.x, vel.y, 
            static_cast<int>(player->getSelectedBlock()));
        
        // Receive and process updates from other players
        // This would be handled through the callbacks set up in setupMultiplayerCallbacks()
    }
}

HexCoord Game::pixelToHex(sf::Vector2f pixel) {
    // Convert pixel coordinates to axial hex coordinates
    // Using flat-top hexagon orientation
    float q = (std::sqrt(3.0f)/3.0f * pixel.x - 1.0f/3.0f * pixel.y) / HEX_SIZE;
    float r = (2.0f/3.0f * pixel.y) / HEX_SIZE;
    
    // Cube coordinates for rounding
    float x = q;
    float z = r;
    float y = -x - z;

    // Round to nearest integer hex
    int rx = static_cast<int>(std::round(x));
    int ry = static_cast<int>(std::round(y));
    int rz = static_cast<int>(std::round(z));

    // Calculate rounding errors
    float x_diff = std::abs(rx - x);
    float y_diff = std::abs(ry - y);
    float z_diff = std::abs(rz - z);

    // Reset the component with the largest error
    if (x_diff > y_diff && x_diff > z_diff) {
        rx = -ry - rz;
    } else if (y_diff > z_diff) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    // Return axial coordinates (q, r)
    return HexCoord(rx, rz);
}