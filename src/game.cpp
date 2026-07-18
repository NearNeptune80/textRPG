#include "game.h"

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    delete map;
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
        // 1. Declare the object with a capital letter, using the lowercase class
        entity* Player = new entity("player_1", "Oellanix");

        // 2. Declare part objects with capital letters, using the lowercase struct
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

        // 3. Set them
        Player->anatomy.setPart(bodySlot::TAIL, WolfTail);
        Player->anatomy.setPart(bodySlot::LEGS, DemonLegs);

        // 4. Print debug
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

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            // State Toggles
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

            // Map Movement (Only process if exploring)
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

// --- LAYOUT MANAGERS ---

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);

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
    SDL_Rect slotTopLeft = { leftX, colStartY, leftColW, leftStackH };
    SDL_Rect slotMidLeft = { leftX, colStartY + leftStackH + padding, leftColW, leftAvailableH - leftStackH - padding };

    int btnH = (int)(h * 0.15f);
    SDL_Rect slotCenterMain = { centerX, colStartY, centerColW, (colEndY - btnH - padding) - colStartY };
    SDL_Rect slotCenterBottom = { centerX, colEndY - btnH, centerColW, btnH };

    int rightAvailableH = colEndY - colStartY;
    int rightStackH = (rightAvailableH - (2 * padding)) / 3;
    SDL_Rect slotTopRight = { rightX, colStartY, rightColW, rightStackH };
    SDL_Rect slotMidRight = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    SDL_Rect slotBotRight = { rightX, colStartY + (rightStackH + padding) * 2, rightColW, rightAvailableH - (rightStackH * 2 + padding * 2) };

    // 3. Inject Widgets based on State
    // Static widgets (Always show in dashboard)
    renderTitleBar(slotTitle1, slotTitle2, slotTitle3);
    renderPCPanel(slotTopLeft);
    renderCompanionPanel(slotMidLeft);
    renderButtons(slotCenterBottom);

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
            renderRightColumn(slotTopRight, slotMidRight, slotBotRight); // Can swap these later if needed
            break;

        default: break;
    }
}

void game::renderMainMenuLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);

    // Simple placeholder for main menu
    SDL_Rect menuRect = { w / 4, h / 4, w / 2, h / 2 };
    SDL_SetRenderViewport(renderer, &menuRect);
    SDL_SetRenderDrawColor(renderer, 20, 60, 80, 255); // Distinct blueish color
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

    // Purple border
    SDL_SetRenderDrawColor(renderer, 100, 50, 150, 255);
    SDL_FRect border = { (float)padding, (float)padding, (float)rect.w - (padding * 2), (float)rect.h - (padding * 2) };
    SDL_RenderRect(renderer, &border);

    // --- 4x6 Equipment Grid Math ---
    int cols = 6;
    int rows = 6;
    int slotGap = 4;

    // We add extra internal padding so the slots don't touch the purple border
    int internalPadding = padding + 6;

    int availableW = rect.w - (2 * internalPadding) - ((cols - 1) * slotGap);
    int availableH = rect.h - (2 * internalPadding) - ((rows - 1) * slotGap);

    // The slot size is constrained by whichever is tighter: width or height
    int slotSize = std::min(availableW / cols, availableH / rows);

    int gridW = (slotSize * cols) + (slotGap * (cols - 1));
    int gridH = (slotSize * rows) + (slotGap * (rows - 1));

    int offsetX = (rect.w - gridW) / 2;
    int offsetY = (rect.h - gridH) / 2;

    // Draw the empty slots
    SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255); // Darker slot background
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

            // Optional: Draw a subtle border around each slot
            SDL_SetRenderDrawColor(renderer, 60, 55, 65, 255);
            SDL_RenderRect(renderer, &slot);
            SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255); // Reset for next fill
        }
    }
}

void game::renderInventoryPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
    SDL_RenderFillRect(renderer, NULL);

    // Center divider
    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderLine(renderer, rect.w / 2, 20, rect.w / 2, rect.h - 20);

    // --- 6x4 Inventory Grid Math (Per Side) ---
    int cols = 6;
    int rows = 5;
    int slotGap = 4;

    // We are only working with half the width for each grid
    int halfWidth = rect.w / 2;
    int sidePadding = 20; // Distance from the edges and the center divider
    int topPadding = 60;  // Leave room at the top for "Your Inventory" text later

    int availableW = halfWidth - (2 * sidePadding) - ((cols - 1) * slotGap);
    int availableH = rect.h - topPadding - sidePadding - ((rows - 1) * slotGap);

    int slotSize = std::min(availableW / cols, availableH / rows);

    int gridW = (slotSize * cols) + (slotGap * (cols - 1));
    int gridH = (slotSize * rows) + (slotGap * (rows - 1));

    // Offset for Left Grid (Player)
    int leftOffsetX = (halfWidth - gridW) / 2;
    int gridOffsetY = topPadding; // Push down for headers

    // Offset for Right Grid (Floor)
    int rightOffsetX = halfWidth + (halfWidth - gridW) / 2;

    // Draw Player Grid
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

    // Draw Floor Grid
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

void game::renderPCPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
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

void game::renderButtons(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
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

void game::clean()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}