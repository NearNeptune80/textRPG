#include "entities/questComponent.h"

#include "core/eventBus.h"
#include "quest/questDatabase.h"

/**
 * Updates stage for a specific quest and broadcasts a gameEvent::questStageChanged notification.
 */
void questComponent::setQuestStage(const std::string& questId, int stage)
{
    activeQuests[questId] = stage;
    eventBus::getInstance().publishEvent({ gameEvent::questStageChanged, stage, questId, nullptr });
}

/**
 * Retrieves the current stage of a quest (returns 0 if unstarted).
 */
int questComponent::getQuestStage(const std::string& questId) const
{
    auto it = activeQuests.find(questId);
    if (it != activeQuests.end()) return it->second;
    return 0;
}

bool questComponent::hasQuest(const std::string& questId) const
{
    return activeQuests.find(questId) != activeQuests.end();
}

bool questComponent::isCompleted(const std::string& questId) const
{
    auto it = activeQuests.find(questId);
    if (it == activeQuests.end()) return false;

    const auto* q = questDatabase::getQuest(questId);
    if (q && q->completionStage > 0)
    {
        return it->second >= q->completionStage;
    }
    return it->second >= 2;
}

nlohmann::json questComponent::toJson() const
{
    nlohmann::json j;
    j["activeQuests"] = activeQuests;
    j["trackedQuestId"] = trackedQuestId;
    return j;
}

void questComponent::fromJson(const nlohmann::json& j)
{
    if (j.is_object())
    {
        if (j.contains("activeQuests"))
        {
            activeQuests = j["activeQuests"].get<std::unordered_map<std::string, int>>();
        }
        else
        {
            // Backward compatibility for legacy saves
            activeQuests = j.get<std::unordered_map<std::string, int>>();
        }
        trackedQuestId = j.value("trackedQuestId", "");
    }
}