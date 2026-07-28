#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <array>
#include <utility>
#include <inplace_vector>

#include "gameMap.h"
#include "entity.h"
#include "actionButton.h"
#include "questDatabase.h"
#include "timeManager.h"
#include "viewportGuard.h"

enum class GameState
{
    EXPLORATION,
    INVENTORY,
    MAIN_MENU,
    EVENT
};

enum class TargetMode
{
    NONE,
    DIALOGUE,
    COMBAT_ENEMY,
    COMPANION
};

enum class TextAlignment
{
    CENTER,
    LEFT,
    RIGHT
};

struct ColorToken
{
    std::string text;
    SDL_Color color;
};

struct TextCacheKey
{
    std::string fontId;
    std::string textStr;
    SDL_Color color;

    bool operator==(const TextCacheKey& other) const noexcept
    {
        return fontId == other.fontId &&
               textStr == other.textStr &&
               color.r == other.color.r &&
               color.g == other.color.g &&
               color.b == other.color.b &&
               color.a == other.color.a;
    }
};

struct TextCacheKeyHash
{
    std::size_t operator()(const TextCacheKey& k) const noexcept
    {
        std::size_t h1 = std::hash<std::string>{}(k.fontId);
        std::size_t h2 = std::hash<std::string>{}(k.textStr);
        uint32_t colorPacked = (static_cast<uint32_t>(k.color.r) << 24) |
                               (static_cast<uint32_t>(k.color.g) << 16) |
                               (static_cast<uint32_t>(k.color.b) << 8)  |
                                static_cast<uint32_t>(k.color.a);
        return h1 ^ (h2 << 1) ^ std::hash<uint32_t>{}(colorPacked);
    }
};

struct CachedTextTexture
{
    SDL_Texture* texture = nullptr;
    float w = 0.0f;
    float h = 0.0f;
};

struct InventorySlotInfo
{
    std::shared_ptr<item> itemPtr = nullptr;
    int count = 0;
    bool isValid = false;
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

    SDL_FRect inventoryGridRect;
    SDL_FRect inventoryDetailRect;

    SDL_FRect titleBox1;
    SDL_FRect titleBox2;
    SDL_FRect titleBox3;

    SDL_FRect rightStackTop;
    SDL_FRect rightStackMid;
    SDL_FRect rightStackBot;

    SDL_FRect playerAvatarRect;
    SDL_FRect targetAvatarRect;
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

    int actionGridPage = 0;
    GameState currentState;
    DashboardLayout layout;
    std::unordered_map<std::string, gameMap> mapCache;

    int currentInventoryPage = 0;
    int currentRightInventoryPage = 0;
    float descriptionScrollY = 0.0f;
    float maxDescriptionScrollY = 0.0f;

    std::unordered_map<std::string, TTF_Font*> fonts;
    std::inplace_vector<actionButton, 15> activeButtons;
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

    void loadScene(const std::string& sceneId);
    void processChoice(const dialogueChoice& choice);
    bool checkConditions(const std::vector<gameCondition>& conditions);

    std::shared_ptr<entity> generateEncounterNPC();
    void triggerEncounter(std::shared_ptr<entity> npc);
    std::array<actionButton, 15> getSlotsForCurrentActionPage();

    InventorySlotInfo getInventorySlotItem(int side, int absoluteIndex);
    std::pair<equipSlot, std::shared_ptr<item>> getEquippedAtGridIndex(entity* target, int gridIdx);

    bool loadFont(const std::string& id, const std::string& path, int ptSize);

    void renderTextAligned(const std::string& textStr, SDL_FRect destRect,
        TextAlignment align = TextAlignment::CENTER,
        bool fitToBox = true,
        const std::string& fontId = "button_font",
        SDL_Color color = { 255, 255, 255, 255 });

    void renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color = { 255, 255, 255, 255 });
    float renderTextLeftSegment(const std::vector<ColorToken>& tokens, float startX, float startY, float maxH, const std::string& fontId);

    SDL_Texture* getOrRenderText(const std::string& text, const std::string& fontId, SDL_Color color, float& outW, float& outH);
    void clearTextCache();

    void updateLayoutBounds(int w, int h);
    int getEquipmentGridIndex(equipSlot slot);
    std::string formatEquipSlotName(equipSlot slot);

private:
    std::unordered_map<TextCacheKey, CachedTextTexture, TextCacheKeyHash> textCache;

    void renderDashboardLayout();
    void renderMainMenuLayout();

    void renderTitleBar(SDL_FRect t1, SDL_FRect t2, SDL_FRect t3);
    void renderCompanionPanel(SDL_FRect rect);
    void renderTextPanel(SDL_FRect rect);
    void renderRightColumn(SDL_FRect top, SDL_FRect mid, SDL_FRect bot);
};