#include "entities/servantComponent.h"

servantComponent::servantComponent()
{
    hourlySchedule.fill("rest");
}

void servantComponent::setJobForHour(int hour, const std::string& jobId)
{
    if (hour >= 0 && hour < 24)
    {
        hourlySchedule[static_cast<size_t>(hour)] = jobId.empty() ? "rest" : jobId;
    }
}

const std::string& servantComponent::getJobForHour(int hour) const
{
    static const std::string restJob = "rest";
    if (hour >= 0 && hour < 24)
    {
        return hourlySchedule[static_cast<size_t>(hour)];
    }
    return restJob;
}

void servantComponent::fillSchedule(const std::string& jobId)
{
    hourlySchedule.fill(jobId.empty() ? "rest" : jobId);
}

nlohmann::json servantComponent::toJson() const
{
    nlohmann::json j;
    j["servantId"] = servantId;
    j["obedience"] = stats.obedience;
    j["morale"] = stats.morale;
    j["affection"] = stats.affection;
    j["stamina"] = stats.stamina;
    j["maxStamina"] = stats.maxStamina;
    j["isExhausted"] = stats.isExhausted;

    nlohmann::json sched = nlohmann::json::array();
    for (int i = 0; i < 24; ++i)
    {
        sched.push_back(hourlySchedule[static_cast<size_t>(i)]);
    }
    j["hourlySchedule"] = sched;
    return j;
}

void servantComponent::fromJson(const nlohmann::json& j)
{
    if (j.contains("servantId")) servantId = j["servantId"].get<std::string>();
    if (j.contains("obedience")) stats.obedience = j["obedience"].get<float>();
    if (j.contains("morale")) stats.morale = j["morale"].get<float>();
    if (j.contains("affection")) stats.affection = j["affection"].get<float>();
    if (j.contains("stamina")) stats.stamina = j["stamina"].get<float>();
    if (j.contains("maxStamina")) stats.maxStamina = j["maxStamina"].get<float>();
    if (j.contains("isExhausted")) stats.isExhausted = j["isExhausted"].get<bool>();

    if (j.contains("hourlySchedule") && j["hourlySchedule"].is_array())
    {
        const auto& sched = j["hourlySchedule"];
        for (size_t i = 0; i < 24 && i < sched.size(); ++i)
        {
            hourlySchedule[i] = sched[i].get<std::string>();
        }
    }
    stats.clamp();
}

