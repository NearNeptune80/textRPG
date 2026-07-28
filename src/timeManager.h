#pragma once
#include <string>
#include <vector>
#include <format>
#include <algorithm>

enum class TimePhase
{
    NIGHT,
    DAWN,
    DAY,
    DUSK
};

class timeManager
{
public:
    int minute = 56;
    int hour = 7;
    int day = 30;
    int month = 6;
    int year = 1;
    int dayOfWeek = 3;

    void advanceTime(int mins)
    {
        minute += mins;
        while (minute >= 60)
        {
            minute -= 60;
            hour++;
            if (hour >= 24)
            {
                hour -= 24;
                day++;
                dayOfWeek = (dayOfWeek + 1) % 7;
                if (day > 30)
                {
                    day = 1;
                    month++;
                    if (month > 12)
                    {
                        month = 1;
                        year++;
                    }
                }
            }
        }
    }

    float getSunriseHour() const
    {
        static const float sunrises[13] = { 0.0f, 7.5f, 7.0f, 6.0f, 5.5f, 5.0f, 4.7f, 5.2f, 5.8f, 6.5f, 7.2f, 7.8f, 8.0f };
        return (month >= 1 && month <= 12) ? sunrises[month] : 6.0f;
    }

    float getSunsetHour() const
    {
        static const float sunsets[13] = { 0.0f, 16.5f, 17.5f, 18.5f, 19.5f, 20.5f, 21.2f, 20.8f, 19.5f, 18.2f, 17.0f, 16.2f, 16.0f };
        return (month >= 1 && month <= 12) ? sunsets[month] : 18.0f;
    }

    std::string getFormattedTime() const
    {
        return std::format("{:02d}:{:02d}", hour, minute);
    }

    std::string getFormattedDate() const
    {
        static const std::vector<std::string> months = {
            "", "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };

        std::string suffix = "th";
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

        std::string monthStr = (month >= 1 && month <= 12) ? months[month] : "Month";
        return std::to_string(day) + suffix + " " + monthStr;
    }

    TimePhase getPhase() const
    {
        float sr = getSunriseHour();
        float ss = getSunsetHour();
        float current = hour + (minute / 60.0f);

        if (current >= sr - 1.0f && current < sr + 1.0f) return TimePhase::DAWN;
        if (current >= sr + 1.0f && current < ss - 1.0f) return TimePhase::DAY;
        if (current >= ss - 1.0f && current < ss + 1.0f) return TimePhase::DUSK;
        return TimePhase::NIGHT;
    }

    float getDayProgress() const
    {
        return static_cast<float>(hour * 60 + minute) / 1440.0f;
    }
};