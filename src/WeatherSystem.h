/*
 * Weather System
 * Handles biome-based weather patterns
 */

#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include "Utils.h"
#include "TimeSystem.h"
#include <random>
#include <SFML/Graphics.hpp>

class WeatherSystem {
private:
    WeatherType currentWeather;
    float weatherDuration;
    float weatherTimer;
    
    // Particles for visual effects
    struct WeatherParticle {
        sf::Vector2f position;
        sf::Vector2f velocity;
        float lifetime;
        float size;
    };
    std::vector<WeatherParticle> particles;
    
    // Random number generator
    std::mt19937 rng;
    
    // Get biome type for a position
    BiomeType getBiomeAt(float x) const;
    
    // Weather probabilities based on season and biome
    float getRainProbability(Season season, BiomeType biome) const;
    float getSnowProbability(Season season, BiomeType biome) const;
    float getHailProbability(Season season, BiomeType biome) const;
    float getBlizzardProbability(Season season, BiomeType biome) const;
    float getCloudyProbability(Season season, BiomeType biome) const;
    
    // Update weather effects
    void updateParticles(float deltaTime, const sf::View& view);
    
public:
    WeatherSystem();
    
    // Update
    void update(float deltaTime, Season season, const sf::Vector2f& playerPos, const sf::View& view);
    
    // Rendering
    void render(sf::RenderWindow& window) const;
    
    // Getters
    WeatherType getCurrentWeather() const { return currentWeather; }
    bool isRaining() const { return currentWeather == WeatherType::RAIN; }
    bool isSnowing() const { return currentWeather == WeatherType::SNOW; }
    bool isHailing() const { return currentWeather == WeatherType::HAIL; }
    bool isBlizzard() const { return currentWeather == WeatherType::BLIZZARD; }
    bool isCloudy() const { return currentWeather == WeatherType::CLOUDY; }
    
    // Change weather manually
    void setWeather(WeatherType type, float duration = 60.0f);
    
    // Save/Load
    void saveState(const std::string& filename) const;
    void loadState(const std::string& filename);
};

#endif // WEATHER_SYSTEM_H