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
        questDatabase::loadDatabase("data/quests");

        if (!saveManager::loadGame(this, "data/saves/save_01.json"))
        {
            saveManager::createInitialSave(this, "data/saves/save_01.json");
            saveManager::loadGame(this, "data/saves/save_01.json");
        }
    }
    loadMap("overworld", 1, 1);
    changeState(std::make_unique<explorationState>());
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
            if (!tileData.persistentNPC) tileData.persistentNPC = generateEncounterNPC();
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

bool game::checkSingleCondition(const gameCondition& cond)
{
    if (cond.type == "HAS_ITEM")
    {
        for (const auto& item : Player->inventory.backpack)
        {
            if (item && item->id == cond.target) return true;
        }
        return cond.requiredValue <= 0;
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
    if (choice.nextSceneId == "ENCOUNTER_FIGHT")
    {
        eventBus::getInstance().publishEvent({ gameEvent::combatEnded, 1, activeTargetNPC ? activeTargetNPC->id : "", activeTargetNPC });

        changeState(std::make_unique<eventState>());
        currentScene.id = "encounter_victory";
        currentScene.speakerName = activeTargetNPC ? activeTargetNPC->name : "Enemy";
        currentScene.bodyText = "You defeated " + currentScene.speakerName + " in combat!";
        currentScene.choices.clear();

        dialogueChoice contChoice; contChoice.label = "Continue"; contChoice.nextSceneId = "EXIT"; currentScene.choices.push_back(contChoice);
        dialogueChoice invChoice; invChoice.label = "Inventory"; invChoice.nextSceneId = "VICTORY_INVENTORY"; currentScene.choices.push_back(invChoice);
        dialogueChoice talkChoice; talkChoice.label = "Talk"; talkChoice.nextSceneId = "VICTORY_TALK"; currentScene.choices.push_back(talkChoice);

        activeButtons.clear();
        for (const auto& c : currentScene.choices)
        {
            actionButton btn; btn.label = c.label;
            btn.onClick = [this, c]() { processChoice(c); };
            if (activeButtons.size() < activeButtons.capacity()) activeButtons.push_back(btn);
        }
        return;
    }

    if (choice.nextSceneId == "VICTORY_INVENTORY")
    {
        changeState(std::make_unique<inventoryState>());
        return;
    }

    for (const auto& effect : choice.results)
    {
        if (effect.action == "GIVE_ITEM") Player->inventory.addItem(itemDatabase::getItem(effect.target));
        else if (effect.action == "REMOVE_ITEM") Player->inventory.removeItem(effect.target);
        else if (effect.action == "ADD_STAT") Player->stats.modifyBaseStat(effect.target, static_cast<float>(effect.amount));
        else if (effect.action == "SET_QUEST") Player->quests.setQuestStage(effect.target, effect.amount);
        else if (effect.action == "TELEPORT_MAP")
        {
            size_t c1 = effect.target.find(','), c2 = effect.target.find(',', c1 + 1);
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
        changeState(std::make_unique<explorationState>());
    }
    else
    {
        loadScene(choice.nextSceneId);
    }
}

void game::loadScene(const std::string& sceneId)
{
    changeState(std::make_unique<eventState>());
    currentScene = questDatabase::getScene(sceneId);

    activeButtons.clear();
    for (size_t i = 0; i < currentScene.choices.size(); i++)
    {
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
    static int npcCounter = 1;
    auto npc = std::make_shared<entity>("npc_bandit_" + std::to_string(npcCounter++), "Alleyway Bandit");

    npc->stats.level = 1;
    npc->stats.setBaseStat("health", 50.0f);
    npc->stats.setBaseStat("mana", 30.0f);
    npc->stats.setBaseStat("lust", 100.0f);

    auto shirt = itemDatabase::getItem("item_linen_shirt");
    auto pants = itemDatabase::getItem("item_leather_trousers");
    auto boots = itemDatabase::getItem("item_leather_boots");
    auto potion = itemDatabase::getItem("item_canis_root");

    std::vector<std::string> tags = npc->anatomy.getAllTags();
    if (shirt) { npc->inventory.addItem(shirt); npc->inventory.equipItem(0, equipSlot::TORSO_UNDER, tags); }
    if (pants) { npc->inventory.addItem(pants); npc->inventory.equipItem(0, equipSlot::LEGS_OUTER, tags); }
    if (boots) { npc->inventory.addItem(boots); npc->inventory.equipItem(0, equipSlot::FEET, tags); }
    if (potion) { potion->count = 3; npc->inventory.addItem(potion); }

    return npc;
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