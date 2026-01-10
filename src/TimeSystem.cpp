/*
 * Time System Implementation
 */

#include "TimeSystem.h"
#include <fstream>

TimeSystem::TimeSystem() 
    : elapsedTime(0.0f), dayTime(0.0f), currentSeason(Season::SPRING), 
      isDay(true), dayProgress(0.25f) {
}

void TimeSystem::update(float deltaTime) {
    elapsedTime += deltaTime;
    
    // Update day/night cycle
    dayTime += deltaTime;
    if (dayTime >= DAY_CYCLE_DURATION) {
        dayTime -= DAY_CYCLE_DURATION;
    }
    
    // Calculate day progress (0 to 1)
    dayProgress = dayTime / DAY_CYCLE_DURATION;
    
    // Determine if it's day or night
    // Day is from 6 AM to 6 PM (25% to 75% of cycle)
    isDay = (dayProgress >= 0.25f && dayProgress < 0.75f);
    
    // Update season cycle
    float seasonProgress = (elapsedTime / SEASON_CYCLE_DURATION);
    int seasonIndex = static_cast<int>(seasonProgress) % 4;
    currentSeason = static_cast<Season>(seasonIndex);
}

float TimeSystem::getAmbientLight() const {
    // Calculate ambient light based on time of day
    // Midnight (0.0) = 0.3, Sunrise (0.25) = 0.7, Noon (0.5) = 1.0, Sunset (0.75) = 0.7
    
    float light = 0.3f;  // Base darkness at night
    
    if (dayProgress < 0.25f) {
        // Night to sunrise (0.0 to 0.25)
        light = 0.3f + (0.4f * (dayProgress / 0.25f));
    } else if (dayProgress < 0.5f) {
        // Sunrise to noon (0.25 to 0.5)
        light = 0.7f + (0.3f * ((dayProgress - 0.25f) / 0.25f));
    } else if (dayProgress < 0.75f) {
        // Noon to sunset (0.5 to 0.75)
        light = 1.0f - (0.3f * ((dayProgress - 0.5f) / 0.25f));
    } else {
        // Sunset to midnight (0.75 to 1.0)
        light = 0.7f - (0.4f * ((dayProgress - 0.75f) / 0.25f));
    }
    
    // Adjust based on season (winter is darker)
    switch (currentSeason) {
        case Season::WINTER:
            light *= 0.85f;
            break;
        case Season::SUMMER:
            light *= 1.05f;
            break;
        default:
            break;
    }
    
    return std::max(0.1f, std::min(1.0f, light));
}

void TimeSystem::saveState(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&elapsedTime), sizeof(elapsedTime));
        file.write(reinterpret_cast<const char*>(&dayTime), sizeof(dayTime));
        int seasonInt = static_cast<int>(currentSeason);
        file.write(reinterpret_cast<const char*>(&seasonInt), sizeof(seasonInt));
        file.close();
    }
}

void TimeSystem::loadState(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&elapsedTime), sizeof(elapsedTime));
        file.read(reinterpret_cast<char*>(&dayTime), sizeof(dayTime));
        int seasonInt;
        file.read(reinterpret_cast<char*>(&seasonInt), sizeof(seasonInt));
        currentSeason = static_cast<Season>(seasonInt);
        
        // Recalculate derived values
        dayProgress = dayTime / DAY_CYCLE_DURATION;
        isDay = (dayProgress >= 0.25f && dayProgress < 0.75f);
        
        file.close();
    }
}