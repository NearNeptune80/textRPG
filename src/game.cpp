#include "game.h"
#include "theme.h"
#include "inputHandler.h"
#include "actionGridManager.h"
#include "uiRenderer.h"
#include "uiWidget.h"
#include "saveManager.h"
#include "itemDatabase.h"
#include "questDatabase.h"
#include <iostream>
#include "state/iGameState.h"
#include "state/explorationState.h"
#include "state/eventState.h"
#include "state/inventoryState.h"
#include "textParser.h"
#include "npcGenerator.h"
#include "encounterResolver.h"
#include "saveManager.h"

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), Player(nullptr), gridX(1), gridY(1), currentState(GameState::EXPLORATION) {}

game::~game()
{
    map = nullptr;
    if (Player) delete Player;
}

void game::changeState(std::unique_ptr<iGameState> newState)
{
    if (activeGameState) activeGameState->onExit(this);
    activeGameState = std::move(newState);
    if (activeGameState)
    {
        activeGameState->initialise(this);
        activeGameState->onEnter(this);
    }
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

    Theme::loadFromFile("data/theme.json");

    loadFont("button_font", "data/fonts/Roboto/static/Roboto-Regular.ttf", 18);
    loadFont("title_font", "data/fonts/Roboto/static/Roboto-Bold.ttf", 24);

    if (itemDatabase::loadDatabase("data/items.json"))
    {
        npcGenerator::loadTemplates("data/npc_templates.json");
        questDatabase::loadDatabase("data/quests");
    }

    // 1. MUST instantiate Player if null before trying to load/save JSON
    if (!Player)
    {
        Player = new entity("player_main", "Hero");
    }

    // 2. Load map
    loadMap("overworld", 1, 1);

    // 3. Safe Load or Initial Save creation
    if (!saveManager::loadFromFile(this, "Hero_Initial.json"))
    {
        std::cout << "[Init] Initial save not found. Generating default 'Hero_Initial.json'...\n";
        saveManager::saveNamedGame(this, "Initial");
    }

    changeState(std::make_unique<explorationState>());

    eventBus::getInstance().subscribe(gameEvent::timeAdvanced, [this](const eventData& data) {
        if (this->Player)
        {
            this->Player->anatomy.processMutations(data.numericValue);
        }
    });

    isRunning = true;
}

void game::handleEvents()
{
    InputHandler::handleEvents(this);
}

void game::update()
{
    if (activeGameState) activeGameState->update(this, 0.016f);
}

void game::render()
{
    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, 255);
    SDL_RenderClear(renderer);

    if (currentState == GameState::MAIN_MENU) renderMainMenuLayout();
    else renderDashboardLayout();

    SDL_SetRenderViewport(renderer, NULL);
    SDL_RenderPresent(renderer);
}

void game::refreshActionGrid()
{
    ActionGridManager::refresh(this);
}

InventorySlotInfo game::getInventorySlotItem(int side, int absoluteIndex)
{
    InventorySlotInfo info;
    TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);

    if (side == 0 && Player)
    {
        auto stackedView = Player->inventory.getStackedView();
        if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < stackedView.size())
        {
            info.itemPtr = stackedView[absoluteIndex].itemPtr;
            info.count = stackedView[absoluteIndex].totalCount;
            info.isValid = true;
        }
    }
    else if (side == 1)
    {
        if (activeTargetNPC)
        {
            auto npcView = activeTargetNPC->inventory.getStackedView();
            if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < npcView.size())
            {
                info.itemPtr = npcView[absoluteIndex].itemPtr;
                info.count = npcView[absoluteIndex].totalCount;
                info.isValid = true;
            }
        }
        else if (absoluteIndex >= 0 && static_cast<size_t>(absoluteIndex) < static_cast<int>(tileData.droppedItems.size()))
        {
            info.itemPtr = tileData.droppedItems[absoluteIndex];
            info.count = info.itemPtr->isStackable ? info.itemPtr->count : 1;
            info.isValid = true;
        }
    }

    return info;
}

std::pair<equipSlot, std::shared_ptr<item>> game::getEquippedAtGridIndex(entity* target, int gridIdx)
{
    if (!target) return { equipSlot::NONE, nullptr };

    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        equipSlot eSlot = static_cast<equipSlot>(i);
        const auto& eqItem = target->inventory.equipped[i];
        if (getEquipmentGridIndex(eSlot) == gridIdx && eqItem && !eqItem->id.empty())
        {
            return { eSlot, eqItem };
        }
    }
    return { equipSlot::NONE, nullptr };
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

    // Publish map entry event
    eventBus::getInstance().publishEvent({ gameEvent::mapEntered, 0, mapId, nullptr });

    return true;
}

void game::movePlayer(int nextX, int nextY)
{
    if (!map || !map->isWalkable(nextX, nextY)) return;

    map->clearUnsafeItems(gridX, gridY);

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
        if ((rand() % 100) < chance)
        {
            // If an NPC hasn't been generated for this tile yet, generate and store it!
            if (!tileData.persistentNPC) {
                tileData.persistentNPC = generateEncounterNPC();
            }
            triggerEncounter(tileData.persistentNPC);
            return;
        }
    }

    activeTargetNPC = nullptr;
    activeTargetMode = TargetMode::NONE;
    refreshActionGrid();
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

void game::handleEquipAction(int backpackIndex)
{
    if (!Player || backpackIndex < 0 || static_cast<size_t>(backpackIndex) >= Player->inventory.backpack.size()) return;

    std::shared_ptr<item> targetItem = Player->inventory.backpack[backpackIndex];
    if (!targetItem || !targetItem->isEquippable || targetItem->targetSlot == equipSlot::NONE) return;

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
        selectedInventoryIndex = static_cast<int>(Player->inventory.backpack.size()) - 1;
        descriptionScrollY = 0.0f;
        refreshActionGrid();
    }
}

bool game::checkSingleCondition(const gameCondition& cond) const
{
    if (cond.type == "HAS_ITEM")
    {
        int count = 0;
        for (const auto& item : Player->inventory.backpack)
        {
            if (item && item->id == cond.target)
            {
                count += item->isStackable ? item->count : 1;
            }
        }
        return count >= cond.requiredValue;
    }
    else if (cond.type == "QUEST_STAGE")
    {
        return Player->quests.getQuestStage(cond.target) == cond.requiredValue;
    }
    else if (cond.type == "TIME_PHASE")
    {
        TimePhase currentPhase = gameTime.getPhase();
        std::string phaseStr = "DAY";
        if (currentPhase == TimePhase::NIGHT) phaseStr = "NIGHT";
        else if (currentPhase == TimePhase::DAWN) phaseStr = "DAWN";
        else if (currentPhase == TimePhase::DUSK) phaseStr = "DUSK";
        return phaseStr == cond.target;
    }
    else if (cond.type == "STAT_MIN")
    {
        return Player->getStat(cond.target) >= cond.requiredValue;
    }
    else if (cond.type == "HAS_TAG")
    {
        return Player->anatomy.hasGlobalTag(cond.target);
    }
    else if (cond.type == "DOMINANT_RACE")
    {
        return Player->anatomy.getDominantRace() == cond.target;
    }
    return true;
}

bool game::checkConditions(const std::vector<conditionNode>& conditions)
{
    for (const auto& node : conditions)
    {
        if (!node.evaluate(this)) return false;
    }
    return true;
}

void game::processChoice(const dialogueChoice& choice)
{
    // 1. Handle combat victory resolution
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        std::string currentMapId = map ? map->getId() : "default";

        // Check if activeTargetNPC is null or points to tile persistentNPC directly
        entity* rawTarget = activeTargetNPC;
        if (!rawTarget && map)
        {
            TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
            if (tileData.persistentNPC)
            {
                rawTarget = tileData.persistentNPC.get();
            }
        }

        // Safely pass the target (or nullptr if none exists)
        currentScene = encounterResolver::buildVictoryScene(this, rawTarget, currentMapId);

        // Clear the persistent NPC from the tile AFTER building the scene
        if (map)
        {
            TileRuntimeData& tileData = map->getRuntimeData(gridX, gridY);
            tileData.persistentNPC = nullptr;
        }

        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;

        saveManager::saveAutosave(this);

        // Transition state and refresh UI
        changeState(std::make_unique<eventState>());
        refreshActionGrid();
        return;
    }

    if (choice.nextSceneId == "VICTORY_INVENTORY")
    {
        changeState(std::make_unique<inventoryState>());
        return;
    }

    // 2. Process choice result effects
    for (const auto& effect : choice.results)
    {
        processEffect(effect);
    }

    // 3. Resolve next scene or return to map exploration
    if (choice.nextSceneId == "EXIT" || choice.nextSceneId.empty())
    {
        activeTargetNPC = nullptr;
        activeTargetMode = TargetMode::NONE;
        changeState(std::make_unique<explorationState>());
    }
    else
    {
        loadScene(choice.nextSceneId);
    }
}

void game::processEffect(const gameEffect& eff)
{
    if (eff.action == "ADD_ITEM")
    {
        // Add item logic / lookup from item database
    }
    else if (eff.action == "SET_QUEST_STAGE")
    {
        Player->quests.setQuestStage(eff.target, eff.amount);
    }
    else if (eff.action == "MODIFY_STAT")
    {
        Player->stats.modifyBaseStat(eff.target, eff.amount);
    }
    else if (eff.action == "TRANSFORM")
    {
        // Example: eff.target could specify slot name, amount = growth magnitude
        // Queues a 10-minute dynamic mutation on the player's anatomy
        Player->anatomy.applyTransformation(
            bodySlot::GROIN,
            mutationType::GROWTH_LENGTH,
            static_cast<float>(eff.amount),
            eff.target,
            10,
            "effect_transform"
        );
    }
}

void game::loadScene(const std::string& sceneId)
{
    changeState(std::make_unique<eventState>());
    currentScene = questDatabase::getScene(sceneId);

    // Interpolate body text & speaker name dynamically
    currentScene.bodyText = textParser::interpolate(currentScene.bodyText, Player, activeTargetNPC);
    currentScene.speakerName = textParser::interpolate(currentScene.speakerName, Player, activeTargetNPC);

    activeButtons.clear();
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
        // Interpolate choice labels as well
        currentScene.choices[i].label = textParser::interpolate(currentScene.choices[i].label, Player, activeTargetNPC);

        if (checkConditions(currentScene.choices[i].requirements))
        {
            actionButton btn;
            btn.label = currentScene.choices[i].label;
            dialogueChoice choice = currentScene.choices[i];
            btn.onClick = [this, choice]() { processChoice(choice); };
            if (activeButtons.size() < activeButtons.capacity()) activeButtons.push_back(btn);
        }
    }
}

std::shared_ptr<entity> game::generateEncounterNPC()
{
    auto npc = npcGenerator::generateRandomNPC();
    if (npc) return npc;

    // Fallback if template loading fails
    return std::make_shared<entity>("npc_fallback", "Alleyway Stranger");
}

void game::triggerEncounter(std::shared_ptr<entity> npc)
{
    activeTargetNPC = npc.get();
    activeTargetMode = TargetMode::COMBAT_ENEMY;
    changeState(std::make_unique<eventState>());

    currentScene.id = "encounter_event";
    currentScene.speakerName = npc->name;
    currentScene.bodyText = "A " + npc->name + " steps out of the shadows and demands your attention! What will you do?";
    currentScene.choices.clear();

    dialogueChoice fightChoice; fightChoice.label = "Fight"; fightChoice.nextSceneId = "ENCOUNTER_FIGHT"; currentScene.choices.push_back(fightChoice);
    dialogueChoice payChoice; payChoice.label = "Bribe (25¤)"; payChoice.nextSceneId = "ENCOUNTER_PAY"; currentScene.choices.push_back(payChoice);
    dialogueChoice surrenderChoice; surrenderChoice.label = "Surrender"; surrenderChoice.nextSceneId = "ENCOUNTER_SURRENDER"; currentScene.choices.push_back(surrenderChoice);

    activeButtons.clear();
    for (const auto& choice : currentScene.choices)
    {
        actionButton btn;
        btn.label = choice.label;
        btn.onClick = [this, choice]() { processChoice(choice); };
        if (activeButtons.size() < activeButtons.capacity()) activeButtons.push_back(btn);
    }
}

std::array<actionButton, 15> game::getSlotsForCurrentActionPage()
{
    std::array<actionButton, 15> pageSlots;
    int itemsPerPage = 15;
    std::vector<actionButton> flowButtons;

    for (const auto& btn : activeButtons)
    {
        if (btn.pinnedAllPages && btn.slotIndex >= 0 && btn.slotIndex < 15)
        {
            pageSlots[btn.slotIndex] = btn;
        }
        else if (btn.slotIndex >= 0)
        {
            int btnPage = btn.slotIndex / itemsPerPage;
            int localSlot = btn.slotIndex % itemsPerPage;
            if (btnPage == actionGridPage && localSlot >= 0 && localSlot < 15)
            {
                pageSlots[localSlot] = btn;
            }
        }
        else
        {
            flowButtons.push_back(btn);
        }
    }

    int currentFlowIdx = 0;
    for (int page = 0; page < actionGridPage; page++)
    {
        for (int s = 0; s < 15; s++)
        {
            if (pageSlots[s].label.empty() && currentFlowIdx < static_cast<int>(flowButtons.size())) currentFlowIdx++;
        }
    }

    for (int s = 0; s < 15; s++)
    {
        if (pageSlots[s].label.empty() && currentFlowIdx < static_cast<int>(flowButtons.size()))
        {
            pageSlots[s] = flowButtons[currentFlowIdx++];
        }
    }
    return pageSlots;
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

    layout.titleBox1 = { leftX, padding, leftColW, topBarH };
    layout.titleBox2 = { centerX, padding, centerColW, topBarH };
    layout.titleBox3 = { rightX, padding, rightColW, topBarH };

    float charH = mapSize * 0.82f;
    layout.charRect = { leftX, colStartY, leftColW, charH };

    float timeH = mapSize * 0.18f;
    float timeY = colEndY - mapSize - padding - timeH;
    layout.timeRect = { leftX, timeY, leftColW, timeH };

    float midY = colStartY + charH + padding;
    float midH = timeY - padding - midY;
    layout.companionRect = { leftX, midY, leftColW, midH };

    layout.mapRect = { leftX, colEndY - mapSize, mapSize, mapSize };

    float btnH = h * 0.15f;
    float centerAvailableH = (colEndY - btnH - padding) - colStartY;

    layout.textMainRect = { centerX, colStartY, centerColW, centerAvailableH };

    float gridH = centerAvailableH * 0.52f;
    float detailH = centerAvailableH - gridH - padding;

    layout.inventoryGridRect = { centerX, colStartY, centerColW, gridH };
    layout.inventoryDetailRect = { centerX, colStartY + gridH + padding, centerColW, detailH };

    layout.actionGridRect = { centerX, colEndY - btnH, centerColW, btnH };

    float rightAvailableH = colEndY - colStartY;
    float rightStackH = (rightAvailableH - (2.0f * padding)) / 3.0f;

    layout.rightStackTop = { rightX, colStartY, rightColW, rightStackH };
    layout.rightStackMid = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    layout.rightStackBot = { rightX, colStartY + (rightStackH + padding) * 2.0f, rightColW, rightAvailableH - (rightStackH * 2.0f + padding * 2.0f) };

    layout.equipRect = layout.mapRect;

    float charPadX = layout.charRect.w * 0.04f;
    float charPadY = layout.charRect.h * 0.04f;
    float avatarSize = layout.charRect.h * 0.16f;

    layout.playerAvatarRect = { layout.charRect.x + charPadX, layout.charRect.y + charPadY, avatarSize, avatarSize };
    layout.targetAvatarRect = { rightX + charPadX, colStartY + charPadY, avatarSize, avatarSize };
}

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    updateLayoutBounds(w, h);

    renderTitleBar(layout.titleBox1, layout.titleBox2, layout.titleBox3);
    UI::DrawEntitySummaryCard(renderer, this, layout.charRect, Player, false);
    renderCompanionPanel(layout.companionRect);
    UI::DrawTimePanel(renderer, this, layout.timeRect, gameTime);
    UI::DrawActionGrid(renderer, this, layout.actionGridRect, std::vector<actionButton>(activeButtons.begin(), activeButtons.end()));

    if (activeGameState)
    {
        activeGameState->render(this);
    }
    renderRightColumn(layout.rightStackTop, layout.rightStackMid, layout.rightStackBot);

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

    SDL_FRect panelRect = { static_cast<float>(w) / 4.0f, static_cast<float>(h) / 4.0f, static_cast<float>(w) / 2.0f, static_cast<float>(h) / 2.0f };
    ViewportGuard vpGuard(renderer, panelRect);

    UI::DrawPanel(renderer, { 0.0f, 0.0f, panelRect.w, panelRect.h }, Theme::colors.bgPanel, Theme::colors.borderNormal);
}

void game::renderTitleBar(SDL_FRect t1, SDL_FRect t2, SDL_FRect t3)
{
    SDL_FRect boxes[3] = { t1, t2, t3 };
    for (int i = 0; i < 3; i++)
    {
        ViewportGuard vpGuard(renderer, boxes[i]);
        UI::DrawPanel(renderer, { 0.0f, 0.0f, boxes[i].w, boxes[i].h }, Theme::colors.bgHeader, Theme::colors.borderButtonDisabled);
    }

    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        ViewportGuard vpGuard3(renderer, t3);
        SDL_Color headerColor = Theme::colors.enemy;
        std::string headerTitle = "Enemy";

        if (activeTargetMode == TargetMode::DIALOGUE) { headerColor = Theme::colors.friendly; headerTitle = "Interacting With"; }
        else if (activeTargetMode == TargetMode::COMPANION) { headerColor = Theme::colors.companion; headerTitle = "Ally"; }

        SDL_FRect titleRect = { 0.0f, 0.0f, t3.w, t3.h };
        renderTextAligned(headerTitle, titleRect, TextAlignment::CENTER, true, "title_font", headerColor);
    }
}

void game::renderCompanionPanel(SDL_FRect rect)
{
    ViewportGuard vpGuard(renderer, rect);
    UI::DrawPanel(renderer, { 0.0f, 0.0f, rect.w, rect.h }, Theme::colors.bgPanel, Theme::colors.borderNormal);
}

void game::renderRightColumn(SDL_FRect top, SDL_FRect mid, SDL_FRect bot)
{
    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        SDL_FRect targetCardFRect = { top.x, layout.charRect.y, top.w, layout.charRect.h };
        UI::DrawEntitySummaryCard(renderer, this, targetCardFRect, activeTargetNPC, true);

        SDL_FRect targetEquipFRect = { top.x, layout.mapRect.y, top.w, layout.mapRect.h };
        UI::DrawEquipmentGrid(renderer, this, targetEquipFRect, activeTargetNPC, equipSlot::NONE, 12);

        float winX, winY, mouseX, mouseY;
        SDL_GetMouseState(&winX, &winY);
        SDL_RenderCoordinatesFromWindow(renderer, winX, winY, &mouseX, &mouseY);

        if (UIGridHelper::contains(layout.targetAvatarRect, mouseX, mouseY))
        {
            UI::DrawAnatomyTooltip(renderer, this, activeTargetNPC, mouseX, mouseY);
        }
        return;
    }

    SDL_FRect boxes[3] = { top, mid, bot };
    for (int i = 0; i < 3; i++)
    {
        ViewportGuard vpGuard(renderer, boxes[i]);
        UI::DrawPanel(renderer, { 0.0f, 0.0f, boxes[i].w, boxes[i].h }, Theme::colors.bgPanel, Theme::colors.borderNormal);
    }
}

bool game::loadFont(const std::string& id, const std::string& path, int ptSize)
{
    TTF_Font* newFont = TTF_OpenFont(path.c_str(), ptSize);
    if (!newFont) return false;
    fonts[id] = newFont;
    return true;
}

void game::renderTextAligned(const std::string& textStr, SDL_FRect destRect, TextAlignment align, bool fitToBox, const std::string& fontId, SDL_Color color)
{
    if (textStr.empty()) return;

    float srcW = 0.0f, srcH = 0.0f;
    SDL_Texture* texture = getOrRenderText(textStr, fontId, color, srcW, srcH);
    if (!texture || srcH <= 0.0f) return;

    float scale = 1.0f;
    if (fitToBox)
    {
        scale = destRect.h / srcH;
        if ((srcW * scale) > destRect.w) scale = destRect.w / srcW;
    }

    float drawW = srcW * scale;
    float drawH = srcH * scale;

    float drawX = destRect.x;
    if (align == TextAlignment::CENTER) drawX += (destRect.w - drawW) * 0.5f;
    else if (align == TextAlignment::RIGHT) drawX += destRect.w - drawW;

    float drawY = destRect.y + (destRect.h - drawH) * 0.5f;

    SDL_FRect renderDst = { drawX, drawY, drawW, drawH };
    SDL_RenderTexture(renderer, texture, NULL, &renderDst);
}

void game::renderTextWrapped(const std::string& text, SDL_FRect targetRect, const std::string& fontId, SDL_Color color)
{
    if (text.empty() || fonts.find(fontId) == fonts.end()) return;

    TTF_Font* targetFont = fonts[fontId];
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(targetFont, text.c_str(), 0, color, static_cast<int>(targetRect.w));
    if (!surface) return;

    maxDescriptionScrollY = std::max(0.0f, static_cast<float>(surface->h) - targetRect.h);
    if (descriptionScrollY > maxDescriptionScrollY) descriptionScrollY = maxDescriptionScrollY;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
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

            SDL_FRect renderDst = { currentX, startY, drawW, maxH };
            SDL_RenderTexture(renderer, texture, NULL, &renderDst);
            currentX += drawW;
        }
    }
    return currentX - startX;
}

SDL_Texture* game::getOrRenderText(const std::string& textStr, const std::string& fontId, SDL_Color color, float& outW, float& outH)
{
    if (textStr.empty()) return nullptr;

    TextCacheKey cacheKey{ fontId, textStr, color };

    auto it = textCache.find(cacheKey);
    if (it != textCache.end())
    {
        outW = it->second.w;
        outH = it->second.h;
        return it->second.texture;
    }

    if (textCache.size() > 300) clearTextCache();

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
    for (auto& [key, cached] : textCache) if (cached.texture) SDL_DestroyTexture(cached.texture);
    textCache.clear();
}

int game::getEquipmentGridIndex(equipSlot slot)
{
    switch (slot)
    {
        case equipSlot::EYEWEAR:         return 0;
        case equipSlot::HEADWEAR:        return 1;
        case equipSlot::HAIR_WEAR:       return 2;
        case equipSlot::HORNS_SLOT:      return 3;
        case equipSlot::WEAPON_MAIN:     return 4;
        case equipSlot::WEAPON_OFF:      return 5;
        case equipSlot::MOUTHWEAR:       return 6;
        case equipSlot::TORSO_OVER:      return 7;
        case equipSlot::NECKWEAR:        return 8;
        case equipSlot::WINGS_SLOT:      return 9;
        case equipSlot::PIERCING_EAR:    return 10;
        case equipSlot::PIERCING_NOSE:   return 11;
        case equipSlot::WRISTS:          return 12;
        case equipSlot::TORSO_UNDER:     return 13;
        case equipSlot::CHEST_WEAR:      return 14;
        case equipSlot::NIPPLES_WEAR:    return 15;
        case equipSlot::PIERCING_LIP:    return 16;
        case equipSlot::PIERCING_TONGUE: return 17;
        case equipSlot::HANDS:           return 18;
        case equipSlot::HIPS_WEAR:       return 19;
        case equipSlot::STOMACH_WEAR:    return 20;
        case equipSlot::FINGER_PRIMARY:  return 21;
        case equipSlot::PIERCING_NIPPLE: return 22;
        case equipSlot::PIERCING_NAVEL:  return 23;
        case equipSlot::ANKLES:          return 24;
        case equipSlot::LEGS_OUTER:      return 25;
        case equipSlot::GROIN_OVER:      return 26;
        case equipSlot::TAIL_SLOT:       return 27;
        case equipSlot::PIERCING_COCK:   return 28;
        case equipSlot::PIERCING_VAGINA: return 29;
        case equipSlot::CALVES:          return 30;
        case equipSlot::FEET:            return 31;
        case equipSlot::ASS_WEAR:        return 32;
        case equipSlot::PENIS_WEAR:      return 33;
        case equipSlot::VAGINA_WEAR:     return 34;
        default:                         return -1;
    }
}

std::string game::formatEquipSlotName(equipSlot slot)
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

bool conditionNode::evaluate(const game* gameContext) const {
    if (!gameContext) return false;

    switch (op) {
        case conditionOperator::AND: {
            for (const auto& child : children) {
                if (!child.evaluate(gameContext)) return false;
            }
            return true;
        }
        case conditionOperator::OR: {
            for (const auto& child : children) {
                if (child.evaluate(gameContext)) return true;
            }
            return children.empty();
        }
        case conditionOperator::NOT: {
            if (children.empty()) return true;
            return !children.front().evaluate(gameContext);
        }
        case conditionOperator::LEAF:
        default: {
            return gameContext->checkSingleCondition(condition);
        }
    }
}

void game::clean()
{
    eventBus::getInstance().clearAllListeners();
    clearTextCache();
    for (auto const& [id, f] : fonts) TTF_CloseFont(f);
    fonts.clear();
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}