#include "game.h"

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), Player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    delete map;
    if (Player) delete Player;
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

    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_STRETCH);

    map = new gameMap();
    map->updateDiscovery(gridX, gridY);

    if (TTF_Init() < 0)
    {
        std::cout << "Error initializing SDL_ttf: " << SDL_GetError() << "\n";
    }

    loadFont("button_font", "data/fonts/Roboto/static/Roboto-Regular.ttf", 18);
    loadFont("title_font", "data/fonts/Roboto/static/Roboto-Bold.ttf", 24);

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        Player = new entity("player_1", "Oellanix");

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

        Player->inventory.addItem(itemDatabase::getItem("item_canis_root"));
        Player->inventory.addItem(itemDatabase::getItem("item_leather_collar"));

        Player->stats.setStat("health", 68.0f);
        Player->stats.setStat("mana", 91.0f);
        Player->stats.setStat("lust", 100.0f);
        Player->stats.setStat("corruption", 0.0f);
    }

    questDatabase::loadDatabase("data/quests.json");

    refreshActionGrid();
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
            if (event.key.key == SDLK_I)
            {
                selectedInventoryIndex = -1; // Reset selection
                if (currentState == GameState::EXPLORATION) currentState = GameState::INVENTORY;
                else if (currentState == GameState::INVENTORY) currentState = GameState::EXPLORATION;

                refreshActionGrid();
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
                    refreshActionGrid();
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
    float scale;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);
    if (w == 0 || h == 0) SDL_GetRenderOutputSize(renderer, &w, &h);

    int padding = 12;
    int colEndY = h - padding;

    // --- EXPLORATION & EVENT MODES ---
    if (currentState == GameState::EXPLORATION || currentState == GameState::EVENT)
    {
        // 1. MAP CLICK DETECTION (Exploration only)
        if (currentState == GameState::EXPLORATION)
        {
            int mapSize = (int)(h * 0.30f);
            SDL_Rect mapRect = { padding, colEndY - mapSize, mapSize, mapSize };

            if (mouseX >= mapRect.x && mouseX <= mapRect.x + mapRect.w &&
                mouseY >= mapRect.y && mouseY <= mapRect.y + mapRect.h)
            {
                float localX = mouseX - mapRect.x;
                float localY = mouseY - mapRect.y;

                int tileGap = 2;
                int availableForTiles = mapRect.w - (2 * padding) - (4 * tileGap);
                int drawnTileSize = availableForTiles / 5;
                int totalGridSize = (drawnTileSize * 5) + (tileGap * 4);

                int offsetX = (mapRect.w - totalGridSize) / 2;
                int offsetY = (mapRect.h - totalGridSize) / 2;

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
                            int nextX = gridX + dx;
                            int nextY = gridY + dy;

                            if (map->isWalkable(nextX, nextY))
                            {
                                gridX = nextX;
                                gridY = nextY;
                                map->updateDiscovery(gridX, gridY);
                                refreshActionGrid();
                            }
                        }
                    }
                }
                return;
            }
        }

        // 2. ACTION GRID CLICK DETECTION (Exploration & Event)
        int mapSize = (int)(h * 0.30f);
        int leftColW = mapSize;
        int rightColW = mapSize;
        int centerColW = w - (leftColW + rightColW + (4 * padding));
        int centerX = padding + leftColW + padding;
        int btnH = (int)(h * 0.15f);

        SDL_FRect gridRect = { (float)centerX, (float)(colEndY - btnH), (float)centerColW, (float)btnH };

        if (mouseX >= gridRect.x && mouseX <= gridRect.x + gridRect.w &&
            mouseY >= gridRect.y && mouseY <= gridRect.y + gridRect.h)
        {
            int cols = 5, rows = 3;
            float gap = 8.0f, verticalPadding = 15.0f, horizontalPadding = 40.0f;
            float availableW = gridRect.w - (horizontalPadding * 2) - (gap * (cols - 1));
            float availableH = gridRect.h - (verticalPadding * 2) - (gap * (rows - 1));
            float btnWidth = availableW / cols, btnHeight = availableH / rows;

            float localX = mouseX - gridRect.x - horizontalPadding;
            float localY = mouseY - gridRect.y - verticalPadding;

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
                    }
                }
            }
        }
    }
    // --- INVENTORY MODE ---
    else if (currentState == GameState::INVENTORY)
    {
        int mapSize = (int)(h * 0.30f);
        int leftColW = mapSize;
        int rightColW = mapSize;
        int centerColW = w - (leftColW + rightColW + (4 * padding));
        int centerX = padding + leftColW + padding;
        int colStartY = padding + (int)(h * 0.08f) + padding;

        SDL_Rect equipRect = { padding, colEndY - mapSize, mapSize, mapSize };
        SDL_Rect invRect = { centerX, colStartY, centerColW, (colEndY - (int)(h * 0.15f) - padding) - colStartY };

        // 1. CLICKING 6x6 EQUIPMENT PANEL SLOTS
        if (mouseX >= equipRect.x && mouseX <= equipRect.x + equipRect.w &&
            mouseY >= equipRect.y && mouseY <= equipRect.y + equipRect.h)
        {
            int cols = 6, rows = 6, slotGap = 4;
            int internalPadding = padding + 6;

            int availableW = equipRect.w - (2 * internalPadding) - ((cols - 1) * slotGap);
            int availableH = equipRect.h - (2 * internalPadding) - ((rows - 1) * slotGap);
            int slotSize = std::min(availableW / cols, availableH / rows);

            int gridW = (slotSize * cols) + (slotGap * (cols - 1));
            int gridH = (slotSize * rows) + (slotGap * (rows - 1));

            int offsetX = equipRect.x + (equipRect.w - gridW) / 2;
            int offsetY = equipRect.y + (equipRect.h - gridH) / 2;

            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    int slotIdx = (r * cols) + c;

                    SDL_FRect slot = {
                        (float)(offsetX + (c * (slotSize + slotGap))),
                        (float)(offsetY + (r * (slotSize + slotGap))),
                        (float)slotSize,
                        (float)slotSize
                    };

                    if (mouseX >= slot.x && mouseX <= slot.x + slot.w &&
                        mouseY >= slot.y && mouseY <= slot.y + slot.h)
                    {
                        selectedEquipmentSlot = equipSlot::NONE;
                        selectedInventoryIndex = -1; // Deselect backpack item

                        if (Player)
                        {
                            for (const auto& [eSlot, eqItem] : Player->inventory.equipped)
                            {
                                if (getEquipmentGridIndex(eSlot) == slotIdx && !eqItem->id.empty())
                                {
                                    selectedEquipmentSlot = eSlot;
                                    std::cout << "Selected Equipped Item: " << eqItem->name << "\n";
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

        // 2. CLICKING BACKPACK INVENTORY SLOTS
        if (mouseX >= invRect.x && mouseX <= invRect.x + invRect.w &&
            mouseY >= invRect.y && mouseY <= invRect.y + invRect.h)
        {
            int cols = 6, rows = 5, slotGap = 4;
            int halfWidth = invRect.w / 2;
            int sidePadding = 20, topPadding = 60;

            int availableW = halfWidth - (2 * sidePadding) - ((cols - 1) * slotGap);
            int availableH = invRect.h - topPadding - sidePadding - ((rows - 1) * slotGap);
            int slotSize = std::min(availableW / cols, availableH / rows);

            int gridW = (slotSize * cols) + (slotGap * (cols - 1));
            int leftOffsetX = invRect.x + (halfWidth - gridW) / 2;
            int rightOffsetX = invRect.x + halfWidth + (halfWidth - gridW) / 2;
            int gridOffsetY = invRect.y + topPadding;
            int maxSlotsPerSide = cols * rows;

            for (int i = 0; i < maxSlotsPerSide * 2; i++)
            {
                int side = i / maxSlotsPerSide;
                int localIndex = i % maxSlotsPerSide;
                int r = localIndex / cols;
                int c = localIndex % cols;
                float originX = (side == 0) ? (float)leftOffsetX : (float)rightOffsetX;

                SDL_FRect slot = {
                    originX + (c * (slotSize + slotGap)),
                    (float)(gridOffsetY + (r * (slotSize + slotGap))),
                    (float)slotSize,
                    (float)slotSize
                };

                if (mouseX >= slot.x && mouseX <= slot.x + slot.w &&
                    mouseY >= slot.y && mouseY <= slot.y + slot.h)
                {
                    selectedEquipmentSlot = equipSlot::NONE; // Deselect equipment item
                    if (Player && i < Player->inventory.backpack.size())
                    {
                        selectedInventoryIndex = i;
                        std::cout << "Selected Backpack Item: " << Player->inventory.backpack[i]->name << "\n";
                    }
                    else
                    {
                        selectedInventoryIndex = -1; // Clicked empty slot
                    }
                    refreshActionGrid();
                    return;
                }
            }
        }

        // 3. CLICKING ACTION GRID BUTTONS IN INVENTORY MODE
        int btnH = (int)(h * 0.15f);
        SDL_FRect gridRect = { (float)centerX, (float)(colEndY - btnH), (float)centerColW, (float)btnH };

        if (mouseX >= gridRect.x && mouseX <= gridRect.x + gridRect.w &&
            mouseY >= gridRect.y && mouseY <= gridRect.y + gridRect.h)
        {
            int cols = 5, rows = 3;
            float gap = 8.0f, verticalPadding = 15.0f, horizontalPadding = 40.0f;
            float availableW = gridRect.w - (horizontalPadding * 2) - (gap * (cols - 1));
            float availableH = gridRect.h - (verticalPadding * 2) - (gap * (rows - 1));
            float btnWidth = availableW / cols, btnHeight = availableH / rows;

            float localX = mouseX - gridRect.x - horizontalPadding;
            float localY = mouseY - gridRect.y - verticalPadding;

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
        // Row 0: Head / Face
        case equipSlot::HEADWEAR:        return 1;
        case equipSlot::EYEWEAR:         return 2;
        case equipSlot::HORNS_SLOT:      return 3;
        case equipSlot::PIERCING_EAR:    return 4;

            // Row 1: Neck & Upper Body
        case equipSlot::NECKWEAR:        return 7;
        case equipSlot::SHOULDERS:       return 8;
        case equipSlot::CHEST_WEAR:      return 9;
        case equipSlot::TORSO_OVER:      return 10;

            // Row 2: Arms & Weapons
        case equipSlot::WEAPON_MAIN:     return 12;
        case equipSlot::HANDS:           return 13;
        case equipSlot::TORSO_UNDER:     return 14;
        case equipSlot::WEAPON_OFF:      return 15;

            // Row 3: Midsection & Waist
        case equipSlot::STOMACH_WEAR:    return 19;
        case equipSlot::HIPS_WEAR:       return 20;
        case equipSlot::TAIL_SLOT:       return 21;

            // Row 4: Lower Body & Legs
        case equipSlot::GROIN_OVER:      return 25;
        case equipSlot::LEGS_INNER:      return 26;
        case equipSlot::LEGS_OUTER:      return 27;

            // Row 5: Feet & Accessories
        case equipSlot::FEET:            return 32;
        case equipSlot::WINGS_SLOT:      return 33;

        default:                         return -1;
    }
}

void game::handleEquipAction(int backpackIndex)
{
    if (!Player || backpackIndex < 0 || (size_t)backpackIndex >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];

    if (!targetItem->isEquippable)
    {
        std::cout << targetItem->name << " is not equippable.\n";
        return;
    }

    // Pass targetSlot and tags directly to your inventory component
    std::vector<std::string> bodyTags;
    bool success = Player->inventory.equipItem((size_t)backpackIndex, targetItem->targetSlot, bodyTags);

    if (success)
    {
        selectedInventoryIndex = -1; // Reset selection
        refreshActionGrid();         // Clear the equip button
    }
}

void game::handleUnequipAction(equipSlot slot)
{
    if (!Player || slot == equipSlot::NONE) return;

    bool success = Player->inventory.unequipItem(slot);

    if (success)
    {
        std::cout << "Successfully unequipped item from slot " << (int)slot << "\n";
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
        if (gridX == 2 && gridY == 2)
        {
            actionButton triggerQuest;
            triggerQuest.label = "Talk to Stranger";
            triggerQuest.command = "START_SCENE";
            triggerQuest.payload = "quest_intro_01";
            activeButtons.push_back(triggerQuest);
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
                if (item->id == cond.target) found = true;
            }
            if (!found && cond.requiredValue > 0) return false;
        }
        else if (cond.type == "QUEST_STAGE")
        {
            if (Player->quests.getQuestStage(cond.target) != cond.requiredValue) return false;
        }
    }
    return true;
}

void game::processChoice(const dialogueChoice& choice)
{
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
            Player->stats.modifyStat(effect.target, (float)effect.amount);
        }
        else if (effect.action == "TELEPORT")
        {
            int newX, newY;
            sscanf(effect.target.c_str(), "x_%d_y_%d", &newX, &newY);
            gridX = newX;
            gridY = newY;
            map->updateDiscovery(gridX, gridY);
        }
        else if (effect.action == "SET_QUEST")
        {
            Player->quests.setQuestStage(effect.target, effect.amount);
        }
    }

    if (choice.nextSceneId == "EXIT" || choice.nextSceneId.empty())
    {
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

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

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

    // --- TITLE BAR SLOTS ---
    int titleW = (w - (4 * padding)) / 3;
    SDL_Rect slotTitle1 = { padding, topRowY, titleW, topBarH };
    SDL_Rect slotTitle2 = { padding + titleW + padding, topRowY, titleW, topBarH };
    SDL_Rect slotTitle3 = { padding + (titleW + padding) * 2, topRowY, titleW, topBarH };

    // --- LEFT COLUMN SIZING ---
    // 1. Map / Equipment Panel at the bottom (Fixed Square)
    SDL_Rect slotBottomLeft = { leftX, colEndY - mapSize, mapSize, mapSize };

    // 2. Character Panel height is tied directly to mapSize scale (0.85 ratio)
    // This maintains its proportions identically to the map when window scales!
    int charH = (int)(mapSize * 0.85f);
    SDL_FRect fSlotTopLeft = { (float)leftX, (float)colStartY, (float)leftColW, (float)charH };

    // 3. Middle Slot (Time/Calendar) occupies remaining vertical space
    int midY = colStartY + charH + padding;
    int midH = (colEndY - mapSize - padding) - midY;
    SDL_Rect slotMidLeft = { leftX, midY, leftColW, midH };

    // --- CENTER COLUMN SIZING ---
    int btnH = (int)(h * 0.15f);
    SDL_Rect slotCenterMain = { centerX, colStartY, centerColW, (colEndY - btnH - padding) - colStartY };
    SDL_FRect fSlotCenterBottom = { (float)centerX, (float)(colEndY - btnH), (float)centerColW, (float)btnH };

    // --- RIGHT COLUMN SIZING ---
    int rightAvailableH = colEndY - colStartY;
    int rightStackH = (rightAvailableH - (2 * padding)) / 3;
    SDL_Rect slotTopRight = { rightX, colStartY, rightColW, rightStackH };
    SDL_Rect slotMidRight = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    SDL_Rect slotBotRight = { rightX, colStartY + (rightStackH + padding) * 2, rightColW, rightAvailableH - (rightStackH * 2 + padding * 2) };

    // --- RENDER PANELS ---
    renderTitleBar(slotTitle1, slotTitle2, slotTitle3);
    renderCharacterPanel(fSlotTopLeft, Player);
    renderCompanionPanel(slotMidLeft); // Renders the Time/Calendar panel in the gap
    renderActionGrid(fSlotCenterBottom);

    switch (currentState)
    {
        case GameState::EVENT:
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
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    SDL_Rect menuRect = { w / 4, h / 4, w / 2, h / 2 };
    SDL_SetRenderViewport(renderer, &menuRect);
    SDL_SetRenderDrawColor(renderer, 20, 60, 80, 255);
    SDL_RenderFillRect(renderer, NULL);
}

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
    // 1. Set Viewport to the Equipment Panel Bounds
    SDL_SetRenderViewport(renderer, &rect);

    // 2. Draw Panel Background (Relative to viewport origin 0,0)
    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    SDL_SetRenderDrawColor(renderer, 25, 20, 30, 255);
    SDL_RenderFillRect(renderer, &panelRect);

    SDL_SetRenderDrawColor(renderer, 100, 50, 150, 255);
    SDL_RenderRect(renderer, &panelRect);

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

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int slotIdx = (r * cols) + c;

            SDL_FRect slot = {
                (float)(offsetX + (c * (slotSize + slotGap))),
                (float)(offsetY + (r * (slotSize + slotGap))),
                (float)slotSize,
                (float)slotSize
            };

            // Check if gear is equipped in this slot
            bool isOccupied = false;
            std::string equippedName = "";
            bool isSelectedEquip = false;

            if (Player)
            {
                for (const auto& [eSlot, eqItem] : Player->inventory.equipped)
                {
                    if (getEquipmentGridIndex(eSlot) == slotIdx && !eqItem->id.empty())
                    {
                        isOccupied = true;
                        equippedName = eqItem->name;
                        if (eSlot == selectedEquipmentSlot)
                        {
                            isSelectedEquip = true;
                        }
                        break;
                    }
                }
            }

            // Draw Slot Background
            if (isOccupied)
            {
                SDL_SetRenderDrawColor(renderer, 100, 60, 160, 255); // Purple tint for occupied equipment slots
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255);   // Dark empty slot
            }
            SDL_RenderFillRect(renderer, &slot);

            // Draw Slot Border (Gold outline if selected)
            if (isSelectedEquip)
            {
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Gold
                SDL_RenderRect(renderer, &slot);
                SDL_FRect inset = { slot.x + 1.0f, slot.y + 1.0f, slot.w - 2.0f, slot.h - 2.0f };
                SDL_RenderRect(renderer, &inset);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 80, 75, 95, 255); // Normal border
                SDL_RenderRect(renderer, &slot);
            }

            // Render Text Label with Auto-Scaling inside Viewport
            if (isOccupied && !equippedName.empty() && fonts.find("button_font") != fonts.end())
            {
                SDL_Color goldColor = { 255, 215, 0, 255 };
                SDL_Surface* surface = TTF_RenderText_Blended(fonts["button_font"], equippedName.c_str(), 0, goldColor);

                if (surface)
                {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (texture)
                    {
                        float maxTextW = slot.w - 4.0f;
                        float maxTextH = slot.h - 4.0f;

                        float scaleW = (surface->w > maxTextW) ? (maxTextW / surface->w) : 1.0f;
                        float scaleH = (surface->h > maxTextH) ? (maxTextH / surface->h) : 1.0f;
                        float finalScale = std::min(scaleW, scaleH);

                        float drawW = surface->w * finalScale;
                        float drawH = surface->h * finalScale;

                        float drawX = slot.x + (slot.w - drawW) / 2.0f;
                        float drawY = slot.y + (slot.h - drawH) / 2.0f;

                        SDL_FRect destRect = { drawX, drawY, drawW, drawH };
                        SDL_RenderTexture(renderer, texture, NULL, &destRect);

                        SDL_DestroyTexture(texture);
                    }
                    SDL_DestroySurface(surface);
                }
            }
        }
    }
}

void game::renderInventoryPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, NULL);

    // Panel Background
    SDL_FRect panelRect = { (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
    SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
    SDL_RenderFillRect(renderer, &panelRect);

    // Center Divider Line
    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderLine(renderer, rect.x + rect.w / 2.0f, rect.y + 20.0f, rect.x + rect.w / 2.0f, rect.y + rect.h - 20.0f);

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

    int leftOffsetX = rect.x + (halfWidth - gridW) / 2;
    int gridOffsetY = rect.y + topPadding;
    int rightOffsetX = rect.x + halfWidth + (halfWidth - gridW) / 2;

    int maxSlotsPerSide = cols * rows; // 30 slots per side (60 total)

    const auto& backpack = Player->inventory.backpack;

    // Loop through all 60 potential slots (0 to 29 on left, 30 to 59 on right)
    // Loop through all 60 potential slots (0 to 29 on left, 30 to 59 on right)
    for (int i = 0; i < maxSlotsPerSide * 2; i++)
    {
        int side = i / maxSlotsPerSide; // 0 = Left side, 1 = Right side
        int localIndex = i % maxSlotsPerSide;
        int r = localIndex / cols;
        int c = localIndex % cols;

        float originX = (side == 0) ? (float)leftOffsetX : (float)rightOffsetX;

        SDL_FRect slot = {
            originX + (c * (slotSize + slotGap)),
            (float)(gridOffsetY + (r * (slotSize + slotGap))),
            (float)slotSize,
            (float)slotSize
        };

        // Check if an item exists at this index safely
        bool hasItem = (Player != nullptr) && (i < Player->inventory.backpack.size());

        // 1. Fill Slot Background
        if (hasItem)
        {
            SDL_SetRenderDrawColor(renderer, 60, 70, 90, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
        }
        SDL_RenderFillRect(renderer, &slot);

        // 2. Draw Slot Border (Gold outline + inset border if selected!)
        if (i == selectedInventoryIndex)
        {
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Gold
            SDL_RenderRect(renderer, &slot);

            // Draw a 1px inset rectangle to make the gold selection outline thicker & visible
            SDL_FRect insetSlot = { slot.x + 1.0f, slot.y + 1.0f, slot.w - 2.0f, slot.h - 2.0f };
            SDL_RenderRect(renderer, &insetSlot);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 70, 70, 85, 255); // Normal border
            SDL_RenderRect(renderer, &slot);
        }

        // Render Item Label if present
        if (hasItem)
        {
            std::string displayName = Player->inventory.backpack[i]->name;
            if (displayName.length() > 8)
            {
                displayName = displayName.substr(0, 7) + ".";
            }
            renderTextCentered(displayName, slot, "button_font");
        }
    }
    if (!Player) return;
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

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    SDL_SetRenderDrawColor(renderer, 30, 28, 35, 255);
    SDL_RenderFillRect(renderer, &panelRect);

    SDL_SetRenderDrawColor(renderer, 60, 55, 65, 255);
    SDL_RenderRect(renderer, &panelRect);

    // Reset Viewport
    SDL_SetRenderViewport(renderer, NULL);
}

void game::renderTextPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, NULL);
    SDL_FRect fRect = { (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };

    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &fRect);

    if (currentState == GameState::EVENT)
    {
        SDL_FRect nameRect = { fRect.x, fRect.y, fRect.w, 40.0f };
        renderTextCentered(currentScene.speakerName, nameRect, "title_font", { 255, 200, 100, 255 });

        SDL_FRect bodyRect = { fRect.x, fRect.y + 40.0f, fRect.w, fRect.h - 40.0f };
        renderTextWrapped(currentScene.bodyText, bodyRect, "button_font", { 220, 220, 220, 255 });
    }
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

void game::renderCharacterPanel(SDL_FRect rect, entity* playerObj)
{
    // 1. Set Viewport to Character Panel Rect
    SDL_Rect viewRect = { (int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h };
    SDL_SetRenderViewport(renderer, &viewRect);

    // 2. Panel Background & Border (Origin 0,0 relative to viewport)
    SDL_FRect panelRect = { 0.0f, 0.0f, (float)viewRect.w, (float)viewRect.h };
    SDL_SetRenderDrawColor(renderer, 30, 28, 35, 255);
    SDL_RenderFillRect(renderer, &panelRect);

    SDL_SetRenderDrawColor(renderer, 60, 55, 65, 255);
    SDL_RenderRect(renderer, &panelRect);

    if (playerObj)
    {
        float padX = viewRect.w * 0.04f;
        float padY = viewRect.h * 0.04f;
        float contentW = viewRect.w - (padX * 2.0f);
        float currentY = padY;
        float dividerGap = viewRect.h * 0.025f;

        auto drawHorizontalDivider = [&](float y)
            {
                SDL_SetRenderDrawColor(renderer, 50, 46, 55, 255);
                SDL_RenderLine(renderer, padX, y, viewRect.w - padX, y);
            };

        auto drawText = [&](const std::string& textStr, SDL_FRect destRect, SDL_Color color)
            {
                float srcW = 0.0f, srcH = 0.0f;
                SDL_Texture* texture = getOrRenderText(textStr, "button_font", color, srcW, srcH);

                if (texture && srcH > 0.0f)
                {
                    float scale = (srcH > destRect.h) ? (destRect.h / srcH) : 1.0f;
                    float drawW = srcW * scale;
                    float drawH = srcH * scale;

                    SDL_FRect renderDst = { destRect.x, destRect.y + (destRect.h - drawH) / 2.0f, drawW, drawH };
                    SDL_RenderTexture(renderer, texture, NULL, &renderDst);
                }
            };

        // --- 1. AVATAR & HEADER ---
        float avatarSize = viewRect.h * 0.16f;
        SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
        SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
        SDL_RenderFillRect(renderer, &avatarRect);
        SDL_SetRenderDrawColor(renderer, 90, 90, 105, 255);
        SDL_RenderRect(renderer, &avatarRect);

        float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
        std::string headerStr = playerObj->name + " - Level " + std::to_string((int)playerObj->stats.getStat("level"));
        float headerTextH = avatarSize * 0.55f;
        SDL_FRect headerTextRect = { headerTextX, currentY, viewRect.w - headerTextX - padX, headerTextH };
        drawText(headerStr, headerTextRect, { 160, 200, 255, 255 });

        // XP Bar
        float xpBarY = currentY + headerTextH + (padY * 0.3f);
        float xpBarW = viewRect.w - headerTextX - padX;
        float xpBarH = avatarSize * 0.18f;

        SDL_FRect xpBg = { headerTextX, xpBarY, xpBarW, xpBarH };
        SDL_SetRenderDrawColor(renderer, 20, 18, 25, 255);
        SDL_RenderFillRect(renderer, &xpBg);

        float currentXp = playerObj->stats.getStat("xp");
        float xpFillPct = std::clamp(currentXp / 100.0f, 0.0f, 1.0f);
        SDL_FRect xpFill = { headerTextX, xpBarY, xpBarW * xpFillPct, xpBarH };
        SDL_SetRenderDrawColor(renderer, 80, 200, 230, 255);
        SDL_RenderFillRect(renderer, &xpFill);

        currentY += avatarSize + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        // --- 2. CURRENCY ---
        float halfWidth = contentW / 2.0f;
        float currencyH = viewRect.h * 0.08f;
        std::string currencyStr = "¤ " + std::to_string((int)playerObj->stats.getStat("currency"));
        drawText(currencyStr, { padX, currentY, halfWidth, currencyH }, { 255, 215, 0, 255 });

        std::string essenceStr = "★ " + std::to_string((int)playerObj->stats.getStat("gems"));
        drawText(essenceStr, { padX + halfWidth, currentY, halfWidth, currencyH }, { 255, 100, 220, 255 });

        currentY += currencyH + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        // --- 3. MINI ATTRIBUTES ---
        float colWidth = contentW / 3.0f;
        float miniStatH = viewRect.h * 0.09f;

        auto drawMiniStat = [&](int colIndex, const std::string& statName, SDL_Color textColor)
            {
                float colX = padX + (colIndex * colWidth);
                SDL_FRect iconBox = { colX, currentY, miniStatH, miniStatH };
                SDL_SetRenderDrawColor(renderer, 45, 42, 50, 255);
                SDL_RenderFillRect(renderer, &iconBox);

                int val = (int)playerObj->stats.getStat(statName);
                SDL_FRect valRect = { colX + miniStatH + 4.0f, currentY, colWidth - miniStatH - 4.0f, miniStatH };
                drawText(std::to_string(val), valRect, textColor);
            };

        drawMiniStat(0, "physique", { 255, 50, 120, 255 });
        drawMiniStat(1, "arcane", { 180, 110, 255, 255 });
        drawMiniStat(2, "corruption", { 100, 200, 255, 255 });

        currentY += miniStatH + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        // --- 4. MAIN VITAL BARS ---
        float barHeight = viewRect.h * 0.075f;
        float iconRadius = barHeight * 1.25f;
        float valueTextWidth = contentW * 0.18f;
        float barW = contentW - iconRadius - valueTextWidth - (padX * 0.5f);
        float barGap = viewRect.h * 0.02f;

        auto drawVitalBar = [&](float y, const std::string& statName, float maxVal, SDL_Color barColor)
            {
                SDL_FRect iconRect = { padX, y, iconRadius, iconRadius };
                SDL_SetRenderDrawColor(renderer, 45, 40, 50, 255);
                SDL_RenderFillRect(renderer, &iconRect);

                float fillX = padX + iconRadius + (padX * 0.5f);
                SDL_FRect bgRect = { fillX, y + (iconRadius - barHeight) / 2.0f, barW, barHeight };
                SDL_SetRenderDrawColor(renderer, 20, 18, 25, 255);
                SDL_RenderFillRect(renderer, &bgRect);

                float currentVal = playerObj->stats.getStat(statName);
                float fillPct = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
                SDL_FRect fillRect = { fillX, y + (iconRadius - barHeight) / 2.0f, barW * fillPct, barHeight };
                SDL_SetRenderDrawColor(renderer, barColor.r, barColor.g, barColor.b, 255);
                SDL_RenderFillRect(renderer, &fillRect);

                SDL_FRect textRect = { fillX + barW + (padX * 0.4f), y, valueTextWidth, iconRadius };
                drawText(std::to_string((int)currentVal), textRect, { 240, 240, 240, 255 });
            };

        drawVitalBar(currentY, "health", 100.0f, { 255, 60, 90, 255 });
        currentY += iconRadius + barGap;

        drawVitalBar(currentY, "arcane", 100.0f, { 220, 130, 255, 255 });
        currentY += iconRadius + barGap;

        drawVitalBar(currentY, "lust", 100.0f, { 230, 50, 150, 255 });
    }

    // ALWAYS Reset Viewport back to full window at the end!
    SDL_SetRenderViewport(renderer, NULL);
}

void game::renderActionGrid(SDL_FRect rect)
{
    SDL_SetRenderViewport(renderer, NULL);

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
            int index = (r * cols) + c;

            SDL_FRect btn = {
                rect.x + horizontalPadding + (c * (btnWidth + gap)),
                rect.y + verticalPadding + (r * (btnHeight + gap)),
                btnWidth,
                btnHeight
            };

            if (index < activeButtons.size())
            {
                SDL_SetRenderDrawColor(renderer, 70, 100, 140, 255);
                SDL_RenderFillRect(renderer, &btn);
                renderTextCentered(activeButtons[index].label, btn, "button_font");
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
                SDL_RenderFillRect(renderer, &btn);
            }

            SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
            SDL_RenderRect(renderer, &btn);
        }
    }
}

SDL_Texture* game::getOrRenderText(const std::string& text, const std::string& fontId, SDL_Color color, float& outW, float& outH)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return nullptr;

    // Create a unique key combining text, font, and color
    std::string key = fontId + "|" + text + "|" +
        std::to_string(color.r) + "," +
        std::to_string(color.g) + "," +
        std::to_string(color.b) + "," +
        std::to_string(color.a);

    auto it = textCache.find(key);
    if (it != textCache.end())
    {
        outW = it->second.w;
        outH = it->second.h;
        return it->second.texture;
    }

    // Cache Miss: Render new text surface & upload texture
    TTF_Font* font = fonts[fontId];
    TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_LEFT);
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);

    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        CachedTextTexture cached;
        cached.texture = texture;
        cached.w = (float)surface->w;
        cached.h = (float)surface->h;

        textCache[key] = cached;

        outW = cached.w;
        outH = cached.h;
    }

    SDL_DestroySurface(surface);
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

void game::clean()
{
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