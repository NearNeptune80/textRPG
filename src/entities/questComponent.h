#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class questComponent
{
public:
    std::unordered_map<std::string, int> activeQuests;
    std::string trackedQuestId;

    void setQuestStage(const std::string& questId, int stage);
    int getQuestStage(const std::string& questId) const;
    bool hasQuest(const std::string& questId) const;
    bool isCompleted(const std::string& questId) const;

    const std::string& getTrackedQuest() const { return trackedQuestId; }
    void setTrackedQuest(const std::string& questId) { trackedQuestId = questId; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};