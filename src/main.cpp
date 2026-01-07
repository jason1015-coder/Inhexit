/*
 * 2.5D Hexagon-Based Sandbox Game
 * Main Entry Point
 */

#include <SFML/Graphics.hpp>
#include <iostream>
#include "Game.h"

int main() {
    try {
        // Create game instance with 1280x720 resolution
        Game game(1280, 720, "Hexagon Sandbox");
        
        // Run the game main loop
        game.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}