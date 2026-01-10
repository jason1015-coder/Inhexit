#!/usr/bin/env python3
"""
Patch script to convert SFML 3 code to SFML 2.5 compatible code
"""
import re
import os

def patch_game_cpp():
    filepath = "game/src/Game.cpp"
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Add fstream include
    if "#include <fstream>" not in content:
        content = content.replace('#include <cmath>', '#include <cmath>\n#include <fstream>')
    
    # Fix VideoMode constructor
    content = re.sub(
        r'window\.create\(sf::VideoMode\(sf::Vector2u\(width, height\)\),',
        'window.create(sf::VideoMode(width, height),',
        content
    )
    
    # Fix pollEvent - SFML 2.5 doesn't return optional
    content = re.sub(
        r'while \(auto event = window\.pollEvent\(\)\) \{',
        '''sf::Event event;
    while (window.pollEvent(event)) {''',
        content
    )
    
    # Fix event.visit pattern - replace with manual event type checking
    # For now, let's create a simpler approach
    content = re.sub(r'event->visit\(\[&\]\(auto&& arg\) \{', 'if (event.type == sf::Event::KeyPressed) {', content)
    
    # Fix scancode references (SFML 2.5 doesn't have scancode)
    replacements = {
        'sf::Keyboard::Scancode::Escape': 'sf::Keyboard::Escape',
        'sf::Keyboard::Scancode::A': 'sf::Keyboard::A',
        'sf::Keyboard::Scancode::D': 'sf::Keyboard::D',
        'sf::Keyboard::Scancode::W': 'sf::Keyboard::W',
        'sf::Keyboard::Scancode::S': 'sf::Keyboard::S',
        'sf::Keyboard::Scancode::Space': 'sf::Keyboard::Space',
        'sf::Keyboard::Scancode::Left': 'sf::Keyboard::Left',
        'sf::Keyboard::Scancode::Right': 'sf::Keyboard::Right',
        'sf::Keyboard::Scancode::Up': 'sf::Keyboard::Up',
        'sf::Keyboard::Scancode::Down': 'sf::Keyboard::Down',
        'sf::Keyboard::Scancode::Num1': 'sf::Keyboard::Num1',
        'sf::Keyboard::Scancode::Num2': 'sf::Keyboard::Num2',
        'sf::Keyboard::Scancode::Num3': 'sf::Keyboard::Num3',
        'sf::Keyboard::Scancode::Num4': 'sf::Keyboard::Num4',
        'sf::Keyboard::Scancode::Num5': 'sf::Keyboard::Num5',
        'sf::Keyboard::Scancode::Num6': 'sf::Keyboard::Num6',
        'sf::Keyboard::Scancode::Num7': 'sf::Keyboard::Num7',
        'sf::Keyboard::Scancode::Num8': 'sf::Keyboard::Num8',
        'sf::Keyboard::Scancode::Num9': 'sf::Keyboard::Num9',
        'sf::Keyboard::Scancode::Q': 'sf::Keyboard::Q',
        'sf::Keyboard::Scancode::F5': 'sf::Keyboard::F5',
        'sf::Keyboard::Scancode::F9': 'sf::Keyboard::F9',
    }
    
    for old, new in replacements.items():
        content = content.replace(old, new)
    
    # Fix keyEvent.scancode to keyEvent.code
    content = re.sub(r'keyEvent\.scancode', 'keyEvent.code', content)
    
    # Fix Text constructor (font parameter order is different)
    content = re.sub(
        r'sf::Text\s+(\w+)\(menu\.getFont\(\),\s*([^,]+),\s*(\d+)\)',
        r'sf::Text \1(\2, menu.getFont(), \3)',
        content
    )
    
    # Fix using event type matching
    content = re.sub(
        r'} else if constexpr \(std::is_same_v<T, sf::Event::',
        '} else if (event.type == sf::Event::',
        content
    )
    
    content = re.sub(r'auto& keyEvent = arg;', 'auto& keyEvent = event.key;', content)
    content = re.sub(r'auto& mouseEvent = arg;', 'auto& mouseEvent = event.mouseButton;', content)
    
    # Remove template declaration
    content = re.sub(r'template<class\.\.\. Ts> struct overloaded : Ts\.\.\. \{ using Ts::operator\(\)\.\.\.; \};', '', content)
    
    with open(filepath, 'w') as f:
        f.write(content)
    
    print("Patched Game.cpp for SFML 2.5 compatibility")

if __name__ == "__main__":
    os.chdir("/workspace")
    patch_game_cpp()