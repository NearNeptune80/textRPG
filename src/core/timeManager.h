#pragma once

#include <string>

enum class TimePhase
{
    NIGHT,
    DAWN,
    DAY,
    DUSK
};

/**
 * World simulation clock & calendar manager.
 * Tracks minute, hour, day, month, year, and day of week.
 * Calculates seasonal sunrise/sunset and light phases.
 */
class timeManager
{
public:
    int minute = 56;
    int hour = 7;
    int day = 30;
    int month = 6;
    int year = 1;
    int dayOfWeek = 3; // 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday

    void advanceTime(int mins);

    float getSunriseHour() const;
    float getSunsetHour() const;
    TimePhase getPhase() const;
    std::string getPhaseString() const;
    std::string getFormattedTime() const;
    std::string getFormattedDate() const;
    std::string getDayOfWeekName() const;
    float getDayProgress() const;
};