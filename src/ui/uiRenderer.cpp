#include "ui/uiRenderer.h"

#include <filesystem>
#include <format>
#include <iostream>

#include "core/characterDescription.h"
#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "save/saveManager.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/loadGameState.h"
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

    std::string stateKey = "";
    iGameState* curState = gameContext->getActiveState();
    if (dynamic_cast<mainMenuState*>(curState)) stateKey = "MAIN_MENU";
    else if (dynamic_cast<loadGameState*>(curState)) stateKey = "LOAD_GAME";
    else if (dynamic_cast<optionsState*>(curState)) stateKey = "SETTINGS";
    else if (dynamic_cast<CombatState*>(curState)) stateKey = "COMBAT";
    else if (dynamic_cast<inventoryState*>(curState)) stateKey = "INVENTORY";
    else if (dynamic_cast<sexState*>(curState)) stateKey = "SEX";
    else if (dynamic_cast<shopState*>(curState)) stateKey = "SHOP";
    else if (dynamic_cast<transformationState*>(curState)) stateKey = "TRANSFORMATION";

    auto panels = m_layoutEngine.computeLayout(static_cast<float>(winW), static_cast<float>(winH), uiScale, stateKey);

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
            float curY = p.rect.y + (6.0f * uiScale) - scrollY;
            bool renderedCenterViewInPanel = false;

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
                else if (wId == "widget_narrative_story" || wId == "SCENE_NARRATIVE" ||
                         wId == "widget_main_menu_hero" || wId == "widget_main_menu_actions" || wId == "widget_save_slot_list" ||
                         wId == "widget_options_content" || wId == "widget_options_demographics" || wId == "widget_options_display_audio" ||
                         wId == "widget_load_game")
                {
                    if (!renderedCenterViewInPanel)
                    {
                        renderedCenterViewInPanel = true;
                        curY += renderCenterPane(renderer, gameContext, p.rect, curY, uiScale);
                    }
                }
                else if (wId == "widget_inventory_dual" || wId == "BACKPACK_INVENTORY")
                {
                    curY += renderInventoryView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_tactical_combat" || wId == "COMBAT_VIEW")
                {
                    curY += renderCombatView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_merchant_dialog" || wId == "widget_merchant_catalog" || wId == "widget_player_sell_grid" || wId == "widget_transaction_cart")
                {
                    curY += renderShopView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_lt_character_card")
                {
                    curY += renderWidgetCharacterCard(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_time_bar" || wId == "TIME_CALENDAR_BAR")
                {
                    curY += renderWidgetTimeBar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_radar_map" || wId == "RADAR_MAP_5X5")
                {
                    curY += renderWidgetRadar(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_lt_options_toolbar" || wId == "OPTIONS_TOOLBAR_5")
                {
                    curY += renderWidgetOptionsToolbar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_dpad_radar")
                {
                    curY += renderWidgetTimeBar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                    curY += renderWidgetRadar(renderer, gameContext, p.rect, curY, uiScale);
                    curY += renderWidgetOptionsToolbar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_characters_present")
                {
                    curY += renderWidgetCharactersPresent(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_items_present")
                {
                    curY += renderWidgetItemsPresent(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_event_log")
                {
                    curY += renderWidgetEventLog(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                }
                else if (wId == "widget_lt_enchanting_screen" || wId == "widget_enchanting_altar")
                {
                    curY += renderEnchantingView(renderer, gameContext, p.rect, curY, uiScale);
                }
                else if (wId == "widget_body_mutations_tree" || wId == "widget_active_enchantments_list")
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
        UIWidget::drawText(renderer, goldStr, rect.x + (rect.w * 0.45f), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textGold, uiScale);
    }

    // Right: Dedicated Menu / Options Button
    float menuBtnW = 90.0f * uiScale;
    float menuBtnH = std::max(20.0f, rect.h - (10.0f * uiScale));
    float menuBtnX = rect.x + rect.w - menuBtnW - (8.0f * uiScale);
    float menuBtnY = rect.y + ((rect.h - menuBtnH) / 2.0f);

    SDL_FRect menuBtnRect = { menuBtnX, menuBtnY, menuBtnW, menuBtnH };
    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    bool menuHovered = (mousePos.x >= menuBtnRect.x && mousePos.x <= menuBtnRect.x + menuBtnRect.w &&
                        mousePos.y >= menuBtnRect.y && mousePos.y <= menuBtnRect.y + menuBtnRect.h);

    bool inMenu = dynamic_cast<optionsState*>(gameContext->getActiveState()) || dynamic_cast<mainMenuState*>(gameContext->getActiveState());
    UIWidget::drawButton(renderer, menuBtnRect, inMenu ? "Close (ESC)" : "⚙ MENU", menuHovered, true, inMenu, uiScale * 0.85f);

    if (menuHovered && clicked)
    {
        if (inMenu)
        {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        }
        else
        {
            gameContext->handleCommand({ CommandType::OPEN_SETTINGS, 0, 0, "" });
        }
        gameContext->input.consumeMouseClick();
    }

    // Right: Map Location Badge
    if (const gameMap* m = gameContext->getActiveMap())
    {
        std::string locStr = std::format("{}", m->getName());
        UIWidget::drawText(renderer, locStr, menuBtnX - (160.0f * uiScale), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textAccent, uiScale);
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
    else if (dynamic_cast<loadGameState*>(state))
    {
        return renderLoadGameView(renderer, gameContext, rect, curY, uiScale);
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
    curY += textH + (16.0f * uiScale);

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

    std::string desc = "You are exploring the district. Use keyboard movement controls (W, A, S, D or Arrow Keys) to navigate surrounding tiles.";

    if (gameContext->map)
    {
        auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
        if (tileData.persistentNPC)
        {
            desc += std::format("\n\n{} is standing here waiting to interact.", tileData.persistentNPC->name);
        }
        if (!tileData.droppedItems.empty())
        {
            desc += std::format("\n\nThere are {} dropped items scattered across the cobblestones.", tileData.droppedItems.size());
        }
    }

    float textH = UIWidget::drawTextWrapped(renderer, desc, padX, curY, innerW, Theme::colors.textPrimary, uiScale);
    curY += textH + (14.0f * uiScale);

    return (curY - startY);
}

void uiRenderer::renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale)
{
    UIWidget::drawPanel(renderer, rect);

    const auto& buttons = gameContext->getActiveActionButtons();
    int totalButtons = static_cast<int>(buttons.size());

    int totalPages = (totalButtons > 0) ? ((totalButtons - 1) / BUTTONS_PER_PAGE) + 1 : 1;
    m_currentPage = std::clamp(m_currentPage, 0, totalPages - 1);

    int startIndex = m_currentPage * BUTTONS_PER_PAGE;
    int endIndex = std::min(startIndex + BUTTONS_PER_PAGE, totalButtons);

    float sideBtnW = 20.0f * uiScale;
    float marginX = 8.0f * uiScale;
    float padY = rect.y + (6.0f * uiScale);
    float gridH = rect.h - (12.0f * uiScale);

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    // 1. Left Side Pagination (< and Q)
    SDL_FRect leftTopRect = { rect.x + marginX, padY, sideBtnW, (gridH / 3.0f) - (2.0f * uiScale) };
    SDL_FRect leftMidRect = { rect.x + marginX, padY + (gridH / 3.0f), sideBtnW, (gridH * 2.0f / 3.0f) };
    bool leftHovered = (mousePos.x >= leftMidRect.x && mousePos.x <= leftMidRect.x + leftMidRect.w &&
                        mousePos.y >= leftMidRect.y && mousePos.y <= leftMidRect.y + leftMidRect.h);

    UIWidget::drawLTActionButton(renderer, leftTopRect, "Q", "", false, false, false, uiScale * 0.75f);
    UIWidget::drawLTActionButton(renderer, leftMidRect, "<", "", leftHovered, m_currentPage > 0, false, uiScale * 0.9f);
    if (leftHovered && clicked && m_currentPage > 0)
    {
        m_currentPage--;
        gameContext->input.consumeMouseClick();
    }

    // 2. Right Side Pagination (> and E)
    float rightX = rect.x + rect.w - marginX - sideBtnW;
    SDL_FRect rightTopRect = { rightX, padY, sideBtnW, (gridH / 3.0f) - (2.0f * uiScale) };
    SDL_FRect rightMidRect = { rightX, padY + (gridH / 3.0f), sideBtnW, (gridH * 2.0f / 3.0f) };
    bool rightHovered = (mousePos.x >= rightMidRect.x && mousePos.x <= rightMidRect.x + rightMidRect.w &&
                         mousePos.y >= rightMidRect.y && mousePos.y <= rightMidRect.y + rightMidRect.h);

    UIWidget::drawLTActionButton(renderer, rightTopRect, "E", "", false, false, false, uiScale * 0.75f);
    UIWidget::drawLTActionButton(renderer, rightMidRect, ">", "", rightHovered, m_currentPage < totalPages - 1, false, uiScale * 0.9f);
    if (rightHovered && clicked && m_currentPage < totalPages - 1)
    {
        m_currentPage++;
        gameContext->input.consumeMouseClick();
    }

    // 3. Middle 3x5 Grid
    float gridX = rect.x + marginX + sideBtnW + (6.0f * uiScale);
    float availableW = (rightX - (6.0f * uiScale)) - gridX;
    float spaceX = 6.0f * uiScale;
    float spaceY = 5.0f * uiScale;
    int cols = 5;
    int rows = 3;
    float btnWidth = (availableW - (spaceX * (cols - 1))) / cols;
    float btnHeight = (gridH - (spaceY * (rows - 1))) / rows;

    static const char* hotkeys[15] = {
        "1", "2", "3", "4", "5",
        "SHIFT + 1", "SHIFT + 2", "SHIFT + 3", "SHIFT + 4", "SHIFT + 5",
        "CTRL + 1", "CTRL + 2", "CTRL + 3", "CTRL + 4", "CTRL + 5"
    };

    for (int slotIdx = 0; slotIdx < BUTTONS_PER_PAGE; ++slotIdx)
    {
        int col = slotIdx % cols;
        int row = slotIdx / cols;
        SDL_FRect btnRect = { gridX + col * (btnWidth + spaceX), padY + row * (btnHeight + spaceY), btnWidth, btnHeight };

        int buttonIdx = startIndex + slotIdx;
        if (buttonIdx < endIndex && !buttons[buttonIdx].label.empty())
        {
            bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                            mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

            UIWidget::drawLTActionButton(renderer, btnRect, buttons[buttonIdx].label, hotkeys[slotIdx], hovered, buttons[buttonIdx].isEnabled, false, uiScale);

            if (hovered && clicked && buttons[buttonIdx].isEnabled && buttons[buttonIdx].onClick)
            {
                buttons[buttonIdx].onClick();
                gameContext->input.consumeMouseClick();
            }
        }
        else
        {
            // Empty slot with hotkey tag
            UIWidget::drawLTActionButton(renderer, btnRect, "", hotkeys[slotIdx], false, false, false, uiScale);
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    const gameMap* m = gameContext->getActiveMap();
    if (!m) return 0.0f;

    float startY = curY;
    const int radius = 2; // 5x5 grid
    const int gridSize = (radius * 2) + 1; // 5
    const float availableW = std::max(20.0f, rect.w - (16.0f * uiScale));
    const float availableH = std::max(20.0f, rect.h - (10.0f * uiScale));
    const float maxDimension = std::min(availableW, availableH);
    const float tileSize = std::max(6.0f, std::min(22.0f * uiScale, maxDimension / static_cast<float>(gridSize)));
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
            if (t.type == TILE_VOID)
            {
                continue; // Do not draw void/off-map cells as tiles
            }

            int manhattanDist = std::abs(dx) + std::abs(dy);
            bool isAdjacent = (manhattanDist == 1); // Strictly orthogonal (up, down, left, right)
            bool isWalkedOn = t.visited || (t.discovery == STATE_REVEALED);

            SDL_Color tileColor;
            SDL_Color borderColor = SDL_Color{ 22, 22, 30, 255 };
            std::string label = "";

            if (dx == 0 && dy == 0)
            {
                // Player Center Tile
                tileColor = Theme::colors.borderButton;
                borderColor = Theme::colors.textGold;
                label = "@";
            }
            else if (isWalkedOn)
            {
                // Tier 1: Walked on / Visited (brightest)
                if (t.type == TILE_WALL) { tileColor = Theme::colors.bgHeader; }
                else if (t.type == TILE_DOOR) { tileColor = Theme::colors.textGold; label = "D"; }
                else { tileColor = Theme::colors.bgSlot; }
                borderColor = Theme::colors.borderNormal;
            }
            else if (isAdjacent)
            {
                // Tier 2: Strictly orthogonal adjacent (up, down, left, right), unwalked
                if (t.type == TILE_WALL) { tileColor = SDL_Color{ 24, 24, 34, 255 }; }
                else if (t.type == TILE_DOOR) { tileColor = SDL_Color{ 130, 95, 20, 255 }; label = "d"; }
                else { tileColor = SDL_Color{ 18, 18, 26, 255 }; }
                borderColor = SDL_Color{ 36, 36, 48, 255 };
            }
            else
            {
                // Tier 3: Diagonal or >1 tile away, unwalked (darkest, but visible tile)
                if (t.type == TILE_WALL) { tileColor = SDL_Color{ 14, 14, 20, 255 }; }
                else if (t.type == TILE_DOOR) { tileColor = SDL_Color{ 35, 28, 10, 255 }; }
                else { tileColor = SDL_Color{ 10, 10, 14, 255 }; }
                borderColor = SDL_Color{ 20, 20, 28, 255 };
            }

            // Draw tile border and fill
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            SDL_RenderRect(renderer, &tileRect);

            SDL_SetRenderDrawColor(renderer, tileColor.r, tileColor.g, tileColor.b, tileColor.a);
            SDL_RenderFillRect(renderer, &tileRect);

            if (!label.empty() && tileSize >= 12.0f * uiScale)
            {
                UIWidget::drawText(renderer, label, tileRect.x + (tileSize * 0.25f), tileRect.y + (tileSize * 0.1f), Theme::colors.textGold, uiScale * (tileSize / 22.0f));
            }
        }
    }

    curY += (totalGridW + 4.0f * uiScale);
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
    float centerX = rect.x + (rect.w / 2.0f);
    float textW = std::min(rect.w - (60.0f * uiScale), 740.0f * uiScale);
    float textX = centerX - (textW / 2.0f);

    curY += (28.0f * uiScale);

    // Title: Lilith's Throne in glowing purple/pink
    std::string mainTitle = "Lilith's Throne";
    float titleTextW = mainTitle.size() * (13.0f * uiScale);
    UIWidget::drawText(renderer, mainTitle, centerX - (titleTextW / 2.0f), curY, SDL_Color{ 235, 145, 255, 255 }, uiScale * 1.8f);
    curY += (38.0f * uiScale);

    // Subtitle: Created by Innoxia
    std::string subTitle = "Created by Innoxia";
    float subTextW = subTitle.size() * (8.5f * uiScale);
    UIWidget::drawText(renderer, subTitle, centerX - (subTextW / 2.0f), curY, SDL_Color{ 200, 140, 235, 255 }, uiScale * 1.15f);
    curY += (34.0f * uiScale);

    // Paragraph 1: Disclaimer
    float p1H = UIWidget::drawTextWrapped(renderer,
        "This game is a text-based erotic RPG, and contains a lot of graphic sexual content. You must agree to the game's disclaimer before playing this game!",
        textX, curY, textW, Theme::colors.textPrimary, uiScale * 0.92f);
    curY += p1H + (20.0f * uiScale);

    // Paragraph 2: Config note
    float p2H = UIWidget::drawTextWrapped(renderer,
        "Use the Options and Content Options commands in the action grid below to customize gameplay mechanics, difficulty, content toggles, themes, and demographics.",
        textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.9f);
    curY += p2H + (20.0f * uiScale);

    // Paragraph 3: Old saves note
    float p3H = UIWidget::drawTextWrapped(renderer,
        "Copy over the contents of your 'data' folder to use your old saves in this version!",
        textX, curY, textW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += p3H + (24.0f * uiScale);

    // Engine version
    std::string verText = "Your engine version: 0.4.11.3 Alpha (C++ Edition)";
    float verW = verText.size() * (6.5f * uiScale);
    UIWidget::drawText(renderer, verText, centerX - (verW / 2.0f), curY, Theme::colors.textMuted, uiScale * 0.85f);
    curY += (24.0f * uiScale);

    return (curY - startY);
}

float uiRenderer::renderLoadGameView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    loadGameState* loadState = dynamic_cast<loadGameState*>(gameContext->getActiveState());
    float startY = curY;
    float centerX = rect.x + (rect.w / 2.0f);
    float padX = rect.x + (24.0f * uiScale);
    float availableW = rect.w - (48.0f * uiScale);

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    bool isSaveMode = (loadState && loadState->getMode() == SaveMenuMode::SAVE_AND_LOAD);
    entity* player = gameContext->getPlayer();
    std::string activeCharName = (player && !player->name.empty()) ? player->name : "Hero";

    // 1. Centered Header Card
    float cardW = std::min(availableW, 400.0f * uiScale);
    float cardH = 34.0f * uiScale;
    UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Save game files", Theme::colors.textPrimary, uiScale);
    curY += cardH + (18.0f * uiScale);

    // 2. Centered "Please Note:"
    std::string noteTitle = "Please Note:";
    float noteTitleW = noteTitle.size() * (7.5f * uiScale);
    UIWidget::drawText(renderer, noteTitle, centerX - (noteTitleW / 2.0f), curY, Theme::colors.textPrimary, uiScale * 0.95f);
    curY += (22.0f * uiScale);

    float textW = std::min(availableW, 680.0f * uiScale);
    float textX = centerX - (textW / 2.0f);

    static const char* notes[] = {
        "1. Only standard characters (letters and numbers) will work for save file names.",
        "2. The 'AutoSave' file is automatically overwritten every time you move between maps.",
        "3. The 'QuickSave' file is automatically overwritten every time you quick save (binding is F5).",
        "4. You cannot save during scenes which restrict your movement, including combat and sex."
    };

    for (int i = 0; i < 4; ++i)
    {
        UIWidget::drawText(renderer, notes[i], textX, curY, Theme::colors.textSecondary, uiScale * 0.86f);
        curY += (18.0f * uiScale);
    }
    curY += (14.0f * uiScale);

    // Confirmation Modals (Overwrite / Delete)
    if (loadState && !loadState->pendingOverwriteSaveName.empty())
    {
        SDL_FRect modalRect = { padX, curY, availableW, 60.0f * uiScale };
        UIWidget::drawPanel(renderer, modalRect, Theme::colors.bgHeader, Theme::colors.enemy);

        std::string warnMsg = std::format("! Overwrite Save: '{}' already exists for {}. Overwrite this file?", loadState->pendingOverwriteSaveName, activeCharName);
        UIWidget::drawText(renderer, warnMsg, padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        float btnW = 120.0f * uiScale;
        float btnH = 24.0f * uiScale;
        SDL_FRect yesBtnRect = { padX + availableW - (btnW * 2.0f) - (20.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };
        SDL_FRect cancelBtnRect = { padX + availableW - btnW - (10.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };

        bool yesHovered = (mousePos.x >= yesBtnRect.x && mousePos.x <= yesBtnRect.x + yesBtnRect.w &&
                           mousePos.y >= yesBtnRect.y && mousePos.y <= yesBtnRect.y + yesBtnRect.h);
        bool cancelHovered = (mousePos.x >= cancelBtnRect.x && mousePos.x <= cancelBtnRect.x + cancelBtnRect.w &&
                              mousePos.y >= cancelBtnRect.y && mousePos.y <= cancelBtnRect.y + cancelBtnRect.h);

        UIWidget::drawButton(renderer, yesBtnRect, "YES, OVERWRITE", yesHovered, true, false, uiScale * 0.75f);
        UIWidget::drawButton(renderer, cancelBtnRect, "CANCEL", cancelHovered, true, false, uiScale * 0.75f);

        if (yesHovered && clicked)
        {
            saveManager::saveNamedGame(gameContext, loadState->pendingOverwriteSaveName);
            loadState->pendingOverwriteSaveName = "";
            gameContext->input.consumeMouseClick();
        }
        else if (cancelHovered && clicked)
        {
            loadState->pendingOverwriteSaveName = "";
            gameContext->input.consumeMouseClick();
        }

        curY += modalRect.h + (14.0f * uiScale);
    }

    if (loadState && !loadState->pendingDeleteFileName.empty())
    {
        SDL_FRect modalRect = { padX, curY, availableW, 60.0f * uiScale };
        UIWidget::drawPanel(renderer, modalRect, Theme::colors.bgHeader, Theme::colors.enemy);

        std::string warnMsg = std::format("! Delete Save: Are you sure you want to permanently delete '{}'?", loadState->pendingDeleteFileName);
        UIWidget::drawText(renderer, warnMsg, padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.enemy, uiScale * 0.85f);

        float btnW = 120.0f * uiScale;
        float btnH = 24.0f * uiScale;
        SDL_FRect yesBtnRect = { padX + availableW - (btnW * 2.0f) - (20.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };
        SDL_FRect cancelBtnRect = { padX + availableW - btnW - (10.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };

        bool yesHovered = (mousePos.x >= yesBtnRect.x && mousePos.x <= yesBtnRect.x + yesBtnRect.w &&
                           mousePos.y >= yesBtnRect.y && mousePos.y <= yesBtnRect.y + yesBtnRect.h);
        bool cancelHovered = (mousePos.x >= cancelBtnRect.x && mousePos.x <= cancelBtnRect.x + cancelBtnRect.w &&
                              mousePos.y >= cancelBtnRect.y && mousePos.y <= cancelBtnRect.y + cancelBtnRect.h);

        UIWidget::drawButton(renderer, yesBtnRect, "YES, DELETE", yesHovered, true, false, uiScale * 0.75f);
        UIWidget::drawButton(renderer, cancelBtnRect, "CANCEL", cancelHovered, true, false, uiScale * 0.75f);

        if (yesHovered && clicked)
        {
            saveManager::deleteSave(loadState->pendingDeleteFileName);
            loadState->pendingDeleteFileName = "";
            gameContext->input.consumeMouseClick();
        }
        else if (cancelHovered && clicked)
        {
            loadState->pendingDeleteFileName = "";
            gameContext->input.consumeMouseClick();
        }

        curY += modalRect.h + (14.0f * uiScale);
    }

    // 3. Table Column Headers: Time | Name | Save | Load | Delete
    float tableX = padX;
    float timeColW = 160.0f * uiScale;
    float actionsColX = padX + availableW - (190.0f * uiScale);

    UIWidget::drawText(renderer, "Time", tableX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.9f);
    UIWidget::drawText(renderer, "Name", tableX + timeColW, curY, Theme::colors.textSecondary, uiScale * 0.9f);
    UIWidget::drawText(renderer, "Save | Load | Delete", actionsColX, curY, Theme::colors.textSecondary, uiScale * 0.9f);
    curY += (24.0f * uiScale);

    // 4. Character Groups & Saves
    auto characterGroups = saveManager::getSavesGroupedByCharacter();
    if (characterGroups.empty())
    {
        UIWidget::drawText(renderer, "No save game files found.", textX, curY, Theme::colors.textMuted, uiScale * 0.88f);
        curY += (24.0f * uiScale);
    }
    else
    {
        for (const auto& group : characterGroups)
        {
            bool isCollapsed = loadState ? loadState->isCharacterCollapsed(group.characterName) : false;

            // Character Header Accordion
            SDL_FRect groupHeader = { padX, curY, availableW, 26.0f * uiScale };
            bool groupHeaderHovered = (mousePos.x >= groupHeader.x && mousePos.x <= groupHeader.x + groupHeader.w &&
                                       mousePos.y >= groupHeader.y && mousePos.y <= groupHeader.y + groupHeader.h);

            UIWidget::drawPanel(renderer, groupHeader, Theme::colors.bgHeader, groupHeaderHovered ? Theme::colors.borderSelected : Theme::colors.borderButton);

            std::string arrow = isCollapsed ? "[ + ]" : "[ - ]";
            std::string headerText = std::format("{} Character: {} ({} saves)", arrow, group.characterName, group.saves.size());
            UIWidget::drawText(renderer, headerText, padX + (10.0f * uiScale), curY + (5.0f * uiScale), groupHeaderHovered ? Theme::colors.textPrimary : Theme::colors.textGold, uiScale * 0.85f);

            if (groupHeaderHovered && clicked && loadState)
            {
                loadState->toggleCharacterCollapsed(group.characterName);
                gameContext->input.consumeMouseClick();
            }

            curY += groupHeader.h + (6.0f * uiScale);

            // Expanded saves
            if (!isCollapsed)
            {
                for (const auto& save : group.saves)
                {
                    SDL_FRect itemRect = { padX, curY, availableW, 28.0f * uiScale };
                    UIWidget::drawPanel(renderer, itemRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    // Time column
                    std::string timeStr = save.timestamp.empty() ? "2026-08-29" : save.timestamp;
                    UIWidget::drawText(renderer, timeStr, padX + (8.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);

                    // Name column
                    std::string displayName = save.saveName.empty() ? save.fileName : save.saveName;
                    UIWidget::drawText(renderer, displayName, padX + timeColW, curY + (5.0f * uiScale), save.isAutosave ? Theme::colors.arcane : Theme::colors.textPrimary, uiScale * 0.82f);

                    // Action buttons: Save | Load | Delete
                    float rightOffset = 6.0f * uiScale;

                    // Delete button
                    float delBtnW = 55.0f * uiScale;
                    float btnH = 20.0f * uiScale;
                    SDL_FRect delBtnRect = { padX + availableW - rightOffset - delBtnW, curY + (4.0f * uiScale), delBtnW, btnH };
                    bool delHovered = (mousePos.x >= delBtnRect.x && mousePos.x <= delBtnRect.x + delBtnRect.w &&
                                       mousePos.y >= delBtnRect.y && mousePos.y <= delBtnRect.y + delBtnRect.h);

                    UIWidget::drawButton(renderer, delBtnRect, "Delete", delHovered, true, false, uiScale * 0.72f);
                    if (delHovered && clicked && loadState)
                    {
                        loadState->pendingDeleteFileName = save.fileName;
                        gameContext->input.consumeMouseClick();
                    }
                    rightOffset += delBtnW + (6.0f * uiScale);

                    // Load button
                    float loadBtnW = 55.0f * uiScale;
                    SDL_FRect loadBtnRect = { padX + availableW - rightOffset - loadBtnW, curY + (4.0f * uiScale), loadBtnW, btnH };
                    bool loadHovered = (mousePos.x >= loadBtnRect.x && mousePos.x <= loadBtnRect.x + loadBtnRect.w &&
                                        mousePos.y >= loadBtnRect.y && mousePos.y <= loadBtnRect.y + loadBtnRect.h);

                    UIWidget::drawButton(renderer, loadBtnRect, "Load", loadHovered, true, false, uiScale * 0.72f);
                    if (loadHovered && clicked)
                    {
                        if (saveManager::loadFromFile(gameContext, save.fileName))
                        {
                            gameContext->changeState(std::make_unique<explorationState>());
                        }
                        gameContext->input.consumeMouseClick();
                    }
                    rightOffset += loadBtnW + (6.0f * uiScale);

                    // Save button
                    if ((isSaveMode || player) && group.characterName == activeCharName && !save.isAutosave)
                    {
                        float saveBtnW = 55.0f * uiScale;
                        SDL_FRect saveBtnRect = { padX + availableW - rightOffset - saveBtnW, curY + (4.0f * uiScale), saveBtnW, btnH };
                        bool saveHovered = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                                            mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

                        UIWidget::drawButton(renderer, saveBtnRect, "Save", saveHovered, true, false, uiScale * 0.72f);
                        if (saveHovered && clicked && loadState)
                        {
                            loadState->pendingOverwriteSaveName = save.saveName.empty() ? save.fileName : save.saveName;
                            gameContext->input.consumeMouseClick();
                        }
                    }

                    curY += itemRect.h + (4.0f * uiScale);
                }
            }

            curY += (8.0f * uiScale);
        }
    }

    return (curY - startY);
}

float uiRenderer::renderOptionsView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    optionsState* opt = dynamic_cast<optionsState*>(gameContext->getActiveState());
    if (!opt) return 0.0f;

    float startY = curY;
    float centerX = rect.x + (rect.w / 2.0f);
    float padX = rect.x + (20.0f * uiScale);
    float availableW = rect.w - (40.0f * uiScale);

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    if (opt->screenMode == OptionsScreenMode::GENERAL_OPTIONS)
    {
        // 1. Centered Header Card: Options
        float cardW = std::min(availableW, 360.0f * uiScale);
        float cardH = 34.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Options", Theme::colors.textPrimary, uiScale);
        curY += cardH + (20.0f * uiScale);

        float textW = std::min(availableW, 700.0f * uiScale);
        float textX = centerX - (textW / 2.0f);

        // Section: Light/Dark theme
        UIWidget::drawText(renderer, "Light/Dark theme:", textX, curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (18.0f * uiScale);
        float descH1 = UIWidget::drawTextWrapped(renderer, "This switches the main display between a light and dark theme. (Work in progress!)", textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.86f);
        curY += descH1 + (18.0f * uiScale);

        // Section: Font-size
        UIWidget::drawText(renderer, "Font-size:", textX, curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (18.0f * uiScale);
        std::string fontDesc = std::format("This cycles the game's base font size. This currently only affects the size of the text in the main dialogue, but in the future I'll expand it to include every display element.\nMinimum font size is 12. Default font size is 18. Maximum font size is 36.\nCurrent font size: {}.", opt->fontSize);
        float descH2 = UIWidget::drawTextWrapped(renderer, fontDesc, textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.86f);
        curY += descH2 + (18.0f * uiScale);

        // Section: Fade-in
        UIWidget::drawText(renderer, "Fade-in:", textX, curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (18.0f * uiScale);
        float descH3 = UIWidget::drawTextWrapped(renderer, "This option is responsible for fading in the main part of the text each time a new scene is displayed. Although it makes scene transitions a little prettier, it is off by default, as it can cause some annoying lag in inventory screens.", textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.86f);
        curY += descH3 + (18.0f * uiScale);

        // Section: Difficulty
        static const char* diffNames[] = { "Human", "Morph", "Demon", "Lilin", "Lilith" };
        std::string diffHeader = std::format("Difficulty (Currently set to {}):", diffNames[opt->difficultyLevel]);
        UIWidget::drawText(renderer, diffHeader, textX, curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (20.0f * uiScale);

        // Colored difficulty tiers
        struct DiffTier { std::string name; std::string desc; SDL_Color col; };
        static const DiffTier tiers[5] = {
            { "Human", "The way the game is meant to be played. No level scaling and no damage modifications.", Theme::colors.textPrimary },
            { "Morph", "Enemies level up alongside your character, but do normal damage.", SDL_Color{ 180, 140, 200, 255 } },
            { "Demon", "Enemies level up alongside your character and do 200% damage.", SDL_Color{ 190, 120, 220, 255 } },
            { "Lilin", "Enemies level up alongside your character, do 200% damage, and take only 50% damage from all sources.", SDL_Color{ 210, 110, 240, 255 } },
            { "Lilith", "Enemies are always 2x your character's level, do 400% damage, and take only 25% damage from all sources. Be prepared to lose. A lot.", SDL_Color{ 240, 90, 110, 255 } }
        };

        for (int i = 0; i < 5; ++i)
        {
            UIWidget::drawText(renderer, tiers[i].name, textX, curY, tiers[i].col, uiScale * 0.88f);
            float nameOffset = (tiers[i].name.size() * (8.5f * uiScale)) + (6.0f * uiScale);
            float tierH = UIWidget::drawTextWrapped(renderer, tiers[i].desc, textX + nameOffset, curY, textW - nameOffset, Theme::colors.textSecondary, uiScale * 0.85f);
            curY += std::max(tierH, 18.0f * uiScale) + (6.0f * uiScale);
        }

        return (curY - startY);
    }
    else // Content Options (Misc., Gameplay, etc.)
    {
        std::string catName = "Misc.";
        if (opt->contentCategory == ContentOptionsCategory::GAMEPLAY) catName = "Gameplay";
        else if (opt->contentCategory == ContentOptionsCategory::SEX_AND_FETISHES) catName = "Sex & Fetishes";
        else if (opt->contentCategory == ContentOptionsCategory::BODIES) catName = "Bodies";
        else if (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS) catName = "Gender preferences";
        else if (opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS) catName = "Orientation preferences";
        else if (opt->contentCategory == ContentOptionsCategory::AGE_PREFS) catName = "Age preferences";
        else if (opt->contentCategory == ContentOptionsCategory::FURRY_PREFS) catName = "Furry preferences";
        else if (opt->contentCategory == ContentOptionsCategory::FETISH_PREFS) catName = "Fetish preferences";

        // Centered Header Card: Content Options (<Category>)
        float cardW = std::min(availableW, 420.0f * uiScale);
        float cardH = 34.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Content Options (" + catName + ")", Theme::colors.textPrimary, uiScale);
        curY += cardH + (18.0f * uiScale);

        // Helper lambda to render a Content Option item card
        auto renderOptionCard = [&](const std::string& title, SDL_Color titleCol, const std::string& description, const std::vector<std::string>& pillLabels, int selectedIndex, std::function<void(int)> onSelect) {
            float cardWidth = availableW;
            float leftW = cardWidth - (180.0f * uiScale);
            float cardMinH = 50.0f * uiScale;

            SDL_FRect cardRect = { padX, curY, cardWidth, cardMinH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            // Title + Description
            UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (7.0f * uiScale), titleCol, uiScale * 0.88f);
            float titleW = (title.size() + 2) * (7.5f * uiScale);

            float descH = UIWidget::drawTextWrapped(renderer, description, padX + (10.0f * uiScale) + titleW, curY + (7.0f * uiScale), leftW - titleW - (10.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

            // Segmented Pills on Right
            float pillTotalW = 150.0f * uiScale;
            float pillH = 24.0f * uiScale;
            float pillItemW = pillTotalW / pillLabels.size();
            float pillStartX = padX + cardWidth - pillTotalW - (12.0f * uiScale);
            float pillY = curY + ((cardMinH - pillH) / 2.0f);

            for (size_t p = 0; p < pillLabels.size(); ++p)
            {
                SDL_FRect pRect = { pillStartX + (p * pillItemW), pillY, pillItemW, pillH };
                bool pHovered = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                 mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                bool isSelected = (static_cast<int>(p) == selectedIndex);

                SDL_Color bgCol = isSelected ? (pillLabels[p] == "OFF" ? SDL_Color{ 160, 45, 55, 240 } : SDL_Color{ 45, 120, 65, 240 }) : (pHovered ? SDL_Color{ 50, 54, 62, 255 } : Theme::colors.bgButton);
                SDL_Color borderCol = isSelected ? Theme::colors.borderSelected : (pHovered ? Theme::colors.textGold : Theme::colors.borderButton);

                SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                SDL_RenderFillRect(renderer, &pRect);
                SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                SDL_RenderRect(renderer, &pRect);

                SDL_Color pTextCol = isSelected ? Theme::colors.textPrimary : (pHovered ? Theme::colors.textGold : Theme::colors.textMuted);
                float labelW = pillLabels[p].size() * (6.0f * uiScale);
                UIWidget::drawText(renderer, pillLabels[p], pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (4.0f * uiScale), pTextCol, uiScale * 0.78f);

                if (pHovered && clicked)
                {
                    onSelect(static_cast<int>(p));
                    gameContext->input.consumeMouseClick();
                }
            }

            float actualCardH = std::max(cardMinH, descH + (14.0f * uiScale));
            curY += actualCardH + (8.0f * uiScale);
        };

        if (opt->contentCategory == ContentOptionsCategory::MISC)
        {
            renderOptionCard("Autosave Frequency", Theme::colors.companion, "Choose how often want the game to autosave when you transition from one map to another.", { "Always", "Daily", "Weekly" }, 0, [](int idx) {});
            renderOptionCard("Artwork", SDL_Color{ 100, 200, 255, 255 }, "Enables artwork to be displayed in characters' information screens.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Thumbnails", SDL_Color{ 100, 200, 255, 255 }, "Enables tooltips containing thumbnail images of the character.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Shared Encyclopedia", Theme::colors.textGold, "When enabled, your character will use the shared Encyclopedia across playthroughs.", { "OFF", "ON" }, 0, [](int idx) {});
            renderOptionCard("Storm interruptions", SDL_Color{ 235, 140, 255, 255 }, "When enabled, arcane storms will interrupt dialogue to let you know that they've started.", { "OFF", "ON" }, 1, [](int idx) {});
        }
        else if (opt->contentCategory == ContentOptionsCategory::GAMEPLAY)
        {
            renderOptionCard("Enchantment Instability", SDL_Color{ 235, 140, 255, 255 }, "Toggle the 'enchantment instability' mechanic, restricting enchanted items.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Bad Ends", SDL_Color{ 255, 110, 120, 255 }, "Toggle the ability to trigger 'bad ends', which end the game when encountered.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Level Drain", SDL_Color{ 255, 90, 100, 255 }, "Toggle the use of the 'orgasmic level drain' perk by unique NPCs.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Opportunistic attackers", SDL_Color{ 255, 110, 120, 255 }, "Makes random attacks more likely when you're high on lust, low health, or exposed.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Offspring Encounters", SDL_Color{ 160, 160, 255, 255 }, "Enables you to randomly encounter your offspring throughout the world.", { "OFF", "ON" }, 1, [](int idx) {});
        }
        else
        {
            renderOptionCard("Category Preferences", Theme::colors.textGold, "Fine tune preferences and content generation rules for this category.", { "OFF", "ON" }, 1, [](int idx) {});
        }

        return (curY - startY);
    }
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

float uiRenderer::renderEnchantingView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    float startY = curY;
    float padX = rect.x + (14.0f * uiScale);
    float availableW = rect.w - (28.0f * uiScale);

    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "ENCHANTING & INFUSION ALTAR", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    float halfW = (availableW - (12.0f * uiScale)) / 2.0f;

    // 1. Primary & Secondary Modifier Headers & Grids
    UIWidget::drawText(renderer, "Primary Modifier", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
    UIWidget::drawText(renderer, "Secondary Modifier", padX + halfW + (12.0f * uiScale), curY, Theme::colors.textAccent, uiScale * 0.95f);
    curY += (18.0f * uiScale);

    // Primary Modifiers Matrix (Left)
    float btnSize = 22.0f * uiScale;
    float gap = 4.0f * uiScale;
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            SDL_FRect mRect = { padX + (c * (btnSize + gap)), curY + (r * (btnSize + gap)), btnSize, btnSize };
            bool isSelected = (r == 0 && c == 0);
            UIWidget::drawPanel(renderer, mRect, isSelected ? Theme::colors.lust : Theme::colors.bgSlot, isSelected ? Theme::colors.borderButton : Theme::colors.borderNormal);
        }
    }

    // Secondary Modifiers Matrix (Right)
    float secX = padX + halfW + (12.0f * uiScale);
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            SDL_FRect mRect = { secX + (c * (btnSize + gap)), curY + (r * (btnSize + gap)), btnSize, btnSize };
            bool isSelected = (r == 1 && c == 1);
            UIWidget::drawPanel(renderer, mRect, isSelected ? Theme::colors.arcane : Theme::colors.bgSlot, isSelected ? Theme::colors.textGold : Theme::colors.borderNormal);
        }
    }

    curY += (3 * (btnSize + gap)) + (12.0f * uiScale);

    // 2. Strength Selector Buttons Row
    static const std::vector<std::string> tiers = { "Major Drain", "Drain", "Minor Drain", "Minor Boost", "Boost", "Major Boost" };
    float tierW = (availableW - (gap * 5)) / 6.0f;
    float tierH = 22.0f * uiScale;

    for (size_t i = 0; i < tiers.size(); ++i)
    {
        SDL_FRect tRect = { padX + (i * (tierW + gap)), curY, tierW, tierH };
        bool isSelected = (i == 5); // Major Boost
        UIWidget::drawButton(renderer, tRect, tiers[i], isSelected, true, isSelected, uiScale * 0.8f);
    }
    curY += tierH + (10.0f * uiScale);

    // 3. Effect To Be Added Banner & Add Button
    SDL_FRect addBarRect = { padX, curY, availableW, 26.0f * uiScale };
    UIWidget::drawPanel(renderer, addBarRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
    UIWidget::drawText(renderer, "Effect to be added: Huge increase in milk regeneration.", padX + (8.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.lust, uiScale * 0.9f);

    float addBtnW = 75.0f * uiScale;
    SDL_FRect addBtnRect = { padX + availableW - addBtnW - (4.0f * uiScale), curY + (3.0f * uiScale), addBtnW, 20.0f * uiScale };
    UIWidget::drawButton(renderer, addBtnRect, "Add | 13*", false, true, false, uiScale * 0.8f);
    curY += addBarRect.h + (12.0f * uiScale);

    // 4. Recipe Craft Container (Input, Name + Effects List, Output)
    SDL_FRect recipeRect = { padX, curY, availableW, 110.0f * uiScale };
    UIWidget::drawPanel(renderer, recipeRect, Theme::colors.bgDark, Theme::colors.borderButton);

    // Input Slot
    UIWidget::drawText(renderer, "Input (x1)", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.85f);
    SDL_FRect inSlotRect = { padX + (10.0f * uiScale), curY + (24.0f * uiScale), 48.0f * uiScale, 48.0f * uiScale };
    UIWidget::drawPanel(renderer, inSlotRect, Theme::colors.bgHeader, Theme::colors.borderButton);
    UIWidget::drawText(renderer, "ELIXIR", inSlotRect.x + (6.0f * uiScale), inSlotRect.y + (16.0f * uiScale), Theme::colors.textGold, uiScale * 0.8f);

    // Effects Middle List
    float midX = padX + (70.0f * uiScale);
    float midW = availableW - (150.0f * uiScale);
    UIWidget::drawText(renderer, "Effects (5/100) | Cost: 46*", midX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

    SDL_FRect nameInputRect = { midX, curY + (22.0f * uiScale), midW, 20.0f * uiScale };
    UIWidget::drawPanel(renderer, nameInputRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
    UIWidget::drawText(renderer, "Bovine elixir", nameInputRect.x + (6.0f * uiScale), nameInputRect.y + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

    static const std::vector<std::string> appliedEffects = {
        "Bovine tail transformation.",
        "Bovine ears transformation.",
        "Grows curved horns.",
        "Huge increase in breast size. (+3 breast size.)"
    };

    float effY = curY + (46.0f * uiScale);
    for (const auto& eff : appliedEffects)
    {
        UIWidget::drawText(renderer, std::format("• {}", eff), midX, effY, Theme::colors.textPrimary, uiScale * 0.8f);
        effY += (14.0f * uiScale);
    }

    // Output Slot
    float outX = padX + availableW - (65.0f * uiScale);
    UIWidget::drawText(renderer, "Output", outX + (4.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textAccent, uiScale * 0.85f);
    SDL_FRect outSlotRect = { outX, curY + (24.0f * uiScale), 48.0f * uiScale, 48.0f * uiScale };
    UIWidget::drawPanel(renderer, outSlotRect, Theme::colors.bgHeader, Theme::colors.lust);
    UIWidget::drawText(renderer, "RUNIC", outSlotRect.x + (6.0f * uiScale), outSlotRect.y + (16.0f * uiScale), Theme::colors.lust, uiScale * 0.8f);

    curY += recipeRect.h + (8.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float availableW = innerW - (20.0f * uiScale);

    UIWidget::drawText(renderer, "CHARACTERS PRESENT", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    bool hasNpc = false;
    if (gameContext->map)
    {
        auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
        if (tileData.persistentNPC)
        {
            hasNpc = true;
            SDL_FRect cardRect = { padX, curY, availableW, 24.0f * uiScale };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, std::format("👤 {}", tileData.persistentNPC->name), padX + (6.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.lust, uiScale * 0.85f);
            curY += (28.0f * uiScale);
        }
    }

    if (!hasNpc)
    {
        UIWidget::drawText(renderer, "None...", padX, curY, Theme::colors.textMuted, uiScale * 0.85f);
        curY += (16.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);

    UIWidget::drawText(renderer, "ITEMS PRESENT", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    auto ground = gameContext->getTileInventoryStacked();
    if (ground.empty())
    {
        UIWidget::drawText(renderer, "None...", padX, curY, Theme::colors.textMuted, uiScale * 0.85f);
        curY += (16.0f * uiScale);
    }
    else
    {
        for (size_t i = 0; i < ground.size() && i < 6; ++i)
        {
            if (ground[i].itemPtr)
            {
                std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                UIWidget::drawText(renderer, line, padX, curY, Theme::colors.textAccent, uiScale * 0.85f);
                curY += (15.0f * uiScale);
            }
        }
    }

    return (curY - startY);
}

float uiRenderer::renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);

    UIWidget::drawText(renderer, "EVENT LOG", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    static const std::vector<std::pair<std::string, SDL_Color>> logEntries = {
        { "Game loaded: QuickSave.json", Theme::colors.friendly },
        { "Encyclopedia: Wolf-morph", Theme::colors.textGold },
        { "Item Added: Impish Brew", Theme::colors.lust },
        { "Item Added: Bunny Juice", Theme::colors.arcane },
        { "Encyclopedia: Gothic boots", Theme::colors.textAccent }
    };

    for (const auto& entry : logEntries)
    {
        UIWidget::drawText(renderer, entry.first, padX, curY, entry.second, uiScale * 0.8f);
        curY += (14.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderWidgetTimeBar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    SDL_FRect timeRect = { padX, curY, availableW, 18.0f * uiScale };
    UIWidget::drawPanel(renderer, timeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

    UIWidget::drawText(renderer, "17th November", padX + (6.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.8f);
    UIWidget::drawText(renderer, "• 20:04", padX + availableW - (52.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.8f);

    curY += timeRect.h + (4.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    static const std::vector<std::pair<std::string, CommandType>> tools = {
        { "OPT", CommandType::OPEN_SETTINGS },
        { "PHONE", CommandType::OPEN_PHONE },
        { "GEAR", CommandType::OPEN_INVENTORY },
        { "INV", CommandType::OPEN_INVENTORY },
        { "LOOK", CommandType::OPEN_TRANSFORMATION }
    };

    float btnGap = 3.0f * uiScale;
    float btnW = (availableW - (btnGap * (tools.size() - 1))) / tools.size();
    float btnH = 20.0f * uiScale;

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    for (size_t i = 0; i < tools.size(); ++i)
    {
        SDL_FRect btnRect = { padX + (i * (btnW + btnGap)), curY, btnW, btnH };
        bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                        mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

        bool isActive = (i == 1 && gameContext->isPhoneMenuOpen);
        UIWidget::drawButton(renderer, btnRect, tools[i].first, hovered, true, isActive, uiScale * 0.7f);

        if (hovered && clicked)
        {
            gameContext->handleCommand(UICommand{ tools[i].second });
            gameContext->input.consumeMouseClick();
        }
    }

    curY += btnH + (4.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    entity* p = gameContext->getPlayer();
    if (!p) return 0.0f;

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    // 1. Zone & Room Header (Dominion in purple, Lilaya's Home in cyan)
    UIWidget::drawText(renderer, "Dominion", padX, curY, SDL_Color{ 208, 112, 255, 255 }, uiScale * 1.05f);
    curY += (17.0f * uiScale);
    UIWidget::drawText(renderer, "Lilaya's Home", padX, curY, SDL_Color{ 96, 208, 255, 255 }, uiScale * 0.9f);
    curY += (15.0f * uiScale);

    // 2. Character Card Frame
    SDL_FRect cardRect = { padX, curY, availableW, 148.0f * uiScale };
    UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgDark, Theme::colors.borderButton);

    float innerPadX = padX + (8.0f * uiScale);
    float cardCurY = curY + (6.0f * uiScale);
    float cardInnerW = availableW - (16.0f * uiScale);

    // Row A: Avatar & Name/Level
    SDL_FRect avatarRect = { innerPadX, cardCurY, 22.0f * uiScale, 22.0f * uiScale };
    UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgHeader, Theme::colors.borderButton);
    UIWidget::drawText(renderer, "H", avatarRect.x + (6.0f * uiScale), avatarRect.y + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

    std::string nameLvl = std::format("{} - Level {}", p->name.empty() ? "Hero" : p->name, p->stats.level);
    UIWidget::drawText(renderer, nameLvl, innerPadX + (28.0f * uiScale), cardCurY + (3.0f * uiScale), Theme::colors.textAccent, uiScale * 0.9f);
    cardCurY += (25.0f * uiScale);

    // Row B: Currency (Gold ¤ and Arcane Essence)
    UIWidget::drawText(renderer, std::format("¤ {:.0f}", p->getStat("currency")), innerPadX, cardCurY, Theme::colors.textGold, uiScale * 0.85f);
    UIWidget::drawText(renderer, "Essence: 0", innerPadX + (cardInnerW * 0.52f), cardCurY, Theme::colors.lust, uiScale * 0.85f);
    cardCurY += (16.0f * uiScale);

    // Row C: Core stats numbers
    UIWidget::drawText(renderer, std::format("PHY: {:.0f}", p->getStat("physique")), innerPadX, cardCurY, Theme::colors.enemy, uiScale * 0.8f);
    UIWidget::drawText(renderer, std::format("ARC: {:.0f}", p->getStat("arcane")), innerPadX + (cardInnerW * 0.35f), cardCurY, Theme::colors.arcane, uiScale * 0.8f);
    UIWidget::drawText(renderer, std::format("COR: {:.0f}", p->getStat("corruption")), innerPadX + (cardInnerW * 0.68f), cardCurY, Theme::colors.corruption, uiScale * 0.8f);
    cardCurY += (16.0f * uiScale);

    // Row D: 3 Vitals Bars (Red/Coral Health, Purple Mana, Pink Lust)
    float barH = 12.0f * uiScale;
    float barW = cardInnerW - (75.0f * uiScale);

    // Health
    float hp = p->getStat("health");
    UIWidget::drawText(renderer, "HP", innerPadX, cardCurY, Theme::colors.health, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, hp, 100.0f, Theme::colors.health, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, std::format("{:.0f}/100", hp), innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 4.0f * uiScale);

    // Mana
    float mana = p->getStat("mana");
    UIWidget::drawText(renderer, "MP", innerPadX, cardCurY, Theme::colors.mana, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, mana, 50.0f, Theme::colors.mana, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, std::format("{:.0f}/50", mana), innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 4.0f * uiScale);

    // Lust
    float lust = p->getStat("lust");
    UIWidget::drawText(renderer, "LU", innerPadX, cardCurY, Theme::colors.lust, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, lust, 100.0f, Theme::colors.lust, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, std::format("{:.0f}/100", lust), innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 6.0f * uiScale);

    // Row E: Status trait badges
    static const std::vector<std::string> statusBadges = { "STR", "RAIN", "FEM", "HORN", "AURA" };
    float badgeW = (cardInnerW - (4.0f * 4.0f * uiScale)) / 5.0f;
    float badgeH = 14.0f * uiScale;
    for (size_t i = 0; i < statusBadges.size(); ++i)
    {
        SDL_FRect bRect = { innerPadX + (i * (badgeW + 4.0f * uiScale)), cardCurY, badgeW, badgeH };
        UIWidget::drawPanel(renderer, bRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, statusBadges[i], bRect.x + (2.0f * uiScale), bRect.y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.65f);
    }

    curY += cardRect.h + (8.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) ||
        dynamic_cast<optionsState*>(gameContext->getActiveState()) ||
        dynamic_cast<loadGameState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
    curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 120.0f * uiScale }, curY, uiScale);
    curY += renderWidgetOptionsToolbar(renderer, gameContext, curX, curY, innerW, uiScale);
    return (curY - startY);
}