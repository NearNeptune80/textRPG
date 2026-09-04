#pragma once

#include <memory>
#include <string>
#include <vector>

#include "state/iGameState.h"

class entity;

struct DefeatedEnemyRecord
{
    std::shared_ptr<entity> npc = nullptr;
    bool isLooted = false;
    bool isStripped = false;
    bool hadSex = false;
    bool isSubjugated = false;
    bool isReleased = false;
};

class encounterResolutionState : public iGameState
{
public:
    encounterResolutionState() = default;
    explicit encounterResolutionState(const std::vector<std::shared_ptr<entity>>& defeatedEnemies);
    ~encounterResolutionState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    // Sub-action Executors
    void handleLootEnemy(game* gameContext);
    void handleStripEnemy(game* gameContext);
    void handleInteractiveSex(game* gameContext);
    void handleSubjugateEnemy(game* gameContext);
    void handleReleaseEnemy(game* gameContext);

    // Snapshot APIs for UI/CLI View Layer
    const std::vector<DefeatedEnemyRecord>& getDefeatedRecords() const { return m_records; }
    size_t getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(size_t index) { if (index < m_records.size()) m_selectedIndex = index; }
    const std::string& getResolutionLog() const { return m_resolutionLog; }

private:
    std::vector<DefeatedEnemyRecord> m_records;
    size_t m_selectedIndex = 0;
    std::string m_resolutionLog = "Defeated enemies are at your mercy.";
};