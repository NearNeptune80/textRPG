#include "entities/questComponent.h"

#include "core/eventBus.h"

void questComponent::setQuestStage(const std::string& questId, int stage)
{
	activeQuests[questId] = stage;
	eventBus::getInstance().publishEvent({ gameEvent::questStageChanged, stage, questId, nullptr });
}

int questComponent::getQuestStage(const std::string& questId) const
{
	if (activeQuests.find(questId) != activeQuests.end()) return activeQuests.at(questId);
	return 0;
}