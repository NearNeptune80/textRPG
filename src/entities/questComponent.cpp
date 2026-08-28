#include "entities/questComponent.h"

#include "core/eventBus.h"

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