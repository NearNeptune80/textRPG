#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/timeManager.h"
#include "entities/entity.h"
#include "input/inputHandler.h"
#include "map/gameMap.h"
#include "quest/questDatabase.h"
#include "settings/gameSettings.h"
#include "state/iGameState.h"
#include "ui/actionButton.h"

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

/**
 * Headless Core Engine Controller.
 * Manages game simulation, active map states, entities, and state transitions.
 * Contains ZERO rendering pointers, window contexts, or UI presentation variables.
 */
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

    // Headless Simulation Selection State
    int selectedInventoryIndex = -1;
    int selectedInventorySide = 0;
    equipSlot selectedEquipmentSlot = equipSlot::NONE;

    gameMap* map = nullptr;
    std::shared_ptr<entity> playerEntity = nullptr;
    entity* Player = nullptr; // Pointer convenience alias to playerEntity.get()
    int gridX = 1, gridY = 1;

    std::unordered_map<std::string, gameMap> mapCache;
    std::vector<actionButton> activeButtons;
    questScene currentScene;

    entity* activeTargetNPC = nullptr;
    TargetMode activeTargetMode = TargetMode::NONE;

    // Command Dispatch
    void handleCommand(const UICommand& cmd);

    // Universal Snapshot & Read-Only Getters
    const questScene& getCurrentScene() const { return currentScene; }
    entity* getPlayer() const { return playerEntity.get(); }
    std::shared_ptr<entity> getPlayerShared() const { return playerEntity; }
    const gameMap* getActiveMap() const { return map; }
    const std::vector<actionButton>& getActiveActionButtons() const { return activeButtons; }
    const timeManager& getTime() const { return gameTime; }
    std::vector<InventorySlot> getPlayerInventoryStacked() const;
    std::vector<InventorySlot> getTileInventoryStacked() const;

    bool loadMap(const std::string& mapId, int startX, int startY);
    void movePlayer(int nextX, int nextY);
    void refreshActionGrid();

    void handleDropAction(int stackedIndex, int quantity);
    void handlePickupAction(int groundIndex, int quantity);
    void handleEquipAction(int backpackIndex);
    void handleUnequipAction(equipSlot slot);
    bool checkSingleCondition(const gameCondition &cond) const;

    void loadScene(const std::string& sceneId);
    void processChoice(const dialogueChoice& choice);
    void processEffect(const gameEffect &eff);
    bool checkConditions(const std::vector<conditionNode> &conditions);

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);

    InventorySlotInfo getInventorySlotItem(int side, int absoluteIndex);
    std::string formatEquipSlotName(equipSlot slot);

private:
    std::unique_ptr<iGameState> activeGameState;
};