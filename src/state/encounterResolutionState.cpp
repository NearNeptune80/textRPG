#include "state/encounterResolutionState.h"

#include <format>
#include <memory>

#include "core/characterDescription.h"
#include "core/game.h"
#include "entities/entity.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/sexState.h"
#include "state/inventoryState.h"
#include "state/transformationState.h"

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
        if (gameContext->settings.gameplay.autoLoot)
        {
            for (size_t i = 0; i < m_records.size(); ++i)
            {
                m_selectedIndex = i;
                handleLootEnemy(gameContext);
            }
            m_selectedIndex = 0;
        }

        for (auto& rec : m_records)
        {
            if (rec.npc)
            {
                bool emptyBag = true;
                for (const auto& it : rec.npc->inventory.backpack)
                {
                    if (it) { emptyBag = false; break; }
                }
                if (emptyBag && rec.npc->getStat("currency") <= 0.0f)
                {
                    rec.isLooted = true;
                }
                int eqCount = 0;
                for (size_t eqIdx = 0; eqIdx < EQUIP_SLOT_COUNT; ++eqIdx)
                {
                    if (rec.npc->inventory.isEquipped(static_cast<equipSlot>(eqIdx))) eqCount++;
                }
                if (eqCount == 0)
                {
                    rec.isStripped = true;
                }
            }
        }

        if (m_selectedIndex < m_records.size() && m_records[m_selectedIndex].npc)
        {
            gameContext->activeTargetNPC = m_records[m_selectedIndex].npc;
            gameContext->activeTargetMode = TargetMode::COMBAT_ENEMY;
        }

        gameContext->refreshActionGrid();
    }
}

void encounterResolutionState::onExit(game* gameContext)
{
    if (gameContext && gameContext->map)
    {
        auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
        for (const auto& rec : m_records)
        {
            if (rec.npc && tileData.ambushState.npc == rec.npc)
            {
                if (rec.isPermanentlyRemoved)
                {
                    tileData.ambushState.isPermanentlyRemoved = false;
                    tileData.ambushState.npc = nullptr;
                    tileData.ambushState.isDefeated = true;
                    tileData.ambushState.restockMinutesRemaining = 1440; // 24 hours
                }
                else
                {
                    tileData.ambushState.isDefeated = true;
                    tileData.ambushState.restockMinutesRemaining = 1440; // 24 hours
                }
                tileData.persistentNPC = nullptr;
            }
        }
    }
}

void encounterResolutionState::update(game* gameContext, float deltaTime) {}

void encounterResolutionState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    switch (cmd.type)
    {
        case CommandType::SELECT_RESOLUTION_TARGET:
            setSelectedIndex(static_cast<size_t>(cmd.intPayload1));
            if (m_selectedIndex < m_records.size() && m_records[m_selectedIndex].npc)
            {
                gameContext->activeTargetNPC = m_records[m_selectedIndex].npc;
                gameContext->activeTargetMode = TargetMode::COMBAT_ENEMY;
            }
            gameContext->refreshActionGrid();
            break;

        case CommandType::OPEN_INVENTORY:
        case CommandType::LOOT_ENEMY:
            handleOpenLootInventory(gameContext);
            break;

        case CommandType::STRIP_ENEMY:
            handleStripEnemy(gameContext);
            break;

        case CommandType::INTERACTIVE_SEX:
        case CommandType::INTERACTIVE_SEX_DOM:
            handleInteractiveSexDom(gameContext);
            break;

        case CommandType::INTERACTIVE_SEX_SUB:
            handleInteractiveSexSub(gameContext);
            break;

        case CommandType::TRANSFORM_ENEMY:
            handleTransformEnemy(gameContext);
            break;

        case CommandType::TALK_ENEMY:
            handleTalkEnemy(gameContext);
            break;

        case CommandType::SUBJUGATE_ENEMY:
            handleSubjugateEnemy(gameContext);
            break;

        case CommandType::RELEASE_ENEMY:
            handleReleaseEnemy(gameContext);
            break;

        case CommandType::PERMANENTLY_REMOVE_ENEMY:
            handlePermanentlyRemoveEnemy(gameContext);
            break;

        case CommandType::CLOSE_MENU:
            gameContext->changeState(std::make_unique<explorationState>());
            break;

        default:
            break;
    }
}

void encounterResolutionState::handleOpenLootInventory(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    gameContext->activeTargetNPC = rec.npc;
    gameContext->activeTargetMode = TargetMode::COMBAT_ENEMY;
    gameContext->selectedInventorySide = 1; // Default to NPC side
    gameContext->selectedInventoryIndex = -1;
    gameContext->selectedEquipmentSlot = equipSlot::NONE;
    gameContext->selectedEquipmentSide = 0;

    auto returnState = std::make_unique<encounterResolutionState>(*this);
    gameContext->changeState(std::make_unique<inventoryState>(std::move(returnState)));
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
    handleInteractiveSexDom(gameContext);
}

void encounterResolutionState::handleInteractiveSexDom(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    bool nonConNever = (gameContext->settings.content.getFetishRating("Non-Consent") == 0);
    bool nonConBlocked = (!gameContext->settings.content.nonConEnabled || nonConNever) &&
                         (gameContext->settings.content.contentFilterMode == ContentFilterMode::BLOCK_AND_SKIP);

    if (nonConBlocked)
    {
        m_resolutionLog = "Interactive encounter blocked: Non-consensual content is disabled in Content Settings.";
        gameContext->refreshActionGrid();
        return;
    }

    rec.hadSex = true;
    m_resolutionLog = std::format("Entered dominant erotic encounter with {}.", rec.npc->name);

    auto sex = std::make_unique<sexState>(rec.npc, SexStance::MISSIONARY, 50.0f);
    if (nonConNever)
    {
        sex->setConsensual(true);
    }
    gameContext->changeState(std::move(sex));
}

void encounterResolutionState::handleInteractiveSexSub(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    bool nonConNever = (gameContext->settings.content.getFetishRating("Non-Consent") == 0);
    bool nonConBlocked = (!gameContext->settings.content.nonConEnabled || nonConNever) &&
                         (gameContext->settings.content.contentFilterMode == ContentFilterMode::BLOCK_AND_SKIP);

    if (nonConBlocked)
    {
        m_resolutionLog = "Interactive encounter blocked: Non-consensual content is disabled in Content Settings.";
        gameContext->refreshActionGrid();
        return;
    }

    rec.hadSex = true;
    m_resolutionLog = std::format("Entered submissive erotic encounter with {}.", rec.npc->name);

    auto sex = std::make_unique<sexState>(rec.npc, SexStance::MISSIONARY, -50.0f);
    if (nonConNever)
    {
        sex->setConsensual(true);
    }
    gameContext->changeState(std::move(sex));
}

void encounterResolutionState::handleTransformEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    if (!rec.npc->canTransform())
    {
        m_resolutionLog = std::format("{} does not possess metamorphic abilities to transform.", rec.npc->name);
        gameContext->refreshActionGrid();
        return;
    }

    m_resolutionLog = std::format("Commenced anatomical metamorphosis on {}.", rec.npc->name);
    auto returnState = std::make_unique<encounterResolutionState>(*this);
    gameContext->changeState(std::make_unique<transformationState>(TransformationTab::CORE, std::move(returnState), rec.npc));
}

void encounterResolutionState::handleTalkEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc) return;

    rec.talkedTo = true;
    m_resolutionLog = std::format("You speak with {}. They look back at you with wary curiosity, listening cautiously to your words.", rec.npc->name);
    gameContext->addLogEntry("[TALK]", std::format("Spoke with {}.", rec.npc->name), { 180, 220, 180, 255 });
    gameContext->refreshActionGrid();
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

void encounterResolutionState::handlePermanentlyRemoveEnemy(game* gameContext)
{
    if (!gameContext || m_selectedIndex >= m_records.size()) return;
    auto& rec = m_records[m_selectedIndex];
    if (!rec.npc || rec.isPermanentlyRemoved) return;

    rec.isPermanentlyRemoved = true;
    if (gameContext->map)
    {
        auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
        if (tileData.ambushState.npc == rec.npc)
        {
            tileData.ambushState.isPermanentlyRemoved = false;
            tileData.ambushState.npc = nullptr;
            tileData.ambushState.isDefeated = true;
            tileData.ambushState.restockMinutesRemaining = 1440;
            tileData.persistentNPC = nullptr;
        }
    }
    m_resolutionLog = std::format("{} has been permanently removed from this area.", rec.npc->name);
    gameContext->refreshActionGrid();
}