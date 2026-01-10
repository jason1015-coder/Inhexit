/*
 * Game Implementation - Enhanced with Multiplayer Support
 */

#include "Game.h"
#include <iostream>
#include <cmath>
#include <fstream>



Game::Game(int width, int height, const std::string& title)
    : width(width), height(height), multiplayerMode(false),
      currentState(MenuState::MAIN_MENU), previousMenuState(MenuState::MAIN_MENU) {
    window.create(sf::VideoMode(width, height), title, sf::Style::Close);
    window.setFramerateLimit(60);
    running = true;
    
    world = nullptr;
    player = nullptr;
    camera = nullptr;
    timeSystem = nullptr;
    weatherSystem = nullptr;
    inventorySystem = nullptr;
    // multiplayer = nullptr;
    
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
    timeSystem = new TimeSystem();
    weatherSystem = new WeatherSystem();
    inventorySystem = new InventorySystem();
    // multiplayer = new MultiplayerClient();

    // setupMultiplayerCallbacks();
    std::cout << "Game Initialized. Spawn at: " << spawnX << "," << spawnY << std::endl;
}

void Game::handleInput() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
            window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            auto& keyEvent = event.key;
            if (currentState == MenuState::GAME) {
                if (keyEvent.code == sf::Keyboard::Escape) {
                    previousMenuState = currentState;
                    currentState = MenuState::PAUSE_MENU;
                }

                // Movement
                if (keyEvent.code == sf::Keyboard::A || keyEvent.code == sf::Keyboard::Left) {
                    player->moveLeft();
                } else if (keyEvent.code == sf::Keyboard::D || keyEvent.code == sf::Keyboard::Right) {
                    player->moveRight();
                } else if (keyEvent.code == sf::Keyboard::W || keyEvent.code == sf::Keyboard::Space || keyEvent.code == sf::Keyboard::Up) {
                    player->jump();
                }

                // Number keys for block selection (1-9)
                if (keyEvent.code >= sf::Keyboard::Num1 && keyEvent.code <= sf::Keyboard::Num9) {
                    int blockIndex = static_cast<int>(keyEvent.code) - static_cast<int>(sf::Keyboard::Num1); // 0-8
                    auto interaction = player->getBlockInteraction();
                    if (interaction) {
                        BlockType selectedType = static_cast<BlockType>(blockIndex);
                        interaction->setSelectedBlockType(selectedType);
                    }
                }
                
                // Inventory key bindings
                if (keyEvent.code == sf::Keyboard::Num1) {
                    inventorySystem->selectLeftHandSlot(0);
                } else if (keyEvent.code == sf::Keyboard::Num2) {
                    inventorySystem->selectLeftHandSlot(1);
                } else if (keyEvent.code == sf::Keyboard::Num3) {
                    inventorySystem->selectLeftHandSlot(2);
                } else if (keyEvent.code == sf::Keyboard::Num4) {
                    inventorySystem->selectRightHandSlot(0);
                } else if (keyEvent.code == sf::Keyboard::Num5) {
                    inventorySystem->selectRightHandSlot(1);
                } else if (keyEvent.code == sf::Keyboard::Num6) {
                    inventorySystem->selectRightHandSlot(2);
                } else if (keyEvent.code == sf::Keyboard::Q) {
                    inventorySystem->toggleBackpack();
                }
                
                // Save/Load game
                if (keyEvent.code == sf::Keyboard::F5) {
                    saveGame("savegame");
                } else if (keyEvent.code == sf::Keyboard::F9) {
                    loadGame("savegame");
                }
            }
        } else if (event.type == sf::Event::KeyReleased) {
            auto& keyEvent = event.key;
            if (currentState == MenuState::GAME) {
                if ((keyEvent.code == sf::Keyboard::A || keyEvent.code == sf::Keyboard::Left) && !sf::Keyboard::isKeyPressed(sf::Keyboard::D) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    player->stopMoving();
                }
                if ((keyEvent.code == sf::Keyboard::D || keyEvent.code == sf::Keyboard::Right) && !sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    player->stopMoving();
                }
            }
        } else if (event.type == sf::Event::MouseButtonPressed) {
            auto& mouseEvent = event.mouseButton;
            if (currentState == MenuState::GAME) {
                sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(mouseEvent.x, mouseEvent.y));
                HexCoord clickedHex = pixelToHex(worldPos);

                auto interaction = player->getBlockInteraction();
                if (interaction) {
                    if (mouseEvent.button == sf::Mouse::Button::Left) {
                        // Place block at clicked location
                        BlockType selectedType = interaction->getSelectedBlockType();
                        interaction->placeBlock(clickedHex, selectedType);
                    } else if (mouseEvent.button == sf::Mouse::Button::Right) {
                        // Start mining block at clicked location
                        BlockType targetBlockType = world->getBlock(clickedHex);
                        interaction->startMining(clickedHex, targetBlockType);
                    }
                }
            }
        } else if (event.type == sf::Event::MouseButtonReleased) {
            auto& mouseEvent = event.mouseButton;
            if (currentState == MenuState::GAME) {
                if (mouseEvent.button == sf::Mouse::Button::Right) {
                    auto interaction = player->getBlockInteraction();
                    if (interaction) {
                        interaction->stopMining();
                    }
                }
            }
        }

        if (currentState != MenuState::GAME) {
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
        
        // Update time system
        timeSystem->update(deltaTime);
        
        // Update weather system
        weatherSystem->update(deltaTime, timeSystem->getSeason(), player->getPosition(), camera->getView());
        
        // Update multiplayer if enabled
        // if (multiplayerMode) {
        //     updateMultiplayer();
        // }
    }
}

void Game::render() {
    // Sky color based on time of day and weather
    sf::Color skyColor = sf::Color(135, 206, 235); // Default sky blue
    
    // Adjust for day/night cycle
    float ambientLight = timeSystem->getAmbientLight();
    skyColor.r = static_cast<sf::Uint8>(skyColor.r * ambientLight);
    skyColor.g = static_cast<sf::Uint8>(skyColor.g * ambientLight);
    skyColor.b = static_cast<sf::Uint8>(skyColor.b * ambientLight);
    
    // Adjust for weather
    if (weatherSystem->isCloudy() || weatherSystem->isRaining()) {
        skyColor.r = static_cast<sf::Uint8>(skyColor.r * 0.7f);
        skyColor.g = static_cast<sf::Uint8>(skyColor.g * 0.7f);
        skyColor.b = static_cast<sf::Uint8>(skyColor.b * 0.8f);
    }
    if (weatherSystem->isBlizzard()) {
        skyColor = sf::Color(200, 210, 220); // White/grey during blizzard
    }
    
    window.clear(skyColor);

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
        
        // Render weather effects
        weatherSystem->render(window);
        
        // Debug Info - Player position
        sf::Text posText("Pos: (" +
                          std::to_string(static_cast<int>(player->getPosition().x)) + ", " +
                          std::to_string(static_cast<int>(player->getPosition().y)) + ")", menu.getFont(), 18);
        posText.setPosition(sf::Vector2f(10, 10));
        posText.setFillColor(sf::Color::White);
        window.draw(posText);
        
        // Show selected block type
        auto interaction = player->getBlockInteraction();
        if (interaction) {
            sf::Text blockText("Selected: " + std::to_string(static_cast<int>(interaction->getSelectedBlockType())), menu.getFont(), 18);
            blockText.setPosition(sf::Vector2f(10, 35));
            blockText.setFillColor(sf::Color::White);
            window.draw(blockText);
        }
        
        // Show time and season info
        std::string seasonStr;
        switch (timeSystem->getSeason()) {
            case Season::SPRING: seasonStr = "Spring"; break;
            case Season::SUMMER: seasonStr = "Summer"; break;
            case Season::AUTUMN: seasonStr = "Autumn"; break;
            case Season::WINTER: seasonStr = "Winter"; break;
        }
        
        std::string timeStr = timeSystem->isDayTime() ? "Day" : "Night";
        sf::Text timeText(timeStr + " | " + seasonStr, menu.getFont(), 16);
        timeText.setPosition(sf::Vector2f(10, 60));
        timeText.setFillColor(sf::Color::White);
        window.draw(timeText);
        
        // Show weather info
        std::string weatherStr = "Clear";
        if (weatherSystem->isRaining()) weatherStr = "Rain";
        else if (weatherSystem->isSnowing()) weatherStr = "Snow";
        else if (weatherSystem->isHailing()) weatherStr = "Hail";
        else if (weatherSystem->isBlizzard()) weatherStr = "Blizzard";
        else if (weatherSystem->isCloudy()) weatherStr = "Cloudy";
        
        sf::Text weatherText("Weather: " + weatherStr, menu.getFont(), 16);
        weatherText.setPosition(sf::Vector2f(10, 80));
        weatherText.setFillColor(sf::Color::White);
        window.draw(weatherText);
        
        // Show controls hint
        sf::Text controlsText("F5: Save | F9: Load | Q: Backpack", menu.getFont(), 14);
        controlsText.setPosition(sf::Vector2f(10, 100));
        controlsText.setFillColor(sf::Color(200, 200, 200));
        window.draw(controlsText);
        
        // Render inventory
        inventorySystem->render(window);

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
    delete timeSystem;
    delete weatherSystem;
    delete inventorySystem;
    // delete multiplayer;
    
    // Clean up other players
    for (auto& pair : otherPlayers) {
        delete pair.second;
    }
    otherPlayers.clear();
}

/*
void Game::setupMultiplayerCallbacks() {
    // Setup callbacks for multiplayer events
    // Example implementation (adjust based on your MultiplayerClient API):
    // multiplayer->onPlayerJoined([this](int playerId, float x, float y) {
    //     if (otherPlayers.find(playerId) == otherPlayers.end()) {
    //         otherPlayers[playerId] = new Player(x, y);
    //     }
    // });
    //
    // multiplayer->onPlayerLeft([this](int playerId) {
    //     auto it = otherPlayers.find(playerId);
    //     if (it != otherPlayers.end()) {
    //         delete it->second;
    //         otherPlayers.erase(it);
    //     }
    // });
    //
    // multiplayer->onPlayerUpdate([this](int playerId, float x, float y) {
    //     auto it = otherPlayers.find(playerId);
    //     if (it != otherPlayers.end()) {
    //         it->second->setPosition(x, y);
    //     }
    // });
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
*/

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

void Game::saveGame(const std::string& filename) {
    // Save world
    world->saveWorld(filename + "_world.dat");
    
    // Save player
    std::ofstream playerFile(filename + "_player.dat", std::ios::binary);
    if (playerFile.is_open()) {
        sf::Vector2f pos = player->getPosition();
        sf::Vector2f spawn = player->getSpawnPosition();
        sf::Color color = player->getPlayerColor();
        
        playerFile.write(reinterpret_cast<const char*>(&pos.x), sizeof(pos.x));
        playerFile.write(reinterpret_cast<const char*>(&pos.y), sizeof(pos.y));
        playerFile.write(reinterpret_cast<const char*>(&spawn.x), sizeof(spawn.x));
        playerFile.write(reinterpret_cast<const char*>(&spawn.y), sizeof(spawn.y));
        playerFile.write(reinterpret_cast<const char*>(&color.r), sizeof(color.r));
        playerFile.write(reinterpret_cast<const char*>(&color.g), sizeof(color.g));
        playerFile.write(reinterpret_cast<const char*>(&color.b), sizeof(color.b));
        playerFile.close();
    }
    
    // Save time system
    timeSystem->saveState(filename + "_time.dat");
    
    // Save weather system
    weatherSystem->saveState(filename + "_weather.dat");
    
    // Save inventory
    inventorySystem->saveState(filename + "_inventory.dat");
    
    std::cout << "Game saved to: " << filename << std::endl;
}

void Game::loadGame(const std::string& filename) {
    // Load world
    world->loadWorld(filename + "_world.dat");
    
    // Load player
    std::ifstream playerFile(filename + "_player.dat", std::ios::binary);
    if (playerFile.is_open()) {
        sf::Vector2f pos, spawn;
        sf::Color color;
        
        playerFile.read(reinterpret_cast<char*>(&pos.x), sizeof(pos.x));
        playerFile.read(reinterpret_cast<char*>(&pos.y), sizeof(pos.y));
        playerFile.read(reinterpret_cast<char*>(&spawn.x), sizeof(spawn.x));
        playerFile.read(reinterpret_cast<char*>(&spawn.y), sizeof(spawn.y));
        playerFile.read(reinterpret_cast<char*>(&color.r), sizeof(color.r));
        playerFile.read(reinterpret_cast<char*>(&color.g), sizeof(color.g));
        playerFile.read(reinterpret_cast<char*>(&color.b), sizeof(color.b));
        
        player->setPosition(pos.x, pos.y);
        player->setSpawnPosition(spawn.x, spawn.y);
        player->setPlayerColor(color);
        
        playerFile.close();
    }
    
    // Load time system
    timeSystem->loadState(filename + "_time.dat");
    
    // Load weather system
    weatherSystem->loadState(filename + "_weather.dat");
    
    // Load inventory
    inventorySystem->loadState(filename + "_inventory.dat");
    
    std::cout << "Game loaded from: " << filename << std::endl;
}