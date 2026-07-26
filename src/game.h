#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <functional>

#include "gameMap.h"
#include "entity.h"
#include "itemDatabase.h"
#include "actionButton.h"
#include "questDatabase.h"
#include "timeManager.h"
#include "viewportGuard.h"
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

struct DashboardLayout
{
    SDL_FRect mapRect;
    SDL_FRect timeRect;
    SDL_FRect charRect;
    SDL_FRect companionRect;
    SDL_FRect textMainRect;
    SDL_FRect actionGridRect;
    SDL_FRect equipRect;

    // Inventory Split Bounds
    SDL_FRect inventoryGridRect;
    SDL_FRect inventoryDetailRect;

    // Header Title Boxes
    SDL_FRect titleBox1;
    SDL_FRect titleBox2;
    SDL_FRect titleBox3;

    // Right Column Stack
    SDL_FRect rightStackTop;
    SDL_FRect rightStackMid;
    SDL_FRect rightStackBot;

    // Hover Bounds
    SDL_FRect playerAvatarRect;
    SDL_FRect targetAvatarRect;
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
    int selectedInventorySide = 0;
    int selectedInventoryIndex = -1;
    equipSlot selectedEquipmentSlot = equipSlot::NONE;

    gameMap* map;
    entity* Player;
    int gridX, gridY;

    int actionGridPage = 0; // Current active page in the action grid

    GameState currentState;
    DashboardLayout layout;
    std::unordered_map<std::string, gameMap> mapCache;

    int currentInventoryPage = 0; // 0..5 = Pages I to VI, 6 = Key Items
    int currentRightInventoryPage = 0; // Right Grid Page (0..5 = Pages I-VI, 6 = Key)
    float descriptionScrollY = 0.0f;
    float maxDescriptionScrollY = 0.0f;

    std::unordered_map<std::string, TTF_Font*> fonts;
    std::vector<actionButton> activeButtons;
    questScene currentScene;

    bool loadMap(const std::string& mapId, int startX, int startY);
    void movePlayer(int nextX, int nextY);
    bool loadFont(const std::string& id, const std::string& path, int ptSize);

    void drawTextFit(const std::string& textStr, SDL_FRect destRect, SDL_Color color, const std::string& fontId = "button_font");
    void renderTextCentered(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color = { 255, 255, 255, 255 });
    void renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color = { 255, 255, 255, 255 });

    void refreshActionGrid();
    void handleMouseClick(float windowX, float windowY);
    int getEquipmentGridIndex(equipSlot slot);
    void handleDropAction(int stackedIndex, int quantity);
    void handlePickupAction(int groundIndex, int quantity);
    void handleEquipAction(int backpackIndex);
    void handleUnequipAction(equipSlot slot);

    void loadScene(const std::string& sceneId);
    void processChoice(const dialogueChoice& choice);
    bool checkConditions(const std::vector<gameCondition>& conditions);

    void updateLayoutBounds(int w, int h);
    void renderDashboardLayout();
    void renderMainMenuLayout();

    void renderTitleBar(SDL_FRect t1, SDL_FRect t2, SDL_FRect t3);
    void renderCompanionPanel(SDL_FRect rect);
    void renderTextPanel(SDL_FRect rect);
    void renderRightColumn(SDL_FRect top, SDL_FRect mid, SDL_FRect bot);

    float renderTextLeftSegment(const std::vector<ColorToken>& tokens, float startX, float startY, float maxH, const std::string& fontId);

    SDL_Texture* getOrRenderText(const std::string& text, const std::string& fontId, SDL_Color color, float& outW, float& outH);
    void clearTextCache();

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);

    std::array<actionButton, 15> getSlotsForCurrentActionPage();

    entity* activeTargetNPC = nullptr;
    TargetMode activeTargetMode = TargetMode::NONE;

private:
    std::unordered_map<std::string, CachedTextTexture> textCache;
};