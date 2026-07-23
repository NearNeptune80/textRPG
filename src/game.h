#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>

#include "gameMap.h"
#include "entity.h"
#include "itemDatabase.h"
#include "actionButton.h"
#include "questDatabase.h"
#include "timeManager.h"

enum class GameState
{
    EXPLORATION,
    INVENTORY,
    MAIN_MENU,
    EVENT
};

struct CachedTextTexture
{
    SDL_Texture* texture = nullptr;
    float w = 0.0f;
    float h = 0.0f;
};

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

    timeManager gameTime;
    bool isRunning;
    SDL_Window* window;
    SDL_Renderer* renderer;
    int selectedInventoryIndex = -1; // -1 means no item selected
    equipSlot selectedEquipmentSlot = equipSlot::NONE;

    gameMap* map;
    entity* Player;
    int gridX, gridY;

    GameState currentState;

    // UI & Text
    std::unordered_map<std::string, TTF_Font*> fonts;
    std::vector<actionButton> activeButtons;
    questScene currentScene;

    // Engine Core Helpers
    bool loadMap(const std::string& mapId, int startX, int startY);
    void movePlayer(int nextX, int nextY);
    bool loadFont(const std::string& id, const std::string& path, int ptSize);
    void renderTextCentered(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color = { 255, 255, 255, 255 });
    void renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color = { 255, 255, 255, 255 });
    void refreshActionGrid();
    void handleMouseClick(float windowX, float windowY);
    int getEquipmentGridIndex(equipSlot slot);
    void handleEquipAction(int backpackIndex);
    void handleUnequipAction(equipSlot slot);

    // Event & Quest Processing
    void loadScene(const std::string& sceneId);
    void processChoice(const dialogueChoice& choice);
    bool checkConditions(const std::vector<gameCondition>& conditions);

    // Layout Managers
    void renderDashboardLayout();
    void renderMainMenuLayout();

    // UI Widgets
    void renderMapPanel(SDL_Rect rect, int padding);
    void renderEquipmentPanel(SDL_Rect rect, int padding);
    void renderInventoryPanel(SDL_Rect rect);
    void renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3);
    void renderCompanionPanel(SDL_Rect rect);
    void renderTextPanel(SDL_Rect rect);
    void renderRightColumn(SDL_Rect top, SDL_Rect mid, SDL_Rect bot);
    void renderCharacterPanel(SDL_FRect rect, entity* playerObj);
    void renderActionGrid(SDL_FRect rect);
    void renderTimePanel(SDL_Rect rect);

    SDL_Texture* getOrRenderText(const std::string& text, const std::string& fontId, SDL_Color color, float& outW, float& outH);
    void clearTextCache(); // Call when changing scenes or fonts

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);
    void handleEncounterAction(const std::string& actionType);

private:
    std::unordered_map<std::string, CachedTextTexture> textCache;
    std::unordered_map<std::string, gameMap> mapCache;
};