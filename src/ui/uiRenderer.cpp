#include "ui/uiRenderer.h"

#include <algorithm>
#include <format>
#include <iostream>

#include "core/characterDescription.h"
#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/sexState.h"
#include "ui/actionGridManager.h"
#include "ui/uiWidget.h"

uiRenderer::uiRenderer()
{
    // Attempt loading custom layout from data/layouts/default_layout.json
    if (!m_layoutEngine.loadFromFile("data/layouts/default_layout.json"))
    {
        m_layoutEngine.loadDefaultLayout();
    }
}

uiRenderer::~uiRenderer() = default;

void uiRenderer::render(SDL_Renderer* renderer, game* gameContext)
{
    if (!renderer || !gameContext) return;

    // Refresh action buttons based on current state
    ActionGridManager::refresh(gameContext);

    // Query native screen/window render output size for non-stretched pixel-perfect drawing
    int winW = 1280, winH = 720;
    SDL_GetRenderOutputSize(renderer, &winW, &winH);

    float uiScale = std::clamp(static_cast<float>(winH) / 720.0f, 0.75f, 3.0f);
    auto panels = m_layoutEngine.computeLayout(static_cast<float>(winW), static_cast<float>(winH), uiScale);

    // Clear Screen with Dark Theme Background
    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, Theme::colors.bgDark.a);
    SDL_RenderClear(renderer);

    // Dispatch panel renderers matching calculated layout bounds
    for (const auto& p : panels)
    {
        if (p.id == "top_bar")
        {
            renderTopBar(renderer, gameContext, p.rect);
        }
        else if (p.id == "left_pane")
        {
            renderLeftPane(renderer, gameContext, p.rect);
        }
        else if (p.id == "center_pane")
        {
            renderCenterPane(renderer, gameContext, p.rect);
        }
        else if (p.id == "right_pane")
        {
            renderRightPane(renderer, gameContext, p.rect);
        }
        else if (p.id == "bottom_action_grid")
        {
            renderBottomActionGrid(renderer, gameContext, p.rect);
        }
        else
        {
            UIWidget::drawPanel(renderer, p.rect);
        }
    }

    SDL_RenderPresent(renderer);
}

void uiRenderer::renderTopBar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    UIWidget::drawPanel(renderer, rect, Theme::colors.bgHeader, Theme::colors.borderNormal);

    std::string title = "textRPG Engine (Decoupled Modern C++26 Simulation)";
    UIWidget::drawText(renderer, title, rect.x + 10.0f, rect.y + 10.0f, Theme::colors.textGold, 1.0f);

    std::string timeStr = std::format("{} | {}, {}",
                                      gameContext->getTime().getFormattedTime(),
                                      gameContext->getTime().getFormattedDate(),
                                      gameContext->getTime().getPhaseString());
    UIWidget::drawText(renderer, timeStr, rect.x + (rect.w * 0.40f), rect.y + 10.0f, Theme::colors.textSecondary, 1.0f);

    if (entity* p = gameContext->getPlayer())
    {
        std::string goldStr = std::format("Gold: {:.0f}¤", p->getStat("currency"));
        UIWidget::drawText(renderer, goldStr, rect.x + (rect.w * 0.75f), rect.y + 10.0f, Theme::colors.textGold, 1.0f);
    }

    if (const gameMap* m = gameContext->getActiveMap())
    {
        std::string locStr = std::format("Map: {}", m->getName());
        UIWidget::drawText(renderer, locStr, rect.x + (rect.w * 0.88f), rect.y + 10.0f, Theme::colors.textAccent, 1.0f);
    }
}

void uiRenderer::renderLeftPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    UIWidget::drawPanel(renderer, rect);

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "CHARACTER STATUS");

    entity* p = gameContext->getPlayer();
    if (!p) return;

    float curY = rect.y + 40.0f;
    float padX = rect.x + 10.0f;
    float innerW = rect.w - 20.0f;

    UIWidget::drawText(renderer, std::format("Name: {}", p->name), padX, curY, Theme::colors.textPrimary, 1.0f); curY += 16.0f;
    UIWidget::drawText(renderer, std::format("Title: {}", p->anatomy.getRacialTitle()), padX, curY, Theme::colors.textAccent, 1.0f); curY += 16.0f;
    UIWidget::drawText(renderer, std::format("Gender: {}", genderArchetypeToString(p->anatomy.getGenderArchetype())), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 20.0f;

    // Resource Bars
    float hp = p->getStat("health");
    UIWidget::drawProgressBar(renderer, { padX, curY, innerW, 18.0f }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("HP: {:.0f}/100", hp)); curY += 24.0f;

    float mana = p->getStat("mana");
    UIWidget::drawProgressBar(renderer, { padX, curY, innerW, 18.0f }, mana, 50.0f, Theme::colors.mana, Theme::colors.bgDark, std::format("Mana: {:.0f}/50", mana)); curY += 24.0f;

    float lust = p->getStat("lust");
    UIWidget::drawProgressBar(renderer, { padX, curY, innerW, 18.0f }, lust, 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Lust: {:.0f}/100", lust)); curY += 28.0f;

    // Attributes
    UIWidget::drawText(renderer, "ATTRIBUTES", padX, curY, Theme::colors.textGold, 1.0f); curY += 18.0f;
    UIWidget::drawText(renderer, std::format("Physique:  {:.0f}", p->getStat("physique")), padX, curY, Theme::colors.physique, 1.0f); curY += 16.0f;
    UIWidget::drawText(renderer, std::format("Agility:   {:.0f}", p->getStat("agility")), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 16.0f;
    UIWidget::drawText(renderer, std::format("Arcane:    {:.0f}", p->getStat("arcane")), padX, curY, Theme::colors.arcane, 1.0f); curY += 16.0f;
    UIWidget::drawText(renderer, std::format("Corruption:{:.0f}", p->getStat("corruption")), padX, curY, Theme::colors.corruption, 1.0f); curY += 24.0f;

    // Anatomy & Fluid Stores
    UIWidget::drawText(renderer, "ANATOMY & FLUIDS", padX, curY, Theme::colors.textGold, 1.0f); curY += 18.0f;
    UIWidget::drawText(renderer, std::format("Height: {:.2f}m", p->anatomy.heightMeters), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 16.0f;

    if (const bodyPart* b = p->anatomy.getPart(bodySlot::BREASTS))
    {
        UIWidget::drawText(renderer, std::format("Breasts: {}-Cup ({:.0f}/{:.0f}ml milk)", bodyPart::getCupSizeName(b->cupSize), b->currentFluidMl, b->maxFluidMl), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 16.0f;
    }
    if (const bodyPart* g = p->anatomy.getPart(bodySlot::GROIN))
    {
        if (p->anatomy.hasPenis())
        {
            UIWidget::drawText(renderer, std::format("Penis: {:.1f}cm x {:.1f}cm ({:.0f}/{:.0f}ml cum)", g->length, g->diameter, g->currentFluidMl, g->maxFluidMl), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 16.0f;
        }
    }
}

void uiRenderer::renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    UIWidget::drawPanel(renderer, rect);

    iGameState* state = gameContext->getActiveState();
    if (!state) return;

    if (dynamic_cast<sexState*>(state))
    {
        renderSexView(renderer, gameContext, rect);
    }
    else if (dynamic_cast<CombatState*>(state))
    {
        renderCombatView(renderer, gameContext, rect);
    }
    else if (dynamic_cast<encounterResolutionState*>(state))
    {
        renderResolutionView(renderer, gameContext, rect);
    }
    else if (dynamic_cast<inventoryState*>(state))
    {
        renderInventoryView(renderer, gameContext, rect);
    }
    else if (dynamic_cast<eventState*>(state))
    {
        renderSceneView(renderer, gameContext, rect);
    }
    else
    {
        renderExplorationView(renderer, gameContext, rect);
    }
}

void uiRenderer::renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    const questScene& scene = gameContext->getCurrentScene();
    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, scene.speakerName.empty() ? "NARRATIVE SCENE" : scene.speakerName, Theme::colors.bgHeader, Theme::colors.textGold);

    UIWidget::drawTextWrapped(renderer, scene.bodyText, rect.x + 15.0f, rect.y + 45.0f, rect.w - 30.0f, Theme::colors.textPrimary, 1.0f);
}

void uiRenderer::renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    sexState* sex = dynamic_cast<sexState*>(gameContext->getActiveState());
    if (!sex) return;

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "INTERACTIVE CYOA EROTIC ENCOUNTER", Theme::colors.bgHeader, Theme::colors.lust);

    entity* partner = sex->getPartner();
    std::string partnerName = partner ? partner->name : "Partner";

    float curY = rect.y + 40.0f;
    float padX = rect.x + 15.0f;
    float innerW = rect.w - 30.0f;
    float halfW = (innerW - 10.0f) / 2.0f;

    UIWidget::drawText(renderer, std::format("Partner: {} | Stance: {}", partnerName, sexStanceToString(sex->getStance())), padX, curY, Theme::colors.textGold, 1.0f); curY += 20.0f;

    UIWidget::drawProgressBar(renderer, { padX, curY, halfW, 16.0f }, sex->getPlayerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Your Arousal: {:.0f}/100", sex->getPlayerArousal()));
    UIWidget::drawProgressBar(renderer, { padX + halfW + 10.0f, curY, halfW, 16.0f }, sex->getPartnerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("{} Arousal: {:.0f}/100", partnerName, sex->getPartnerArousal()));
    curY += 24.0f;

    float dom = sex->getPlayerDominance();
    UIWidget::drawProgressBar(renderer, { padX, curY, innerW, 16.0f }, dom + 100.0f, 200.0f, Theme::colors.textAccent, Theme::colors.bgDark, std::format("Dominance Continuum: {:.0f} ({})", dom, sex->isPlayerDominant() ? "Dominant" : "Submissive"));
    curY += 28.0f;

    UIWidget::drawText(renderer, "NARRATIVE LOG:", padX, curY, Theme::colors.textGold, 1.0f); curY += 18.0f;
    UIWidget::drawTextWrapped(renderer, sex->getNarrativeLog(), padX, curY, innerW, Theme::colors.textSecondary, 1.0f);
}

void uiRenderer::renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    CombatState* combat = dynamic_cast<CombatState*>(gameContext->getActiveState());
    if (!combat) return;

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, std::format("TACTICAL COMBAT (Round {})", combat->getEngine().getCurrentRound()), Theme::colors.bgHeader, Theme::colors.enemy);

    float curY = rect.y + 40.0f;
    float padX = rect.x + 15.0f;
    float innerW = rect.w - 30.0f;
    float halfW = (innerW - 10.0f) / 2.0f;

    UIWidget::drawText(renderer, "PARTY STATUS", padX, curY, Theme::colors.textGold, 1.0f); curY += 18.0f;

    for (const auto& p : combat->getEngine().getPlayerParty())
    {
        if (p.character)
        {
            float hp = p.character->getStat("health");
            UIWidget::drawProgressBar(renderer, { padX, curY, halfW, 16.0f }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("{} HP: {:.0f} (AP: {})", p.character->name, hp, p.currentAp));
        }
    }

    for (const auto& enemyP : combat->getEngine().getEnemyParty())
    {
        if (enemyP.character)
        {
            float hp = enemyP.character->getStat("health");
            UIWidget::drawProgressBar(renderer, { padX + halfW + 10.0f, curY, halfW, 16.0f }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("{} HP: {:.0f}", enemyP.character->name, hp));
        }
    }
    curY += 30.0f;

    UIWidget::drawText(renderer, "COMBAT LOG:", padX, curY, Theme::colors.textGold, 1.0f); curY += 18.0f;
    for (const auto& logEntry : combat->getEngine().getCombatLog())
    {
        UIWidget::drawText(renderer, logEntry, padX, curY, Theme::colors.textSecondary, 1.0f);
        curY += 14.0f;
        if (curY > rect.y + rect.h - 20.0f) break;
    }
}

void uiRenderer::renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    encounterResolutionState* res = dynamic_cast<encounterResolutionState*>(gameContext->getActiveState());
    if (!res) return;

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "POST-COMBAT RESOLUTION HUB", Theme::colors.bgHeader, Theme::colors.textGold);

    float curY = rect.y + 40.0f;
    float padX = rect.x + 15.0f;

    UIWidget::drawText(renderer, res->getResolutionLog(), padX, curY, Theme::colors.textAccent, 1.0f); curY += 30.0f;
    UIWidget::drawText(renderer, "DEFEATED ENEMIES AT YOUR MERCY:", padX, curY, Theme::colors.textGold, 1.0f); curY += 20.0f;

    const auto& records = res->getDefeatedRecords();
    size_t selected = res->getSelectedIndex();

    for (size_t i = 0; i < records.size(); ++i)
    {
        const auto& rec = records[i];
        std::string line = std::format("{} [{}] {} | Looted: {} | Stripped: {} | Sex: {} | Subjugated: {}",
                                       (i == selected ? "->" : "  "), i,
                                       (rec.npc ? rec.npc->name : "Enemy"),
                                       rec.isLooted ? "Yes" : "No",
                                       rec.isStripped ? "Yes" : "No",
                                       rec.hadSex ? "Yes" : "No",
                                       rec.isSubjugated ? "Yes" : "No");

        SDL_Color c = (i == selected) ? Theme::colors.textGold : Theme::colors.textPrimary;
        UIWidget::drawText(renderer, line, padX, curY, c, 1.0f);
        curY += 18.0f;
    }
}

void uiRenderer::renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "INVENTORY & CONTAINER VIEW", Theme::colors.bgHeader, Theme::colors.textGold);

    float curY = rect.y + 40.0f;
    float padX = rect.x + 15.0f;
    float halfW = (rect.w - 30.0f) / 2.0f;

    UIWidget::drawText(renderer, "PLAYER BACKPACK (Side 0)", padX, curY, Theme::colors.textGold, 1.0f);
    UIWidget::drawText(renderer, "GROUND / CONTAINER (Side 1)", padX + halfW, curY, Theme::colors.textGold, 1.0f);
    curY += 20.0f;

    auto backpack = gameContext->getPlayerInventoryStacked();
    for (size_t i = 0; i < backpack.size() && i < 15; ++i)
    {
        if (backpack[i].itemPtr)
        {
            bool isSelected = (gameContext->selectedInventorySide == 0 && gameContext->selectedInventoryIndex == static_cast<int>(i));
            std::string line = std::format("[{}] {} (x{})", i, backpack[i].itemPtr->name, backpack[i].totalCount);
            UIWidget::drawText(renderer, line, padX, curY + (i * 16.0f), isSelected ? Theme::colors.textGold : Theme::colors.textPrimary, 1.0f);
        }
    }

    auto ground = gameContext->getTileInventoryStacked();
    for (size_t i = 0; i < ground.size() && i < 15; ++i)
    {
        if (ground[i].itemPtr)
        {
            bool isSelected = (gameContext->selectedInventorySide == 1 && gameContext->selectedInventoryIndex == static_cast<int>(i));
            std::string line = std::format("[{}] {} (x{})", i, ground[i].itemPtr->name, ground[i].totalCount);
            UIWidget::drawText(renderer, line, padX + halfW, curY + (i * 16.0f), isSelected ? Theme::colors.textGold : Theme::colors.textPrimary, 1.0f);
        }
    }
}

void uiRenderer::renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "OVERWORLD EXPLORATION", Theme::colors.bgHeader, Theme::colors.textGold);

    const gameMap* m = gameContext->getActiveMap();
    if (!m) return;

    float curY = rect.y + 40.0f;
    float padX = rect.x + 15.0f;
    float innerW = rect.w - 30.0f;

    UIWidget::drawText(renderer, std::format("Current Location: {} at [{}, {}]", m->getName(), gameContext->gridX, gameContext->gridY), padX, curY, Theme::colors.textAccent, 1.0f); curY += 24.0f;
    UIWidget::drawTextWrapped(renderer, "You are exploring the district. Use movement keys (W, A, S, D) or click action grid commands to navigate surrounding tiles.", padX, curY, innerW, Theme::colors.textPrimary, 1.0f);
}

void uiRenderer::renderRightPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    UIWidget::drawPanel(renderer, rect);

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 30.0f };
    UIWidget::drawHeader(renderer, headerRect, "WORLD MAP & TARGET");

    const gameMap* m = gameContext->getActiveMap();
    if (!m) return;

    float padX = rect.x + 10.0f;
    float startY = rect.y + 40.0f;
    float tileSize = std::clamp((rect.w - 20.0f) / 9.0f, 16.0f, 32.0f);
    int radius = 4;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            int mapX = gameContext->gridX + dx;
            int mapY = gameContext->gridY + dy;

            SDL_FRect tileRect = { padX + (dx + radius) * tileSize, startY + (dy + radius) * tileSize, tileSize - 2.0f, tileSize - 2.0f };

            Tile t = m->getTile(mapX, mapY);
            SDL_Color tileColor = Theme::colors.bgDark;
            std::string label = "";

            if (t.discovery == STATE_HIDDEN)
            {
                tileColor = SDL_Color{ 15, 15, 20, 255 };
            }
            else
            {
                if (t.type == TILE_WALL) tileColor = Theme::colors.bgHeader;
                else if (t.type == TILE_FLOOR) tileColor = Theme::colors.bgSlot;
                else if (t.type == TILE_DOOR) { tileColor = Theme::colors.textGold; label = "D"; }

                if (dx == 0 && dy == 0)
                {
                    tileColor = Theme::colors.borderButton;
                    label = "@";
                }
            }

            SDL_SetRenderDrawColor(renderer, tileColor.r, tileColor.g, tileColor.b, tileColor.a);
            SDL_RenderFillRect(renderer, &tileRect);

            if (!label.empty())
            {
                UIWidget::drawText(renderer, label, tileRect.x + 6.0f, tileRect.y + 4.0f, Theme::colors.textGold, 1.0f);
            }
        }
    }

    // Target NPC Inspector Section
    float curY = startY + (9.0f * tileSize) + 15.0f;
    UIWidget::drawText(renderer, "PROXIMITY TARGET", padX, curY, Theme::colors.textGold, 1.0f); curY += 20.0f;

    if (entity* npc = gameContext->getActiveTargetNPC())
    {
        UIWidget::drawText(renderer, std::format("Name: {}", npc->name), padX, curY, Theme::colors.textPrimary, 1.0f); curY += 16.0f;
        UIWidget::drawText(renderer, std::format("Level: {} | Race: {}", npc->stats.level, npc->anatomy.getDominantRace()), padX, curY, Theme::colors.textSecondary, 1.0f); curY += 16.0f;

        float hp = npc->getStat("health");
        UIWidget::drawProgressBar(renderer, { padX, curY, rect.w - 20.0f, 16.0f }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("HP: {:.0f}", hp));
    }
    else
    {
        UIWidget::drawText(renderer, "No active target in range.", padX, curY, Theme::colors.textMuted, 1.0f);
    }
}

void uiRenderer::renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect)
{
    UIWidget::drawPanel(renderer, rect);

    SDL_FRect headerRect = { rect.x, rect.y, rect.w, 24.0f };
    UIWidget::drawHeader(renderer, headerRect, "ACTION COMMANDS");

    const auto& buttons = gameContext->getActiveActionButtons();
    int totalButtons = static_cast<int>(buttons.size());

    int totalPages = (totalButtons > 0) ? ((totalButtons - 1) / BUTTONS_PER_PAGE) + 1 : 1;
    m_currentPage = std::clamp(m_currentPage, 0, totalPages - 1);

    int startIndex = m_currentPage * BUTTONS_PER_PAGE;
    int endIndex = std::min(startIndex + BUTTONS_PER_PAGE, totalButtons);

    float padX = rect.x + 10.0f;
    float startY = rect.y + 30.0f;
    float availableW = rect.w - 20.0f;
    float spaceX = 10.0f;
    float spaceY = 8.0f;
    int cols = 5;
    int rows = 2;
    float btnWidth = (availableW - (spaceX * (cols - 1))) / cols;
    float btnHeight = (rect.h - 40.0f - (spaceY * (rows - 1))) / rows;

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    for (int i = startIndex; i < endIndex; ++i)
    {
        int slotIdx = i - startIndex;
        int col = slotIdx % cols;
        int row = slotIdx / cols;

        SDL_FRect btnRect = { padX + col * (btnWidth + spaceX), startY + row * (btnHeight + spaceY), btnWidth, btnHeight };

        bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                        mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

        UIWidget::drawButton(renderer, btnRect, buttons[i].label, hovered, buttons[i].isEnabled);

        if (hovered && clicked && buttons[i].isEnabled && buttons[i].onClick)
        {
            buttons[i].onClick();
            gameContext->input.consumeMouseClick();
            break;
        }
    }

    // Pagination Controls
    if (totalPages > 1)
    {
        SDL_FRect prevRect = { rect.x + rect.w - 130.0f, rect.y + 2.0f, 60.0f, 20.0f };
        SDL_FRect nextRect = { rect.x + rect.w - 65.0f, rect.y + 2.0f, 60.0f, 20.0f };

        bool prevHovered = (mousePos.x >= prevRect.x && mousePos.x <= prevRect.x + prevRect.w && mousePos.y >= prevRect.y && mousePos.y <= prevRect.y + prevRect.h);
        bool nextHovered = (mousePos.x >= nextRect.x && mousePos.x <= nextRect.x + nextRect.w && mousePos.y >= nextRect.y && mousePos.y <= nextRect.y + nextRect.h);

        UIWidget::drawButton(renderer, prevRect, "< Prev", prevHovered, m_currentPage > 0);
        UIWidget::drawButton(renderer, nextRect, "Next >", nextHovered, m_currentPage < totalPages - 1);

        if (prevHovered && clicked && m_currentPage > 0)
        {
            m_currentPage--;
            gameContext->input.consumeMouseClick();
        }
        else if (nextHovered && clicked && m_currentPage < totalPages - 1)
        {
            m_currentPage++;
            gameContext->input.consumeMouseClick();
        }
    }
}