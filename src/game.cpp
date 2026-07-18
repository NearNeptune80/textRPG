#include "game.h"
#include <iostream> // Needed for debug output

// Ensure game.h has 'entity* Player;' instead of 'Player'
game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), Player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    delete map;
    if (Player) delete Player;
}

void game::init(const char* title, int width, int height, bool fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;

    window = SDL_CreateWindow(title, width, height, (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
    renderer = SDL_CreateRenderer(window, NULL);

    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_STRETCH);

    map = new gameMap();
    map->updateDiscovery(gridX, gridY);

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        // Notice we removed 'entity*' here so it assigns to the class member, not a local variable
        Player = new entity("Player_1", "Oellanix");

        bodyPart WolfTail;
        WolfTail.id = "tail_wolf";
        WolfTail.name = "Fluffy Wolf Tail";
        WolfTail.race = "wolf";
        WolfTail.covering = "fur";
        WolfTail.color = "grey";
        WolfTail.tags = { "canine", "prehensile_false" };

        bodyPart DemonLegs;
        DemonLegs.id = "legs_demon";
        DemonLegs.name = "Demonic Digitigrade Legs";
        DemonLegs.race = "demon";
        DemonLegs.covering = "skin";
        DemonLegs.color = "crimson";
        DemonLegs.tags = { "bipedal", "digitigrade" };

        Player->anatomy.setPart(bodySlot::TAIL, WolfTail);
        Player->anatomy.setPart(bodySlot::LEGS, DemonLegs);

        std::cout << "Entity Created: " << Player->name << "\n";
        Player->anatomy.printDebug();

        Player->inventory.addItem(itemDatabase::getItem("item_canis_root"));
        Player->inventory.addItem(itemDatabase::getItem("item_leather_collar"));

        std::cout << "\n=== BACKPACK CONTENTS ===\n";
        for (const auto& bagItem : Player->inventory.backpack)
        {
            std::cout << "- " << bagItem.name << " [ID: " << bagItem.id << "]\n";
        }
        std::cout << "=========================\n";

        Player->stats.setStat("health", 68.0f);
        Player->stats.setStat("mana", 91.0f);
        Player->stats.setStat("lust", 100.0f);
        Player->stats.setStat("corruption", 0.0f);

        Player->stats.printDebug();
    }
    isRunning = true;
}

void game::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT) isRunning = false;

        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int newWidth = event.window.data1;
            int newHeight = event.window.data2;
            SDL_SetRenderLogicalPresentation(renderer, newWidth, newHeight, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        // --- NEW: Mouse Interaction ---
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                // SDL3 handles the logical scaling automatically for mouse coordinates!
                handleMouseClick(event.button.x, event.button.y);
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_I)
            {
                if (currentState == GameState::EXPLORATION)
                {
                    currentState = GameState::INVENTORY;
                }
                else if (currentState == GameState::INVENTORY)
                {
                    currentState = GameState::EXPLORATION;
                }
            }
            if (event.key.key == SDLK_M)
            {
                currentState = (currentState == GameState::MAIN_MENU) ? GameState::EXPLORATION : GameState::MAIN_MENU;
            }

            if (currentState == GameState::EXPLORATION)
            {
                int nextX = gridX, nextY = gridY;
                switch (event.key.key)
                {
                    case SDLK_UP:    nextY--; break;
                    case SDLK_DOWN:  nextY++; break;
                    case SDLK_LEFT:  nextX--; break;
                    case SDLK_RIGHT: nextX++; break;
                }
                if (map->isWalkable(nextX, nextY))
                {
                    gridX = nextX;
                    gridY = nextY;
                    map->updateDiscovery(gridX, gridY);
                }
            }
        }
    }
}

// --- NEW: Mouse Logic Routing ---
void game::handleMouseClick(float mouseX, float mouseY)
{
    // A simple test to verify it works. We will build the bounding box logic here next.
    std::cout << "Mouse Clicked at X: " << mouseX << " Y: " << mouseY << "\n";

    if (currentState == GameState::INVENTORY)
    {
        // Logic to check if an equipment slot or backpack item was clicked
    }
    else if (currentState == GameState::EXPLORATION)
    {
        // Logic to check if an action button was clicked
    }
}

void game::update() {}

void game::render()
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    if (currentState == GameState::MAIN_MENU)
    {
        renderMainMenuLayout();
    }
    else
    {
        renderDashboardLayout();
    }

    SDL_SetRenderViewport(renderer, NULL);
    SDL_RenderPresent(renderer);
}

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    // Updated signature
    bool success = SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);
    if (!success) return;

    // 1. Core Layout Math
    int padding = 12;
    int topBarH = (int)(h * 0.08f);
    int mapSize = (int)(h * 0.30f);

    int topRowY = padding;
    int colStartY = topRowY + topBarH + padding;
    int colEndY = h - padding;

    int leftColW = mapSize;
    int rightColW = mapSize;
    int centerColW = w - (leftColW + rightColW + (4 * padding));

    int leftX = padding;
    int centerX = leftX + leftColW + padding;
    int rightX = centerX + centerColW + padding;

    // 2. Define Slots
    int titleW = (w - (4 * padding)) / 3;
    SDL_Rect slotTitle1 = { padding, topRowY, titleW, topBarH };
    SDL_Rect slotTitle2 = { padding + titleW + padding, topRowY, titleW, topBarH };
    SDL_Rect slotTitle3 = { padding + (titleW + padding) * 2, topRowY, titleW, topBarH };

    SDL_Rect slotBottomLeft = { leftX, colEndY - mapSize, mapSize, mapSize };

    int leftAvailableH = (colEndY - mapSize - padding) - colStartY;
    int leftStackH = (leftAvailableH - padding) / 2;

    // Top Left slot for the character panel
    SDL_Rect slotTopLeft = { leftX, colStartY, leftColW, leftStackH };
    SDL_FRect fSlotTopLeft = { (float)leftX, (float)colStartY, (float)leftColW, (float)leftStackH };

    SDL_Rect slotMidLeft = { leftX, colStartY + leftStackH + padding, leftColW, leftAvailableH - leftStackH - padding };

    int btnH = (int)(h * 0.15f);
    SDL_Rect slotCenterMain = { centerX, colStartY, centerColW, (colEndY - btnH - padding) - colStartY };

    // Center bottom slot for the action grid
    SDL_FRect fSlotCenterBottom = { (float)centerX, (float)(colEndY - btnH), (float)centerColW, (float)btnH };

    int rightAvailableH = colEndY - colStartY;
    int rightStackH = (rightAvailableH - (2 * padding)) / 3;
    SDL_Rect slotTopRight = { rightX, colStartY, rightColW, rightStackH };
    SDL_Rect slotMidRight = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    SDL_Rect slotBotRight = { rightX, colStartY + (rightStackH + padding) * 2, rightColW, rightAvailableH - (rightStackH * 2 + padding * 2) };

    // 3. Inject Widgets based on State
    // Static widgets (Always show in dashboard)
    renderTitleBar(slotTitle1, slotTitle2, slotTitle3);

    // Replaced renderPCPanel with the actual Character Panel
    renderCharacterPanel(fSlotTopLeft, Player);

    renderCompanionPanel(slotMidLeft);

    // Replaced renderButtons with the Action Grid
    renderActionGrid(fSlotCenterBottom);

    // Dynamic widgets (Change based on state)
    switch (currentState)
    {
        case GameState::EXPLORATION:
            renderMapPanel(slotBottomLeft, padding);
            renderTextPanel(slotCenterMain);
            renderRightColumn(slotTopRight, slotMidRight, slotBotRight);
            break;

        case GameState::INVENTORY:
            renderEquipmentPanel(slotBottomLeft, padding);
            renderInventoryPanel(slotCenterMain);
            renderRightColumn(slotTopRight, slotMidRight, slotBotRight);
            break;

        default: break;
    }
}

void game::renderMainMenuLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    bool success = SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);
    if (!success) return;

    SDL_Rect menuRect = { w / 4, h / 4, w / 2, h / 2 };
    SDL_SetRenderViewport(renderer, &menuRect);
    SDL_SetRenderDrawColor(renderer, 20, 60, 80, 255);
    SDL_RenderFillRect(renderer, NULL);
}

// --- UI WIDGETS ---

void game::renderMapPanel(SDL_Rect rect, int padding)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, NULL);

    int tileGap = 2;
    int availableForTiles = rect.w - (2 * padding) - (4 * tileGap);
    int drawnTileSize = availableForTiles / 5;
    int totalGridSize = (drawnTileSize * 5) + (tileGap * 4);

    int offsetX = (rect.w - totalGridSize) / 2;
    int offsetY = (rect.h - totalGridSize) / 2;

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            int mapX = gridX + x;
            int mapY = gridY + y;

            if (mapX >= 0 && mapX < gameMap::WIDTH && mapY >= 0 && mapY < gameMap::HEIGHT)
            {
                Tile t = map->getTile(mapX, mapY);
                if (t.discovery == STATE_HIDDEN) continue;

                int renderX = x + 2;
                int renderY = y + 2;

                SDL_FRect r = {
                    (float)(offsetX + (renderX * (drawnTileSize + tileGap))),
                    (float)(offsetY + (renderY * (drawnTileSize + tileGap))),
                    (float)drawnTileSize,
                    (float)drawnTileSize
                };

                if (t.discovery == STATE_PARTIAL) SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                else if (t.type == TILE_FLOOR) SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                else SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    SDL_FRect p = {
        (float)(offsetX + (2 * (drawnTileSize + tileGap))),
        (float)(offsetY + (2 * (drawnTileSize + tileGap))),
        (float)drawnTileSize,
        (float)drawnTileSize
    };
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderFillRect(renderer, &p);
}

void game::renderEquipmentPanel(SDL_Rect rect, int padding)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 25, 20, 30, 255);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 100, 50, 150, 255);
    SDL_FRect border = { (float)padding, (float)padding, (float)rect.w - (padding * 2), (float)rect.h - (padding * 2) };
    SDL_RenderRect(renderer, &border);

    int cols = 6;
    int rows = 6;
    int slotGap = 4;
    int internalPadding = padding + 6;

    int availableW = rect.w - (2 * internalPadding) - ((cols - 1) * slotGap);
    int availableH = rect.h - (2 * internalPadding) - ((rows - 1) * slotGap);

    int slotSize = std::min(availableW / cols, availableH / rows);

    int gridW = (slotSize * cols) + (slotGap * (cols - 1));
    int gridH = (slotSize * rows) + (slotGap * (rows - 1));

    int offsetX = (rect.w - gridW) / 2;
    int offsetY = (rect.h - gridH) / 2;

    SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255);
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect slot = {
                (float)(offsetX + (c * (slotSize + slotGap))),
                (float)(offsetY + (r * (slotSize + slotGap))),
                (float)slotSize,
                (float)slotSize
            };
            SDL_RenderFillRect(renderer, &slot);
            SDL_SetRenderDrawColor(renderer, 60, 55, 65, 255);
            SDL_RenderRect(renderer, &slot);
            SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255);
        }
    }
}

void game::renderInventoryPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderLine(renderer, rect.w / 2, 20, rect.w / 2, rect.h - 20);

    int cols = 6;
    int rows = 5;
    int slotGap = 4;

    int halfWidth = rect.w / 2;
    int sidePadding = 20;
    int topPadding = 60;

    int availableW = halfWidth - (2 * sidePadding) - ((cols - 1) * slotGap);
    int availableH = rect.h - topPadding - sidePadding - ((rows - 1) * slotGap);

    int slotSize = std::min(availableW / cols, availableH / rows);

    int gridW = (slotSize * cols) + (slotGap * (cols - 1));
    int gridH = (slotSize * rows) + (slotGap * (rows - 1));

    int leftOffsetX = (halfWidth - gridW) / 2;
    int gridOffsetY = topPadding;

    int rightOffsetX = halfWidth + (halfWidth - gridW) / 2;

    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect slot = {
                (float)(leftOffsetX + (c * (slotSize + slotGap))),
                (float)(gridOffsetY + (r * (slotSize + slotGap))),
                (float)slotSize,
                (float)slotSize
            };
            SDL_RenderFillRect(renderer, &slot);
        }
    }

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect slot = {
                (float)(rightOffsetX + (c * (slotSize + slotGap))),
                (float)(gridOffsetY + (r * (slotSize + slotGap))),
                (float)slotSize,
                (float)slotSize
            };
            SDL_RenderFillRect(renderer, &slot);
        }
    }
}

void game::renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3)
{
    SDL_Rect boxes[3] = { t1, t2, t3 };
    for (int i = 0; i < 3; i++)
    {
        SDL_SetRenderViewport(renderer, &boxes[i]);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, NULL);
    }
}

void game::renderCompanionPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderTextPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderRightColumn(SDL_Rect top, SDL_Rect mid, SDL_Rect bot)
{
    SDL_Rect boxes[3] = { top, mid, bot };
    for (int i = 0; i < 3; i++)
    {
        SDL_SetRenderViewport(renderer, &boxes[i]);
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, NULL);
    }
}

// Ensure game.h signature is updated to: void renderCharacterPanel(SDL_FRect rect, entity* Player);
void game::renderCharacterPanel(SDL_FRect rect, entity* PlayerObj)
{
    // Important: Remove any set viewport commands inside this function if it exists, 
    // because SDL_FRect positioning in our dashboard expects absolute positioning over the whole screen.

    SDL_SetRenderViewport(renderer, NULL); // Reset viewport

    SDL_SetRenderDrawColor(renderer, 30, 28, 35, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 60, 55, 65, 255);
    SDL_RenderRect(renderer, &rect);

    if (!PlayerObj) return;

    float padding = 15.0f;
    float barWidth = rect.w - (padding * 2);
    float barHeight = 12.0f;
    float startY = 80.0f;
    float barSpacing = 25.0f;

    auto drawBar = [&](float y, float current, float max, Uint8 r, Uint8 g, Uint8 b)
        {
            SDL_FRect bgRect = { rect.x + padding, rect.y + y, barWidth, barHeight };
            SDL_SetRenderDrawColor(renderer, 20, 18, 25, 255);
            SDL_RenderFillRect(renderer, &bgRect);

            float fillPercentage = std::clamp(current / max, 0.0f, 1.0f);
            SDL_FRect fillRect = { rect.x + padding, rect.y + y, barWidth * fillPercentage, barHeight };
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderFillRect(renderer, &fillRect);
        };

    drawBar(startY, PlayerObj->stats.getStat("health"), 100.0f, 220, 50, 70);
    drawBar(startY + barSpacing, PlayerObj->stats.getStat("mana"), 100.0f, 150, 80, 220);
    drawBar(startY + (barSpacing * 2), PlayerObj->stats.getStat("lust"), 100.0f, 255, 50, 150);
}

// Ensure game.h signature is updated to: void renderActionGrid(SDL_FRect rect);
void game::renderActionGrid(SDL_FRect rect)
{
    SDL_SetRenderViewport(renderer, NULL); // Reset viewport

    SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderLine(renderer, rect.x, rect.y, rect.x + rect.w, rect.y);

    int cols = 5;
    int rows = 3;
    float gap = 8.0f;
    float verticalPadding = 15.0f;
    float horizontalPadding = 40.0f;

    float availableW = rect.w - (horizontalPadding * 2) - (gap * (cols - 1));
    float availableH = rect.h - (verticalPadding * 2) - (gap * (rows - 1));

    float btnWidth = availableW / cols;
    float btnHeight = availableH / rows;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect btn = {
                rect.x + horizontalPadding + (c * (btnWidth + gap)),
                rect.y + verticalPadding + (r * (btnHeight + gap)),
                btnWidth,
                btnHeight
            };

            SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
            SDL_RenderFillRect(renderer, &btn);

            SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
            SDL_RenderRect(renderer, &btn);
        }
    }
}

void game::clean()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}