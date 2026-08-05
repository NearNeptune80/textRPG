#pragma once

#include <inplace_vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "core/timeManager.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "quest/questDatabase.h"
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
 * Manages simulation state, active maps, entities, and state transitions.
 * Stripped of all UI layout math, grid pagination, and pixel helpers.
 */
class game
{
public:
    game();
    ~game();

    void init(const char* title, int width, int height, bool fullscreen);
    void handleEvents();
    void update();
    void render();
    void clean();

    void changeState(std::unique_ptr<iGameState> newState);
    iGameState* getActiveState() const { return activeGameState.get(); }

    timeManager gameTime;
    bool isRunning;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Headless Item Selection State
    int selectedInventoryIndex = -1;
    equipSlot selectedEquipmentSlot = equipSlot::NONE;

    gameMap* map = nullptr;
    entity* Player = nullptr;
    int gridX = 1, gridY = 1;

    std::unordered_map<std::string, gameMap> mapCache;
    std::vector<actionButton> activeButtons;
    questScene currentScene;

    entity* activeTargetNPC = nullptr;
    TargetMode activeTargetMode = TargetMode::NONE;

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