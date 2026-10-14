#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct ServantPsychology
{
    float obedience = 50.0f;
    float morale = 75.0f;
    float affection = 0.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    bool isExhausted = false;

    void clamp()
    {
        obedience = std::clamp(obedience, 0.0f, 100.0f);
        morale = std::clamp(morale, 0.0f, 100.0f);
        affection = std::clamp(affection, -100.0f, 100.0f);
        stamina = std::clamp(stamina, 0.0f, maxStamina);
        isExhausted = (stamina <= 0.0f);
    }
};

class servantComponent
{
public:
    std::string servantId = "";
    ServantPsychology stats;
    std::array<std::string, 24> hourlySchedule; // Slots 0..23; "" or "rest" denotes rest

    servantComponent();

    void setJobForHour(int hour, const std::string& jobId);
    const std::string& getJobForHour(int hour) const;
    void fillSchedule(const std::string& jobId);

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

