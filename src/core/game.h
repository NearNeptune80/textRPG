#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/eventBus.h"
#include "core/timeManager.h"
#include "entities/entity.h"
#include "input/inputHandler.h"
#include "map/gameMap.h"
#include "quest/questDatabase.h"
#include "settings/gameSettings.h"
#include "state/iGameState.h"
#include "core/actionSlot.h"

enum class TargetMode
{
    NONE,
    DIALOGUE,
    COMBAT_ENEMY,
    COMPANION
};

struct InventorySlotInfo
{
    std::shared_ptr<item> itemPtr = nullptr;
    int count = 0;
    bool isValid = false;
};

struct LogColor
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

struct EventLogEntry
{
    std::string tag;
    std::string text;
    LogColor color{ 220, 220, 220, 255 };
    std::string timeStr;
};

class game
{
public:
    game();
    ~game();

    void init();
    void handleEvents();
    void update(float deltaTime);
    void clean();

    void changeState(std::unique_ptr<iGameState> newState);
    iGameState* getActiveState() const { return activeGameState.get(); }

    GameSettings settings;
    timeManager gameTime;
    inputHandler input;
    bool isRunning{false};

    int selectedInventoryIndex = -1;
    int selectedInventorySide = 0;
    equipSlot selectedEquipmentSlot = equipSlot::NONE;
    int selectedEquipmentSide = 0; // 0 for player, 1 for target NPC

    gameMap* map = nullptr;
    std::shared_ptr<entity> playerEntity = nullptr;
    entity* Player = nullptr; // Pointer convenience alias to playerEntity.get()
    int gridX = 1, gridY = 1;

    std::unordered_map<std::string, gameMap> mapCache;
    std::vector<ActionSlot> activeActionSlots;
    questScene currentScene;
    bool isPhoneMenuOpen = false;

    std::vector<std::string> sceneStack;
    void pushScene(const std::string& sceneId);
    void popScene();

    std::shared_ptr<entity> activeTargetNPC = nullptr;
    TargetMode activeTargetMode = TargetMode::NONE;
    std::vector<std::shared_ptr<entity>> partyCompanions;

    int currentActionPage = 0;
    std::vector<std::string> actionCategories;
    std::string activeActionCategory = "";

    void setActionCategories(const std::vector<std::string>& categories, const std::string& defaultCategory = "");
    void clearActionCategories();
    void selectActionCategory(const std::string& category);
    const std::vector<std::string>& getActionCategories() const { return actionCategories; }
    const std::string& getActiveActionCategory() const { return activeActionCategory; }

    void triggerActionButton(int slotIndex);
    void previousActionPage();
    void nextActionPage();
    void handleCommand(const UICommand& cmd);

    const questScene& getCurrentScene() const { return currentScene; }
    entity* getPlayer() const { return playerEntity.get(); }
    std::shared_ptr<entity> getPlayerShared() const { return playerEntity; }
    entity* getActiveTargetNPC() const { return activeTargetNPC.get(); }
    std::shared_ptr<entity> getActiveTargetNPCShared() const { return activeTargetNPC; }
    const std::vector<std::shared_ptr<entity>>& getCompanions() const { return partyCompanions; }
    void addCompanion(std::shared_ptr<entity> comp) { partyCompanions.push_back(comp); }
    void removeCompanion(const std::string& compId) {
        std::erase_if(partyCompanions, [&](const auto& c){ return c && c->id == compId; });
    }
    const gameMap* getActiveMap() const { return map; }
    const std::vector<ActionSlot>& getActiveActionSlots() const { return activeActionSlots; }
    const timeManager& getTime() const { return gameTime; }
    std::vector<InventorySlot> getPlayerInventoryStacked() const;
    std::vector<InventorySlot> getTileInventoryStacked() const;

    bool loadMap(const std::string& mapId, int startX, int startY);
    void movePlayer(int nextX, int nextY);
    void refreshActionGrid();
    std::vector<std::shared_ptr<entity>> getTileNPCs() const;
    void ensureTileAmbushNPC(TileRuntimeData& tileData) const;
    void syncTileTarget(bool forceSelect = false);

    void handleDropAction(int stackedIndex, int quantity);
    void handlePickupAction(int groundIndex, int quantity);
    void handleEquipAction(int backpackIndex, equipSlot targetSlotOverride = equipSlot::NONE);
    void handleEquipGroundAction(int groundIndex, equipSlot slot = equipSlot::NONE);
    void handleUseItemAction(int backpackIndex);
    void handleUnequipAction(equipSlot slot);
    void handleLootAllAction();
    void handleUnequipAllAction();
    void handleStripToUnderwearAction();
    void handleResetAllDisplacementsAction();
    void handleUnequipNPCAction(equipSlot slot);
    void handleTakeNPCEquipmentAction(equipSlot slot);
    void handleStripAllNPCEquipmentAction();
    bool handleEquipOnNPCAction(int backpackIndex, equipSlot targetSlotOverride = equipSlot::NONE, entity* targetNpc = nullptr);
    static bool isEnslavingItem(const item* it);
    void handleTakeAllNPCItemsAction();
    void handleTakeNPCGoldAction();
    float getTileCurrency() const;
    void modifyTileCurrency(float delta);
    bool isCurrentTileStorageSafe() const;
    void transferCurrencyToSide1(float amount);
    void transferCurrencyFromSide1(float amount);
    bool checkSingleCondition(const gameCondition &cond) const;

    void loadScene(const std::string& sceneId, bool bypassWarning = false);
    void processChoice(const dialogueChoice& choice);
    void processEffect(const gameEffect &eff);
    bool checkConditions(const std::vector<conditionNode> &conditions);

    std::unordered_set<std::string> expandedDropdowns;
    void toggleDropdown(const std::string& dropdownId) {
        if (expandedDropdowns.contains(dropdownId)) {
            expandedDropdowns.erase(dropdownId);
        } else {
            expandedDropdowns.insert(dropdownId);
        }
    }
    bool isDropdownExpanded(const std::string& dropdownId) const {
        return expandedDropdowns.contains(dropdownId);
    }
    void clearDropdowns() { expandedDropdowns.clear(); }

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);
    void exploreTile();

    InventorySlotInfo getInventorySlotItem(int side, int absoluteIndex);
    std::string formatEquipSlotName(equipSlot slot);

    const std::vector<EventLogEntry>& getEventLog() const { return eventLog; }
    void addLogEntry(const std::string& tag, const std::string& text, LogColor color = { 220, 220, 220, 255 });
    void clearEventLog() { eventLog.clear(); }

    std::vector<std::string> pendingTransformationScenes;
    bool isPlayerOccupied() const;
    void triggerTransformationInterrupt(const std::string& triggerKey);
    void queueTransformationInterrupt(const std::string& triggerKey);
    bool hasPendingTransformationInterrupt() const { return !pendingTransformationScenes.empty(); }
    void triggerNextPendingTransformationInterrupt();

private:
    std::unique_ptr<iGameState> activeGameState;
    std::vector<std::pair<gameEvent, callbackID>> eventSubscriptions;
    std::vector<EventLogEntry> eventLog;
};