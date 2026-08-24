#include "core/game.h"

#include <iostream>

#include "core/eventBus.h"
#include "core/textParser.h"
#include "entities/npcGenerator.h"
#include "events/gameEvents.h"
#include "items/itemDatabase.h"
#include "map/encounterResolver.h"
#include "quest/questDatabase.h"
#include "save/saveManager.h"
#include "settings/settingsManager.h"
#include "state/combatState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "ui/actionGridManager.h"

game::game() : isRunning(false), map(nullptr), playerEntity(nullptr), Player(nullptr), gridX(1), gridY(1) {}

game::~game()
{
    map = nullptr;
    playerEntity = nullptr;
    Player = nullptr;
}

void game::changeState(std::unique_ptr<iGameState> newState)
{
    if (activeGameState) activeGameState->onExit(this);
    activeGameState = std::move(newState);
    if (activeGameState)
    {
        activeGameState->initialise(this);
        activeGameState->onEnter(this);
    }
}

void game::init()
{
    settingsManager::loadFromFile(settings, "data/settings.json");

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        npcGenerator::loadTemplates("data/npc_templates.json");
        questDatabase::loadDatabase("data/quests");
    }

    if (!playerEntity)
    {
        playerEntity = std::make_shared<entity>("player_main", "Hero");
        Player = playerEntity.get();

        Player->stats.setBaseStat("health", 100.0f);
        Player->stats.setBaseStat("mana", 50.0f);
        Player->stats.setBaseStat("lust", 0.0f);
        Player->stats.setBaseStat("physique", 25.0f);
        Player->stats.setBaseStat("agility", 15.0f);
        Player->stats.setBaseStat("currency", 150.0f);
    }
    else
    {
        Player = playerEntity.get();
    }

    loadMap("overworld", 1, 1);

    if (!saveManager::loadFromFile(this, "Hero_Initial.json"))
    {
        std::cout << "[Init] Initial save not found. Generating default 'Hero_Initial.json'...\n";
        saveManager::saveNamedGame(this, "Initial");
    }

    changeState(std::make_unique<explorationState>());

    eventBus::getInstance().subscribe(gameEvent::timeAdvanced, [this](const eventData& data) {
        if (this->Player)
        {
            this->Player->anatomy.processMutations(data.numericValue);
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
    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

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
        else if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < static_cast<int>(tileData.droppedItems.size()))
        {
            info.itemPtr = tileData.droppedItems[absoluteIndex];
            info.count = info.itemPtr->isStackable ? info.itemPtr->count : 1;
            info.isValid = true;
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

    map->updateDiscovery(gridX, gridY);
    refreshActionGrid();

    eventBus::getInstance().publishEvent({ gameEvent::mapEntered, 0, mapId, nullptr });

    return true;
}

void game::movePlayer(int nextX, int nextY)
{
    if (!map || !map->isWalkable(nextX, nextY)) return;

    map->clearUnsafeItems(gridX, gridY);

    gridX = nextX;
    gridY = nextY;

    gameTime.advanceTime(2);
    map->updateDiscovery(gridX, gridY);

    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
    int bonusDanger = (gameTime.getPhase() == TimePhase::NIGHT) ? 1 : 0;
    int dangerLevel = tileData.getEffectiveDangerLevel() + bonusDanger;

    if (dangerLevel > 0)
    {
        int chance = std::min(100, dangerLevel * 20);
        if ((rand() % 100) < chance)
        {
            if (!tileData.persistentNPC) {
                tileData.persistentNPC = generateEncounterNPC();
            }
            triggerEncounter(tileData.persistentNPC);
            return;
        }
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

    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

    bool merged = false;
    if (slotData.itemPtr->isStackable)
    {
        for (auto& gItem : tileData.droppedItems)
        {
            if (gItem && gItem->id == targetItemId)
            {
                gItem->count += actualDropCount;
                merged = true;
                break;
            }
        }
    }

    if (!merged)
    {
        auto droppedCopy = std::make_shared<item>(*slotData.itemPtr);
        droppedCopy->count = actualDropCount;
        tileData.droppedItems.push_back(droppedCopy);
    }

    Player->inventory.removeItem(targetItemId, actualDropCount);
    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handlePickupAction(int groundIndex, int quantity)
{
    if (!Player) return;

    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
    if (groundIndex < 0 || static_cast<size_t>(groundIndex) >= tileData.droppedItems.size()) return;

    auto groundItem = tileData.droppedItems[groundIndex];
    if (!groundItem) return;

    int totalGroundCount = groundItem->isStackable ? groundItem->count : 1;
    int actualTakeCount = std::min(quantity, totalGroundCount);

    if (groundItem->isStackable)
    {
        bool mergedInBackpack = false;
        for (auto& bpItem : Player->inventory.backpack)
        {
            if (bpItem && bpItem->id == groundItem->id)
            {
                bpItem->count += actualTakeCount;
                mergedInBackpack = true;
                break;
            }
        }

        if (!mergedInBackpack)
        {
            auto playerCopy = std::make_shared<item>(*groundItem);
            playerCopy->count = actualTakeCount;
            Player->inventory.addItem(playerCopy);
        }

        groundItem->count -= actualTakeCount;
        if (groundItem->count <= 0)
        {
            tileData.droppedItems.erase(tileData.droppedItems.begin() + groundIndex);
        }
    }
    else
    {
        Player->inventory.addItem(groundItem);
        tileData.droppedItems.erase(tileData.droppedItems.begin() + groundIndex);
    }

    selectedInventoryIndex = -1;
    refreshActionGrid();
}

void game::handleEquipAction(int backpackIndex)
{
    if (!Player || backpackIndex < 0 || static_cast<size_t>(backpackIndex) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];
    if (!targetItem || !targetItem->isEquippable || targetItem->targetSlot == equipSlot::NONE) return;

    std::vector<std::string> bodyTags = Player->anatomy.getAllTags();
    if (Player->inventory.equipItem(static_cast<size_t>(backpackIndex), targetItem->targetSlot, bodyTags))
    {
        selectedInventoryIndex = -1;
        selectedEquipmentSlot = targetItem->targetSlot;
        refreshActionGrid();
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    if (Player->inventory.unequipItem(slot))
    {
        selectedEquipmentSlot = equipSlot::NONE;
        selectedInventoryIndex = static_cast<int>(Player->inventory.backpack.size()) - 1;
        refreshActionGrid();
    }
}

bool game::checkSingleCondition(const gameCondition& cond) const
{
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
        return count >= cond.requiredValue;
    }
    else if (cond.type == "QUEST_STAGE")
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
    else if (cond.type == "STAT_MIN")
    {
        return Player->getStat(cond.target) >= cond.requiredValue;
    }
    else if (cond.type == "HAS_TAG")
    {
        return Player->anatomy.hasGlobalTag(cond.target);
    }
    else if (cond.type == "DOMINANT_RACE")
    {
        return Player->anatomy.getDominantRace() == cond.target;
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

void game::processChoice(const dialogueChoice& choice)
{
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        std::vector<std::shared_ptr<entity>> playerParty;
        if (playerEntity)
        {
            playerParty.push_back(playerEntity);
        }

        std::vector<std::shared_ptr<entity>> enemyParty;
        TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

        if (tileData.persistentNPC)
        {
            enemyParty.push_back(tileData.persistentNPC);
        }
        else if (activeTargetNPC)
        {
            enemyParty.push_back(std::shared_ptr<entity>(std::shared_ptr<entity>(), activeTargetNPC));
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

    if (choice.nextSceneId == "EXIT" || choice.nextSceneId.empty())
    {
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
    if (eff.action == "SET_QUEST_STAGE")
    {
        Player->quests.setQuestStage(eff.target, eff.amount);
    }
    else if (eff.action == "MODIFY_STAT")
    {
        Player->stats.modifyBaseStat(eff.target, eff.amount);
    }
    else if (eff.action == "TRANSFORM")
    {
        Player->anatomy.applyTransformation(
            bodySlot::GROIN,
            mutationType::GROWTH_LENGTH,
            static_cast<float>(eff.amount),
            eff.target,
            10,
            "effect_transform"
        );
    }
}

void game::loadScene(const std::string& sceneId)
{
    changeState(std::make_unique<eventState>());
    currentScene = questDatabase::getScene(sceneId);

    currentScene.bodyText = textParser::interpolate(currentScene.bodyText, Player, activeTargetNPC);
    currentScene.speakerName = textParser::interpolate(currentScene.speakerName, Player, activeTargetNPC);

    activeButtons.clear();
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
        currentScene.choices[i].label = textParser::interpolate(currentScene.choices[i].label, Player, activeTargetNPC);

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
    auto npc = npcGenerator::generateRandomNPC();
    if (npc) return npc;

    return std::make_shared<entity>("npc_fallback", "Alleyway Stranger");
}

void game::triggerEncounter(std::shared_ptr<entity> npc)
{
    if (!npc) return;

    activeTargetNPC = npc.get();
    activeTargetMode = TargetMode::COMBAT_ENEMY;

    currentScene.id = "encounter_event";
    currentScene.speakerName = npc->name;
    currentScene.bodyText = "A " + npc->name + " steps out of the shadows and demands your attention! What will you do?";
    currentScene.choices.clear();

    dialogueChoice fightChoice;
    fightChoice.label = "Fight";
    fightChoice.nextSceneId = "ENCOUNTER_FIGHT";
    currentScene.choices.push_back(fightChoice);

    dialogueChoice payChoice;
    payChoice.label = "Bribe (25¤)";
    payChoice.nextSceneId = "ENCOUNTER_BRIBE";
    currentScene.choices.push_back(payChoice);

    dialogueChoice surrenderChoice;
    surrenderChoice.label = "Surrender";
    surrenderChoice.nextSceneId = "ENCOUNTER_SURRENDER";
    currentScene.choices.push_back(surrenderChoice);

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
    if (!map) return {};
    const TileRuntimeData& tileData = const_cast<gameMap*>(map)->getRuntimeData(gridX, gridY);
    std::vector<InventorySlot> view;
    for (size_t i = 0; i < tileData.droppedItems.size(); ++i)
    {
        const auto& itm = tileData.droppedItems[i];
        if (!itm) continue;
        view.push_back(InventorySlot{itm, itm->isStackable ? itm->count : 1, static_cast<int>(i)});
    }
    return view;
}