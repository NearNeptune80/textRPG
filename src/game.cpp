#include "game.h"
#include "uiWidget.h"

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), Player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    map = nullptr;
    if (Player) delete Player;
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

    // 1. Add to tile ground stack or create new item entry
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

    // 2. Remove items from player's backpack
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

    // 1. Add to player inventory
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

        // Subtract from ground tile stack
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

static std::string formatEquipSlotName(equipSlot slot)
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

bool game::loadMap(const std::string& mapId, int startX, int startY)
{
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
    return true;
}

void game::movePlayer(int nextX, int nextY)
{
    if (!map || !map->isWalkable(nextX, nextY)) return;

    // 1. Event-driven cleanup: Clear dropped items on current tile if it's unsafe BEFORE moving
    map->clearUnsafeItems(gridX, gridY);

    // 2. Update player coordinates
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
        int roll = rand() % 100;

        if (roll < chance)
        {
            if (!tileData.persistentNPC)
            {
                tileData.persistentNPC = generateEncounterNPC();
            }
            triggerEncounter(tileData.persistentNPC);
            return;
        }
    }

    // Step onto safe tile: clear active target
    activeTargetNPC = nullptr;
    activeTargetMode = TargetMode::NONE;

    refreshActionGrid();
}

bool game::loadFont(const std::string& id, const std::string& path, int ptSize)
{
    TTF_Font* newFont = TTF_OpenFont(path.c_str(), ptSize);
    if (!newFont)
    {
        std::cout << "Failed to load font " << id << " from " << path << " : " << SDL_GetError() << "\n";
        return false;
    }
    fonts[id] = newFont;
    return true;
}

void game::init(const char* title, int width, int height, bool fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;

    window = SDL_CreateWindow(title, width, height, (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
    renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_STRETCH);

    if (TTF_Init() < 0)
    {
        std::cout << "Error initializing SDL_ttf: " << SDL_GetError() << "\n";
    }

    loadFont("button_font", "data/fonts/Roboto/static/Roboto-Regular.ttf", 18);
    loadFont("title_font", "data/fonts/Roboto/static/Roboto-Bold.ttf", 24);

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        questDatabase::loadDatabase("data/quests");

        if (!saveManager::loadGame(this, "data/saves/save_01.json"))
        {
            saveManager::createInitialSave(this, "data/saves/save_01.json");
            saveManager::loadGame(this, "data/saves/save_01.json");
        }
    }
    loadMap("overworld", 1, 1);

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
            SDL_SetRenderLogicalPresentation(renderer, event.window.data1, event.window.data2, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                handleMouseClick(event.button.x, event.button.y);
            }
        }

        // MOVED HERE: Separate top-level event check for mouse wheel
        if (event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            if (currentState == GameState::INVENTORY)
            {
                // Smoothly adjust scroll position
                descriptionScrollY -= event.wheel.y * 18.0f;

                // Immediate boundary clamping prevents the single-frame glitch/jump
                descriptionScrollY = std::clamp(descriptionScrollY, 0.0f, maxDescriptionScrollY);
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_I)
            {
                if (currentState == GameState::EVENT) return;

                selectedInventoryIndex = -1;
                selectedEquipmentSlot = equipSlot::NONE;

                if (currentState == GameState::EXPLORATION) currentState = GameState::INVENTORY;
                else if (currentState == GameState::INVENTORY) currentState = GameState::EXPLORATION;

                refreshActionGrid();
                return;
            }
            if (event.key.key == SDLK_M)
            {
                currentState = (currentState == GameState::MAIN_MENU) ? GameState::EXPLORATION : GameState::MAIN_MENU;
                return;
            }
            if (event.key.key == SDLK_F5)
            {
                saveManager::saveGame(this, "data/saves/save_01.json");
                return;
            }
            if (event.key.key == SDLK_F9)
            {
                saveManager::loadGame(this, "data/saves/save_01.json");
                return;
            }

            if (currentState == GameState::EXPLORATION)
            {
                int nextX = gridX, nextY = gridY;
                bool isMoveKey = false;

                switch (event.key.key)
                {
                    case SDLK_UP:    nextY--; isMoveKey = true; break;
                    case SDLK_DOWN:  nextY++; isMoveKey = true; break;
                    case SDLK_LEFT:  nextX--; isMoveKey = true; break;
                    case SDLK_RIGHT: nextX++; isMoveKey = true; break;
                }

                if (isMoveKey) movePlayer(nextX, nextY);
            }
        }
    }
}

void game::updateLayoutBounds(int w, int h)
{
    float padding = 12.0f;
    float topBarH = h * 0.05f;
    float mapSize = h * 0.30f;

    float colStartY = padding + topBarH + padding;
    float colEndY = static_cast<float>(h) - padding;

    float leftColW = mapSize;
    float rightColW = mapSize;
    float centerColW = static_cast<float>(w) - (leftColW + rightColW + (4.0f * padding));

    float leftX = padding;
    float centerX = leftX + leftColW + padding;
    float rightX = centerX + centerColW + padding;

    // Header Titles
    layout.titleBox1 = { leftX, padding, leftColW, topBarH };
    layout.titleBox2 = { centerX, padding, centerColW, topBarH };
    layout.titleBox3 = { rightX, padding, rightColW, topBarH };

    // Left Column Bounds
    float charH = mapSize * 0.82f;
    layout.charRect = { leftX, colStartY, leftColW, charH };

    float timeH = mapSize * 0.18f; // Reduced height to fit the compact design
    float timeY = colEndY - mapSize - padding - timeH;
    layout.timeRect = { leftX, timeY, leftColW, timeH };

    float midY = colStartY + charH + padding;
    float midH = timeY - padding - midY;
    layout.companionRect = { leftX, midY, leftColW, midH };

    layout.mapRect = { leftX, colEndY - mapSize, mapSize, mapSize };

    // Center Column Bounds
    float btnH = h * 0.15f;

    float centerAvailableH = (colEndY - btnH - padding) - colStartY;
    
    // Standard Text Box (Exploration / Events)
    layout.textMainRect = { centerX, colStartY, centerColW, centerAvailableH };

    // Split Inventory View
    float gridH = centerAvailableH * 0.52f;
    float detailH = centerAvailableH - gridH - padding;

    layout.inventoryGridRect = { centerX, colStartY, centerColW, gridH };
    layout.inventoryDetailRect = { centerX, colStartY + gridH + padding, centerColW, detailH };

    layout.actionGridRect = { centerX, colEndY - btnH, centerColW, btnH };

    // Right Column Stack
    float rightAvailableH = colEndY - colStartY;
    float rightStackH = (rightAvailableH - (2.0f * padding)) / 3.0f;

    layout.rightStackTop = { rightX, colStartY, rightColW, rightStackH };
    layout.rightStackMid = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    layout.rightStackBot = { rightX, colStartY + (rightStackH + padding) * 2.0f, rightColW, rightAvailableH - (rightStackH * 2.0f + padding * 2.0f) };

    layout.equipRect = layout.mapRect;
    

    // Hover Avatar Regions
    float charPadX = layout.charRect.w * 0.04f;
    float charPadY = layout.charRect.h * 0.04f;
    float avatarSize = layout.charRect.h * 0.16f;

    layout.playerAvatarRect = {
        layout.charRect.x + charPadX,
        layout.charRect.y + charPadY,
        avatarSize,
        avatarSize
    };

    layout.targetAvatarRect = {
        rightX + charPadX,
        colStartY + charPadY,
        avatarSize,
        avatarSize
    };
}

void game::handleMouseClick(float windowX, float windowY)
{
    float mouseX, mouseY;
    SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, &mouseX, &mouseY);

    int w = 0, h = 0;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) SDL_GetRenderOutputSize(renderer, &w, &h);

    updateLayoutBounds(w, h);
    float padding = 12.0f;

    // 1. Action Grid Clicks
    if (UIGridHelper::contains(layout.actionGridRect, mouseX, mouseY))
    {
        float padding = 8.0f;
        float arrowW = layout.actionGridRect.w * 0.03f;
        float localX = mouseX - layout.actionGridRect.x;
        float localY = mouseY - layout.actionGridRect.y;

        // Left (<) Arrow Click
        SDL_FRect leftArrowRect = { padding, padding, arrowW, layout.actionGridRect.h - (2.0f * padding) };
        if (UIGridHelper::contains(leftArrowRect, localX, localY))
        {
            if (actionGridPage > 0) actionGridPage--;
            return;
        }

        // Right (>) Arrow Click
        SDL_FRect rightArrowRect = { layout.actionGridRect.w - arrowW - padding, padding, arrowW, layout.actionGridRect.h - (2.0f * padding) };
        if (UIGridHelper::contains(rightArrowRect, localX, localY))
        {
            actionGridPage++;
            return;
        }

        // Center Grid Slot Clicks
        SDL_FRect gridBounds = UIGridHelper::getActionGridBounds(layout.actionGridRect);
        auto currentSlots = getSlotsForCurrentActionPage();

        int cols = 5, rows = 3;
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                SDL_FRect btnRect = UIGridHelper::getActionButtonRect(gridBounds, c, r, cols, rows, padding);
                if (UIGridHelper::contains(btnRect, localX, localY))
                {
                    int slotIdx = (r * cols) + c;
                    if (!currentSlots[slotIdx].label.empty() && currentSlots[slotIdx].isEnabled && currentSlots[slotIdx].onClick)
                    {
                        currentSlots[slotIdx].onClick();
                    }
                    return;
                }
            }
        }
    }

    // 2. Map Clicks (Exploration)
    if (currentState == GameState::EXPLORATION)
    {
        if (UIGridHelper::contains(layout.mapRect, mouseX, mouseY))
        {
            for (int r = 0; r < 5; r++)
            {
                for (int c = 0; c < 5; c++)
                {
                    SDL_FRect tileSlot = UIGridHelper::getMapTileRect(layout.mapRect, c, r, padding);
                    if (UIGridHelper::contains(tileSlot, mouseX, mouseY))
                    {
                        int dx = c - 2;
                        int dy = r - 2;
                        if (std::abs(dx) + std::abs(dy) == 1) movePlayer(gridX + dx, gridY + dy);
                        return;
                    }
                }
            }
        }
    }
    // 3. Equipment & Inventory Slot Selection
    else if (currentState == GameState::INVENTORY)
    {
        SDL_FRect playerEquipRect = layout.equipRect;
        SDL_FRect rightEquipRect = { layout.rightStackTop.x, layout.mapRect.y, layout.rightStackTop.w, layout.mapRect.h };

        // A. Left Player Equipment Grid Clicks
        if (UIGridHelper::contains(playerEquipRect, mouseX, mouseY))
        {
            int cols = 6, rows = 6;
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(playerEquipRect, c, r, cols, rows, 4.0f, padding);
                    if (UIGridHelper::contains(slot, mouseX, mouseY))
                    {
                        int slotIdx = (r * cols) + c;
                        selectedEquipmentSlot = equipSlot::NONE;
                        selectedInventoryIndex = -1;
                        selectedInventorySide = 0;

                        if (Player)
                        {
                            for (const auto& [eSlot, eqItem] : Player->inventory.equipped)
                            {
                                if (getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                                {
                                    selectedEquipmentSlot = eSlot;
                                    break;
                                }
                            }
                        }
                        refreshActionGrid();
                        return;
                    }
                }
            }
        }
        // B. Right Target NPC Equipment Grid Clicks
        else if (activeTargetNPC && UIGridHelper::contains(rightEquipRect, mouseX, mouseY))
        {
            int cols = 6, rows = 6;
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(rightEquipRect, c, r, cols, rows, 4.0f, padding);
                    if (UIGridHelper::contains(slot, mouseX, mouseY))
                    {
                        int slotIdx = (r * cols) + c;
                        selectedEquipmentSlot = equipSlot::NONE;
                        selectedInventoryIndex = -1;
                        selectedInventorySide = 1;

                        for (const auto& [eSlot, eqItem] : activeTargetNPC->inventory.equipped)
                        {
                            if (getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                            {
                                selectedEquipmentSlot = eSlot;
                                break;
                            }
                        }
                        refreshActionGrid();
                        return;
                    }
                }
            }
        }

        // C. Twin Inventory Grid Clicks (Left = Player, Right = NPC or Ground Tile)
        if (UIGridHelper::contains(layout.inventoryGridRect, mouseX, mouseY))
        {
            float localMouseX = mouseX - layout.inventoryGridRect.x;
            float localMouseY = mouseY - layout.inventoryGridRect.y;
            SDL_FRect localBounds = { 0.0f, 0.0f, layout.inventoryGridRect.w, layout.inventoryGridRect.h };

            // Page Tabs
            for (int side = 0; side < 2; side++)
            {
                for (int t = 0; t < 7; t++)
                {
                    SDL_FRect tabRect = UIGridHelper::getInventoryTabRect(localBounds, side, t);
                    if (UIGridHelper::contains(tabRect, localMouseX, localMouseY))
                    {
                        if (side == 0) currentInventoryPage = t;
                        else currentRightInventoryPage = t;

                        selectedInventoryIndex = -1;
                        descriptionScrollY = 0.0f;
                        refreshActionGrid();
                        return;
                    }
                }
            }

            // Inventory Item Slots
            int cols = 6, rows = 5;
            int itemsPerPage = cols * rows;
            TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

            for (int side = 0; side < 2; side++)
            {
                int activePage = (side == 0) ? currentInventoryPage : currentRightInventoryPage;
                int pageOffset = activePage * itemsPerPage;

                for (int i = 0; i < itemsPerPage; i++)
                {
                    int gridSlotIdx = (side * itemsPerPage) + i;
                    SDL_FRect slot = UIGridHelper::getInventorySlotRect(localBounds, gridSlotIdx, cols, rows);

                    if (UIGridHelper::contains(slot, localMouseX, localMouseY))
                    {
                        selectedEquipmentSlot = equipSlot::NONE;
                        int absoluteItemIdx = pageOffset + i;

                        if (side == 0 && Player)
                        {
                            selectedInventorySide = 0;
                            auto stackedView = Player->inventory.getStackedView();
                            if (absoluteItemIdx < static_cast<int>(stackedView.size()))
                            {
                                selectedInventoryIndex = absoluteItemIdx;
                            }
                            else
                            {
                                selectedInventoryIndex = -1;
                            }
                        }
                        else if (side == 1)
                        {
                            selectedInventorySide = 1;
                            if (activeTargetNPC)
                            {
                                auto npcView = activeTargetNPC->inventory.getStackedView();
                                if (absoluteItemIdx < static_cast<int>(npcView.size()))
                                {
                                    selectedInventoryIndex = absoluteItemIdx;
                                }
                                else
                                {
                                    selectedInventoryIndex = -1;
                                }
                            }
                            else
                            {
                                if (absoluteItemIdx < static_cast<int>(tileData.droppedItems.size()))
                                {
                                    selectedInventoryIndex = absoluteItemIdx;
                                }
                                else
                                {
                                    selectedInventoryIndex = -1;
                                }
                            }
                        }

                        descriptionScrollY = 0.0f;
                        refreshActionGrid();
                        return;
                    }
                }
            }
        }
    }
}

int game::getEquipmentGridIndex(equipSlot slot)
{
    switch (slot)
    {
        // Row 1
        case equipSlot::EYEWEAR:         return 0;  // Eyes
        case equipSlot::HEADWEAR:        return 1;  // Head
        case equipSlot::HAIR_WEAR:       return 2;  // Hair
        case equipSlot::HORNS_SLOT:      return 3;  // Horns
        case equipSlot::WEAPON_MAIN:     return 4;  // Primary Weapon
        case equipSlot::WEAPON_OFF:      return 5;  // Secondary Weapon

            // Row 2
        case equipSlot::MOUTHWEAR:       return 6;  // Mouth
        case equipSlot::TORSO_OVER:      return 7;  // Over-torso
        case equipSlot::NECKWEAR:        return 8;  // Neck
        case equipSlot::WINGS_SLOT:      return 9;  // Wings
        case equipSlot::PIERCING_EAR:    return 10; // Ear Piercing
        case equipSlot::PIERCING_NOSE:   return 11; // Nose Piercing

            // Row 3
        case equipSlot::WRISTS:          return 12; // Wrists
        case equipSlot::TORSO_UNDER:     return 13; // Torso
        case equipSlot::CHEST_WEAR:      return 14; // Chest
        case equipSlot::NIPPLES_WEAR:    return 15; // Nipples
        case equipSlot::PIERCING_LIP:    return 16; // Lip Piercing
        case equipSlot::PIERCING_TONGUE: return 17; // Tongue Piercing

            // Row 4
        case equipSlot::HANDS:           return 18; // Hands
        case equipSlot::HIPS_WEAR:       return 19; // Hips
        case equipSlot::STOMACH_WEAR:    return 20; // Stomach
        case equipSlot::FINGER_PRIMARY:  return 21; // Fingers
        case equipSlot::PIERCING_NIPPLE: return 22; // Nipple Piercing
        case equipSlot::PIERCING_NAVEL:  return 23; // Navel Piercing

            // Row 5
        case equipSlot::ANKLES:          return 24; // Ankles
        case equipSlot::LEGS_OUTER:      return 25; // Legs
        case equipSlot::GROIN_OVER:      return 26; // Groin
        case equipSlot::TAIL_SLOT:       return 27; // Tail
        case equipSlot::PIERCING_COCK:   return 28; // Cock Piercing
        case equipSlot::PIERCING_VAGINA: return 29; // Vaginal Piercing

            // Row 6
        case equipSlot::CALVES:          return 30; // Calves
        case equipSlot::FEET:            return 31; // Feet
        case equipSlot::ASS_WEAR:        return 32; // Anus
        case equipSlot::PENIS_WEAR:      return 33; // Penis
        case equipSlot::VAGINA_WEAR:     return 34; // Vagina
            // Slot 35 is left blank for the special button box!

        default:                         return -1;
    }
}

void game::handleEquipAction(int backpackIndex)
{
    if (!Player || backpackIndex < 0 || static_cast<size_t>(backpackIndex) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];
    if (!targetItem || !targetItem->isEquippable) return;

    // Safety Guard: Don't equip items assigned to equipSlot::NONE
    if (targetItem->targetSlot == equipSlot::NONE)
    {
        std::cout << "[Warning] Item '" << targetItem->name << "' has no valid target equipSlot mapping.\n";
        return;
    }

    std::vector<std::string> bodyTags = Player->anatomy.getAllTags();
    if (Player->inventory.equipItem(static_cast<size_t>(backpackIndex), targetItem->targetSlot, bodyTags))
    {
        selectedInventoryIndex = -1;
        selectedEquipmentSlot = targetItem->targetSlot;
        descriptionScrollY = 0.0f;
        refreshActionGrid();
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    if (Player->inventory.unequipItem(slot))
    {
        selectedEquipmentSlot = equipSlot::NONE;
        // Point selection to the newly returned item at the end of the backpack
        selectedInventoryIndex = static_cast<int>(Player->inventory.backpack.size()) - 1;
        descriptionScrollY = 0.0f;
        refreshActionGrid();
    }
}

void game::refreshActionGrid()
{
    activeButtons.clear();

    if (currentState == GameState::INVENTORY)
    {
        // 1. Pinned "Close inventory" Button (Bottom-Right Slot 14)
        actionButton closeBtn;
        closeBtn.label = "Close inventory";
        closeBtn.slotIndex = 14;
        closeBtn.pinnedAllPages = true;
        closeBtn.onClick = [this]()
            {
                selectedInventoryIndex = -1;
                selectedEquipmentSlot = equipSlot::NONE;

                if (activeTargetNPC)
                {
                    // Re-open victory scene options when returning from NPC inventory
                    dialogueChoice fightSim;
                    fightSim.nextSceneId = "ENCOUNTER_FIGHT";
                    processChoice(fightSim);
                }
                else
                {
                    currentState = GameState::EXPLORATION;
                    refreshActionGrid();
                }
            };
        activeButtons.push_back(closeBtn);

        TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

        // 2. Equipped Item Selected Actions (Player or Target NPC)
        if (selectedEquipmentSlot != equipSlot::NONE)
        {
            entity* targetChar = (selectedInventorySide == 1 && activeTargetNPC) ? activeTargetNPC : Player;

            if (targetChar && targetChar->inventory.isEquipped(selectedEquipmentSlot))
            {
                auto eqItem = targetChar->inventory.getEquippedItem(selectedEquipmentSlot);

                // Slot 0: Drop or Take
                actionButton dropBtn;
                dropBtn.label = (targetChar == Player) ? "Drop" : "Take (1)";
                dropBtn.slotIndex = 0;
                dropBtn.onClick = [this, targetChar]()
                    {
                        if (targetChar->inventory.unequipItem(selectedEquipmentSlot))
                        {
                            selectedEquipmentSlot = equipSlot::NONE;
                            refreshActionGrid();
                        }
                    };
                activeButtons.push_back(dropBtn);

                // Slot 3: Dye
                actionButton dyeBtn;
                dyeBtn.label = "Dye";
                dyeBtn.slotIndex = 3;
                activeButtons.push_back(dyeBtn);

                // Slot 4: Enchant
                actionButton enchantBtn;
                enchantBtn.label = "Enchant";
                enchantBtn.slotIndex = 4;
                activeButtons.push_back(enchantBtn);

                // Slot 5: Unequip
                actionButton unequipBtn;
                unequipBtn.label = "Unequip";
                unequipBtn.slotIndex = 5;
                unequipBtn.onClick = dropBtn.onClick;
                activeButtons.push_back(unequipBtn);

                // Slot 10: Pull down
                actionButton pullBtn;
                pullBtn.label = "Pull down";
                pullBtn.slotIndex = 10;
                activeButtons.push_back(pullBtn);

                // Slot 11: Shift aside
                actionButton shiftBtn;
                shiftBtn.label = "Shift aside";
                shiftBtn.slotIndex = 11;
                activeButtons.push_back(shiftBtn);
            }
        }
        // 3. Left Grid (Player Backpack) Selection Actions
        else if (selectedInventorySide == 0 && selectedInventoryIndex >= 0 && Player)
        {
            auto stackedView = Player->inventory.getStackedView();
            if (static_cast<size_t>(selectedInventoryIndex) < stackedView.size())
            {
                const auto& slotData = stackedView[selectedInventoryIndex];
                auto selItem = slotData.itemPtr;
                int totalCount = slotData.totalCount;
                int idx = selectedInventoryIndex;

                // --- ROW 1: Drop Management Slots ---
                actionButton drop1Btn;
                drop1Btn.label = "Drop (1)";
                drop1Btn.slotIndex = 0;
                drop1Btn.isEnabled = (totalCount >= 1);
                drop1Btn.onClick = [this, idx]() { handleDropAction(idx, 1); };
                activeButtons.push_back(drop1Btn);

                actionButton drop5Btn;
                drop5Btn.label = "Drop (5)";
                drop5Btn.slotIndex = 1;
                drop5Btn.isEnabled = (totalCount >= 5);
                drop5Btn.onClick = [this, idx]() { handleDropAction(idx, 5); };
                activeButtons.push_back(drop5Btn);

                actionButton dropAllBtn;
                dropAllBtn.label = "Drop (All)";
                dropAllBtn.slotIndex = 2;
                dropAllBtn.isEnabled = (totalCount >= 1);
                dropAllBtn.onClick = [this, idx, totalCount]() { handleDropAction(idx, totalCount); };
                activeButtons.push_back(dropAllBtn);

                actionButton enchantBtn;
                enchantBtn.label = "Enchant";
                enchantBtn.slotIndex = 4;
                activeButtons.push_back(enchantBtn);

                // --- ROW 2: Primary Use / Equip / Consume Slots ---
                if (selItem->isEquippable)
                {
                    actionButton equipBtn;
                    equipBtn.label = "Equip: " + formatEquipSlotName(selItem->targetSlot);
                    equipBtn.slotIndex = 5;
                    equipBtn.onClick = [this, slotData]()
                        {
                            handleEquipAction(slotData.firstBackpackIndex);
                        };
                    activeButtons.push_back(equipBtn);
                }
                else if (selItem->isConsumable)
                {
                    actionButton eatBtn;
                    eatBtn.label = "Eat (Self)";
                    eatBtn.slotIndex = 5;
                    eatBtn.onClick = [this, selItem]()
                        {
                            Player->inventory.removeItem(selItem->id, 1);
                            selectedInventoryIndex = -1;
                            refreshActionGrid();
                        };
                    activeButtons.push_back(eatBtn);

                    actionButton eatAllBtn;
                    eatAllBtn.label = "Eat all (Self)";
                    eatAllBtn.slotIndex = 6;
                    eatAllBtn.isEnabled = (totalCount >= 1);
                    eatAllBtn.onClick = [this, selItem, totalCount]()
                        {
                            Player->inventory.removeItem(selItem->id, totalCount);
                            selectedInventoryIndex = -1;
                            refreshActionGrid();
                        };
                    activeButtons.push_back(eatAllBtn);
                }
            }
        }
        // 4. Right Grid Selection Actions (Take 1, Take 5, Take All - NPC or Ground)
        else if (selectedInventorySide == 1 && selectedInventoryIndex >= 0)
        {
            if (activeTargetNPC)
            {
                auto npcView = activeTargetNPC->inventory.getStackedView();
                if (static_cast<size_t>(selectedInventoryIndex) < npcView.size())
                {
                    const auto& slotData = npcView[selectedInventoryIndex];
                    auto selItem = slotData.itemPtr;
                    int totalCount = slotData.totalCount;

                    // Take (1) -> Slot 0
                    actionButton take1Btn;
                    take1Btn.label = "Take (1)";
                    take1Btn.slotIndex = 0;
                    take1Btn.isEnabled = (totalCount >= 1);
                    take1Btn.onClick = [this, selItem]()
                        {
                            if (activeTargetNPC && activeTargetNPC->inventory.removeItem(selItem->id, 1))
                            {
                                auto copy = std::make_shared<item>(*selItem);
                                copy->count = 1;
                                Player->inventory.addItem(copy);
                                selectedInventoryIndex = -1;
                                refreshActionGrid();
                            }
                        };
                    activeButtons.push_back(take1Btn);

                    // Take (5) -> Slot 1
                    actionButton take5Btn;
                    take5Btn.label = "Take (5)";
                    take5Btn.slotIndex = 1;
                    take5Btn.isEnabled = (totalCount >= 5);
                    take5Btn.onClick = [this, selItem]()
                        {
                            if (activeTargetNPC && activeTargetNPC->inventory.removeItem(selItem->id, 5))
                            {
                                auto copy = std::make_shared<item>(*selItem);
                                copy->count = 5;
                                Player->inventory.addItem(copy);
                                selectedInventoryIndex = -1;
                                refreshActionGrid();
                            }
                        };
                    activeButtons.push_back(take5Btn);

                    // Take (All) -> Slot 2
                    actionButton takeAllBtn;
                    takeAllBtn.label = "Take (All)";
                    takeAllBtn.slotIndex = 2;
                    takeAllBtn.isEnabled = (totalCount >= 1);
                    takeAllBtn.onClick = [this, selItem, totalCount]()
                        {
                            if (activeTargetNPC && activeTargetNPC->inventory.removeItem(selItem->id, totalCount))
                            {
                                auto copy = std::make_shared<item>(*selItem);
                                copy->count = totalCount;
                                Player->inventory.addItem(copy);
                                selectedInventoryIndex = -1;
                                refreshActionGrid();
                            }
                        };
                    activeButtons.push_back(takeAllBtn);

                    // Equip directly from NPC -> Slot 5
                    if (selItem->isEquippable)
                    {
                        actionButton equipBtn;
                        equipBtn.label = "Equip: " + formatEquipSlotName(selItem->targetSlot);
                        equipBtn.slotIndex = 5;
                        equipBtn.onClick = [this, selItem]()
                            {
                                if (activeTargetNPC && activeTargetNPC->inventory.removeItem(selItem->id, 1))
                                {
                                    auto copy = std::make_shared<item>(*selItem);
                                    copy->count = 1;
                                    Player->inventory.addItem(copy);

                                    int newBackpackIdx = static_cast<int>(Player->inventory.backpack.size()) - 1;
                                    handleEquipAction(newBackpackIdx);
                                }
                            };
                        activeButtons.push_back(equipBtn);
                    }
                }
            }
            else if (static_cast<size_t>(selectedInventoryIndex) < tileData.droppedItems.size())
            {
                auto selItem = tileData.droppedItems[selectedInventoryIndex];
                int totalCount = selItem->isStackable ? selItem->count : 1;
                int idx = selectedInventoryIndex;

                // Take (1) -> Slot 0
                actionButton take1Btn;
                take1Btn.label = "Take (1)";
                take1Btn.slotIndex = 0;
                take1Btn.isEnabled = (totalCount >= 1);
                take1Btn.onClick = [this, idx]() { handlePickupAction(idx, 1); };
                activeButtons.push_back(take1Btn);

                // Take (5) -> Slot 1
                actionButton take5Btn;
                take5Btn.label = "Take (5)";
                take5Btn.slotIndex = 1;
                take5Btn.isEnabled = (totalCount >= 5);
                take5Btn.onClick = [this, idx]() { handlePickupAction(idx, 5); };
                activeButtons.push_back(take5Btn);

                // Take (All) -> Slot 2
                actionButton takeAllBtn;
                takeAllBtn.label = "Take (All)";
                takeAllBtn.slotIndex = 2;
                takeAllBtn.isEnabled = (totalCount >= 1);
                takeAllBtn.onClick = [this, idx, totalCount]() { handlePickupAction(idx, totalCount); };
                activeButtons.push_back(takeAllBtn);

                // Equip directly from ground -> Slot 5
                if (selItem->isEquippable)
                {
                    actionButton equipBtn;
                    equipBtn.label = "Equip: " + formatEquipSlotName(selItem->targetSlot);
                    equipBtn.slotIndex = 5;
                    equipBtn.onClick = [this, idx]()
                        {
                            TileRuntimeData& tData = map->getRuntimeData(gridX, gridY);
                            if (idx < static_cast<int>(tData.droppedItems.size()))
                            {
                                auto groundItem = tData.droppedItems[idx];
                                tData.droppedItems.erase(tData.droppedItems.begin() + idx);
                                Player->inventory.addItem(groundItem);

                                int newBackpackIdx = static_cast<int>(Player->inventory.backpack.size()) - 1;
                                handleEquipAction(newBackpackIdx);
                            }
                        };
                    activeButtons.push_back(equipBtn);
                }
            }
        }
    }
    else if (currentState == GameState::EXPLORATION)
    {
        // 1. Map Triggers on Current Tile (Flows naturally from left-to-right)
        auto triggers = questDatabase::getTriggersForLocation(map->getId(), gridX, gridY);
        for (const auto& trig : triggers)
        {
            if (checkConditions(trig.conditions))
            {
                actionButton trigBtn;
                trigBtn.label = trig.label;
                std::string sId = trig.sceneId;
                trigBtn.onClick = [this, sId]() { loadScene(sId); };
                activeButtons.push_back(trigBtn);
            }
        }

        // 2. Map Warps on Current Tile
        MapWarp w;
        if (map->checkWarp(gridX, gridY, w))
        {
            actionButton warpBtn;
            warpBtn.label = "Enter Door";
            warpBtn.onClick = [this, w]() { loadMap(w.targetMap, w.targetX, w.targetY); };
            activeButtons.push_back(warpBtn);
        }
    }
}

bool game::checkConditions(const std::vector<gameCondition>& conditions)
{
    for (const auto& cond : conditions)
    {
        if (cond.type == "HAS_ITEM")
        {
            bool found = false;
            for (const auto& item : Player->inventory.backpack)
            {
                if (item && item->id == cond.target) found = true;
            }
            if (!found && cond.requiredValue > 0) return false;
        }
        else if (cond.type == "QUEST_STAGE")
        {
            if (Player->quests.getQuestStage(cond.target) != cond.requiredValue) return false;
        }
        else if (cond.type == "TIME_PHASE")
        {
            TimePhase currentPhase = gameTime.getPhase();
            std::string phaseStr = "DAY";
            if (currentPhase == TimePhase::NIGHT) phaseStr = "NIGHT";
            else if (currentPhase == TimePhase::DAWN) phaseStr = "DAWN";
            else if (currentPhase == TimePhase::DUSK) phaseStr = "DUSK";

            if (phaseStr != cond.target) return false;
        }
        else if (cond.type == "STAT_MIN")
        {
            if (Player->getStat(cond.target) < cond.requiredValue) return false;
        }
        else if (cond.type == "HAS_TAG")
        {
            if (!Player->anatomy.hasGlobalTag(cond.target)) return false;
        }
    }
    return true;
}

void game::processChoice(const dialogueChoice& choice)
{
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        // Transition to victory scene
        currentScene.id = "encounter_victory";
        currentScene.speakerName = activeTargetNPC ? activeTargetNPC->name : "Enemy";
        currentScene.bodyText = "You defeated " + currentScene.speakerName + " in combat!";
        currentScene.choices.clear();

        // 1. Continue Button (Slot 0) -> Exit to Exploration
        dialogueChoice contChoice;
        contChoice.label = "Continue";
        contChoice.nextSceneId = "EXIT";
        currentScene.choices.push_back(contChoice);

        // 2. Inventory Button (Slot 1) -> Open Dual Inventory View
        dialogueChoice invChoice;
        invChoice.label = "Inventory";
        invChoice.nextSceneId = "VICTORY_INVENTORY";
        currentScene.choices.push_back(invChoice);

        // 3. Talk Button (Slot 2) -> Placeholder
        dialogueChoice talkChoice;
        talkChoice.label = "Talk";
        talkChoice.nextSceneId = "VICTORY_TALK";
        currentScene.choices.push_back(talkChoice);

        activeButtons.clear();
        for (const auto& c : currentScene.choices)
        {
            actionButton btn;
            btn.label = c.label;
            btn.onClick = [this, c]() { processChoice(c); };
            activeButtons.push_back(btn);
        }
        return;
    }

    if (choice.nextSceneId == "VICTORY_INVENTORY")
    {
        currentState = GameState::INVENTORY;
        selectedInventoryIndex = -1;
        selectedEquipmentSlot = equipSlot::NONE;
        refreshActionGrid();
        return;
    }

    for (const auto& effect : choice.results)
    {
        if (effect.action == "GIVE_ITEM")
        {
            Player->inventory.addItem(itemDatabase::getItem(effect.target));
        }
        else if (effect.action == "REMOVE_ITEM")
        {
            Player->inventory.removeItem(effect.target);
        }
        else if (effect.action == "ADD_STAT")
        {
            Player->stats.modifyBaseStat(effect.target, static_cast<float>(effect.amount));
        }
        else if (effect.action == "SET_QUEST")
        {
            Player->quests.setQuestStage(effect.target, effect.amount);
        }
        else if (effect.action == "TELEPORT_MAP")
        {
            size_t c1 = effect.target.find(',');
            size_t c2 = effect.target.find(',', c1 + 1);

            if (c1 != std::string::npos && c2 != std::string::npos)
            {
                std::string targetMap = effect.target.substr(0, c1);
                int targetX = std::stoi(effect.target.substr(c1 + 1, c2 - c1 - 1));
                int targetY = std::stoi(effect.target.substr(c2 + 1));
                loadMap(targetMap, targetX, targetY);
            }
        }
    }

    if (choice.nextSceneId == "EXIT" || choice.nextSceneId.empty())
    {
        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        currentState = GameState::EXPLORATION;
        refreshActionGrid();
    }
    else
    {
        loadScene(choice.nextSceneId);
    }
}

void game::loadScene(const std::string& sceneId)
{
    currentState = GameState::EVENT;
    currentScene = questDatabase::getScene(sceneId);

    activeButtons.clear();
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
        if (checkConditions(currentScene.choices[i].requirements))
        {
            actionButton btn;
            btn.label = currentScene.choices[i].label;
            dialogueChoice choice = currentScene.choices[i];
            btn.onClick = [this, choice]()
                {
                    processChoice(choice);
                };
            activeButtons.push_back(btn);
        }
    }
}

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    updateLayoutBounds(w, h);

    renderTitleBar(layout.titleBox1, layout.titleBox2, layout.titleBox3);

    // Character Summary Card
    UI::DrawEntitySummaryCard(renderer, this, layout.charRect, Player, false);

    renderCompanionPanel(layout.companionRect);

    // Time Panel Widget
    UI::DrawTimePanel(renderer, this, layout.timeRect, gameTime);

    // Action Grid Widget
    UI::DrawActionGrid(renderer, this, layout.actionGridRect, activeButtons);

    switch (currentState)
    {
        case GameState::EVENT:
        case GameState::EXPLORATION:
        {
            UI::DrawMapGrid(renderer, this, layout.mapRect, map, gridX, gridY, 12);
            renderTextPanel(layout.textMainRect);
            renderRightColumn(layout.rightStackTop, layout.rightStackMid, layout.rightStackBot);
        }
        break;

        case GameState::INVENTORY:
        {
            UI::DrawEquipmentGrid(renderer, this, layout.equipRect, Player, selectedEquipmentSlot, 12);
            UI::DrawInventoryGrid(renderer, this, layout.inventoryGridRect, Player, selectedInventoryIndex);
            UI::DrawItemDetailPanel(renderer, this, layout.inventoryDetailRect, Player, selectedInventoryIndex);
            renderRightColumn(layout.rightStackTop, layout.rightStackMid, layout.rightStackBot);
        }
        break;
        break;

        default: break;
    }

    // Hover Tooltip Check using precalculated layout bounds
    float winX, winY, mouseX, mouseY;
    SDL_GetMouseState(&winX, &winY);
    SDL_RenderCoordinatesFromWindow(renderer, winX, winY, &mouseX, &mouseY);

    if (UIGridHelper::contains(layout.playerAvatarRect, mouseX, mouseY))
    {
        UI::DrawAnatomyTooltip(renderer, this, Player, mouseX, mouseY);
    }
}

void game::renderMainMenuLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    SDL_Rect menuRect = { w / 4, h / 4, w / 2, h / 2 };
    ViewportGuard vpGuard(renderer, &menuRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, static_cast<float>(menuRect.w), static_cast<float>(menuRect.h) };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 20, 60, 80, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 120, 160, 255 });
}

void game::drawTextFit(const std::string& textStr, SDL_FRect destRect, SDL_Color color, const std::string& fontId)
{
    float srcW = 0.0f, srcH = 0.0f;
    SDL_Texture* texture = getOrRenderText(textStr, fontId, color, srcW, srcH);

    if (texture && srcH > 0.0f)
    {
        float scale = destRect.h / srcH;
        if ((srcW * scale) > destRect.w) scale = destRect.w / srcW;

        float drawW = srcW * scale;
        float drawH = srcH * scale;

        float drawX = destRect.x + (destRect.w - drawW) / 2.0f;
        float drawY = destRect.y + (destRect.h - drawH) / 2.0f;

        SDL_FRect renderDst = { drawX, drawY, drawW, drawH };
        SDL_RenderTexture(renderer, texture, NULL, &renderDst);
    }
}

void game::renderTextCentered(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty()) return;

    float srcW = 0.0f, srcH = 0.0f;
    SDL_Texture* texture = getOrRenderText(text, fontId, color, srcW, srcH);

    if (texture && srcH > 0.0f)
    {
        float textX = targetRect.x + (targetRect.w - srcW) / 2.0f;
        float textY = targetRect.y + (targetRect.h - srcH) / 2.0f;
        SDL_FRect destRect = { textX, textY, srcW, srcH };
        SDL_RenderTexture(renderer, texture, NULL, &destRect);
    }
}

void game::renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(targetFont, text.c_str(), 0, color, static_cast<int>(targetRect.w));
    if (!surface) return;

    // Calculate maximum scroll height based on surface dimensions
    maxDescriptionScrollY = std::max(0.0f, static_cast<float>(surface->h) - targetRect.h);

    if (descriptionScrollY > maxDescriptionScrollY)
    {
        descriptionScrollY = maxDescriptionScrollY;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        // Render starting at the local viewport targetRect.y position
        SDL_FRect destRect = { targetRect.x, targetRect.y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
        SDL_RenderTexture(renderer, texture, NULL, &destRect);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

float game::renderTextLeftSegment(const std::vector<ColorToken>& tokens, float startX, float startY, float maxH, const std::string& fontId)
{
    float currentX = startX;

    for (const auto& token : tokens)
    {
        if (token.text.empty()) continue;

        float srcW = 0.0f, srcH = 0.0f;
        SDL_Texture* texture = getOrRenderText(token.text, fontId, token.color, srcW, srcH);

        if (texture && srcH > 0.0f)
        {
            float scale = maxH / srcH;
            float drawW = srcW * scale;
            float drawH = maxH;

            SDL_FRect renderDst = { currentX, startY, drawW, drawH };
            SDL_RenderTexture(renderer, texture, NULL, &renderDst);

            currentX += drawW;
        }
    }

    return currentX - startX;
}

void game::update() {}

void game::render()
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    if (currentState == GameState::MAIN_MENU) renderMainMenuLayout();
    else renderDashboardLayout();

    SDL_SetRenderViewport(renderer, NULL);
    SDL_RenderPresent(renderer);
}

SDL_Texture* game::getOrRenderText(const std::string& textStr, const std::string& fontId, SDL_Color color, float& outW, float& outH)
{
    if (textStr.empty()) return nullptr;

    std::string cacheKey = fontId + "_" + textStr + "_" +
        std::to_string(color.r) + "_" +
        std::to_string(color.g) + "_" +
        std::to_string(color.b) + "_" +
        std::to_string(color.a);

    auto it = textCache.find(cacheKey);
    if (it != textCache.end())
    {
        outW = static_cast<float>(it->second.w);
        outH = static_cast<float>(it->second.h);
        return it->second.texture;
    }

    if (textCache.size() > 300)
    {
        clearTextCache();
    }

    TTF_Font* font = fonts[fontId];
    if (!font) return nullptr;

    SDL_Surface* surface = TTF_RenderText_Blended(font, textStr.c_str(), 0, color);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    outW = static_cast<float>(surface->w);
    outH = static_cast<float>(surface->h);
    SDL_DestroySurface(surface);

    if (texture) textCache[cacheKey] = { texture, outW, outH };
    return texture;
}

void game::clearTextCache()
{
    for (auto& [key, cached] : textCache)
    {
        if (cached.texture) SDL_DestroyTexture(cached.texture);
    }
    textCache.clear();
}

std::shared_ptr<entity> game::generateEncounterNPC()
{
    static int npcCounter = 1;
    auto npc = std::make_shared<entity>("npc_bandit_" + std::to_string(npcCounter++), "Alleyway Bandit");

    npc->stats.level = 1;
    npc->stats.setBaseStat("health", 50.0f);
    npc->stats.setBaseStat("mana", 30.0f);
    npc->stats.setBaseStat("lust", 100.0f);

    // Populate initial NPC inventory and equipment
    auto shirt = itemDatabase::getItem("item_linen_shirt");
    auto pants = itemDatabase::getItem("item_leather_trousers");
    auto boots = itemDatabase::getItem("item_leather_boots");
    auto potion = itemDatabase::getItem("item_canis_root");

    std::vector<std::string> tags = npc->anatomy.getAllTags();

    if (shirt)
    {
        npc->inventory.addItem(shirt);
        npc->inventory.equipItem(0, equipSlot::TORSO_UNDER, tags);
    }
    if (pants)
    {
        npc->inventory.addItem(pants);
        npc->inventory.equipItem(0, equipSlot::LEGS_OUTER, tags);
    }
    if (boots)
    {
        npc->inventory.addItem(boots);
        npc->inventory.equipItem(0, equipSlot::FEET, tags);
    }
    if (potion)
    {
        potion->count = 3;
        npc->inventory.addItem(potion);
    }

    return npc;
}

void game::triggerEncounter(std::shared_ptr<entity> npc)
{
    activeTargetNPC = npc.get();
    activeTargetMode = TargetMode::COMBAT_ENEMY;
    currentState = GameState::EVENT;

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
    payChoice.nextSceneId = "ENCOUNTER_PAY";
    currentScene.choices.push_back(payChoice);

    dialogueChoice surrenderChoice;
    surrenderChoice.label = "Surrender";
    surrenderChoice.nextSceneId = "ENCOUNTER_SURRENDER";
    currentScene.choices.push_back(surrenderChoice);

    activeButtons.clear();
    for (const auto& choice : currentScene.choices)
    {
        actionButton btn;
        btn.label = choice.label;
        btn.onClick = [this, choice]()
            {
                processChoice(choice);
            };
        activeButtons.push_back(btn);
    }
}

std::array<actionButton, 15> game::getSlotsForCurrentActionPage()
{
    std::array<actionButton, 15> pageSlots;
    int itemsPerPage = 15;

    // 1. Separate pinned/fixed buttons from flow buttons
    std::vector<actionButton> flowButtons;

    for (const auto& btn : activeButtons)
    {
        // Pinned to all pages (e.g., Close Inventory at slot 14)
        if (btn.pinnedAllPages && btn.slotIndex >= 0 && btn.slotIndex < 15)
        {
            pageSlots[btn.slotIndex] = btn;
        }
        // Pinned to a specific slot on a specific page
        else if (btn.slotIndex >= 0)
        {
            int btnPage = btn.slotIndex / itemsPerPage;
            int localSlot = btn.slotIndex % itemsPerPage;
            if (btnPage == actionGridPage && localSlot >= 0 && localSlot < 15)
            {
                pageSlots[localSlot] = btn;
            }
        }
        // Sequential/Flow button (fills next available slot without gaps)
        else
        {
            flowButtons.push_back(btn);
        }
    }

    // 2. Fill flow buttons into the open slots across pages sequentially
    int currentFlowIdx = 0;
    int targetStartFlowIdx = actionGridPage * itemsPerPage; // Offset for current page

    // Skip flow buttons belonging to earlier pages
    for (int page = 0; page < actionGridPage; page++)
    {
        int slotsFilledOnPage = 0;
        for (int s = 0; s < 15; s++)
        {
            // If slot wasn't reserved/pinned, it consumes a flow button
            if (pageSlots[s].label.empty() && currentFlowIdx < static_cast<int>(flowButtons.size()))
            {
                currentFlowIdx++;
                slotsFilledOnPage++;
            }
        }
    }

    // Populate current page open slots with current flow buttons
    for (int s = 0; s < 15; s++)
    {
        if (pageSlots[s].label.empty() && currentFlowIdx < static_cast<int>(flowButtons.size()))
        {
            pageSlots[s] = flowButtons[currentFlowIdx++];
        }
    }

    return pageSlots;
}

void game::clean()
{
    clearTextCache();

    for (auto const& [id, f] : fonts) TTF_CloseFont(f);
    fonts.clear();
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}