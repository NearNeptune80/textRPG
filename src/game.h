// In src/game.h
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
#include "viewportGuard.h" // Adopt ViewportGuard everywhere!
#include "uiRenderer.h"
#include "saveManager.h"

enum class GameState
{
    EXPLORATION,
    INVENTORY,
    MAIN_MENU,
    EVENT
};

struct ColorToken
{
    std::string text;
    SDL_Color color;
};

struct CachedTextTexture
{
    SDL_Texture* texture = nullptr;
    float w = 0.0f;
    float h = 0.0f;
};

// Cached layout bounds to eliminate duplicated coordinate math in clicks
struct DashboardLayout
{
    SDL_Rect mapRect;
    SDL_Rect timeRect;
    SDL_Rect charRect;
    SDL_Rect companionRect;
    SDL_Rect textMainRect;
    SDL_FRect actionGridRect;
    SDL_Rect equipRect;
    SDL_Rect inventoryRect;
};

enum class TargetMode
{
    NONE,
    DIALOGUE,
    COMBAT_ENEMY,
    COMPANION
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
    int selectedInventoryIndex = -1;
    equipSlot selectedEquipmentSlot = equipSlot::NONE;

    gameMap* map;
    entity* Player;
    int gridX, gridY;

    GameState currentState;
    DashboardLayout layout; // Holds calculated screen panel bounds
    std::unordered_map<std::string, gameMap> mapCache;

    // UI & Text
    std::unordered_map<std::string, TTF_Font*> fonts;
    std::vector<actionButton> activeButtons;
    questScene currentScene;

    // Engine Core Helpers
    bool loadMap(const std::string& mapId, int startX, int startY);
    void movePlayer(int nextX, int nextY);
    bool loadFont(const std::string& id, const std::string& path, int ptSize);

    // Shared Text Renderers
    void drawTextFit(const std::string& textStr, SDL_FRect destRect, SDL_Color color, const std::string& fontId = "button_font");
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

    // Layout Managers & UI Widgets
    void updateLayoutBounds(int w, int h);
    void renderDashboardLayout();
    void renderMainMenuLayout();

    void renderMapPanel(SDL_Rect rect, int padding);
    void renderEquipmentPanel(SDL_Rect rect, int padding, entity* targetEntity = nullptr);
    void renderInventoryPanel(SDL_Rect rect);
    void renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3);
    void renderCompanionPanel(SDL_Rect rect);
    void renderTextPanel(SDL_Rect rect);
    void renderRightColumn(SDL_Rect top, SDL_Rect mid, SDL_Rect bot);
    void renderCharacterPanel(SDL_FRect rect, entity* playerObj);
    void renderActionGrid(SDL_FRect rect);
    void renderTimePanel(SDL_Rect rect);

    float renderTextLeftSegment(const std::vector<ColorToken>& tokens, float startX, float startY, float maxH, const std::string& fontId);
    void renderAnatomyTooltip(float mouseX, float mouseY);

    SDL_Texture* getOrRenderText(const std::string& text, const std::string& fontId, SDL_Color color, float& outW, float& outH);
    void clearTextCache();

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);

    // Active NPC Target Tracking
    entity* activeTargetNPC = nullptr;
    TargetMode activeTargetMode = TargetMode::NONE;

    // UI Rendering Helper
    void renderNPCTargetPanel(float x, float y, float w, float h);
    void renderNPCAnatomyTooltip(float mouseX, float mouseY);

private:
    std::unordered_map<std::string, CachedTextTexture> textCache;
};