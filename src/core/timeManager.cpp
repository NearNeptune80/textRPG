#include "core/timeManager.h"

#include <algorithm>
#include <format>
#include <vector>

#include "core/eventBus.h"

void timeManager::advanceTime(int mins)
{
    if (mins <= 0) return;

    int totalMinutes = minute + mins;
    minute = totalMinutes % 60;
    int hoursToAdd = totalMinutes / 60;

    if (hoursToAdd > 0)
    {
        int totalHours = hour + hoursToAdd;
        hour = totalHours % 24;
        int daysToAdd = totalHours / 24;

        if (daysToAdd > 0)
        {
            dayOfWeek = (dayOfWeek + daysToAdd) % 7;
            day += daysToAdd;

            while (day > 30) // Standard 30 days per month in simulation calendar
            {
                day -= 30;
                month++;
                if (month > 12)
                {
                    month = 1;
                    year++;
                }
            }
        }
    }

    eventBus::getInstance().publishEvent({ gameEvent::timeAdvanced, mins, "", nullptr });
}

float timeManager::getSunriseHour() const
{
    static const float sunrises[13] = { 0.0f, 7.5f, 7.0f, 6.0f, 5.5f, 5.0f, 4.7f, 5.2f, 5.8f, 6.5f, 7.2f, 7.8f, 8.0f };
    return (month >= 1 && month <= 12) ? sunrises[month] : 6.0f;
}

float timeManager::getSunsetHour() const
{
    static const float sunsets[13] = { 0.0f, 16.5f, 17.5f, 18.5f, 19.5f, 20.5f, 21.2f, 20.8f, 19.5f, 18.2f, 17.0f, 16.2f, 16.0f };
    return (month >= 1 && month <= 12) ? sunsets[month] : 18.0f;
}

TimePhase timeManager::getPhase() const
{
    float sr = getSunriseHour();
    float ss = getSunsetHour();
    float current = static_cast<float>(hour) + (static_cast<float>(minute) / 60.0f);

    if (current >= sr - 1.0f && current < sr + 1.0f) return TimePhase::DAWN;
    if (current >= sr + 1.0f && current < ss - 1.0f) return TimePhase::DAY;
    if (current >= ss - 1.0f && current < ss + 1.0f) return TimePhase::DUSK;
    return TimePhase::NIGHT;
}

std::string timeManager::getPhaseString() const
{
    switch (getPhase())
    {
        case TimePhase::DAWN:  return "Dawn";
        case TimePhase::DAY:   return "Day";
        case TimePhase::DUSK:  return "Dusk";
        case TimePhase::NIGHT: return "Night";
        default:               return "Day";
    }
}

std::string timeManager::getDayOfWeekName() const
{
    static constexpr std::string_view days[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    int idx = std::clamp(dayOfWeek, 0, 6);
    return std::string(days[idx]);
}

std::string timeManager::getFormattedTime() const
{
    return std::format("{:02d}:{:02d}", hour, minute);
}

std::string timeManager::getFormattedDate() const
{
    static constexpr std::string_view months[13] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    std::string_view suffix = "th";
    int d = day % 100;
    if (d < 11 || d > 13)
    {
        switch (day % 10)
        {
            case 1: suffix = "st"; break;
            case 2: suffix = "nd"; break;
            case 3: suffix = "rd"; break;
        }
    }

    std::string_view monthStr = (month >= 1 && month <= 12) ? months[month] : "Month";
    std::string dayName = getDayOfWeekName();
    return std::format("{}, {}{} {}", dayName, day, suffix, monthStr);
}

float timeManager::getDayProgress() const
{
    return static_cast<float>(hour * 60 + minute) / 1440.0f;
}