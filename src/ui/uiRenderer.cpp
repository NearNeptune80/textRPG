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
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/sexState.h"
#include "state/shopState.h"
#include "state/transformationState.h"
#include "settings/settingsManager.h"
#include "ui/actionGridManager.h"
#include "ui/fontManager.h"
#include "ui/uiWidget.h"

uiRenderer::uiRenderer()
{
    // Attempt loading custom layout from active settings or candidate paths
    GameSettings settings;
    settingsManager::loadFromFile(settings);
    std::string layoutName = settings.display.activeLayout.empty() ? "default" : settings.display.activeLayout;

    std::vector<std::string> candidatePaths = {
        layoutName,
        "data/layouts/" + layoutName + ".json",
        "data/layouts/layout_" + layoutName + ".json",
        "data/layouts/" + layoutName,
        "layouts/" + layoutName + ".json",
        "layouts/layout_" + layoutName + ".json",
        "data/layouts/custom_tile_layout.json",
        "data/layouts/default_layout.json"
    };

    bool loaded = false;
    for (const auto& path : candidatePaths)
    {
        if (m_layoutEngine.loadFromFile(path))
        {
            std::cout << "[uiRenderer] Initialised layout from: " << path << "\n";
            loaded = true;
            break;
        }
    }

    if (!loaded)
    {
        m_layoutEngine.loadDefaultLayout();
    }

    // Attempt loading primary TTF font with fallback to embedded 8x8 font
    fontManager::getInstance().loadFont("data/fonts/Roboto/static/Roboto-Medium.ttf", 14.0f);
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
    fontManager::getInstance().setScale(uiScale);
    auto panels = m_layoutEngine.computeLayout(static_cast<float>(winW), static_cast<float>(winH), uiScale);

    // Clear Screen with Dark Theme Background
    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, Theme::colors.bgDark.a);
    SDL_RenderClear(renderer);

    // Helper lambda to check if a widget id or tag is present in a panel
    auto hasWidgetTag = [](const std::vector<std::string>& widgets, const std::string& target) {
        return std::ranges::find(widgets, target) != widgets.end();
    };

    // Handle Mouse Wheel Scrolling on Hovered Panel (Strictly clamped to content height)
    const auto mousePos = gameContext->input.getMousePosition();
    const float wheelY = gameContext->input.getMouseWheelY();
    if (wheelY != 0.0f)
    {
        for (const auto& p : panels)
        {
            if (mousePos.x >= p.rect.x && mousePos.x <= p.rect.x + p.rect.w &&
                mousePos.y >= p.rect.y && mousePos.y <= p.rect.y + p.rect.h)
            {
                float maxScroll = m_panelMaxScrollY.contains(p.id) ? m_panelMaxScrollY[p.id] : 0.0f;
                m_panelScrollY[p.id] -= wheelY * (32.0f * uiScale);
                m_panelScrollY[p.id] = std::clamp(m_panelScrollY[p.id], 0.0f, maxScroll);
                break;
            }
        }
        gameContext->input.consumeMouseWheel();
    }

    // Dispatch panel renderers matching calculated layout bounds
    for (const auto& p : panels)
    {
        // Set Scissor Clip Rect so content never overflows its panel bounds
        SDL_Rect clipRect = {
            static_cast<int>(p.rect.x),
            static_cast<int>(p.rect.y),
            static_cast<int>(p.rect.w),
            static_cast<int>(p.rect.h)
        };
        SDL_SetRenderClipRect(renderer, &clipRect);

        const bool hasTopBar = hasWidgetTag(p.widgets, "widget_top_bar_full") ||
                               hasWidgetTag(p.widgets, "TOP_STATUS_BAR") ||
                               p.id == "top_bar";

        const bool hasActionGrid = hasWidgetTag(p.widgets, "widget_action_commands") ||
                                   hasWidgetTag(p.widgets, "ACTION_GRID") ||
                                   p.id == "bottom_action_grid";

        if (hasTopBar)
        {
            renderTopBar(renderer, gameContext, p.rect, uiScale);
        }
        else if (hasActionGrid)
        {
            renderBottomActionGrid(renderer, gameContext, p.rect, uiScale);
        }
        else if (p.widgets.empty())
        {
            UIWidget::drawPanel(renderer, p.rect);
        }
        else
        {
            // General Multi-Widget Box Container
            UIWidget::drawPanel(renderer, p.rect);

            float scrollY = m_panelScrollY.contains(p.id) ? m_panelScrollY[p.id] : 0.0f;
            float curY = p.rect.y + (10.0f * uiScale) - scrollY;

            for (const auto& wId : p.widgets)
            {
                if (wId == "widget_char_overview")
                {
                    curY += renderWidgetCharOverview(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_vitals_gauges")
                {
                    curY += renderWidgetVitals(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_attributes_table")
                {
                    curY += renderWidgetAttributes(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_anatomy_fluids")
                {
                    curY += renderWidgetAnatomy(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_paperdoll_equipment")
                {
                    curY += renderWidgetPaperdoll(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_item_details_inspector")
                {
                    curY += renderWidgetItemInspector(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_minimap_radar" || wId == "MINIMAP_RADAR")
                {
                    curY += renderWidgetRadar(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_target_inspector" || wId == "TARGET_INSPECTOR")
                {
                    curY += renderWidgetTarget(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_narrative_story" || wId == "SCENE_NARRATIVE")
                {
                    curY += renderCenterPane(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_inventory_dual" || wId == "BACKPACK_INVENTORY")
                {
                    curY += renderInventoryView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_tactical_combat" || wId == "COMBAT_VIEW")
                {
                    curY += renderCombatView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_main_menu_hero" || wId == "widget_main_menu_actions" || wId == "widget_save_slot_list")
                {
                    curY += renderMainMenu(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_options_content" || wId == "widget_options_demographics" || wId == "widget_options_display_audio")
                {
                    curY += renderOptionsView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_merchant_dialog" || wId == "widget_merchant_catalog" || wId == "widget_player_sell_grid" || wId == "widget_transaction_cart")
                {
                    curY += renderShopView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_body_mutations_tree" || wId == "widget_active_enchantments_list" || wId == "widget_enchanting_altar")
                {
                    curY += renderTransformationView(renderer, gameContext, p.rect, curY, uiScale);
                }
            }

            float totalContentH = (curY + scrollY) - p.rect.y + (10.0f * uiScale);
            float calculatedMaxScroll = std::max(0.0f, totalContentH - p.rect.h);
            m_panelMaxScrollY[p.id] = calculatedMaxScroll;
            m_panelScrollY[p.id] = std::clamp(m_panelScrollY[p.id], 0.0f, calculatedMaxScroll);
            drawScrollbar(renderer, p.rect, totalContentH, m_panelScrollY[p.id], uiScale);
        }

        // Reset clip rect
        SDL_SetRenderClipRect(renderer, nullptr);
    }

    SDL_RenderPresent(renderer);
}

void uiRenderer::drawScrollbar(SDL_Renderer* renderer, const SDL_FRect& panelRect, float contentHeight, float currentScroll, float uiScale)
{
    if (contentHeight <= panelRect.h) return;

    float barW = 4.0f * uiScale;
    float trackX = panelRect.x + panelRect.w - barW - (2.0f * uiScale);
    float trackY = panelRect.y + (2.0f * uiScale);
    float trackH = panelRect.h - (4.0f * uiScale);

    // Track background
    SDL_FRect trackRect = { trackX, trackY, barW, trackH };
    SDL_SetRenderDrawColor(renderer, 20, 20, 28, 140);
    SDL_RenderFillRect(renderer, &trackRect);

    // Scroll thumb
    float thumbH = std::max(16.0f * uiScale, (panelRect.h / contentHeight) * trackH);
    float maxScroll = contentHeight - panelRect.h;
    float thumbY = trackY + (maxScroll > 0.0f ? (currentScroll / maxScroll) * (trackH - thumbH) : 0.0f);

    SDL_FRect thumbRect = { trackX, thumbY, barW, thumbH };
    SDL_SetRenderDrawColor(renderer, Theme::colors.textGold.r, Theme::colors.textGold.g, Theme::colors.textGold.b, 220);
    SDL_RenderFillRect(renderer, &thumbRect);
}

void uiRenderer::renderTopBar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale)
{
    UIWidget::drawPanel(renderer, rect, Theme::colors.bgHeader, Theme::colors.borderNormal);

    // Left: Game Title
    UIWidget::drawText(renderer, "textRPG v0.4.0-DEV", rect.x + (12.0f * uiScale), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textGold, uiScale);

    // Center: Currency
    if (entity* p = gameContext->getPlayer())
    {
        std::string goldStr = std::format("{:.0f}¤", p->getStat("currency"));
        UIWidget::drawText(renderer, goldStr, rect.x + (rect.w * 0.48f), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textGold, uiScale);
    }

    // Right: Map Location
    if (const gameMap* m = gameContext->getActiveMap())
    {
        std::string locStr = std::format("{}", m->getName());
        UIWidget::drawText(renderer, locStr, rect.x + rect.w - (140.0f * uiScale), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textAccent, uiScale);
    }
}

float uiRenderer::renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    iGameState* state = gameContext->getActiveState();
    if (!state) return 0.0f;

    if (dynamic_cast<mainMenuState*>(state))
    {
        return renderMainMenu(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<optionsState*>(state))
    {
        return renderOptionsView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<shopState*>(state))
    {
        return renderShopView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<transformationState*>(state))
    {
        return renderTransformationView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<sexState*>(state))
    {
        return renderSexView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<CombatState*>(state))
    {
        return renderCombatView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<encounterResolutionState*>(state))
    {
        return renderResolutionView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<inventoryState*>(state))
    {
        return renderInventoryView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<eventState*>(state))
    {
        return renderSceneView(renderer, gameContext, rect, curY, uiScale);
    }
    else
    {
        return renderExplorationView(renderer, gameContext, rect, curY, uiScale);
    }
}

float uiRenderer::renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    const questScene& scene = gameContext->getCurrentScene();
    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, scene.speakerName.empty() ? "NARRATIVE SCENE" : scene.speakerName, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    float padX = rect.x + (12.0f * uiScale);
    float innerW = rect.w - (24.0f * uiScale);
    float textH = UIWidget::drawTextWrapped(renderer, scene.bodyText, padX, curY, innerW, Theme::colors.textPrimary, uiScale);
    curY += textH + (6.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    sexState* sex = dynamic_cast<sexState*>(gameContext->getActiveState());
    if (!sex) return 0.0f;

    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "INTERACTIVE CYOA EROTIC ENCOUNTER", Theme::colors.bgHeader, Theme::colors.lust, uiScale);
    curY += headerH + (10.0f * uiScale);

    entity* partner = sex->getPartner();
    std::string partnerName = partner ? partner->name : "Partner";

    float padX = rect.x + (12.0f * uiScale);
    float innerW = rect.w - (24.0f * uiScale);
    float halfW = (innerW - (10.0f * uiScale)) / 2.0f;
    float barH = 18.0f * uiScale;

    UIWidget::drawText(renderer, std::format("Partner: {} | Stance: {}", partnerName, sexStanceToString(sex->getStance())), padX, curY, Theme::colors.textGold, uiScale);
    curY += (22.0f * uiScale);

    UIWidget::drawProgressBar(renderer, { padX, curY, halfW, barH }, sex->getPlayerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Your Arousal: {:.0f}/100", sex->getPlayerArousal()), uiScale);
    UIWidget::drawProgressBar(renderer, { padX + halfW + (10.0f * uiScale), curY, halfW, barH }, sex->getPartnerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("{} Arousal: {:.0f}/100", partnerName, sex->getPartnerArousal()), uiScale);
    curY += (barH + 8.0f * uiScale);

    float dom = sex->getPlayerDominance();
    UIWidget::drawProgressBar(renderer, { padX, curY, innerW, barH }, dom + 100.0f, 200.0f, Theme::colors.textAccent, Theme::colors.bgDark, std::format("Dominance Continuum: {:.0f} ({})", dom, sex->isPlayerDominant() ? "Dominant" : "Submissive"), uiScale);
    curY += (barH + 10.0f * uiScale);

    UIWidget::drawText(renderer, "NARRATIVE LOG:", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);
    float textH = UIWidget::drawTextWrapped(renderer, sex->getNarrativeLog(), padX, curY, innerW, Theme::colors.textSecondary, uiScale);
    curY += textH + (6.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    CombatState* combat = dynamic_cast<CombatState*>(gameContext->getActiveState());
    if (!combat) return 0.0f;

    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, std::format("TACTICAL COMBAT (Round {})", combat->getEngine().getCurrentRound()), Theme::colors.bgHeader, Theme::colors.enemy, uiScale);
    curY += headerH + (10.0f * uiScale);

    float padX = rect.x + (12.0f * uiScale);
    float innerW = rect.w - (24.0f * uiScale);
    float halfW = (innerW - (10.0f * uiScale)) / 2.0f;
    float barH = 18.0f * uiScale;

    UIWidget::drawText(renderer, "PARTY STATUS", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    for (const auto& p : combat->getEngine().getPlayerParty())
    {
        if (p.character)
        {
            float hp = p.character->getStat("health");
            UIWidget::drawProgressBar(renderer, { padX, curY, halfW, barH }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("{} HP: {:.0f} (AP: {})", p.character->name, hp, p.currentAp), uiScale);
        }
    }

    for (const auto& enemyP : combat->getEngine().getEnemyParty())
    {
        if (enemyP.character)
        {
            float hp = enemyP.character->getStat("health");
            UIWidget::drawProgressBar(renderer, { padX + halfW + (10.0f * uiScale), curY, halfW, barH }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("{} HP: {:.0f}", enemyP.character->name, hp), uiScale);
        }
    }
    curY += (barH + 12.0f * uiScale);

    UIWidget::drawText(renderer, "COMBAT LOG:", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);
    for (const auto& logEntry : combat->getEngine().getCombatLog())
    {
        UIWidget::drawText(renderer, logEntry, padX, curY, Theme::colors.textSecondary, uiScale);
        curY += (16.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    encounterResolutionState* res = dynamic_cast<encounterResolutionState*>(gameContext->getActiveState());
    if (!res) return 0.0f;

    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "POST-COMBAT RESOLUTION HUB", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    float padX = rect.x + (12.0f * uiScale);
    UIWidget::drawText(renderer, res->getResolutionLog(), padX, curY, Theme::colors.textAccent, uiScale);
    curY += (26.0f * uiScale);

    UIWidget::drawText(renderer, "DEFEATED ENEMIES AT YOUR MERCY:", padX, curY, Theme::colors.textGold, uiScale);
    curY += (20.0f * uiScale);

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
        UIWidget::drawText(renderer, line, padX, curY, c, uiScale);
        curY += (18.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "INVENTORY & CONTAINER VIEW", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    float padX = rect.x + (12.0f * uiScale);
    float halfW = (rect.w - (24.0f * uiScale)) / 2.0f;
    float lineH = 18.0f * uiScale;

    UIWidget::drawText(renderer, "PLAYER BACKPACK (Side 0)", padX, curY, Theme::colors.textGold, uiScale);
    UIWidget::drawText(renderer, "GROUND / CONTAINER (Side 1)", padX + halfW, curY, Theme::colors.textGold, uiScale);
    curY += (20.0f * uiScale);

    auto backpack = gameContext->getPlayerInventoryStacked();
    for (size_t i = 0; i < backpack.size() && i < 15; ++i)
    {
        if (backpack[i].itemPtr)
        {
            bool isSelected = (gameContext->selectedInventorySide == 0 && gameContext->selectedInventoryIndex == static_cast<int>(i));
            std::string line = std::format("[{}] {} (x{})", i, backpack[i].itemPtr->name, backpack[i].totalCount);
            UIWidget::drawText(renderer, line, padX, curY + (i * lineH), isSelected ? Theme::colors.textGold : Theme::colors.textPrimary, uiScale);
        }
    }

    auto ground = gameContext->getTileInventoryStacked();
    for (size_t i = 0; i < ground.size() && i < 15; ++i)
    {
        if (ground[i].itemPtr)
        {
            bool isSelected = (gameContext->selectedInventorySide == 1 && gameContext->selectedInventoryIndex == static_cast<int>(i));
            std::string line = std::format("[{}] {} (x{})", i, ground[i].itemPtr->name, ground[i].totalCount);
            UIWidget::drawText(renderer, line, padX + halfW, curY + (i * lineH), isSelected ? Theme::colors.textGold : Theme::colors.textPrimary, uiScale);
        }
    }

    size_t maxRows = std::max(backpack.size(), ground.size());
    curY += (std::min(maxRows, size_t(15)) * lineH) + (10.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    const gameMap* m = gameContext->getActiveMap();
    if (!m) return 0.0f;

    float startY = curY;
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "OVERWORLD EXPLORATION", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    float padX = rect.x + (12.0f * uiScale);
    float innerW = rect.w - (24.0f * uiScale);

    UIWidget::drawText(renderer, std::format("Current Location: {} at [{}, {}]", m->getName(), gameContext->gridX, gameContext->gridY), padX, curY, Theme::colors.textAccent, uiScale);
    curY += (22.0f * uiScale);

    float textH = UIWidget::drawTextWrapped(renderer, "You are exploring the district. Use movement keys (W, A, S, D) or click action grid commands to navigate surrounding tiles.", padX, curY, innerW, Theme::colors.textPrimary, uiScale);
    curY += textH + (6.0f * uiScale);

    return (curY - startY);
}

void uiRenderer::renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale)
{
    UIWidget::drawPanel(renderer, rect);

    float headerH = 24.0f * uiScale;
    SDL_FRect headerRect = { rect.x, rect.y, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "ACTION COMMANDS", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);

    const auto& buttons = gameContext->getActiveActionButtons();
    int totalButtons = static_cast<int>(buttons.size());

    int totalPages = (totalButtons > 0) ? ((totalButtons - 1) / BUTTONS_PER_PAGE) + 1 : 1;
    m_currentPage = std::clamp(m_currentPage, 0, totalPages - 1);

    int startIndex = m_currentPage * BUTTONS_PER_PAGE;
    int endIndex = std::min(startIndex + BUTTONS_PER_PAGE, totalButtons);

    float padX = rect.x + (10.0f * uiScale);
    float startY = rect.y + headerH + (6.0f * uiScale);
    float availableW = rect.w - (20.0f * uiScale);
    float spaceX = 10.0f * uiScale;
    float spaceY = 8.0f * uiScale;
    int cols = 5;
    int rows = 2;
    float btnWidth = (availableW - (spaceX * (cols - 1))) / cols;
    float btnHeight = (rect.h - headerH - (12.0f * uiScale) - (spaceY * (rows - 1))) / rows;

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

        UIWidget::drawButton(renderer, btnRect, buttons[i].label, hovered, buttons[i].isEnabled, false, uiScale);

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
        float pBtnW = 60.0f * uiScale;
        float pBtnH = 20.0f * uiScale;
        SDL_FRect prevRect = { rect.x + rect.w - (pBtnW * 2.0f) - (10.0f * uiScale), rect.y + (2.0f * uiScale), pBtnW, pBtnH };
        SDL_FRect nextRect = { rect.x + rect.w - pBtnW - (5.0f * uiScale), rect.y + (2.0f * uiScale), pBtnW, pBtnH };

        bool prevHovered = (mousePos.x >= prevRect.x && mousePos.x <= prevRect.x + prevRect.w && mousePos.y >= prevRect.y && mousePos.y <= prevRect.y + prevRect.h);
        bool nextHovered = (mousePos.x >= nextRect.x && mousePos.x <= nextRect.x + nextRect.w && mousePos.y >= nextRect.y && mousePos.y <= nextRect.y + nextRect.h);

        UIWidget::drawButton(renderer, prevRect, "< Prev", prevHovered, m_currentPage > 0, false, uiScale * 0.85f);
        UIWidget::drawButton(renderer, nextRect, "Next >", nextHovered, m_currentPage < totalPages - 1, false, uiScale * 0.85f);

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

float uiRenderer::renderWidgetCharOverview(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    entity* p = gameContext->getPlayer();
    if (!p) return 0.0f;

    float lineH = 18.0f * uiScale;
    float startY = curY;
    float padX = curX + (10.0f * uiScale);

    UIWidget::drawText(renderer, std::format("Name: {}", p->name), padX, curY, Theme::colors.textPrimary, uiScale); curY += lineH;
    UIWidget::drawText(renderer, std::format("Title: {}", p->anatomy.getRacialTitle()), padX, curY, Theme::colors.textAccent, uiScale); curY += lineH;
    UIWidget::drawText(renderer, std::format("Gender: {}", genderArchetypeToString(p->anatomy.getGenderArchetype())), padX, curY, Theme::colors.textSecondary, uiScale); curY += (lineH + 4.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderWidgetVitals(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    entity* p = gameContext->getPlayer();
    if (!p) return 0.0f;

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float barW = innerW - (20.0f * uiScale);
    float barH = 18.0f * uiScale;

    float hp = p->getStat("health");
    UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("HP: {:.0f}/100", hp), uiScale); curY += (barH + 6.0f * uiScale);

    float mana = p->getStat("mana");
    UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, mana, 50.0f, Theme::colors.mana, Theme::colors.bgDark, std::format("Mana: {:.0f}/50", mana), uiScale); curY += (barH + 6.0f * uiScale);

    float lust = p->getStat("lust");
    UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, lust, 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Lust: {:.0f}/100", lust), uiScale); curY += (barH + 8.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderWidgetAttributes(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    entity* p = gameContext->getPlayer();
    if (!p) return 0.0f;

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float lineH = 18.0f * uiScale;

    UIWidget::drawText(renderer, "ATTRIBUTES", padX, curY, Theme::colors.textGold, uiScale); curY += (lineH + 2.0f * uiScale);
    UIWidget::drawText(renderer, std::format("Physique:   {:.0f}", p->getStat("physique")), padX, curY, Theme::colors.physique, uiScale); curY += lineH;
    UIWidget::drawText(renderer, std::format("Agility:    {:.0f}", p->getStat("agility")), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
    UIWidget::drawText(renderer, std::format("Arcane:     {:.0f}", p->getStat("arcane")), padX, curY, Theme::colors.arcane, uiScale); curY += lineH;
    UIWidget::drawText(renderer, std::format("Corruption: {:.0f}", p->getStat("corruption")), padX, curY, Theme::colors.corruption, uiScale); curY += (lineH + 8.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderWidgetAnatomy(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    entity* p = gameContext->getPlayer();
    if (!p) return 0.0f;

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float lineH = 18.0f * uiScale;

    UIWidget::drawText(renderer, "ANATOMY & FLUIDS", padX, curY, Theme::colors.textGold, uiScale); curY += (lineH + 2.0f * uiScale);
    UIWidget::drawText(renderer, std::format("Height: {:.2f}m", p->anatomy.heightMeters), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;

    if (const bodyPart* b = p->anatomy.getPart(bodySlot::BREASTS))
    {
        UIWidget::drawText(renderer, std::format("Breasts: {}-Cup ({:.0f}/{:.0f}ml milk)", bodyPart::getCupSizeName(b->cupSize), b->currentFluidMl, b->maxFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
    }
    if (const bodyPart* g = p->anatomy.getPart(bodySlot::GROIN))
    {
        if (p->anatomy.hasPenis())
        {
            UIWidget::drawText(renderer, std::format("Penis: {:.1f}cm x {:.1f}cm ({:.0f}/{:.0f}ml cum)", g->length, g->diameter, g->currentFluidMl, g->maxFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
        }
    }
    curY += (4.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    const gameMap* m = gameContext->getActiveMap();
    if (!m) return 0.0f;

    float startY = curY;
    const int radius = 3;
    const int gridSize = (radius * 2) + 1; // 7
    // Calculate static grid dimensions independent of current scroll offset
    const float availableW = std::max(20.0f, rect.w - (20.0f * uiScale));
    const float availableH = std::max(20.0f, rect.h - (20.0f * uiScale));
    const float maxDimension = std::min(availableW, availableH);
    const float tileSize = std::max(4.0f, std::min(30.0f * uiScale, maxDimension / static_cast<float>(gridSize)));
    const float totalGridW = tileSize * static_cast<float>(gridSize);

    const float padX = rect.x + ((rect.w - totalGridW) / 2.0f);

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            int mapX = gameContext->gridX + dx;
            int mapY = gameContext->gridY + dy;

            SDL_FRect tileRect = {
                padX + static_cast<float>(dx + radius) * tileSize,
                curY + static_cast<float>(dy + radius) * tileSize,
                std::max(1.0f, tileSize - (1.5f * uiScale)),
                std::max(1.0f, tileSize - (1.5f * uiScale))
            };

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

            if (!label.empty() && tileSize >= 14.0f * uiScale)
            {
                UIWidget::drawText(renderer, label, tileRect.x + (tileSize * 0.25f), tileRect.y + (tileSize * 0.1f), Theme::colors.textGold, uiScale * (tileSize / 24.0f));
            }
        }
    }

    curY += (totalGridW + 8.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetTarget(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float lineH = 16.0f * uiScale;

    UIWidget::drawText(renderer, "PROXIMITY TARGET", padX, curY, Theme::colors.textGold, uiScale); curY += (18.0f * uiScale);

    if (entity* npc = gameContext->getActiveTargetNPC())
    {
        UIWidget::drawText(renderer, std::format("Name: {}", npc->name), padX, curY, Theme::colors.textPrimary, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Level: {} | {}", npc->stats.level, npc->anatomy.getDominantRace()), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;

        float hp = npc->getStat("health");
        UIWidget::drawProgressBar(renderer, { padX, curY, innerW - (20.0f * uiScale), 16.0f * uiScale }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("HP: {:.0f}", hp), uiScale);
        curY += (20.0f * uiScale);
    }
    else
    {
        UIWidget::drawText(renderer, "No active target.", padX, curY, Theme::colors.textMuted, uiScale); curY += lineH;
    }

    return (curY - startY);
}

float uiRenderer::renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    float startY = curY;
    float padX = curX + (10.0f * uiScale);

    UIWidget::drawText(renderer, "EQUIPMENT PAPERDOLL", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    static const std::vector<std::pair<std::string, equipSlot>> slots = {
        { "HEAD", equipSlot::HEADWEAR }, { "CHEST", equipSlot::TORSO_OVER }, { "HANDS", equipSlot::HANDS },
        { "MAIN", equipSlot::WEAPON_MAIN }, { "OFF", equipSlot::WEAPON_OFF }, { "LEGS", equipSlot::LEGS_OUTER },
        { "FEET", equipSlot::FEET }, { "NECK", equipSlot::NECKWEAR }, { "RING", equipSlot::FINGER_PRIMARY }
    };

    float slotW = (innerW - (28.0f * uiScale)) / 3.0f;
    float slotH = 34.0f * uiScale;

    for (size_t i = 0; i < slots.size(); ++i)
    {
        int col = i % 3;
        int row = i / 3;
        float slotX = padX + (col * (slotW + 4.0f * uiScale));
        float slotY = curY + (row * (slotH + 4.0f * uiScale));

        SDL_FRect sRect = { slotX, slotY, slotW, slotH };
        bool isSelected = (gameContext->selectedEquipmentSlot == slots[i].second);
        UIWidget::drawPanel(renderer, sRect, isSelected ? Theme::colors.bgHeader : Theme::colors.bgSlot, isSelected ? Theme::colors.borderButton : Theme::colors.borderNormal);

        UIWidget::drawText(renderer, slots[i].first, slotX + 4.0f * uiScale, slotY + 3.0f * uiScale, Theme::colors.textSecondary, uiScale * 0.8f);

        std::string equippedName = "---";
        if (entity* player = gameContext->getPlayer())
        {
            if (auto eq = player->inventory.getEquippedItem(slots[i].second))
            {
                equippedName = eq->name;
            }
        }
        UIWidget::drawText(renderer, equippedName, slotX + 4.0f * uiScale, slotY + 16.0f * uiScale, Theme::colors.textPrimary, uiScale * 0.85f);
    }

    curY += (3 * (slotH + 4.0f * uiScale)) + (6.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float availableW = innerW - (20.0f * uiScale);

    UIWidget::drawText(renderer, "ITEM DETAILS & LORE", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    if (gameContext->selectedInventoryIndex >= 0)
    {
        auto items = (gameContext->selectedInventorySide == 0)
            ? gameContext->getPlayerInventoryStacked()
            : gameContext->getTileInventoryStacked();

        if (gameContext->selectedInventoryIndex < static_cast<int>(items.size()))
        {
            const auto& slot = items[gameContext->selectedInventoryIndex];
            if (slot.itemPtr)
            {
                UIWidget::drawText(renderer, std::format("Name: {} (x{})", slot.itemPtr->name, slot.totalCount), padX, curY, Theme::colors.textGold, uiScale);
                curY += (16.0f * uiScale);
                UIWidget::drawText(renderer, std::format("Type: Item | Value: {}¤", slot.itemPtr->baseValue), padX, curY, Theme::colors.textSecondary, uiScale);
                curY += (16.0f * uiScale);

                float descH = UIWidget::drawTextWrapped(renderer, slot.itemPtr->description, padX, curY, availableW, Theme::colors.textPrimary, uiScale);
                curY += descH + (6.0f * uiScale);
                return (curY - startY);
            }
        }
    }

    UIWidget::drawText(renderer, "No item selected. Select an item from inventory to inspect details.", padX, curY, Theme::colors.textMuted, uiScale);
    curY += (16.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderMainMenu(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "CHRONICLES OF LILITH • MAIN MENU", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (12.0f * uiScale);

    UIWidget::drawText(renderer, "Welcome to textRPG", padX, curY, Theme::colors.textGold, uiScale * 1.3f);
    curY += (24.0f * uiScale);

    float descH = UIWidget::drawTextWrapped(renderer, "A modular, rich text-based RPG featuring dynamic corruption, deep anatomy mutations, and CYOA tactical combat. Choose an option from the Action Commands below to begin.", padX, curY, availableW, Theme::colors.textPrimary, uiScale);
    curY += descH + (16.0f * uiScale);

    UIWidget::drawText(renderer, "AVAILABLE SAVE PROFILES:", padX, curY, Theme::colors.textAccent, uiScale);
    curY += (18.0f * uiScale);

    for (int i = 1; i <= 3; ++i)
    {
        SDL_FRect slotRect = { padX, curY, availableW, 28.0f * uiScale };
        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, std::format("Profile Slot {}: [Save Data]", i), padX + 8.0f * uiScale, curY + 6.0f * uiScale, Theme::colors.textSecondary, uiScale);
        curY += (32.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderOptionsView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "GAME SETTINGS & CONFIGURATION", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (12.0f * uiScale);

    UIWidget::drawText(renderer, "CONTENT & MECHANICS:", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    UIWidget::drawText(renderer, std::format("Pregnancy Simulation: {}", gameContext->settings.content.pregnancyEnabled ? "ENABLED" : "DISABLED"), padX, curY, Theme::colors.textPrimary, uiScale);
    curY += (16.0f * uiScale);
    UIWidget::drawText(renderer, std::format("Lactation System: {}", gameContext->settings.content.lactationEnabled ? "ENABLED" : "DISABLED"), padX, curY, Theme::colors.textPrimary, uiScale);
    curY += (16.0f * uiScale);

    curY += (10.0f * uiScale);
    UIWidget::drawText(renderer, "DISPLAY & THEME:", padX, curY, Theme::colors.textAccent, uiScale);
    curY += (18.0f * uiScale);
    UIWidget::drawText(renderer, std::format("Active Layout: {}", gameContext->settings.display.activeLayout.empty() ? "Default" : gameContext->settings.display.activeLayout), padX, curY, Theme::colors.textSecondary, uiScale);
    curY += (16.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderShopView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "ENCHANTED APOTHECARY & SHOP", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (12.0f * uiScale);

    float greetH = UIWidget::drawTextWrapped(renderer, "\"Welcome, traveler. Browse my finest potions, mystical essences, and enchanted wares. Select an item from the command grid to purchase.\"", padX, curY, availableW, Theme::colors.textAccent, uiScale);
    curY += greetH + (14.0f * uiScale);

    if (auto shop = dynamic_cast<shopState*>(gameContext->getActiveState()))
    {
        const auto& catalog = shop->getCatalog();
        for (size_t i = 0; i < catalog.size(); ++i)
        {
            SDL_FRect itemRect = { padX, curY, availableW, 30.0f * uiScale };
            UIWidget::drawPanel(renderer, itemRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, std::format("[{}] {} ({})", i, catalog[i].name, catalog[i].type), padX + 8.0f * uiScale, curY + 6.0f * uiScale, Theme::colors.textPrimary, uiScale);
            UIWidget::drawText(renderer, std::format("Price: {}¤ | Stock: {}", catalog[i].price, catalog[i].stock), padX + availableW - 130.0f * uiScale, curY + 6.0f * uiScale, Theme::colors.textGold, uiScale);
            curY += (34.0f * uiScale);
        }
    }

    return (curY - startY);
}

float uiRenderer::renderTransformationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "BODY TRANSFORMATIONS & ANATOMY", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (12.0f * uiScale);

    if (entity* player = gameContext->getPlayer())
    {
        UIWidget::drawText(renderer, std::format("Current Dominant Archetype: {}", player->anatomy.getDominantRace()), padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);

        std::string anatDesc = characterDescription::generateFullDescription(player);
        float descH = UIWidget::drawTextWrapped(renderer, anatDesc, padX, curY, availableW, Theme::colors.textPrimary, uiScale);
        curY += descH + (14.0f * uiScale);
    }

    return (curY - startY);
}