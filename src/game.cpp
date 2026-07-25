#include "game.h"

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), Player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    map = nullptr;
    if (Player) delete Player;
}

bool game::loadMap(const std::string& mapId, int startX, int startY)
{
    if (mapCache.find(mapId) == mapCache.end())
    {
        gameMap newMap;
        if (!newMap.loadFromFile("data/maps/" + mapId + ".json"))
        {
            return false;
        }
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
            saveManager::createInitialSave(this, "saves/save_01.json");
            saveManager::loadGame(this, "saves/save_01.json");
        }
    }

    questDatabase::loadDatabase("data/quests");
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
            int newWidth = event.window.data1;
            int newHeight = event.window.data2;
            SDL_SetRenderLogicalPresentation(renderer, newWidth, newHeight, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                handleMouseClick(event.button.x, event.button.y);
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            // 1. Hotkeys & Menus
            if (event.key.key == SDLK_I)
            {
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
                saveManager::saveGame(this, "saves/save_01.json");
                return;
            }
            if (event.key.key == SDLK_F9)
            {
                saveManager::loadGame(this, "saves/save_01.json");
                return;
            }

            // 2. Exploration Movement (ONLY move if arrow keys are actually pressed!)
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

                if (isMoveKey)
                {
                    movePlayer(nextX, nextY);
                }
            }
        }
    }
}

void game::handleMouseClick(float windowX, float windowY)
{
    float mouseX, mouseY;
    SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, &mouseX, &mouseY);

    int w = 0, h = 0;
    SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);
    if (w == 0 || h == 0) SDL_GetRenderOutputSize(renderer, &w, &h);

    updateLayoutBounds(w, h);
    int padding = 12;

    if (currentState == GameState::EXPLORATION || currentState == GameState::EVENT)
    {
        if (currentState == GameState::EXPLORATION)
        {
            if (mouseX >= layout.mapRect.x && mouseX <= layout.mapRect.x + layout.mapRect.w &&
                mouseY >= layout.mapRect.y && mouseY <= layout.mapRect.y + layout.mapRect.h)
            {
                float localX = mouseX - layout.mapRect.x;
                float localY = mouseY - layout.mapRect.y;

                int tileGap = 2;
                int availableForTiles = layout.mapRect.w - (2 * padding) - (4 * tileGap);
                int drawnTileSize = availableForTiles / 5;
                int totalGridSize = (drawnTileSize * 5) + (tileGap * 4);

                int offsetX = (layout.mapRect.w - totalGridSize) / 2;
                int offsetY = (layout.mapRect.h - totalGridSize) / 2;

                if (localX >= offsetX && localY >= offsetY)
                {
                    int gridCol = (int)((localX - offsetX) / (drawnTileSize + tileGap));
                    int gridRow = (int)((localY - offsetY) / (drawnTileSize + tileGap));

                    if (gridCol >= 0 && gridCol < 5 && gridRow >= 0 && gridRow < 5)
                    {
                        int dx = gridCol - 2;
                        int dy = gridRow - 2;

                        if (std::abs(dx) + std::abs(dy) == 1)
                        {
                            movePlayer(gridX + dx, gridY + dy);
                        }
                    }
                }
                return;
            }
        }

        if (mouseX >= layout.actionGridRect.x && mouseX <= layout.actionGridRect.x + layout.actionGridRect.w &&
            mouseY >= layout.actionGridRect.y && mouseY <= layout.actionGridRect.y + layout.actionGridRect.h)
        {
            int cols = 5, rows = 3;
            float gap = 8.0f, verticalPadding = 15.0f, horizontalPadding = 40.0f;
            float availableW = layout.actionGridRect.w - (horizontalPadding * 2) - (gap * (cols - 1));
            float availableH = layout.actionGridRect.h - (verticalPadding * 2) - (gap * (rows - 1));
            float btnWidth = availableW / cols, btnHeight = availableH / rows;

            float localX = mouseX - layout.actionGridRect.x - horizontalPadding;
            float localY = mouseY - layout.actionGridRect.y - verticalPadding;

            if (localX >= 0 && localY >= 0)
            {
                int clickedCol = (int)(localX / (btnWidth + gap));
                int clickedRow = (int)(localY / (btnHeight + gap));

                if (clickedCol >= 0 && clickedCol < cols && clickedRow >= 0 && clickedRow < rows)
                {
                    int index = (clickedRow * cols) + clickedCol;

                    if (index < activeButtons.size())
                    {
                        if (activeButtons[index].command == "SCENE_CHOICE")
                        {
                            int choiceIdx = std::stoi(activeButtons[index].payload);
                            processChoice(currentScene.choices[choiceIdx]);
                        }
                        else if (activeButtons[index].command == "START_SCENE")
                        {
                            loadScene(activeButtons[index].payload);
                        }
                        else if (activeButtons[index].command == "MAP_WARP")
                        {
                            std::string payload = activeButtons[index].payload;
                            size_t c1 = payload.find(',');
                            size_t c2 = payload.find(',', c1 + 1);

                            if (c1 != std::string::npos && c2 != std::string::npos)
                            {
                                std::string targetMap = payload.substr(0, c1);
                                int targetX = std::stoi(payload.substr(c1 + 1, c2 - c1 - 1));
                                int targetY = std::stoi(payload.substr(c2 + 1));

                                loadMap(targetMap, targetX, targetY);
                            }
                        }
                    }
                }
            }
        }
    }
    else if (currentState == GameState::INVENTORY)
    {
        if (mouseX >= layout.equipRect.x && mouseX <= layout.equipRect.x + layout.equipRect.w &&
            mouseY >= layout.equipRect.y && mouseY <= layout.equipRect.y + layout.equipRect.h)
        {
            int cols = 6, rows = 6, slotGap = 4;
            int internalPadding = padding + 6;

            int availableW = layout.equipRect.w - (2 * internalPadding) - ((cols - 1) * slotGap);
            int availableH = layout.equipRect.h - (2 * internalPadding) - ((rows - 1) * slotGap);
            int slotSize = std::min(availableW / cols, availableH / rows);

            int gridW = (slotSize * cols) + (slotGap * (cols - 1));
            int gridH = (slotSize * rows) + (slotGap * (rows - 1));

            int offsetX = layout.equipRect.x + (layout.equipRect.w - gridW) / 2;
            int offsetY = layout.equipRect.y + (layout.equipRect.h - gridH) / 2;

            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    int slotIdx = (r * cols) + c;
                    SDL_FRect slot = { (float)(offsetX + (c * (slotSize + slotGap))), (float)(offsetY + (r * (slotSize + slotGap))), (float)slotSize, (float)slotSize };

                    if (mouseX >= slot.x && mouseX <= slot.x + slot.w &&
                        mouseY >= slot.y && mouseY <= slot.y + slot.h)
                    {
                        selectedEquipmentSlot = equipSlot::NONE;
                        selectedInventoryIndex = -1;

                        if (Player)
                        {
                            for (const auto& [eSlot, eqItem] : Player->inventory.equipped)
                            {
                                if (getEquipmentGridIndex(eSlot) == slotIdx && !eqItem->id.empty())
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

        if (mouseX >= layout.inventoryRect.x && mouseX <= layout.inventoryRect.x + layout.inventoryRect.w &&
            mouseY >= layout.inventoryRect.y && mouseY <= layout.inventoryRect.y + layout.inventoryRect.h)
        {
            int cols = 6, rows = 5, slotGap = 4;
            int halfWidth = layout.inventoryRect.w / 2;
            int sidePadding = 20, topPadding = 60;

            int availableW = halfWidth - (2 * sidePadding) - ((cols - 1) * slotGap);
            int availableH = layout.inventoryRect.h - topPadding - sidePadding - ((rows - 1) * slotGap);
            int slotSize = std::min(availableW / cols, availableH / rows);

            int gridW = (slotSize * cols) + (slotGap * (cols - 1));
            int leftOffsetX = layout.inventoryRect.x + (halfWidth - gridW) / 2;
            int rightOffsetX = layout.inventoryRect.x + halfWidth + (halfWidth - gridW) / 2;
            int gridOffsetY = layout.inventoryRect.y + topPadding;
            int maxSlotsPerSide = cols * rows;

            for (int i = 0; i < maxSlotsPerSide * 2; i++)
            {
                int side = i / maxSlotsPerSide;
                int localIndex = i % maxSlotsPerSide;
                int r = localIndex / cols;
                int c = localIndex % cols;
                float originX = (side == 0) ? (float)leftOffsetX : (float)rightOffsetX;

                SDL_FRect slot = { originX + (c * (slotSize + slotGap)), (float)(gridOffsetY + (r * (slotSize + slotGap))), (float)slotSize, (float)slotSize };

                if (mouseX >= slot.x && mouseX <= slot.x + slot.w &&
                    mouseY >= slot.y && mouseY <= slot.y + slot.h)
                {
                    selectedEquipmentSlot = equipSlot::NONE;
                    if (Player && i < Player->inventory.backpack.size())
                    {
                        selectedInventoryIndex = i;
                    }
                    else
                    {
                        selectedInventoryIndex = -1;
                    }
                    refreshActionGrid();
                    return;
                }
            }
        }

        if (mouseX >= layout.actionGridRect.x && mouseX <= layout.actionGridRect.x + layout.actionGridRect.w &&
            mouseY >= layout.actionGridRect.y && mouseY <= layout.actionGridRect.y + layout.actionGridRect.h)
        {
            int cols = 5, rows = 3;
            float gap = 8.0f, verticalPadding = 15.0f, horizontalPadding = 40.0f;
            float availableW = layout.actionGridRect.w - (horizontalPadding * 2) - (gap * (cols - 1));
            float availableH = layout.actionGridRect.h - (verticalPadding * 2) - (gap * (rows - 1));
            float btnWidth = availableW / cols, btnHeight = availableH / rows;

            float localX = mouseX - layout.actionGridRect.x - horizontalPadding;
            float localY = mouseY - layout.actionGridRect.y - verticalPadding;

            if (localX >= 0 && localY >= 0)
            {
                int clickedCol = (int)(localX / (btnWidth + gap));
                int clickedRow = (int)(localY / (btnHeight + gap));

                if (clickedCol >= 0 && clickedCol < cols && clickedRow >= 0 && clickedRow < rows)
                {
                    int index = (clickedRow * cols) + clickedCol;

                    if (index < activeButtons.size())
                    {
                        if (activeButtons[index].command == "EQUIP_ITEM")
                        {
                            handleEquipAction(std::stoi(activeButtons[index].payload));
                        }
                        else if (activeButtons[index].command == "UNEQUIP_ITEM")
                        {
                            equipSlot slotToUnequip = (equipSlot)std::stoi(activeButtons[index].payload);
                            handleUnequipAction(slotToUnequip);
                        }
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
        case equipSlot::HEADWEAR:        return 1;
        case equipSlot::EYEWEAR:         return 2;
        case equipSlot::HORNS_SLOT:      return 3;
        case equipSlot::PIERCING_EAR:    return 4;

        case equipSlot::NECKWEAR:        return 7;
        case equipSlot::SHOULDERS:       return 8;
        case equipSlot::CHEST_WEAR:      return 9;
        case equipSlot::TORSO_OVER:      return 10;

        case equipSlot::WEAPON_MAIN:     return 12;
        case equipSlot::HANDS:           return 13;
        case equipSlot::TORSO_UNDER:     return 14;
        case equipSlot::WEAPON_OFF:      return 15;

        case equipSlot::STOMACH_WEAR:    return 19;
        case equipSlot::HIPS_WEAR:       return 20;
        case equipSlot::TAIL_SLOT:       return 21;

        case equipSlot::GROIN_OVER:      return 25;
        case equipSlot::LEGS_INNER:      return 26;
        case equipSlot::LEGS_OUTER:      return 27;

        case equipSlot::FEET:            return 32;
        case equipSlot::WINGS_SLOT:      return 33;

        default:                         return -1;
    }
}

void game::handleEquipAction(int backpackIndex)
{
    if (!Player || backpackIndex < 0 || (size_t)backpackIndex >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];
    if (!targetItem->isEquippable) return;

    std::vector<std::string> bodyTags;
    bool success = Player->inventory.equipItem((size_t)backpackIndex, targetItem->targetSlot, bodyTags);

    if (success)
    {
        selectedInventoryIndex = -1;
        refreshActionGrid();
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    bool success = Player->inventory.unequipItem(slot);
    if (success)
    {
        selectedEquipmentSlot = equipSlot::NONE;
        selectedInventoryIndex = -1;
        refreshActionGrid();
    }
}

void game::refreshActionGrid()
{
    activeButtons.clear();

    if (currentState == GameState::EXPLORATION)
    {
        // 1. Check tile door warps
        MapWarp warp;
        if (map && map->checkWarp(gridX, gridY, warp))
        {
            actionButton warpBtn;
            warpBtn.label = "Enter Door";
            warpBtn.command = "MAP_WARP";
            warpBtn.payload = warp.targetMap + "," + std::to_string(warp.targetX) + "," + std::to_string(warp.targetY);
            activeButtons.push_back(warpBtn);
        }

        // 2. Fetch active dynamic triggers from the Quest Database for the current map & tile
        if (map)
        {
            auto triggers = questDatabase::getTriggersForLocation(map->getId(), gridX, gridY);
            for (const auto& trig : triggers)
            {
                // Evaluates quest stage, time phase, player stats, traits, and required items
                if (checkConditions(trig.conditions))
                {
                    actionButton triggerBtn;
                    triggerBtn.label = trig.label;
                    triggerBtn.command = "START_SCENE";
                    triggerBtn.payload = trig.sceneId;
                    activeButtons.push_back(triggerBtn);
                }
            }
        }
    }
    else if (currentState == GameState::INVENTORY)
    {
        // 1. If an item in the backpack is selected -> Show "Equip"
        if (selectedInventoryIndex >= 0 && Player && (size_t)selectedInventoryIndex < Player->inventory.backpack.size())
        {
            const std::shared_ptr<item> selItem = Player->inventory.backpack[selectedInventoryIndex];
            if (selItem->isEquippable)
            {
                actionButton equipBtn;
                equipBtn.label = "Equip " + selItem->name;
                equipBtn.command = "EQUIP_ITEM";
                equipBtn.payload = std::to_string(selectedInventoryIndex);
                activeButtons.push_back(equipBtn);
            }
        }
        // 2. If an equipped item is selected -> Show "Unequip"
        else if (selectedEquipmentSlot != equipSlot::NONE && Player && Player->inventory.isEquipped(selectedEquipmentSlot))
        {
            std::shared_ptr<item> eqItem = Player->inventory.getEquippedItem(selectedEquipmentSlot);
            if (eqItem)
            {
                actionButton unequipBtn;
                unequipBtn.label = "Unequip " + eqItem->name;
                unequipBtn.command = "UNEQUIP_ITEM";
                unequipBtn.payload = std::to_string((int)selectedEquipmentSlot);
                activeButtons.push_back(unequipBtn);
            }
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
            // Checks if current time phase matches target e.g. "NIGHT", "DAY", "DAWN", "DUSK"
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
    // Handle Special Encounter Exit States
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        std::cout << "[Encounter] Combat Initiated!\n";
        // TODO: Transition to combat state/loop here
        currentState = GameState::EXPLORATION;
        refreshActionGrid();
        return;
    }
    else if (choice.nextSceneId == "ENCOUNTER_PAY")
    {
        std::cout << "[Encounter] Paid / Bribed Enemy!\n";
        Player->stats.modifyBaseStat("currency", -25.0f);
        currentState = GameState::EXPLORATION;
        refreshActionGrid();
        return;
    }
    else if (choice.nextSceneId == "ENCOUNTER_SURRENDER")
    {
        std::cout << "[Encounter] Surrendered!\n";
        currentState = GameState::EXPLORATION;
        refreshActionGrid();
        return;
    }

    // Process Regular Quest Effects
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
            Player->stats.modifyBaseStat(effect.target, (float)effect.amount);
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
            btn.command = "SCENE_CHOICE";
            btn.payload = std::to_string(i);
            activeButtons.push_back(btn);
        }
    }
}

void game::drawTextFit(const std::string& textStr, SDL_FRect destRect, SDL_Color color, const std::string& fontId)
{
    float srcW = 0.0f, srcH = 0.0f;
    SDL_Texture* texture = getOrRenderText(textStr, fontId, color, srcW, srcH);

    if (texture && srcH > 0.0f)
    {
        // Scale to fit target height while preserving aspect ratio
        float scale = destRect.h / srcH;

        // If scaled width exceeds bounding box, constrain by width instead
        if ((srcW * scale) > destRect.w)
        {
            scale = destRect.w / srcW;
        }

        float drawW = srcW * scale;
        float drawH = srcH * scale;

        // Center horizontally & vertically inside destRect
        float drawX = destRect.x + (destRect.w - drawW) / 2.0f;
        float drawY = destRect.y + (destRect.h - drawH) / 2.0f;

        SDL_FRect renderDst = { drawX, drawY, drawW, drawH };
        SDL_RenderTexture(renderer, texture, NULL, &renderDst);
    }
}

void game::renderTextCentered(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended(targetFont, text.c_str(), 0, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        float textX = targetRect.x + (targetRect.w - surface->w) / 2.0f;
        float textY = targetRect.y + (targetRect.h - surface->h) / 2.0f;
        SDL_FRect destRect = { textX, textY, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(renderer, texture, NULL, &destRect);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

void game::renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(targetFont, text.c_str(), 0, color, (int)targetRect.w - 40);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_FRect destRect = { targetRect.x + 20.0f, targetRect.y + 20.0f, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(renderer, texture, NULL, &destRect);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
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

    // Safety check: Prevent unbounded growth by clearing cache if it exceeds 300 unique text styles
    if (textCache.size() > 300)
    {
        for (auto& pair : textCache)
        {
            if (pair.second.texture) SDL_DestroyTexture(pair.second.texture);
        }
        textCache.clear();
    }

    TTF_Font* font = fonts[fontId];
    if (!font) return nullptr;

    SDL_Surface* surface = TTF_RenderText_Blended(font, textStr.c_str(), 0, color);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    outW = static_cast<float>(surface->w);
    outH = static_cast<float>(surface->h);

    float surfW = static_cast<float>(surface->w);
    float surfH = static_cast<float>(surface->h);
    SDL_DestroySurface(surface);

    if (texture)
    {
        textCache[cacheKey] = { texture, surfW, surfH };
    }

    return texture;
}

void game::clearTextCache()
{
    for (auto& [key, cached] : textCache)
    {
        if (cached.texture)
        {
            SDL_DestroyTexture(cached.texture);
        }
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
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
        actionButton btn;
        btn.label = currentScene.choices[i].label;
        btn.command = "SCENE_CHOICE";
        btn.payload = std::to_string(i);
        activeButtons.push_back(btn);
    }
}

void game::renderNPCTargetPanel(float x, float y, float w, float h)
{
    if (!activeTargetNPC) return;

    // 1. Establish Local Viewport (0, 0 is now top-left of the target panel)
    SDL_Rect panelBox = { (int)x, (int)y, (int)w, (int)h };
    ViewportGuard vpGuard(renderer, &panelBox);

    // 2. Local Outer Card Bounds
    SDL_FRect cardRect = { 0.0f, 0.0f, w, h };
    renderFillRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    float padX = w * 0.04f;
    float padY = h * 0.04f;
    float contentW = w - (padX * 2.0f);
    float currentY = padY;
    float dividerGap = h * 0.025f;

    // 3. Avatar & Header Text (Local Coordinates)
    float avatarSize = h * 0.16f;
    SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
    renderFillRoundedRect(renderer, avatarRect, 4.0f, { 50, 50, 60, 255 });
    renderDrawRoundedRect(renderer, avatarRect, 4.0f, { 255, 120, 170, 255 });

    float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
    std::string nameLevelStr = activeTargetNPC->name + " - Level " + std::to_string(activeTargetNPC->stats.level);
    float headerTextH = avatarSize * 0.55f;

    SDL_FRect nameRect = { headerTextX, currentY, w - headerTextX - padX, headerTextH };
    drawTextFit(nameLevelStr, nameRect, { 255, 120, 170, 255 }, "title_font");

    currentY += avatarSize + dividerGap;

    // 4. Vital Bars (Health, Mana, Lust - Local Coordinates)
    float barHeight = h * 0.065f;
    float iconRadius = barHeight * 1.20f;
    float valueTextWidth = contentW * 0.18f;
    float barW = contentW - iconRadius - valueTextWidth - (padX * 0.5f);
    float barGap = h * 0.02f;

    auto drawVitalBar = [&](float barYPos, const std::string& statName, float maxVal, SDL_Color barColor)
        {
            SDL_FRect iconRect = { padX, barYPos, iconRadius, iconRadius };
            renderFillRoundedRect(renderer, iconRect, 3.0f, { 45, 40, 50, 255 });

            float fillX = padX + iconRadius + (padX * 0.5f);
            float fillY = barYPos + (iconRadius - barHeight) / 2.0f;

            SDL_FRect bgRect = { fillX, fillY, barW, barHeight };
            renderFillRoundedRect(renderer, bgRect, 3.0f, { 20, 18, 25, 255 });

            float currentVal = activeTargetNPC->getStat(statName);
            float fillPct = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
            if (fillPct > 0.0f)
            {
                SDL_FRect fillRect = { fillX, fillY, barW * fillPct, barHeight };
                renderFillRoundedRect(renderer, fillRect, 3.0f, barColor);
            }

            SDL_FRect textRect = { fillX + barW + (padX * 0.4f), barYPos, valueTextWidth, iconRadius };
            drawTextFit(std::to_string((int)currentVal), textRect, { 240, 240, 240, 255 });
        };

    drawVitalBar(currentY, "health", 100.0f, { 255, 60, 90, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "mana", 100.0f, { 220, 130, 255, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "lust", 100.0f, { 230, 50, 150, 255 });
}

void game::renderNPCAnatomyTooltip(float mouseX, float mouseY)
{
    if (!activeTargetNPC) return;

    int screenW = 1280, screenH = 720;
    SDL_GetRenderOutputSize(renderer, &screenW, &screenH);

    static const std::vector<bodySlot> anatomicalOrder = {
        bodySlot::HAIR, bodySlot::HEAD, bodySlot::EYES, bodySlot::EARS, bodySlot::HORNS,
        bodySlot::MOUTH, bodySlot::NECK, bodySlot::TORSO, bodySlot::BREASTS, bodySlot::STOMACH,
        bodySlot::BACK, bodySlot::ARMS, bodySlot::HANDS, bodySlot::FINGERS, bodySlot::HIPS,
        bodySlot::GROIN, bodySlot::ASS, bodySlot::TAIL, bodySlot::LEGS, bodySlot::FEET,
        bodySlot::WINGS, bodySlot::TENTACLES, bodySlot::ANTENNAE
    };

    float headerH = screenH * 0.032f;
    float subHeaderH = screenH * 0.024f;
    float lineH = screenH * 0.025f;
    float fontH = lineH * 0.80f;
    float padding = screenW * 0.008f;
    float bulletSize = fontH * 0.45f;

    struct RenderRowData
    {
        bool isOccupied = false;
        SDL_Color bulletColor = { 100, 100, 110, 255 };
        std::vector<ColorToken> tokens;
    };

    std::vector<RenderRowData> rows;
    float maxContentW = screenW * 0.18f;

    for (bodySlot slot : anatomicalOrder)
    {
        RenderRowData row;
        const bodyPart* part = activeTargetNPC->anatomy.getPart(slot);

        if (part != nullptr)
        {
            row.isOccupied = true;
            row.bulletColor = getColorFromName(part->primaryColor);

            std::string prefixStr = "";
            if (slot == bodySlot::GROIN && part->length > 0.0f)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%s (%gcm long, %gcm diameter)", part->name.c_str(), part->length, part->diameter);
                prefixStr = std::string(buf);
            }
            else if (slot == bodySlot::BREASTS && part->cupSize > 0)
            {
                prefixStr = part->name + " (" + bodyPart::getCupSizeName(part->cupSize) + "-cup)";
            }
            else
            {
                if (part->count == 2) prefixStr = "Two ";
                else if (part->count > 2) prefixStr = std::to_string(part->count) + " ";

                if (!part->style.empty()) prefixStr += part->style + " ";
                prefixStr += part->name;
            }

            std::string coveringNoun = getCoveringNoun(part->covering);

            row.tokens.push_back({ prefixStr + ": ", { 220, 220, 230, 255 } });
            row.tokens.push_back({ part->race + " - ", { 180, 100, 255, 255 } });

            if (!part->secondaryColor.empty())
            {
                row.tokens.push_back({ part->secondaryColor, getColorFromName(part->secondaryColor) });
                row.tokens.push_back({ "-rimmed, ", { 220, 220, 230, 255 } });
            }

            row.tokens.push_back({ part->primaryColor + " ", row.bulletColor });
            row.tokens.push_back({ coveringNoun, { 200, 200, 210, 255 } });
        }
        else
        {
            row.isOccupied = false;
            row.bulletColor = { 65, 65, 75, 255 };

            std::string slotLabel = getSlotName(slot);
            row.tokens.push_back({ slotLabel + ": ", { 110, 110, 125, 255 } });
            row.tokens.push_back({ "None", { 140, 140, 150, 255 } });
        }

        float rowW = 0.0f;
        for (const auto& tok : row.tokens)
        {
            float srcW = 0.0f, srcH = 0.0f;
            getOrRenderText(tok.text, "button_font", tok.color, srcW, srcH);
            if (srcH > 0.0f) rowW += srcW * (fontH / srcH);
        }

        if (rowW > maxContentW) maxContentW = rowW;
        rows.push_back(row);
    }

    float textStartXOffset = bulletSize + (padding * 0.8f);
    float boxWidth = maxContentW + textStartXOffset + (padding * 2.5f);

    int itemLines = (int)rows.size();
    float boxHeight = headerH + subHeaderH + padding + (itemLines * lineH) + padding;

    // Position card to the LEFT of the mouse cursor
    float boxX = mouseX - boxWidth - 12.0f;
    float boxY = mouseY;

    if (boxX < 10.0f) boxX = mouseX + 12.0f; // Edge flip guard
    if (boxY + boxHeight > screenH) boxY = screenH - boxHeight - 10.0f;

    SDL_FRect tooltipRect = { boxX, boxY, boxWidth, boxHeight };

    // Card Frame
    renderFillRoundedRect(renderer, tooltipRect, 6.0f, { 25, 23, 30, 250 });
    renderDrawRoundedRect(renderer, tooltipRect, 6.0f, { 255, 120, 170, 255 });

    // Headers
    SDL_FRect titleRect = { boxX + padding, boxY + padding, boxWidth - (padding * 2.0f), headerH };
    drawTextFit(activeTargetNPC->name, titleRect, { 255, 120, 170, 255 }, "title_font");

    char subTitleBuffer[128];
    snprintf(subTitleBuffer, sizeof(subTitleBuffer), "Masculine | Fit body | %.2fm tall", activeTargetNPC->anatomy.heightMeters);

    SDL_FRect subTitleRect = { boxX + padding, boxY + padding + headerH, boxWidth - (padding * 2.0f), subHeaderH };
    drawTextFit(subTitleBuffer, subTitleRect, { 100, 200, 255, 255 }, "button_font");

    float dividerY = boxY + padding + headerH + subHeaderH + (padding * 0.4f);
    SDL_SetRenderDrawColor(renderer, 60, 50, 75, 255);
    SDL_RenderLine(renderer, boxX + padding, dividerY, boxX + boxWidth - padding, dividerY);

    float currentY = dividerY + (padding * 0.4f);

    // Body Part Rows
    for (const auto& row : rows)
    {
        float textY = currentY + (lineH - fontH) * 0.5f;
        float bulletY = textY + (fontH - bulletSize) * 0.5f;

        SDL_FRect colorBullet = { boxX + padding, bulletY, bulletSize, bulletSize };
        renderFillRoundedRect(renderer, colorBullet, 2.0f, row.bulletColor);

        float textX = boxX + padding + textStartXOffset;
        renderTextLeftSegment(row.tokens, textX, textY, fontH, "button_font");

        currentY += lineH;
    }
}

void game::clean()
{
    clearTextCache();

    for (auto const& [id, f] : fonts)
    {
        TTF_CloseFont(f);
    }
    fonts.clear();
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}