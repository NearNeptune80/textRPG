#include "game.h"
#include "uiWidget.h"

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
            SDL_SetRenderLogicalPresentation(renderer, event.window.data1, event.window.data2, SDL_LOGICAL_PRESENTATION_STRETCH);
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
            if (event.key.key == SDLK_I)
            {
                // Softlock Prevention: Cannot open/close inventory during active cutscenes or encounter events
                if (currentState == GameState::EVENT)
                {
                    return;
                }

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
    layout.textMainRect = { centerX, colStartY, centerColW, (colEndY - btnH - padding) - colStartY };
    layout.actionGridRect = { centerX, colEndY - btnH, centerColW, btnH };

    // Right Column Stack
    float rightAvailableH = colEndY - colStartY;
    float rightStackH = (rightAvailableH - (2.0f * padding)) / 3.0f;

    layout.rightStackTop = { rightX, colStartY, rightColW, rightStackH };
    layout.rightStackMid = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    layout.rightStackBot = { rightX, colStartY + (rightStackH + padding) * 2.0f, rightColW, rightAvailableH - (rightStackH * 2.0f + padding * 2.0f) };

    layout.equipRect = layout.mapRect;
    layout.inventoryRect = layout.textMainRect;

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

    // 1. Action Grid Clicks (Direct Lambda Invocation)
    if (UIGridHelper::contains(layout.actionGridRect, mouseX, mouseY))
    {
        int cols = 5, rows = 3;
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                SDL_FRect btn = UIGridHelper::getActionButtonRect(layout.actionGridRect, c, r, cols, rows);
                if (UIGridHelper::contains(btn, mouseX, mouseY))
                {
                    int index = (r * cols) + c;
                    if (index < static_cast<int>(activeButtons.size()) && activeButtons[index].onClick)
                    {
                        activeButtons[index].onClick();
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
        if (UIGridHelper::contains(layout.equipRect, mouseX, mouseY))
        {
            int cols = 6, rows = 6;
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(layout.equipRect, c, r, cols, rows, 4, padding);
                    if (UIGridHelper::contains(slot, mouseX, mouseY))
                    {
                        int slotIdx = (r * cols) + c;
                        selectedEquipmentSlot = equipSlot::NONE;
                        selectedInventoryIndex = -1;

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

        if (UIGridHelper::contains(layout.inventoryRect, mouseX, mouseY))
        {
            int cols = 6, rows = 5;
            int maxSlots = cols * rows * 2;

            for (int i = 0; i < maxSlots; i++)
            {
                SDL_FRect slot = UIGridHelper::getInventorySlotRect(layout.inventoryRect, i, cols, rows);
                if (UIGridHelper::contains(slot, mouseX, mouseY))
                {
                    selectedEquipmentSlot = equipSlot::NONE;
                    if (Player && i < static_cast<int>(Player->inventory.backpack.size())) selectedInventoryIndex = i;
                    else selectedInventoryIndex = -1;

                    refreshActionGrid();
                    return;
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
    if (!Player || backpackIndex < 0 || static_cast<size_t>(backpackIndex) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];
    if (!targetItem->isEquippable) return;

    std::vector<std::string> bodyTags;
    if (Player->inventory.equipItem(static_cast<size_t>(backpackIndex), targetItem->targetSlot, bodyTags))
    {
        selectedInventoryIndex = -1;
        refreshActionGrid();
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    if (Player->inventory.unequipItem(slot))
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
        MapWarp warp;
        if (map && map->checkWarp(gridX, gridY, warp))
        {
            actionButton warpBtn;
            warpBtn.label = "Enter Door";
            warpBtn.onClick = [this, warp]()
                {
                    loadMap(warp.targetMap, warp.targetX, warp.targetY);
                };
            activeButtons.push_back(warpBtn);
        }

        if (map)
        {
            auto triggers = questDatabase::getTriggersForLocation(map->getId(), gridX, gridY);
            for (const auto& trig : triggers)
            {
                if (checkConditions(trig.conditions))
                {
                    actionButton triggerBtn;
                    triggerBtn.label = trig.label;
                    triggerBtn.onClick = [this, trig]()
                        {
                            loadScene(trig.sceneId);
                        };
                    activeButtons.push_back(triggerBtn);
                }
            }
        }
    }
    else if (currentState == GameState::INVENTORY)
    {
        if (selectedInventoryIndex >= 0 && Player && static_cast<size_t>(selectedInventoryIndex) < Player->inventory.backpack.size())
        {
            const std::shared_ptr<item> selItem = Player->inventory.backpack[selectedInventoryIndex];
            if (selItem->isEquippable)
            {
                actionButton equipBtn;
                equipBtn.label = "Equip " + selItem->name;
                int idx = selectedInventoryIndex;
                equipBtn.onClick = [this, idx]()
                    {
                        handleEquipAction(idx);
                    };
                activeButtons.push_back(equipBtn);
            }
        }
        else if (selectedEquipmentSlot != equipSlot::NONE && Player && Player->inventory.isEquipped(selectedEquipmentSlot))
        {
            std::shared_ptr<item> eqItem = Player->inventory.getEquippedItem(selectedEquipmentSlot);
            if (eqItem)
            {
                actionButton unequipBtn;
                unequipBtn.label = "Unequip " + eqItem->name;
                equipSlot slot = selectedEquipmentSlot;
                unequipBtn.onClick = [this, slot]()
                    {
                        handleUnequipAction(slot);
                    };
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
    if (choice.nextSceneId == "ENCOUNTER_FIGHT" ||
        choice.nextSceneId == "ENCOUNTER_PAY" ||
        choice.nextSceneId == "ENCOUNTER_SURRENDER")
    {
        if (choice.nextSceneId == "ENCOUNTER_PAY")
        {
            Player->stats.modifyBaseStat("currency", -25.0f);
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        currentState = GameState::EXPLORATION;
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
            UI::DrawInventoryGrid(renderer, this, layout.inventoryRect, Player, selectedInventoryIndex);
            renderRightColumn(layout.rightStackTop, layout.rightStackMid, layout.rightStackBot);
        }
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
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended(targetFont, text.c_str(), 0, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        float textX = targetRect.x + (targetRect.w - surface->w) / 2.0f;
        float textY = targetRect.y + (targetRect.h - surface->h) / 2.0f;
        SDL_FRect destRect = { textX, textY, static_cast<float>(surface->w), static_cast<float>(surface->h) };
        SDL_RenderTexture(renderer, texture, NULL, &destRect);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

void game::renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(targetFont, text.c_str(), 0, color, static_cast<int>(targetRect.w) - 40);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_FRect destRect = { targetRect.x + 20.0f, targetRect.y + 20.0f, static_cast<float>(surface->w), static_cast<float>(surface->h) };
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