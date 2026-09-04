#include "core/game.h"

#include <algorithm>
#include <iostream>

#include "common/randomEngine.h"
#include "core/eventBus.h"
#include "core/textParser.h"
#include "entities/npcGenerator.h"
#include "events/gameEvents.h"
#include "items/itemDatabase.h"
#include "items/merchantValuation.h"
#include "map/encounterResolver.h"
#include "quest/questDatabase.h"
#include "save/saveManager.h"
#include "settings/settingsManager.h"
#include "state/characterCreationState.h"
#include "state/combatState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/loadGameState.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/phoneAppsState.h"
#include "state/shopState.h"
#include "state/transformationState.h"
#include "ui/actionGridManager.h"
#include "ui/theme.h"

game::game() : isRunning(false), map(nullptr), playerEntity(nullptr), Player(nullptr), gridX(1), gridY(1) {}

game::~game()
{
    map = nullptr;
    playerEntity = nullptr;
    Player = nullptr;
}

void game::changeState(std::unique_ptr<iGameState> newState)
{
    currentActionPage = 0;
    Player = playerEntity.get();
    if (activeGameState) activeGameState->onExit(this);
    activeGameState = std::move(newState);
    if (activeGameState)
    {
        activeGameState->initialise(this);
        activeGameState->onEnter(this);
    }
}

void game::triggerActionButton(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 15) return;
    int targetIdx = (currentActionPage * 15) + slotIndex;
    if (targetIdx >= 0 && static_cast<size_t>(targetIdx) < activeButtons.size())
    {
        if (activeButtons[targetIdx].isEnabled && activeButtons[targetIdx].onClick)
        {
            auto cb = activeButtons[targetIdx].onClick;
            cb();
        }
    }
}

void game::previousActionPage()
{
    if (currentActionPage > 0)
    {
        currentActionPage--;
    }
}

void game::nextActionPage()
{
    int totalButtons = static_cast<int>(activeButtons.size());
    int totalPages = (totalButtons > 0) ? ((totalButtons - 1) / 15) + 1 : 1;
    if (currentActionPage < totalPages - 1)
    {
        currentActionPage++;
    }
}

void game::init()
{
    settingsManager::loadFromFile(settings, "data/settings.json");
    Theme::applyTheme(settings.display.activeTheme);

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        npcGenerator::loadTemplates("data/npc_templates.json");
        questDatabase::loadDatabase("data/quests");
    }

    playerEntity = nullptr;
    Player = nullptr;
    map = nullptr;

    changeState(std::make_unique<mainMenuState>());

    // Milestone 9: Time Advancement Biological Pipeline & Scheduled Maintenance
    eventBus::getInstance().subscribe(gameEvent::timeAdvanced, [this](const eventData& data) {
        int mins = data.numericValue;
        if (mins <= 0) return;

        // 1. Unsafe tile dropped item decay & map tile runtime updates
        if (this->map)
        {
            this->map->processTimePassage(mins);
        }

        // 2. Player Biological Pipeline
        if (this->Player)
        {
            // Advance active mutations
            this->Player->anatomy.processMutations(mins);

            // Regenerate bodily fluids (milk, cum, girlcum) & recover orifice stretch
            this->Player->anatomy.processBiologicalRecovery(mins);

            // Natural arousal (lust) decay out of combat
            float currentLust = this->Player->getStat("lust");
            if (currentLust > 0.0f)
            {
                float lustDecayPerHour = 5.0f;
                float decayAmount = (lustDecayPerHour * static_cast<float>(mins)) / 60.0f;
                this->Player->stats.modifyBaseStat("lust", -std::min(currentLust, decayAmount));
            }

            // Status effect expiration ticks
            int turnTicks = std::max(1, mins / 5);
            for (int t = 0; t < turnTicks; ++t)
            {
                this->Player->updateStatusEffectsOnTurn();
            }

            // Advance active pregnancy & process birth
            if (this->Player->gestation.isPregnant)
            {
                bool readyToGiveBirth = this->Player->gestation.processGestationMinutes(mins);
                if (readyToGiveBirth || this->Player->gestation.gestationDaysRemaining <= 0)
                {
                    auto offspring = this->Player->gestation.giveBirth(this->Player->id);
                    if (!offspring.empty())
                    {
                        std::cout << "[Biological Pipeline] " << this->Player->name
                                  << " has given birth to " << offspring.size() << " offspring!\n";
                    }
                }
            }
        }

        // 3. Target NPC Biological Pipeline
        if (this->activeTargetNPC)
        {
            this->activeTargetNPC->anatomy.processMutations(mins);
            this->activeTargetNPC->anatomy.processBiologicalRecovery(mins);
            if (this->activeTargetNPC->gestation.isPregnant)
            {
                this->activeTargetNPC->gestation.processGestationMinutes(mins);
            }
        }

        // 4. Scheduled World Maintenance (Daily 06:00 Restock)
        if (this->gameTime.hour >= 6)
        {
            if (this->activeTargetNPC)
            {
                merchantValuation::merchantRestock(this->activeTargetNPC.get(), this->gameTime.day);
            }
            if (this->map)
            {
                for (int y = 0; y < this->map->getHeight(); ++y)
                {
                    for (int x = 0; x < this->map->getWidth(); ++x)
                    {
                        auto& tileData = this->map->getRuntimeData(x, y);
                        if (tileData.persistentNPC)
                        {
                            merchantValuation::merchantRestock(tileData.persistentNPC.get(), this->gameTime.day);
                        }
                    }
                }
            }
        }
    });

    eventBus::getInstance().subscribe(gameEvent::combatEnded, [this](const eventData& data) {
        switch (static_cast<CombatOutcome>(data.numericValue))
        {
            case CombatOutcome::VICTORY:
                std::cout << "[Combat] Victory achieved!\n";
                break;
            case CombatOutcome::DEFEAT:
                std::cout << "[Combat] Defeated! Applied currency penalty.\n";
                break;
            case CombatOutcome::ESCAPE:
                std::cout << "[Combat] Successfully escaped.\n";
                break;
            case CombatOutcome::SURRENDER:
                std::cout << "[Combat] Surrendered to the enemy.\n";
                break;
        }
    });

    isRunning = true;
}

void game::handleEvents()
{
    input.update(this);
}

void game::update(float deltaTime)
{
    if (activeGameState) activeGameState->update(this, deltaTime);
}

void game::refreshActionGrid()
{
    ActionGridManager::refresh(this);
}

InventorySlotInfo game::getInventorySlotItem(int side, int absoluteIndex)
{
    InventorySlotInfo info;

    if (side == 0 && Player)
    {
        auto stackedView = Player->inventory.getStackedView();
        if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < stackedView.size())
        {
            info.itemPtr = stackedView[absoluteIndex].itemPtr;
            info.count = stackedView[absoluteIndex].totalCount;
            info.isValid = true;
        }
    }
    else if (side == 1)
    {
        if (activeTargetNPC)
        {
            auto npcView = activeTargetNPC->inventory.getStackedView();
            if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < npcView.size())
            {
                info.itemPtr = npcView[absoluteIndex].itemPtr;
                info.count = npcView[absoluteIndex].totalCount;
                info.isValid = true;
            }
        }
        else
        {
            auto tileView = getTileInventoryStacked();
            if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < tileView.size())
            {
                info.itemPtr = tileView[absoluteIndex].itemPtr;
                info.count = tileView[absoluteIndex].totalCount;
                info.isValid = true;
            }
        }
    }

    return info;
}

bool game::loadMap(const std::string& mapId, int startX, int startY)
{
    if (map)
    {
        map->clearUnsafeItems(gridX, gridY);
    }

    if (mapCache.find(mapId) == mapCache.end())
    {
        gameMap newMap;
        if (!newMap.loadFromFile("data/maps/" + mapId + ".json")) return false;
        mapCache[mapId] = newMap;
    }

    map = &mapCache[mapId];
    gridX = startX;
    gridY = startY;

    map->updateDiscovery(gridX, gridY, 3);
    refreshActionGrid();

    eventBus::getInstance().publishEvent({ gameEvent::mapEntered, 0, mapId, nullptr });

    // Task 10.3: Auto-save trigger on map transitions
    if (settings.gameplay.autoSaveOnMapChange && playerEntity)
    {
        saveManager::saveAutosave(this, settings.gameplay.maxAutoSaves);
    }

    return true;
}

void game::movePlayer(int nextX, int nextY)
{
    if (!map || !map->isWalkable(nextX, nextY)) return;

    map->clearUnsafeItems(gridX, gridY);

    gridX = nextX;
    gridY = nextY;

    gameTime.advanceTime(2);
    map->updateDiscovery(gridX, gridY, 3);

    // Note: Map & Quest Triggers are presented as interactive buttons in the Action Grid
    // rather than abruptly hijacking player movement on step.

    // 3. Check Dynamic Encounters
    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
    int bonusDanger = (gameTime.getPhase() == TimePhase::NIGHT) ? 1 : 0;
    int dangerLevel = tileData.getEffectiveDangerLevel() + bonusDanger;

    float playerStealth = Player ? Player->getStat("agility") : 0.0f;

    if (dangerLevel > 0 && encounterResolver::shouldTriggerEncounter(dangerLevel, gameTime.getPhase(), playerStealth))
    {
        if (!tileData.persistentNPC)
        {
            tileData.persistentNPC = encounterResolver::createEncounterNPC(dangerLevel, settings);
        }
        triggerEncounter(tileData.persistentNPC);
        return;
    }

    activeTargetNPC = nullptr;
    activeTargetMode = TargetMode::NONE;
    refreshActionGrid();
}

void game::handleDropAction(int stackedIndex, int quantity)
{
    if (!Player) return;

    auto stackedView = Player->inventory.getStackedView();
    if (stackedIndex < 0 || static_cast<size_t>(stackedIndex) >= stackedView.size()) return;

    const auto& slotData = stackedView[stackedIndex];
    int actualDropCount = std::min(quantity, slotData.totalCount);
    std::string targetItemId = slotData.itemPtr->id;

    if (activeTargetNPC)
    {
        auto transferCopy = std::make_shared<item>(*slotData.itemPtr);
        transferCopy->count = actualDropCount;
        activeTargetNPC->inventory.addItem(transferCopy);
    }
    else
    {
        characterCreationState* cc = dynamic_cast<characterCreationState*>(getActiveState());
        if (!cc)
        {
            if (auto* inv = dynamic_cast<inventoryState*>(getActiveState()))
            {
                cc = dynamic_cast<characterCreationState*>(inv->getReturnState());
            }
        }

        if (cc)
        {
            auto droppedCopy = std::make_shared<item>(*slotData.itemPtr);
            droppedCopy->count = actualDropCount;
            cc->availableWardrobe.push_back(droppedCopy);
        }
        else if (map)
        {
            TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

            bool merged = false;
            if (slotData.itemPtr->isStackable)
            {
                for (auto& entry : tileData.droppedItems)
                {
                    if (entry.itemPtr && entry.itemPtr->id == targetItemId)
                    {
                        entry.itemPtr->count += actualDropCount;
                        entry.minutesRemaining = 120; // Refresh decay timer on merge
                        merged = true;
                        break;
                    }
                }
            }

            if (!merged)
            {
                auto droppedCopy = std::make_shared<item>(*slotData.itemPtr);
                droppedCopy->count = actualDropCount;
                tileData.addDroppedItem(droppedCopy, 120);
            }
        }
    }

    Player->inventory.removeItem(targetItemId, actualDropCount);
    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handlePickupAction(int groundIndex, int quantity)
{
    if (!Player) return;

    int actualGroundIdx = groundIndex;
    if (!activeTargetNPC)
    {
        auto tileView = getTileInventoryStacked();
        if (groundIndex >= 0 && static_cast<size_t>(groundIndex) < tileView.size())
        {
            actualGroundIdx = tileView[groundIndex].firstBackpackIndex;
        }
    }

    characterCreationState* cc = dynamic_cast<characterCreationState*>(getActiveState());
    if (!cc)
    {
        if (auto* inv = dynamic_cast<inventoryState*>(getActiveState()))
        {
            cc = dynamic_cast<characterCreationState*>(inv->getReturnState());
        }
    }

    if (activeTargetNPC)
    {
        auto npcView = activeTargetNPC->inventory.getStackedView();
        if (groundIndex < 0 || static_cast<size_t>(groundIndex) >= npcView.size()) return;

        const auto& slotData = npcView[groundIndex];
        int actualTakeCount = std::min(quantity, slotData.totalCount);
        std::string targetItemId = slotData.itemPtr->id;

        auto playerCopy = std::make_shared<item>(*slotData.itemPtr);
        playerCopy->count = actualTakeCount;
        Player->inventory.addItem(playerCopy);
        activeTargetNPC->inventory.removeItem(targetItemId, actualTakeCount);
    }
    else if (cc)
    {
        if (actualGroundIdx < 0 || static_cast<size_t>(actualGroundIdx) >= cc->availableWardrobe.size()) return;

        auto groundItem = cc->availableWardrobe[actualGroundIdx];
        if (!groundItem) return;

        int totalGroundCount = groundItem->isStackable ? groundItem->count : 1;
        int actualTakeCount = std::min(quantity, totalGroundCount);

        auto playerCopy = std::make_shared<item>(*groundItem);
        playerCopy->count = actualTakeCount;
        Player->inventory.addItem(playerCopy);

        if (groundItem->isStackable && totalGroundCount > actualTakeCount)
        {
            groundItem->count -= actualTakeCount;
        }
        else
        {
            cc->availableWardrobe.erase(cc->availableWardrobe.begin() + actualGroundIdx);
        }
    }
    else if (map)
    {
        TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
        if (actualGroundIdx < 0 || static_cast<size_t>(actualGroundIdx) >= tileData.droppedItems.size()) return;

        auto groundItem = tileData.droppedItems[actualGroundIdx].itemPtr;
        if (!groundItem) return;

        int totalGroundCount = groundItem->isStackable ? groundItem->count : 1;
        int actualTakeCount = std::min(quantity, totalGroundCount);

        auto playerCopy = std::make_shared<item>(*groundItem);
        playerCopy->count = actualTakeCount;
        Player->inventory.addItem(playerCopy);

        if (groundItem->isStackable && totalGroundCount > actualTakeCount)
        {
            groundItem->count -= actualTakeCount;
        }
        else
        {
            tileData.droppedItems.erase(tileData.droppedItems.begin() + actualGroundIdx);
        }
    }

    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handleEquipAction(int backpackIndex, equipSlot targetSlotOverride)
{
    if (!Player) return;

    int actualIdx = backpackIndex;
    auto stackedView = Player->inventory.getStackedView();
    if (backpackIndex >= 0 && static_cast<size_t>(backpackIndex) < stackedView.size())
    {
        actualIdx = stackedView[backpackIndex].firstBackpackIndex;
    }

    if (actualIdx < 0 || static_cast<size_t>(actualIdx) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[actualIdx];
    if (!targetItem || !targetItem->isEquippable) return;

    equipSlot slot = (targetSlotOverride != equipSlot::NONE) ? targetSlotOverride : targetItem->targetSlot;
    if (slot == equipSlot::NONE) return;

    std::vector<std::string> bodyTags = Player->anatomy.getAllTags();
    if (Player->inventory.equipItem(static_cast<size_t>(actualIdx), slot, bodyTags))
    {
        selectedInventoryIndex = -1;
        selectedEquipmentSlot = slot;
        refreshActionGrid();
    }
}

void game::handleEquipGroundAction(int groundIndex, equipSlot slot)
{
    if (!Player) return;

    size_t prevBackpackSize = Player->inventory.backpack.size();
    handlePickupAction(groundIndex, 1);

    if (Player->inventory.backpack.size() > prevBackpackSize)
    {
        int newBackpackIdx = static_cast<int>(Player->inventory.backpack.size()) - 1;
        handleEquipAction(newBackpackIdx, slot);
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    if (Player->inventory.unequipItem(slot))
    {
        characterCreationState* cc = dynamic_cast<characterCreationState*>(getActiveState());
        if (!cc)
        {
            if (auto* inv = dynamic_cast<inventoryState*>(getActiveState()))
            {
                cc = dynamic_cast<characterCreationState*>(inv->getReturnState());
            }
        }

        if (cc)
        {
            if (!Player->inventory.backpack.empty())
            {
                auto unequippedItem = Player->inventory.backpack.back();
                Player->inventory.backpack.pop_back();
                cc->availableWardrobe.push_back(unequippedItem);
            }
        }
        else
        {
            selectedEquipmentSlot = equipSlot::NONE;
            selectedInventoryIndex = static_cast<int>(Player->inventory.backpack.size()) - 1;
        }

        selectedEquipmentSlot = equipSlot::NONE;
        refreshActionGrid();
    }
}

void game::handleUseItemAction(int backpackIndex)
{
    if (!Player) return;

    int actualIdx = backpackIndex;
    auto stackedView = Player->inventory.getStackedView();
    if (backpackIndex >= 0 && static_cast<size_t>(backpackIndex) < stackedView.size())
    {
        actualIdx = stackedView[backpackIndex].firstBackpackIndex;
    }

    if (actualIdx < 0 || static_cast<size_t>(actualIdx) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[actualIdx];
    if (!targetItem || !targetItem->isConsumable) return;

    // Apply consumable stat effects
    float hpRestore = 0.0f;
    float mpRestore = 0.0f;

    for (const auto& mod : targetItem->statModifiers)
    {
        if (mod.statName == "health") hpRestore += mod.flatValue;
        else if (mod.statName == "mana") mpRestore += mod.flatValue;
        else Player->stats.modifyBaseStat(mod.statName, mod.flatValue);
    }

    // Heuristic fallbacks if item data has no explicit statModifiers
    if (hpRestore == 0.0f && targetItem->id.find("health") != std::string::npos) hpRestore = 50.0f;
    else if (hpRestore == 0.0f && targetItem->id.find("canis") != std::string::npos) hpRestore = 45.0f;
    if (mpRestore == 0.0f && targetItem->id.find("mana") != std::string::npos) mpRestore = 40.0f;

    if (hpRestore > 0.0f)
    {
        float maxHp = Player->getStat("max_health");
        float curHp = Player->getStat("health");
        Player->stats.setBaseStat("health", std::min(maxHp, curHp + hpRestore));
        std::cout << std::format("[Inventory] You consumed {} and restored {:.0f} HP.\n", targetItem->name, hpRestore);
    }
    if (mpRestore > 0.0f)
    {
        float maxMp = Player->getStat("max_mana");
        float curMp = Player->getStat("mana");
        Player->stats.setBaseStat("mana", std::min(maxMp, curMp + mpRestore));
        std::cout << std::format("[Inventory] You consumed {} and restored {:.0f} MP.\n", targetItem->name, mpRestore);
    }

    // Decrement item count or remove
    if (targetItem->isStackable && targetItem->count > 1)
    {
        targetItem->count--;
    }
    else
    {
        Player->inventory.backpack.erase(Player->inventory.backpack.begin() + actualIdx);
        selectedInventoryIndex = -1;
    }

    refreshActionGrid();
}

void game::handleLootAllAction()
{
    if (!Player) return;

    characterCreationState* cc = dynamic_cast<characterCreationState*>(getActiveState());
    if (!cc)
    {
        if (auto* inv = dynamic_cast<inventoryState*>(getActiveState()))
        {
            cc = dynamic_cast<characterCreationState*>(inv->getReturnState());
        }
    }

    if (cc)
    {
        for (const auto& it : cc->availableWardrobe)
        {
            if (it) Player->inventory.addItem(std::make_shared<item>(*it));
        }
        cc->availableWardrobe.clear();
    }
    else if (map)
    {
        TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
        for (const auto& entry : tileData.droppedItems)
        {
            if (entry.itemPtr) Player->inventory.addItem(std::make_shared<item>(*entry.itemPtr));
        }
        tileData.droppedItems.clear();
    }

    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handleUnequipAllAction()
{
    if (!Player) return;
    for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
    {
        equipSlot slot = static_cast<equipSlot>(s);
        if (Player->inventory.isEquipped(slot))
        {
            handleUnequipAction(slot);
        }
    }
    selectedEquipmentSlot = equipSlot::NONE;
    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handleStripToUnderwearAction()
{
    if (!Player) return;
    for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
    {
        equipSlot slot = static_cast<equipSlot>(s);
        if (!Player->inventory.isEquipped(slot)) continue;

        // Keep underwear / piercings / intimate items
        if (slot == equipSlot::CHEST_WEAR || slot == equipSlot::NIPPLES_WEAR ||
            slot == equipSlot::GROIN_OVER || slot == equipSlot::PENIS_WEAR ||
            slot == equipSlot::VAGINA_WEAR || slot == equipSlot::ASS_WEAR ||
            slot == equipSlot::PIERCING_EAR || slot == equipSlot::PIERCING_NOSE ||
            slot == equipSlot::PIERCING_LIP || slot == equipSlot::PIERCING_TONGUE ||
            slot == equipSlot::PIERCING_NIPPLE || slot == equipSlot::PIERCING_NAVEL ||
            slot == equipSlot::PIERCING_COCK || slot == equipSlot::PIERCING_VAGINA)
        {
            continue;
        }

        handleUnequipAction(slot);
    }
    selectedEquipmentSlot = equipSlot::NONE;
    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handleResetAllDisplacementsAction()
{
    if (!Player) return;
    Player->inventory.resetAllDisplacements();
    refreshActionGrid();
}

bool game::checkSingleCondition(const gameCondition& cond) const
{
    if (!Player) return false;

    if (cond.type == "HAS_ITEM")
    {
        int count = 0;
        for (const auto& item : Player->inventory.backpack)
        {
            if (item && item->id == cond.target)
            {
                count += item->isStackable ? item->count : 1;
            }
        }
        int req = cond.requiredValue > 0 ? cond.requiredValue : 1;
        return count >= req;
    }
    else if (cond.type == "QUEST_STAGE" || cond.type == "QUEST_FLAG")
    {
        return Player->quests.getQuestStage(cond.target) == cond.requiredValue;
    }
    else if (cond.type == "TIME_PHASE")
    {
        TimePhase currentPhase = gameTime.getPhase();
        std::string phaseStr = "DAY";
        if (currentPhase == TimePhase::NIGHT) phaseStr = "NIGHT";
        else if (currentPhase == TimePhase::DAWN) phaseStr = "DAWN";
        else if (currentPhase == TimePhase::DUSK) phaseStr = "DUSK";
        return phaseStr == cond.target;
    }
    else if (cond.type == "TIME_HOUR_BETWEEN")
    {
        int start = cond.minValue;
        int end = cond.maxValue;
        int currentHour = gameTime.hour;
        if (start <= end) return currentHour >= start && currentHour <= end;
        else return currentHour >= start || currentHour <= end;
    }
    else if (cond.type == "STAT_MIN")
    {
        float reqVal = cond.floatValue != 0.0f ? cond.floatValue : static_cast<float>(cond.requiredValue);
        return Player->getStat(cond.target) >= reqVal;
    }
    else if (cond.type == "STAT_MAX")
    {
        float reqVal = cond.floatValue != 0.0f ? cond.floatValue : static_cast<float>(cond.requiredValue);
        return Player->getStat(cond.target) <= reqVal;
    }
    else if (cond.type == "STAT_CHECK")
    {
        float val = Player->getStat(cond.target);
        if (cond.minValue != 0 || cond.maxValue != 0)
        {
            return val >= cond.minValue && val <= cond.maxValue;
        }
        float reqVal = cond.floatValue != 0.0f ? cond.floatValue : static_cast<float>(cond.requiredValue);
        return val >= reqVal;
    }
    else if (cond.type == "EQUIPPED_SLOT")
    {
        equipSlot slot = stringToEquipSlot(cond.target);
        if (slot == equipSlot::NONE) return false;
        if (!cond.stringValue.empty())
        {
            auto itemPtr = Player->inventory.getEquippedItem(slot);
            return itemPtr && itemPtr->id == cond.stringValue;
        }
        return Player->inventory.isEquipped(slot);
    }
    else if (cond.type == "HAS_TAG")
    {
        return Player->anatomy.hasGlobalTag(cond.target);
    }
    else if (cond.type == "HAS_MUTATION")
    {
        for (const auto& mut : Player->anatomy.activeMutations)
        {
            if (mut.id == cond.target) return true;
        }
        return false;
    }
    else if (cond.type == "IS_PREGNANT")
    {
        return Player->gestation.isPregnant;
    }
    else if (cond.type == "GENDER_IS")
    {
        std::string archStr = genderArchetypeToString(Player->anatomy.getGenderArchetype());
        return (archStr == cond.target || archStr == cond.stringValue);
    }
    else if (cond.type == "RACE_IS" || cond.type == "DOMINANT_RACE")
    {
        std::string domRace = Player->anatomy.getDominantRace();
        return (domRace == cond.target || domRace == cond.stringValue);
    }
    else if (cond.type == "DOMINANCE_BETWEEN")
    {
        float dom = Player->getStat("dominance");
        return dom >= cond.minValue && dom <= cond.maxValue;
    }

    return true;
}

bool game::checkConditions(const std::vector<conditionNode>& conditions)
{
    for (const auto& node : conditions)
    {
        if (!node.evaluate(this)) return false;
    }
    return true;
}

void game::pushScene(const std::string& sceneId)
{
    if (!currentScene.id.empty())
    {
        sceneStack.push_back(currentScene.id);
    }
    loadScene(sceneId);
}

void game::popScene()
{
    if (!sceneStack.empty())
    {
        std::string prevSceneId = sceneStack.back();
        sceneStack.pop_back();
        loadScene(prevSceneId);
    }
    else
    {
        // Task 10.3: Auto-save trigger on scene exit
        if (settings.gameplay.autoSaveOnSceneExit && playerEntity)
        {
            saveManager::saveAutosave(this, settings.gameplay.maxAutoSaves);
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        changeState(std::make_unique<explorationState>());
    }
}

void game::processChoice(const dialogueChoice& choice)
{
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        std::vector<std::shared_ptr<entity>> playerParty;
        if (playerEntity) playerParty.push_back(playerEntity);

        std::vector<std::shared_ptr<entity>> enemyParty;
        TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

        if (tileData.persistentNPC)
        {
            enemyParty.push_back(tileData.persistentNPC);
        }
        else if (activeTargetNPC)
        {
            enemyParty.push_back(activeTargetNPC);
        }

        changeState(std::make_unique<CombatState>(playerParty, enemyParty));
        return;
    }

    if (choice.nextSceneId == "ENCOUNTER_BRIBE")
    {
        if (Player && Player->getStat("currency") >= 25.0f)
        {
            Player->stats.modifyBaseStat("currency", -25.0f);
            TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
            tileData.persistentNPC = nullptr;
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        changeState(std::make_unique<explorationState>());
        return;
    }

    if (choice.nextSceneId == "ENCOUNTER_SURRENDER")
    {
        if (Player)
        {
            float currentMoney = Player->getStat("currency");
            Player->stats.modifyBaseStat("currency", -(currentMoney * 0.15f));
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        changeState(std::make_unique<explorationState>());
        return;
    }

    if (choice.nextSceneId == "VICTORY_INVENTORY")
    {
        changeState(std::make_unique<inventoryState>());
        return;
    }

    for (const auto& effect : choice.results)
    {
        processEffect(effect);
    }

    if (choice.nextSceneId == "POP_SCENE" || choice.nextSceneId == "RETURN")
    {
        popScene();
    }
    else if (choice.nextSceneId == "EXIT" || choice.nextSceneId.empty())
    {
        sceneStack.clear();

        // Task 10.3: Auto-save trigger on scene exit
        if (settings.gameplay.autoSaveOnSceneExit && playerEntity)
        {
            saveManager::saveAutosave(this, settings.gameplay.maxAutoSaves);
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        changeState(std::make_unique<explorationState>());
    }
    else
    {
        loadScene(choice.nextSceneId);
    }
}

void game::processEffect(const gameEffect& eff)
{
    if (!Player) return;

    if (eff.action == "TELEPORT")
    {
        std::string targetMapId = eff.target.empty() ? (map ? map->getId() : "overworld") : eff.target;
        int targetX = eff.x != 0 ? eff.x : eff.amount;
        int targetY = eff.y;
        loadMap(targetMapId, targetX, targetY);
    }
    else if (eff.action == "GIVE_ITEM")
    {
        int count = eff.amount > 0 ? eff.amount : 1;
        auto itemPtr = itemDatabase::getItem(eff.target);
        if (itemPtr)
        {
            itemPtr->count = count;
            Player->inventory.addItem(itemPtr);
        }
    }
    else if (eff.action == "REMOVE_ITEM")
    {
        int count = eff.amount > 0 ? eff.amount : 1;
        Player->inventory.removeItem(eff.target, count);
    }
    else if (eff.action == "TRANSFER_CURRENCY")
    {
        float amount = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        if (eff.target == "player" || eff.target.empty())
        {
            Player->stats.modifyBaseStat("currency", -amount);
            if (activeTargetNPC) activeTargetNPC->stats.modifyBaseStat("currency", amount);
        }
        else if (eff.target == "target" || eff.target == "npc")
        {
            if (activeTargetNPC) activeTargetNPC->stats.modifyBaseStat("currency", -amount);
            Player->stats.modifyBaseStat("currency", amount);
        }
    }
    else if (eff.action == "MODIFY_STAT" || eff.action == "ADD_STAT")
    {
        float delta = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        Player->stats.modifyBaseStat(eff.target, delta);
    }
    else if (eff.action == "TRANSFORM_PART" || eff.action == "TRANSFORM")
    {
        bodySlot slot = stringToBodySlot(eff.target);
        float sizeDelta = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        std::string race = !eff.stringVal.empty() ? eff.stringVal : eff.secondaryTarget;

        Player->anatomy.applyTransformation(slot, mutationType::GROWTH_LENGTH, sizeDelta, race, 10, "effect_transform");

        bodyPart* part = Player->anatomy.getPart(slot);
        if (part)
        {
            if (!race.empty()) part->race = race;
            if (!eff.extraString.empty()) part->covering = stringToCoveringType(eff.extraString);
        }
    }
    else if (eff.action == "FILL_FLUID")
    {
        float amount = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        bodySlot slot = stringToBodySlot(eff.secondaryTarget.empty() ? "BREASTS" : eff.secondaryTarget);

        if (Player->anatomy.hasOrifice(slot))
        {
            Player->anatomy.transferFluidToOrifice(slot, eff.target, amount);
        }
        else
        {
            bodyPart* part = Player->anatomy.getPart(slot);
            if (part) part->currentFluidMl = std::min(part->maxFluidMl, part->currentFluidMl + amount);
        }
    }
    else if (eff.action == "DRAIN_FLUID")
    {
        float amount = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        bodySlot slot = stringToBodySlot(eff.secondaryTarget.empty() ? "BREASTS" : eff.secondaryTarget);

        bodyPart* part = Player->anatomy.getPart(slot);
        if (part) part->currentFluidMl = std::max(0.0f, part->currentFluidMl - amount);
    }
    else if (eff.action == "STRETCH_ORIFICE")
    {
        float delta = eff.floatAmount != 0.0f ? eff.floatAmount : static_cast<float>(eff.amount);
        bodySlot slot = stringToBodySlot(eff.target);
        Player->anatomy.stretchOrifice(slot, delta);
    }
    else if (eff.action == "IMPREGNATE")
    {
        entity* mother = (eff.target == "player" || eff.target.empty()) ? Player : activeTargetNPC.get();
        std::string fatherId = !eff.secondaryTarget.empty() ? eff.secondaryTarget : "stranger";
        std::string fatherRace = !eff.stringVal.empty() ? eff.stringVal : "Human";
        int litter = eff.amount > 0 ? eff.amount : 1;

        if (mother && mother->anatomy.hasVagina())
        {
            mother->gestation.impregnate(fatherId, fatherId, fatherRace, mother->anatomy.getDominantRace(), litter);
        }
    }
    else if (eff.action == "INDUCE_BIRTH")
    {
        entity* mother = (eff.target == "player" || eff.target.empty()) ? Player : activeTargetNPC.get();
        if (mother && mother->gestation.isPregnant)
        {
            mother->gestation.giveBirth(mother->id);
        }
    }
    else if (eff.action == "DISPLACE_CLOTHING")
    {
        equipSlot slot = stringToEquipSlot(eff.target);
        DisplacementMode mode = stringToDisplacementMode(eff.stringVal);
        Player->inventory.setDisplacement(slot, mode);
    }
    else if (eff.action == "RESTORE_CLOTHING")
    {
        Player->inventory.resetAllDisplacements();
    }
    else if (eff.action == "SET_FLAG" || eff.action == "SET_QUEST" || eff.action == "SET_QUEST_STAGE")
    {
        Player->quests.setQuestStage(eff.target, eff.amount);
    }
    else if (eff.action == "CALL_SUB_SCENE")
    {
        std::string subScene = !eff.target.empty() ? eff.target : eff.stringVal;
        if (!subScene.empty())
        {
            pushScene(subScene);
        }
    }
    else if (eff.action == "RANDOM_BRANCH")
    {
        if (!eff.branches.empty())
        {
            std::vector<int> activeWeights = eff.weights;
            if (activeWeights.size() < eff.branches.size())
            {
                activeWeights.resize(eff.branches.size(), 1);
            }
            size_t chosenIdx = dice::rollWeighted(activeWeights);
            loadScene(eff.branches[chosenIdx]);
        }
    }
    else if (eff.action == "SPAWN_NPC")
    {
        int spawnX = eff.x != 0 ? eff.x : gridX;
        int spawnY = eff.y != 0 ? eff.y : gridY;
        auto npc = npcGenerator::generateFromTemplate(eff.target, &settings);
        if (npc && map)
        {
            map->getRuntimeData(spawnX, spawnY).persistentNPC = npc;
        }
    }
    else if (eff.action == "DESPAWN_NPC")
    {
        if (map)
        {
            map->getRuntimeData(gridX, gridY).persistentNPC = nullptr;
        }
        if (activeTargetNPC && activeTargetNPC->id == eff.target)
        {
            activeTargetNPC = nullptr;
        }
    }
}

void game::loadScene(const std::string& sceneId)
{
    changeState(std::make_unique<eventState>());
    currentScene = questDatabase::getScene(sceneId);

    currentScene.bodyText = textParser::interpolate(currentScene.bodyText, Player, activeTargetNPC.get());
    currentScene.speakerName = textParser::interpolate(currentScene.speakerName, Player, activeTargetNPC.get());

    activeButtons.clear();
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
        currentScene.choices[i].label = textParser::interpolate(currentScene.choices[i].label, Player, activeTargetNPC.get());

        if (checkConditions(currentScene.choices[i].requirements))
        {
            actionButton btn;
            btn.label = currentScene.choices[i].label;
            dialogueChoice choice = currentScene.choices[i];
            btn.onClick = [this, choice]() { processChoice(choice); };
            activeButtons.push_back(btn);
        }
    }
}

std::shared_ptr<entity> game::generateEncounterNPC()
{
    auto npc = npcGenerator::generateRandomNPC(&settings);
    if (npc) return npc;

    return std::make_shared<entity>("npc_fallback", "Alleyway Stranger");
}

void game::triggerEncounter(std::shared_ptr<entity> npc)
{
    if (!npc) return;

    activeTargetNPC = npc;
    activeTargetMode = TargetMode::COMBAT_ENEMY;

    currentScene = encounterResolver::buildEncounterScene(this, npc);
    changeState(std::make_unique<eventState>());
    refreshActionGrid();
}

std::string game::formatEquipSlotName(equipSlot slot)
{
    switch (slot)
    {
        case equipSlot::EYEWEAR:         return "Eyes";
        case equipSlot::HEADWEAR:        return "Head";
        case equipSlot::HAIR_WEAR:       return "Hair";
        case equipSlot::HORNS_SLOT:      return "Horns";
        case equipSlot::WEAPON_MAIN:     return "Main Hand";
        case equipSlot::WEAPON_OFF:      return "Off Hand";
        case equipSlot::MOUTHWEAR:       return "Mouth";
        case equipSlot::TORSO_OVER:      return "Over-torso";
        case equipSlot::NECKWEAR:        return "Neck";
        case equipSlot::WINGS_SLOT:      return "Wings";
        case equipSlot::WRISTS:          return "Wrists";
        case equipSlot::TORSO_UNDER:     return "Torso";
        case equipSlot::CHEST_WEAR:      return "Chest";
        case equipSlot::NIPPLES_WEAR:    return "Nipples";
        case equipSlot::HANDS:           return "Hands";
        case equipSlot::HIPS_WEAR:       return "Hips";
        case equipSlot::STOMACH_WEAR:    return "Stomach";
        case equipSlot::FINGER_PRIMARY:  return "Fingers";
        case equipSlot::ANKLES:          return "Ankles";
        case equipSlot::LEGS_OUTER:      return "Legs";
        case equipSlot::GROIN_OVER:      return "Groin";
        case equipSlot::TAIL_SLOT:       return "Tail";
        case equipSlot::CALVES:          return "Calves";
        case equipSlot::FEET:            return "Feet";
        case equipSlot::ASS_WEAR:        return "Anus";
        case equipSlot::PENIS_WEAR:      return "Penis";
        case equipSlot::VAGINA_WEAR:     return "Vagina";
        default:                         return "Equip";
    }
}

void game::clean()
{
    eventBus::getInstance().clearAllListeners();
}

void game::handleCommand(const UICommand& cmd)
{
    if (cmd.type == CommandType::OPEN_MAIN_MENU)
    {
        changeState(std::make_unique<mainMenuState>());
        return;
    }
    else if (cmd.type == CommandType::OPEN_SETTINGS)
    {
        changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS));
        return;
    }
    else if (cmd.type == CommandType::OPEN_CONTENT_OPTIONS)
    {
        changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS));
        return;
    }
    else if (cmd.type == CommandType::OPEN_INVENTORY)
    {
        if (auto cc = dynamic_cast<characterCreationState*>(activeGameState.get()))
        {
            auto curState = std::move(activeGameState);
            changeState(std::make_unique<inventoryState>(std::move(curState)));
            return;
        }
        changeState(std::make_unique<inventoryState>());
        return;
    }
    else if (cmd.type == CommandType::OPEN_SHOP)
    {
        changeState(std::make_unique<shopState>());
        return;
    }
    else if (cmd.type == CommandType::OPEN_TRANSFORMATION)
    {
        changeState(std::make_unique<transformationState>());
        return;
    }
    else if (cmd.type == CommandType::OPEN_LOAD_MENU)
    {
        changeState(std::make_unique<loadGameState>(SaveMenuMode::SAVE_AND_LOAD, std::make_unique<explorationState>()));
        return;
    }
    else if (cmd.type == CommandType::OPEN_PHONE)
    {
        if (dynamic_cast<phoneAppsState*>(activeGameState.get()))
        {
            changeState(std::make_unique<explorationState>());
            return;
        }
        changeState(std::make_unique<phoneAppsState>(PhoneAppMode::HOME));
        return;
    }
    else if (cmd.type == CommandType::QUICK_SAVE)
    {
        saveManager::saveNamedGame(this, "QuickSave");
        return;
    }
    else if (cmd.type == CommandType::QUICK_LOAD)
    {
        entity* p = getPlayer();
        std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
        std::string fileName = charName + "_QuickSave.json";
        if (saveManager::loadFromFile(this, fileName))
        {
            refreshActionGrid();
        }
        return;
    }
    else if (cmd.type == CommandType::TRIGGER_ACTION_BUTTON)
    {
        triggerActionButton(cmd.intPayload1);
        return;
    }
    else if (cmd.type == CommandType::PREVIOUS_ACTION_PAGE)
    {
        previousActionPage();
        return;
    }
    else if (cmd.type == CommandType::NEXT_ACTION_PAGE)
    {
        nextActionPage();
        return;
    }
    else if (cmd.type == CommandType::START_NEW_GAME)
    {
        playerEntity = nullptr;
        Player = nullptr;
        map = nullptr;
        isPhoneMenuOpen = false;
        currentScene = questScene();
        changeState(std::make_unique<characterCreationState>(0));
        return;
    }
    else if (cmd.type == CommandType::QUIT_GAME)
    {
        isRunning = false;
        return;
    }

    if (activeGameState)
    {
        activeGameState->handleCommand(this, cmd);
    }
}

std::vector<InventorySlot> game::getPlayerInventoryStacked() const
{
    if (!playerEntity) return {};
    return playerEntity->inventory.getStackedView();
}

std::vector<InventorySlot> game::getTileInventoryStacked() const
{
    characterCreationState* cc = dynamic_cast<characterCreationState*>(const_cast<game*>(this)->getActiveState());
    if (!cc)
    {
        if (auto* inv = dynamic_cast<inventoryState*>(const_cast<game*>(this)->getActiveState()))
        {
            cc = dynamic_cast<characterCreationState*>(inv->getReturnState());
        }
    }

    if (cc)
    {
        std::vector<InventorySlot> view;
        for (size_t i = 0; i < cc->availableWardrobe.size(); ++i)
        {
            const auto& itemPtr = cc->availableWardrobe[i];
            if (!itemPtr) continue;
            view.push_back(InventorySlot{ itemPtr, itemPtr->isStackable ? itemPtr->count : 1, static_cast<int>(i) });
        }
        std::sort(view.begin(), view.end(), [](const InventorySlot& a, const InventorySlot& b) {
            if (!a.itemPtr && !b.itemPtr) return false;
            if (!a.itemPtr) return false;
            if (!b.itemPtr) return true;
            return compareItemsNatural(*a.itemPtr, *b.itemPtr);
        });
        return view;
    }

    if (!map) return {};
    const TileRuntimeData& tileData = const_cast<gameMap*>(map)->getRuntimeData(gridX, gridY);
    std::vector<InventorySlot> view;
    for (size_t i = 0; i < tileData.droppedItems.size(); ++i)
    {
        const auto& entry = tileData.droppedItems[i];
        if (!entry.itemPtr) continue;
        view.push_back(InventorySlot{ entry.itemPtr, entry.itemPtr->isStackable ? entry.itemPtr->count : 1, static_cast<int>(i) });
    }
    std::sort(view.begin(), view.end(), [](const InventorySlot& a, const InventorySlot& b) {
        if (!a.itemPtr && !b.itemPtr) return false;
        if (!a.itemPtr) return false;
        if (!b.itemPtr) return true;
        return compareItemsNatural(*a.itemPtr, *b.itemPtr);
    });
    return view;
}