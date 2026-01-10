/*
 * Weather System Implementation
 */

#include "WeatherSystem.h"
#include <cmath>
#include <fstream>

WeatherSystem::WeatherSystem() 
    : currentWeather(WeatherType::CLEAR), weatherDuration(30.0f), 
      weatherTimer(0.0f), rng(std::random_device{}()) {
}

BiomeType WeatherSystem::getBiomeAt(float x) const {
    // Simple biome generation based on x position
    // Every 2000 pixels is a biome region
    float biomeRegion = std::floor(x / 2000.0f);
    
    // Use some noise-like pattern
    int biomeHash = static_cast<int>(biomeRegion * 12345.6789f);
    
    // Alternating biomes: 0-2 = Normal, 3 = Desert, etc.
    if ((biomeHash % 4) == 3) {
        return BiomeType::DESERT;
    }
    return BiomeType::NORMAL;
}

float WeatherSystem::getRainProbability(Season season, BiomeType biome) const {
    if (biome == BiomeType::DESERT) {
        return 0.001f;  // Almost impossible in desert
    }
    
    switch (season) {
        case Season::WINTER: return 0.35f;  // More likely in winter
        case Season::SPRING: return 0.30f;
        case Season::AUTUMN: return 0.28f;
        case Season::SUMMER: return 0.15f;  // Less likely in summer
        default: return 0.25f;
    }
}

float WeatherSystem::getSnowProbability(Season season, BiomeType biome) const {
    if (biome == BiomeType::DESERT) {
        return 0.0001f;  // Almost impossible in desert
    }
    
    switch (season) {
        case Season::WINTER: return 0.40f;  // Very likely in winter
        case Season::AUTUMN: return 0.10f;
        case Season::SPRING: return 0.08f;
        case Season::SUMMER: return 0.005f;  // Very unlikely in summer
        default: return 0.10f;
    }
}

float WeatherSystem::getHailProbability(Season season, BiomeType biome) const {
    if (biome == BiomeType::DESERT) {
        return 0.0005f;  // Almost impossible in desert
    }
    
    switch (season) {
        case Season::SUMMER: return 0.08f;  // More likely in summer
        case Season::SPRING: return 0.05f;
        case Season::AUTUMN: return 0.03f;
        case Season::WINTER: return 0.02f;
        default: return 0.04f;
    }
}

float WeatherSystem::getBlizzardProbability(Season season, BiomeType biome) const {
    if (biome == BiomeType::DESERT) {
        return 0.0001f;  // Almost impossible in desert
    }
    
    switch (season) {
        case Season::WINTER: return 0.15f;  // More likely in winter
        default: return 0.005f;  // Very unlikely other seasons
    }
}

float WeatherSystem::getCloudyProbability(Season season, BiomeType biome) const {
    if (biome == BiomeType::DESERT) {
        return 0.02f;  // Very unlikely in desert
    }
    
    switch (season) {
        case Season::SUMMER: return 0.15f;  // Less cloudy in summer
        case Season::SPRING: return 0.30f;
        case Season::AUTUMN: return 0.35f;
        case Season::WINTER: return 0.45f;  // More cloudy in winter
        default: return 0.30f;
    }
}

void WeatherSystem::updateParticles(float deltaTime, const sf::View& view) {
    // Remove dead particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const WeatherParticle& p) { return p.lifetime <= 0.0f; }),
        particles.end()
    );
    
    // Update existing particles
    sf::Vector2f viewSize = view.getSize();
    sf::Vector2f viewCenter = view.getCenter();
    
    for (auto& particle : particles) {
        particle.position += particle.velocity * deltaTime;
        particle.lifetime -= deltaTime;
    }
    
    // Spawn new particles based on weather
    std::uniform_real_distribution<float> spawnX(viewCenter.x - viewSize.x/2, viewCenter.x + viewSize.x/2);
    std::uniform_real_distribution<float> spawnY(viewCenter.y - viewSize.y/2 - 100, viewCenter.y - viewSize.y/2);
    
    int particlesToSpawn = 0;
    
    switch (currentWeather) {
        case WeatherType::RAIN:
            particlesToSpawn = 5;
            for (int i = 0; i < particlesToSpawn; i++) {
                WeatherParticle p;
                p.position = sf::Vector2f(spawnX(rng), spawnY(rng));
                p.velocity = sf::Vector2f(0, 400 + rng() % 100);
                p.lifetime = 2.0f;
                p.size = 2.0f;
                particles.push_back(p);
            }
            break;
            
        case WeatherType::SNOW:
            particlesToSpawn = 3;
            for (int i = 0; i < particlesToSpawn; i++) {
                WeatherParticle p;
                p.position = sf::Vector2f(spawnX(rng), spawnY(rng));
                p.velocity = sf::Vector2f((rng() % 100 - 50), 100 + rng() % 50);
                p.lifetime = 5.0f;
                p.size = 3.0f;
                particles.push_back(p);
            }
            break;
            
        case WeatherType::HAIL:
            particlesToSpawn = 2;
            for (int i = 0; i < particlesToSpawn; i++) {
                WeatherParticle p;
                p.position = sf::Vector2f(spawnX(rng), spawnY(rng));
                p.velocity = sf::Vector2f((rng() % 60 - 30), 500 + rng() % 150);
                p.lifetime = 1.5f;
                p.size = 5.0f;
                particles.push_back(p);
            }
            break;
            
        case WeatherType::BLIZZARD:
            particlesToSpawn = 15;
            for (int i = 0; i < particlesToSpawn; i++) {
                WeatherParticle p;
                p.position = sf::Vector2f(spawnX(rng), spawnY(rng));
                p.velocity = sf::Vector2f(200 + rng() % 150, 200 + rng() % 100);
                p.lifetime = 3.0f;
                p.size = 4.0f;
                particles.push_back(p);
            }
            break;
            
        default:
            break;
    }
}

void WeatherSystem::update(float deltaTime, Season season, const sf::Vector2f& playerPos, const sf::View& view) {
    weatherTimer += deltaTime;
    
    // Time to change weather?
    if (weatherTimer >= weatherDuration) {
        weatherTimer = 0.0f;
        weatherDuration = 30.0f + static_cast<float>(rng() % 120);  // 30-150 seconds
        
        BiomeType biome = getBiomeAt(playerPos.x);
        
        // Roll for new weather
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        float result = roll(rng);
        
        // Check weather types in order of priority
        if (result < getBlizzardProbability(season, biome)) {
            currentWeather = WeatherType::BLIZZARD;
        } else if (result < getBlizzardProbability(season, biome) + getHailProbability(season, biome)) {
            currentWeather = WeatherType::HAIL;
        } else if (result < getBlizzardProbability(season, biome) + getHailProbability(season, biome) + getSnowProbability(season, biome)) {
            currentWeather = WeatherType::SNOW;
        } else if (result < getBlizzardProbability(season, biome) + getHailProbability(season, biome) + getSnowProbability(season, biome) + getRainProbability(season, biome)) {
            currentWeather = WeatherType::RAIN;
        } else if (result < getBlizzardProbability(season, biome) + getHailProbability(season, biome) + getSnowProbability(season, biome) + getRainProbability(season, biome) + getCloudyProbability(season, biome)) {
            currentWeather = WeatherType::CLOUDY;
        } else {
            currentWeather = WeatherType::CLEAR;
        }
    }
    
    updateParticles(deltaTime, view);
}

void WeatherSystem::render(sf::RenderWindow& window) const {
    for (const auto& particle : particles) {
        if (currentWeather == WeatherType::RAIN) {
            sf::RectangleShape shape(sf::Vector2f(1, particle.size * 2));
            shape.setFillColor(sf::Color(100, 150, 255, 200));
            shape.setPosition(particle.position);
            window.draw(shape);
        } else {
            sf::CircleShape shape(particle.size);
            
            switch (currentWeather) {
                case WeatherType::SNOW:
                    shape.setFillColor(sf::Color(255, 255, 255, 230));
                    break;
                case WeatherType::HAIL:
                    shape.setFillColor(sf::Color(200, 220, 255, 255));
                    break;
                case WeatherType::BLIZZARD:
                    shape.setFillColor(sf::Color(240, 250, 255, 200));
                    break;
                default:
                    break;
            }
            
            shape.setPosition(particle.position);
            window.draw(shape);
        }
    }
}

void WeatherSystem::setWeather(WeatherType type, float duration) {
    currentWeather = type;
    weatherDuration = duration;
    weatherTimer = 0.0f;
    particles.clear();
}

void WeatherSystem::saveState(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int weatherInt = static_cast<int>(currentWeather);
        file.write(reinterpret_cast<const char*>(&weatherInt), sizeof(weatherInt));
        file.write(reinterpret_cast<const char*>(&weatherDuration), sizeof(weatherDuration));
        file.write(reinterpret_cast<const char*>(&weatherTimer), sizeof(weatherTimer));
        file.close();
    }
}

void WeatherSystem::loadState(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int weatherInt;
        file.read(reinterpret_cast<char*>(&weatherInt), sizeof(weatherInt));
        file.read(reinterpret_cast<char*>(&weatherDuration), sizeof(weatherDuration));
        file.read(reinterpret_cast<char*>(&weatherTimer), sizeof(weatherTimer));
        currentWeather = static_cast<WeatherType>(weatherInt);
        particles.clear();
        file.close();
    }
}