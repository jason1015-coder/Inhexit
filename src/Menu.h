/*
 * Menu System - Enhanced with Animated Graphics
 * Handles main menu, settings menu, and menu navigation
 */

#ifndef MENU_H
#define MENU_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "Utils.h"

enum class MenuState {
    MAIN_MENU,
    SETTINGS,
    GAME,
    PAUSE_MENU,
    MULTIPLAYER,
    QUIT
};

class Menu {
private:
    MenuState currentState;
    
    // UI elements
    sf::Font font;
    sf::Text titleText;
    std::vector<sf::Text> menuItems;
    int selectedIndex;
    
    // Settings
    float volume;
    std::string playerName;
    
    // Enhanced graphics
    std::vector<sf::ConvexShape> backgroundHexagons;
    sf::Clock animationClock;
    float backgroundOffset;
    
    // UI helper methods
    void setupMainMenu();
    void setupSettingsMenu();
    void setupMultiplayerMenu();
    void updateMenuItems();
    void processSelection();
    
    // Enhanced graphics helpers
    void createAnimatedBackground();
    void updateAnimatedBackground();
    void renderAnimatedBackground(sf::RenderWindow& window);
    void createParticleEffect(float x, float y);
    sf::Color hslToRgb(float h, float s, float l);
    
public:
    Menu();
    
    // Menu state management
    void setState(MenuState state);
    MenuState getState() const { return currentState; }
    
    // Input handling
    void handleInput(sf::RenderWindow& window, const sf::Event& event, MenuState& currentState);
    
    // Rendering
    void render(sf::RenderWindow& window);
    
    // Getters for settings
    float getVolume() const { return volume; }
    std::string getPlayerName() const { return playerName; }
    sf::Font& getFont() { return font; }  // Added for UI rendering in Game
};

#endif // MENU_H