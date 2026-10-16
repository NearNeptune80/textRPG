#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

class questComponent
{
public:
    std::unordered_map<std::string, int> activeQuests;
    std::string trackedQuestId;
    std::unordered_set<std::string> pinnedQuests;

    void setQuestStage(const std::string& questId, int stage);
    int getQuestStage(const std::string& questId) const;
    bool hasQuest(const std::string& questId) const;
    bool isCompleted(const std::string& questId) const;

    const std::string& getTrackedQuest() const { return trackedQuestId; }
    void setTrackedQuest(const std::string& questId) { trackedQuestId = questId; }

    bool isQuestPinned(const std::string& questId) const { return pinnedQuests.contains(questId); }
    void setQuestPinned(const std::string& questId, bool pinned) {
        if (pinned) pinnedQuests.insert(questId);
        else pinnedQuests.erase(questId);
    }
    void togglePinQuest(const std::string& questId) {
        if (pinnedQuests.contains(questId)) pinnedQuests.erase(questId);
        else pinnedQuests.insert(questId);
    }
    const std::unordered_set<std::string>& getPinnedQuests() const { return pinnedQuests; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};