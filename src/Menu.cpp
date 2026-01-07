/*
 * Menu Implementation - Enhanced with Animated Graphics
 */

#include "Menu.h"
#include <cmath>
#include <algorithm>
#include <iostream>

Menu::Menu() : currentState(MenuState::MAIN_MENU), selectedIndex(0), 
               volume(100.0f), playerName("Player"), backgroundOffset(0.0f) {
    
    // Load font with multiple fallback paths
    bool fontLoaded = false;
    std::vector<std::string> fontPaths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "Arial.ttf"  // Windows fallback
    };
    
    for (const auto& fontPath : fontPaths) {
        if (font.loadFromFile(fontPath)) {
            fontLoaded = true;
            std::cout << "Font loaded from: " << fontPath << std::endl;
            break;
        }
    }
    
    if (!fontLoaded) {
        std::cerr << "WARNING: Failed to load any font, using default system font" << std::endl;
        // SFML will use a default font, but we should warn the user
    }
    
    // Setup main menu
    setupMainMenu();
    
    // Create animated background
    createAnimatedBackground();
}

void Menu::setState(MenuState state) {
    currentState = state;
    selectedIndex = 0;
    
    switch (state) {
        case MenuState::MAIN_MENU:
            setupMainMenu();
            break;
        case MenuState::SETTINGS:
            setupSettingsMenu();
            break;
        case MenuState::MULTIPLAYER:
            setupMultiplayerMenu();
            break;
        default:
            break;
    }
}

void Menu::setupMainMenu() {
    menuItems.clear();
    
    // Title
    titleText.setFont(font);
    titleText.setString("HEXA WORLD");
    titleText.setCharacterSize(72);
    titleText.setFillColor(sf::Color(255, 200, 100));
    titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getLocalBounds().width / 2, 100);
    
    // Menu options
    std::vector<std::string> options = {
        "Play Singleplayer",
        "Play Multiplayer",
        "Settings",
        "Quit"
    };
    
    float startY = 300;
    for (size_t i = 0; i < options.size(); i++) {
        sf::Text item;
        item.setFont(font);
        item.setString(options[i]);
        item.setCharacterSize(36);
        item.setFillColor(sf::Color(200, 200, 200));
        item.setPosition(WINDOW_WIDTH / 2 - item.getLocalBounds().width / 2, startY + i * 60);
        menuItems.push_back(item);
    }
    
    std::cout << "Main Menu: Use UP/DOWN arrows to navigate, ENTER to select, or press SPACE to quick-start singleplayer" << std::endl;
}

void Menu::setupSettingsMenu() {
    menuItems.clear();
    
    // Title
    titleText.setFont(font);
    titleText.setString("SETTINGS");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color(255, 200, 100));
    titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getLocalBounds().width / 2, 100);
    
    // Settings options
    std::vector<std::string> options = {
        "Volume: " + std::to_string(static_cast<int>(volume)),
        "Player Name: " + playerName,
        "Back"
    };
    
    float startY = 300;
    for (size_t i = 0; i < options.size(); i++) {
        sf::Text item;
        item.setFont(font);
        item.setString(options[i]);
        item.setCharacterSize(32);
        item.setFillColor(sf::Color(200, 200, 200));
        item.setPosition(WINDOW_WIDTH / 2 - item.getLocalBounds().width / 2, startY + i * 60);
        menuItems.push_back(item);
    }
}

void Menu::setupMultiplayerMenu() {
    menuItems.clear();
    
    // Title
    titleText.setFont(font);
    titleText.setString("MULTIPLAYER");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color(255, 200, 100));
    titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getLocalBounds().width / 2, 100);
    
    // Multiplayer options
    std::vector<std::string> options = {
        "Host Game",
        "Join Game",
        "Back"
    };
    
    float startY = 300;
    for (size_t i = 0; i < options.size(); i++) {
        sf::Text item;
        item.setFont(font);
        item.setString(options[i]);
        item.setCharacterSize(32);
        item.setFillColor(sf::Color(200, 200, 200));
        item.setPosition(WINDOW_WIDTH / 2 - item.getLocalBounds().width / 2, startY + i * 60);
        menuItems.push_back(item);
    }
}

void Menu::updateMenuItems() {
    for (size_t i = 0; i < menuItems.size(); i++) {
        if (i == static_cast<size_t>(selectedIndex)) {
            menuItems[i].setFillColor(sf::Color(255, 255, 100));
            menuItems[i].setCharacterSize(40);
        } else {
            menuItems[i].setFillColor(sf::Color(200, 200, 200));
            menuItems[i].setCharacterSize(36);
        }
    }
}

void Menu::createAnimatedBackground() {
    backgroundHexagons.clear();
    
    // Create a grid of hexagons for the background
    int hexRows = 15;
    int hexCols = 20;
    float hexSize = 40;
    
    for (int row = 0; row < hexRows; row++) {
        for (int col = 0; col < hexCols; col++) {
            sf::ConvexShape hex(6);
            
            float x = col * hexSize * 1.5f + 50;
            float y = row * hexSize * std::sqrt(3.0f) + 50;
            
            if (col % 2 == 1) {
                y += hexSize * std::sqrt(3.0f) / 2.0f;
            }
            
            std::vector<sf::Vector2f> vertices = getHexagonVertices(hexSize, sf::Vector2f(x, y));
            for (int i = 0; i < 6; ++i) {
                hex.setPoint(i, vertices[i]);
            }
            
            // Random colors for variety
            float hue = (static_cast<float>(rand()) / RAND_MAX) * 360.0f;
            hex.setFillColor(hslToRgb(hue, 0.3f, 0.4f));
            hex.setOutlineColor(sf::Color(100, 100, 100, 50));
            hex.setOutlineThickness(1);
            
            backgroundHexagons.push_back(hex);
        }
    }
}

void Menu::updateAnimatedBackground() {
    float deltaTime = animationClock.restart().asSeconds();
    backgroundOffset += deltaTime * 20.0f;
    
    // Slightly oscillate hexagon positions
    for (size_t i = 0; i < backgroundHexagons.size(); i++) {
        float oscillation = std::sin(backgroundOffset * 0.1f + i * 0.1f) * 5.0f;
        sf::Vector2f pos = backgroundHexagons[i].getPosition();
        pos.y += oscillation * deltaTime;
        backgroundHexagons[i].setPosition(pos);
        
        // Pulse alpha
        sf::Uint8 alpha = 150 + static_cast<sf::Uint8>(std::sin(backgroundOffset * 0.05f + i * 0.2f) * 50);
        sf::Color color = backgroundHexagons[i].getFillColor();
        color.a = alpha;
        backgroundHexagons[i].setFillColor(color);
    }
}

void Menu::renderAnimatedBackground(sf::RenderWindow& window) {
    for (const auto& hex : backgroundHexagons) {
        window.draw(hex);
    }
}

void Menu::render(sf::RenderWindow& window) {
    // Clear with gradient background
    window.clear(sf::Color(30, 30, 40));
    
    // Render animated background
    updateAnimatedBackground();
    renderAnimatedBackground(window);
    
    // Render title with glow effect
    titleText.setOutlineThickness(3);
    titleText.setOutlineColor(sf::Color(100, 50, 0, 100));
    window.draw(titleText);
    titleText.setOutlineThickness(0);
    
    // Render menu items
    updateMenuItems();
    for (const auto& item : menuItems) {
        window.draw(item);
    }
    
    // Render control instructions
    if (currentState == MenuState::MAIN_MENU) {
        sf::Text instructions;
        instructions.setFont(font);
        instructions.setString("Controls: UP/DOWN to navigate, ENTER to select, SPACE to quick-start");
        instructions.setCharacterSize(18);
        instructions.setFillColor(sf::Color(150, 150, 150));
        instructions.setPosition(WINDOW_WIDTH / 2 - instructions.getLocalBounds().width / 2, 
                               WINDOW_HEIGHT - 50);
        window.draw(instructions);
    }
    
    // Render decorative elements
    sf::CircleShape glow(100);
    glow.setFillColor(sf::Color(255, 200, 100, 20));
    glow.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 100);
    window.draw(glow);
}

void Menu::handleInput(sf::RenderWindow& window, const sf::Event& event, MenuState& currentState) {
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                selectedIndex = (selectedIndex - 1 + menuItems.size()) % menuItems.size();
                break;
            case sf::Keyboard::Down:
                selectedIndex = (selectedIndex + 1) % menuItems.size();
                break;
            case sf::Keyboard::Enter:
                processSelection();
                currentState = this->currentState;
                break;
            case sf::Keyboard::Space:
                // Quick-start singleplayer
                if (currentState == MenuState::MAIN_MENU) {
                    std::cout << "Quick-starting singleplayer game!" << std::endl;
                    setState(MenuState::GAME);
                    currentState = this->currentState;
                }
                break;
            case sf::Keyboard::Escape:
                if (currentState == MenuState::PAUSE_MENU) {
                    currentState = MenuState::GAME;
                } else if (currentState != MenuState::MAIN_MENU) {
                    setState(MenuState::MAIN_MENU);
                }
                break;
            default:
                break;
        }
    }
    
    // Mouse input
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        for (size_t i = 0; i < menuItems.size(); i++) {
            sf::FloatRect bounds = menuItems[i].getGlobalBounds();
            if (bounds.contains(mousePos.x, mousePos.y)) {
                selectedIndex = i;
            }
        }
    }
    
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            processSelection();
            currentState = this->currentState;
        }
    }
}

void Menu::processSelection() {
    switch (currentState) {
        case MenuState::MAIN_MENU:
            switch (selectedIndex) {
                case 0: // Play Singleplayer
                    setState(MenuState::GAME);
                    break;
                case 1: // Play Multiplayer
                    setState(MenuState::MULTIPLAYER);
                    break;
                case 2: // Settings
                    setState(MenuState::SETTINGS);
                    break;
                case 3: // Quit
                    setState(MenuState::QUIT);
                    break;
            }
            break;
            
        case MenuState::SETTINGS:
            switch (selectedIndex) {
                case 0: // Volume
                    volume = (volume >= 100.0f) ? 0.0f : volume + 25.0f;
                    setupSettingsMenu();
                    break;
                case 1: // Player Name
                    // Toggle between preset names (simplified)
                    playerName = (playerName == "Player") ? "Adventurer" : "Player";
                    setupSettingsMenu();
                    break;
                case 2: // Back
                    setState(MenuState::MAIN_MENU);
                    break;
            }
            break;
            
        case MenuState::MULTIPLAYER:
            switch (selectedIndex) {
                case 0: // Host Game
                    setState(MenuState::GAME);
                    break;
                case 1: // Join Game
                    setState(MenuState::GAME);
                    break;
                case 2: // Back
                    setState(MenuState::MAIN_MENU);
                    break;
            }
            break;
            
        default:
            break;
    }
}

// Helper function to convert HSL to RGB
sf::Color Menu::hslToRgb(float h, float s, float l) {
    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    
    float r, g, b;
    
    if (h < 60.0f) {
        r = c; g = x; b = 0;
    } else if (h < 120.0f) {
        r = x; g = c; b = 0;
    } else if (h < 180.0f) {
        r = 0; g = c; b = x;
    } else if (h < 240.0f) {
        r = 0; g = x; b = c;
    } else if (h < 300.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return sf::Color(
        static_cast<sf::Uint8>((r + m) * 255),
        static_cast<sf::Uint8>((g + m) * 255),
        static_cast<sf::Uint8>((b + m) * 255)
    );
}
