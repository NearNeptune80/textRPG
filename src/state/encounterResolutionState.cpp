#include "state/encounterResolutionState.h"

#include <format>
#include <memory>

#include "core/characterDescription.h"
#include "core/game.h"
#include "entities/entity.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/sexState.h"

encounterResolutionState::encounterResolutionState(const std::vector<std::shared_ptr<entity>>& defeatedEnemies)
{
    m_records.clear();
    for (const auto& enemy : defeatedEnemies)
    {
        if (enemy)
        {
            DefeatedEnemyRecord rec;
            rec.npc = enemy;
            m_records.push_back(rec);
        }
    }
}

void encounterResolutionState::initialise(game* gameContext) {}

void encounterResolutionState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void encounterResolutionState::onExit(game* gameContext) {}

void encounterResolutionState::update(game* gameContext, float deltaTime) {}

void encounterResolutionState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    switch (cmd.type)
    {
        case CommandType::SELECT_RESOLUTION_TARGET:
            setSelectedIndex(static_cast<size_t>(cmd.intPayload1));
            gameContext->refreshActionGrid();
            break;

        case CommandType::LOOT_ENEMY:
            handleLootEnemy(gameContext);
            break;

        case CommandType::STRIP_ENEMY:
            handleStripEnemy(gameContext);
            break;

        case CommandType::INTERACTIVE_SEX:
            handleInteractiveSex(gameContext);
            break;

        case CommandType::SUBJUGATE_ENEMY:
            handleSubjugateEnemy(gameContext);
            break;

        case CommandType::RELEASE_ENEMY:
            handleReleaseEnemy(gameContext);
            break;

        case CommandType::CLOSE_MENU:
            gameContext->changeState(std::make_unique<explorationState>());
            break;

        default:
            break;
    }
}

void encounterResolutionState::handleLootEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc || rec.isLooted) return;

    entity* player = gameContext->getPlayer();
    if (!player) return;

    float gold = rec.npc->getStat("currency");
    player->stats.modifyBaseStat("currency", gold);
    rec.npc->stats.setBaseStat("currency", 0.0f);

    int itemCounts = 0;
    for (const auto& itemPtr : rec.npc->inventory.backpack)
    {
        if (itemPtr)
        {
            player->inventory.addItem(itemPtr);
            itemCounts++;
        }
    }
    rec.npc->inventory.backpack.clear();

    rec.isLooted = true;
    m_resolutionLog = std::format("Looted {:.0f}¤ and {} items from {}.", gold, itemCounts, rec.npc->name);
    gameContext->refreshActionGrid();
}

void encounterResolutionState::handleStripEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc || rec.isStripped) return;

    entity* player = gameContext->getPlayer();
    if (!player) return;

    int strippedCount = 0;
    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        equipSlot slot = static_cast<equipSlot>(i);
        if (rec.npc->inventory.isEquipped(slot))
        {
            auto eqItem = rec.npc->inventory.getEquippedItem(slot);
            if (eqItem)
            {
                player->inventory.addItem(eqItem);
                rec.npc->inventory.equipped[i] = nullptr;
                strippedCount++;
            }
        }
    }

    rec.isStripped = true;
    m_resolutionLog = std::format("Stripped {} garments from {}.", strippedCount, rec.npc->name);
    gameContext->refreshActionGrid();
}

void encounterResolutionState::handleInteractiveSex(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    rec.hadSex = true;
    m_resolutionLog = std::format("Entered interactive sex encounter with {}.", rec.npc->name);

    gameContext->changeState(std::make_unique<sexState>(rec.npc));
}

void encounterResolutionState::handleSubjugateEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc || rec.isSubjugated) return;

    rec.isSubjugated = true;
    rec.npc->quests.setQuestStage("subjugated", 1);
    m_resolutionLog = std::format("{} has been subjugated and bound to your will.", rec.npc->name);
    gameContext->refreshActionGrid();
}

void encounterResolutionState::handleReleaseEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc || rec.isReleased) return;

    rec.isReleased = true;
    m_resolutionLog = std::format("You released {}. They scramble away into the distance.", rec.npc->name);
    gameContext->refreshActionGrid();
}