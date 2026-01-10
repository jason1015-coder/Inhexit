/*
 * Time System
 * Handles day/night cycle and season cycle
 */

#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#include "Utils.h"
#include <SFML/System.hpp>

class TimeSystem {
private:
    // Time tracking
    float elapsedTime;  // Total elapsed time in real seconds
    
    // Day/Night cycle (30 real minutes = 1 game day)
    static constexpr float DAY_CYCLE_DURATION = 30.0f * 60.0f;  // 30 minutes in seconds
    float dayTime;  // Current time within day cycle (0 to DAY_CYCLE_DURATION)
    
    // Season cycle (840 real minutes = 1 game year)
    static constexpr float SEASON_CYCLE_DURATION = 840.0f * 60.0f;  // 840 minutes in seconds
    Season currentSeason;
    
    // Day/night state
    bool isDay;
    float dayProgress;  // 0.0 to 1.0 (0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset)
    
public:
    TimeSystem();
    
    // Update
    void update(float deltaTime);
    
    // Getters
    float getDayTime() const { return dayTime; }
    float getDayProgress() const { return dayProgress; }
    bool isDayTime() const { return isDay; }
    Season getSeason() const { return currentSeason; }
    float getTotalTime() const { return elapsedTime; }
    
    // Get ambient light level based on time of day (0.0 to 1.0)
    float getAmbientLight() const;
    
    // Save/Load
    void saveState(const std::string& filename) const;
    void loadState(const std::string& filename);
};

#endif // TIME_SYSTEM_H