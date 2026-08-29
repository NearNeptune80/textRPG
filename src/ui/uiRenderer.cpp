#include "ui/uiRenderer.h"

#include <filesystem>
#include <format>
#include <iostream>

#include "core/characterDescription.h"
#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "save/saveManager.h"
#include "state/characterCreationState.h"
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
    else if (dynamic_cast<characterCreationState*>(state))
    {
        return renderCharacterCreationView(renderer, gameContext, rect, curY, uiScale);
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
    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float innerW = rect.w - (32.0f * uiScale);
    float centerX = rect.x + (rect.w / 2.0f);

    if (gameContext->isPhoneMenuOpen)
    {
        std::string phoneTitle = "Phone home screen";
        float titleW = phoneTitle.size() * (9.5f * uiScale);
        UIWidget::drawText(renderer, phoneTitle, centerX - (titleW / 2.0f), curY, SDL_Color{ 255, 240, 200, 255 }, uiScale * 1.15f);
        curY += (28.0f * uiScale);

        std::string p1 = "You pull out your phone and tap in the unlock code.";
        float h1 = UIWidget::drawTextWrapped(renderer, p1, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += h1 + (16.0f * uiScale);

        std::string p2 = "Using your powerful aura, you've managed to figure out a way to channel the arcane into charging the battery of your phone, although considering that it's the only one in this world, it's not much use for calling anyone. Instead, you're using it as a way to store information about things you've discovered in this strange new world.";
        float h2 = UIWidget::drawTextWrapped(renderer, p2, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += h2 + (16.0f * uiScale);

        return (curY - startY);
    }

    const gameMap* m = gameContext->getActiveMap();
    std::string locTitle = "Corridor";
    if (m && !m->getName().empty() && m->getName() != "Enchanted Forest" && m->getName() != "District Map")
    {
        locTitle = m->getName();
    }

    float titleW = locTitle.size() * (10.0f * uiScale);
    UIWidget::drawText(renderer, locTitle, centerX - (titleW / 2.0f), curY, SDL_Color{ 255, 240, 200, 255 }, uiScale * 1.15f);
    curY += (28.0f * uiScale);

    std::string p1 = "Immaculately-clean red carpet runs down the centre of the wide corridor which you're currently walking down, while the walls are decorated in a pale, light-blue wallpaper that sports a series of delicate white floral patterns. Fine landscape paintings and portraits of beautiful demons are hung up on the walls at regular intervals, while all manner of antique curiosities are perched upon the many wooden cabinets which line the sides of these hallways.";
    float h1 = UIWidget::drawTextWrapped(renderer, p1, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += h1 + (14.0f * uiScale);

    std::string p2 = "As it's currently night time, heavy fabric curtains have been drawn over the corridor's large glass windows, leaving the area to be illuminated by the many arcane-powered wall-lights.";
    float h2 = UIWidget::drawTextWrapped(renderer, p2, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += h2 + (14.0f * uiScale);

    std::string p3 = "This corridor is deserted at the moment, and there doesn't really seem to be much to do here.";
    float h3 = UIWidget::drawTextWrapped(renderer, p3, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += h3 + (16.0f * uiScale);

    return (curY - startY);
}

void uiRenderer::renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale)
{
    UIWidget::drawPanel(renderer, rect);

    const auto& buttons = gameContext->getActiveActionButtons();
    int totalButtons = static_cast<int>(buttons.size());

    int totalPages = (totalButtons > 0) ? ((totalButtons - 1) / BUTTONS_PER_PAGE) + 1 : 1;
    gameContext->currentActionPage = std::clamp(gameContext->currentActionPage, 0, totalPages - 1);

    int startIndex = gameContext->currentActionPage * BUTTONS_PER_PAGE;
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
    UIWidget::drawLTActionButton(renderer, leftMidRect, "<", "", leftHovered, gameContext->currentActionPage > 0, false, uiScale * 0.9f);
    if (leftHovered && clicked && gameContext->currentActionPage > 0)
    {
        gameContext->previousActionPage();
        gameContext->input.consumeMouseClick();
    }

    // 2. Right Side Pagination (> and E)
    float rightX = rect.x + rect.w - marginX - sideBtnW;
    SDL_FRect rightTopRect = { rightX, padY, sideBtnW, (gridH / 3.0f) - (2.0f * uiScale) };
    SDL_FRect rightMidRect = { rightX, padY + (gridH / 3.0f), sideBtnW, (gridH * 2.0f / 3.0f) };
    bool rightHovered = (mousePos.x >= rightMidRect.x && mousePos.x <= rightMidRect.x + rightMidRect.w &&
                         mousePos.y >= rightMidRect.y && mousePos.y <= rightMidRect.y + rightMidRect.h);

    UIWidget::drawLTActionButton(renderer, rightTopRect, "E", "", false, false, false, uiScale * 0.75f);
    UIWidget::drawLTActionButton(renderer, rightMidRect, ">", "", rightHovered, gameContext->currentActionPage < totalPages - 1, false, uiScale * 0.9f);
    if (rightHovered && clicked && gameContext->currentActionPage < totalPages - 1)
    {
        gameContext->nextActionPage();
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

            UIWidget::drawLTActionButton(renderer, btnRect, buttons[buttonIdx].label, hotkeys[slotIdx], hovered, buttons[buttonIdx].isEnabled, buttons[buttonIdx].isSelected, uiScale);

            if (hovered && clicked && buttons[buttonIdx].isEnabled && buttons[buttonIdx].onClick)
            {
                auto cb = buttons[buttonIdx].onClick;
                gameContext->input.consumeMouseClick();
                cb();
                break;
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
    if (!gameContext->getPlayer())
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
            SDL_FRect tileRect = {
                padX + static_cast<float>(dx + radius) * tileSize,
                curY + static_cast<float>(dy + radius) * tileSize,
                std::max(1.0f, tileSize - (1.5f * uiScale)),
                std::max(1.0f, tileSize - (1.5f * uiScale))
            };

            SDL_Color tileColor = SDL_Color{ 20, 22, 28, 255 };
            SDL_Color borderColor = SDL_Color{ 35, 38, 48, 255 };
            SDL_Color textCol = Theme::colors.textGold;
            std::string label = "";

            if (dx == 0 && dy == 0)
            {
                // Player Center Tile (Corridor location pin)
                tileColor = SDL_Color{ 25, 42, 60, 255 };
                borderColor = SDL_Color{ 96, 175, 255, 255 };
                label = "📍";
                textCol = SDL_Color{ 100, 190, 255, 255 };
            }
            else if (dx == 0 && dy == -1)
            {
                tileColor = SDL_Color{ 28, 34, 44, 255 };
                borderColor = SDL_Color{ 60, 75, 95, 255 };
                label = "W";
                textCol = SDL_Color{ 130, 200, 255, 255 };
            }
            else if (dx == -1 && dy == 0)
            {
                tileColor = SDL_Color{ 28, 34, 44, 255 };
                borderColor = SDL_Color{ 60, 75, 95, 255 };
                label = "A";
                textCol = SDL_Color{ 130, 200, 255, 255 };
            }
            else if (dx == 0 && dy == 1)
            {
                tileColor = SDL_Color{ 28, 34, 44, 255 };
                borderColor = SDL_Color{ 60, 75, 95, 255 };
                label = "S";
                textCol = SDL_Color{ 130, 200, 255, 255 };
            }
            else if (dx == 1 && dy == 0)
            {
                tileColor = SDL_Color{ 45, 25, 30, 255 };
                borderColor = SDL_Color{ 160, 50, 60, 255 };
                label = "D";
                textCol = SDL_Color{ 255, 120, 130, 255 };
            }
            else if (std::abs(dx) == 2 || std::abs(dy) == 2 || (std::abs(dx) == 1 && std::abs(dy) == 1))
            {
                tileColor = SDL_Color{ 18, 20, 26, 255 };
                borderColor = SDL_Color{ 32, 35, 44, 255 };
                label = "🛏";
                textCol = SDL_Color{ 110, 115, 130, 200 };
            }

            // Draw tile border and fill
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            SDL_RenderRect(renderer, &tileRect);

            SDL_SetRenderDrawColor(renderer, tileColor.r, tileColor.g, tileColor.b, tileColor.a);
            SDL_RenderFillRect(renderer, &tileRect);

            if (!label.empty() && tileSize >= 12.0f * uiScale)
            {
                float lScale = (label == "📍" || label == "🛏") ? (uiScale * 0.7f) : (uiScale * 0.75f);
                UIWidget::drawText(renderer, label, tileRect.x + (tileSize * 0.2f), tileRect.y + (tileSize * 0.1f), textCol, lScale);
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
    else // Content Options (Misc., Gameplay, Sex & Fetishes, Bodies, Preferences, etc.)
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

        // Centered Header Card
        std::string headerTitle = (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS ||
                                   opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS ||
                                   opt->contentCategory == ContentOptionsCategory::AGE_PREFS ||
                                   opt->contentCategory == ContentOptionsCategory::FURRY_PREFS ||
                                   opt->contentCategory == ContentOptionsCategory::FETISH_PREFS)
                                  ? catName : "Content Options (" + catName + ")";

        float cardW = std::min(availableW, 420.0f * uiScale);
        float cardH = 34.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, headerTitle, Theme::colors.textPrimary, uiScale);
        curY += cardH + (16.0f * uiScale);

        // Helper lambda: Info dropdown banner
        auto renderInfoDropdown = [&]() {
            float dropW = availableW;
            float dropH = 26.0f * uiScale;
            SDL_FRect dropRect = { padX, curY, dropW, dropH };
            UIWidget::drawPanel(renderer, dropRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            std::string infoStr = "► Click for more info.";
            float strW = infoStr.size() * (7.0f * uiScale);
            UIWidget::drawText(renderer, infoStr, padX + ((dropW - strW) / 2.0f), curY + (5.0f * uiScale), SDL_Color{ 180, 130, 255, 255 }, uiScale * 0.86f);
            curY += dropH + (12.0f * uiScale);
        };

        // Helper lambda: Multi-colored distribution proportion bar
        auto renderDistributionBar = [&](const std::vector<std::pair<float, SDL_Color>>& segments) {
            float barW = availableW;
            float barH = 10.0f * uiScale;
            float total = 0.0f;
            for (const auto& s : segments) total += s.first;
            if (total <= 0.0f) total = 1.0f;

            float accX = padX;
            for (const auto& s : segments)
            {
                float segW = (s.first / total) * barW;
                if (segW > 0.0f)
                {
                    SDL_FRect segRect = { accX, curY, segW, barH };
                    SDL_SetRenderDrawColor(renderer, s.second.r, s.second.g, s.second.b, s.second.a);
                    SDL_RenderFillRect(renderer, &segRect);
                    accX += segW;
                }
            }
            curY += barH + (14.0f * uiScale);
        };

        // Helper lambda: Setting Row with Segments (e.g. OFF / ON)
        auto renderOptionCard = [&](const std::string& title, SDL_Color titleCol, const std::string& description, const std::vector<std::string>& pillLabels, int selectedIndex, std::function<void(int)> onSelect) {
            float cardWidth = availableW;
            float leftW = cardWidth - (180.0f * uiScale);
            float cardMinH = 48.0f * uiScale;

            SDL_FRect cardRect = { padX, curY, cardWidth, cardMinH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (7.0f * uiScale), titleCol, uiScale * 0.88f);
            float titleW = (title.size() + 2) * (7.5f * uiScale);

            float descH = UIWidget::drawTextWrapped(renderer, description, padX + (10.0f * uiScale) + titleW, curY + (7.0f * uiScale), leftW - titleW - (10.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

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

        // Helper lambda: 6-pill frequency row ([ Off ] [ Minimal ] [ Low ] [ Average ] [ High ] [ Abundant ])
        auto renderFrequencyRow = [&](const std::string& title, SDL_Color titleCol, const std::string& subtitle, int selectedIndex, std::function<void(int)> onSelect) {
            float cardWidth = availableW;
            float rowMinH = subtitle.empty() ? 32.0f * uiScale : 48.0f * uiScale;
            SDL_FRect rowRect = { padX, curY, cardWidth, rowMinH };
            UIWidget::drawPanel(renderer, rowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, title, padX + (12.0f * uiScale), curY + (6.0f * uiScale), titleCol, uiScale * 0.9f);

            static const std::vector<std::string> freqLabels = { "Off", "Minimal", "Low", "Average", "High", "Abundant" };
            float pillTotalW = 280.0f * uiScale;
            float pillH = 22.0f * uiScale;
            float pillItemW = (pillTotalW - (5.0f * 4.0f * uiScale)) / 6.0f;
            float pillStartX = padX + cardWidth - pillTotalW - (12.0f * uiScale);
            float pillY = curY + (5.0f * uiScale);

            for (size_t p = 0; p < freqLabels.size(); ++p)
            {
                SDL_FRect pRect = { pillStartX + (p * (pillItemW + 4.0f * uiScale)), pillY, pillItemW, pillH };
                bool pHovered = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                 mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                bool isSelected = (static_cast<int>(p) == selectedIndex);

                SDL_Color bgCol = isSelected ? SDL_Color{ 75, 24, 55, 255 } : (pHovered ? SDL_Color{ 50, 54, 62, 255 } : SDL_Color{ 38, 42, 50, 255 });
                SDL_Color borderCol = isSelected ? SDL_Color{ 245, 80, 175, 255 } : (pHovered ? Theme::colors.textGold : SDL_Color{ 58, 62, 72, 255 });

                SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                SDL_RenderFillRect(renderer, &pRect);
                SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                SDL_RenderRect(renderer, &pRect);

                SDL_Color pTextCol = isSelected ? SDL_Color{ 255, 220, 245, 255 } : (pHovered ? Theme::colors.textGold : Theme::colors.textMuted);
                float labelW = freqLabels[p].size() * (6.0f * uiScale);
                UIWidget::drawText(renderer, freqLabels[p], pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), pTextCol, uiScale * 0.75f);

                if (pHovered && clicked)
                {
                    onSelect(static_cast<int>(p));
                    gameContext->input.consumeMouseClick();
                }
            }

            if (!subtitle.empty())
            {
                UIWidget::drawText(renderer, subtitle, padX + (12.0f * uiScale), curY + (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);
            }

            curY += rowMinH + (6.0f * uiScale);
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
        else if (opt->contentCategory == ContentOptionsCategory::SEX_AND_FETISHES)
        {
            renderOptionCard("Non-consent", SDL_Color{ 255, 95, 120, 255 }, "This enables the 'resist' pace in sex scenes, which contains some more extreme non-consensual descriptions, as well as dialogue references and actions related to this content. Please note that bad ends involve non-con content, regardless of whether or not this option is enabled.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Sadistic sex", SDL_Color{ 255, 95, 120, 255 }, "This unlocks 'sadistic' sex actions, such as choking, slapping, and spitting on partners in sex.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Lipstick marking", SDL_Color{ 255, 105, 180, 255 }, "This enables lipstick marking of body parts via kisses during sex.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Voluntary NTR", SDL_Color{ 255, 130, 160, 255 }, "When enabled, you will get the option to offer certain enemies sex with your companions as a way to avoid combat.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Involuntary NTR", SDL_Color{ 255, 95, 120, 255 }, "When enabled, enemies might choose to only have sex with your companion after beating your party in combat. When disabled, all post-combat-loss sex scenes will involve you.", { "OFF", "ON" }, 0, [](int idx) {});
            renderOptionCard("Incest", SDL_Color{ 190, 130, 255, 255 }, "This will enable sexual actions between closely related characters.", { "OFF", "ON" }, 1, [](int idx) {});
        }
        else if (opt->contentCategory == ContentOptionsCategory::BODIES)
        {
            renderOptionCard("Age", SDL_Color{ 185, 230, 110, 255 }, "This enables descriptions of the age that characters appear to be.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Feral", SDL_Color{ 240, 180, 80, 255 }, "This enables feral content, which contains sexual and non-sexual interactions with sapient characters who have fully-animal bodies.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Cum Regeneration", SDL_Color{ 100, 210, 255, 255 }, "This enables cum regeneration related content, such as decreasing quantity for multiple orgasms in one session and the full balls status effect. When disabled, balls will always be treated as full, but without any negative effects.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Futanari Testicles", SDL_Color{ 255, 80, 200, 255 }, "When enabled, futanari NPCs will be able to have external testicles. When disabled, they are locked to always being internal.", { "OFF", "ON" }, 1, [](int idx) {});
            renderOptionCard("Bipedal Cloacas", SDL_Color{ 255, 80, 200, 255 }, "When enabled, certain bipedal races (such as harpies and alligator-morphs) will have cloacas. When disabled, all bipeds with cloacas will be treated as having a regular genitalia configuration. Some special races, such as lamia, always have cloacas, and are not affected by this.", { "OFF", "ON" }, 1, [](int idx) {});
        }
        else if (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS)
        {
            renderFrequencyRow("Butch", SDL_Color{ 100, 160, 255, 255 }, "Butchs have a vagina, no penis, and breasts.", 0, [](int idx) {});
            renderFrequencyRow("Cuntboy", SDL_Color{ 100, 160, 255, 255 }, "Cuntboys have a vagina, no penis, and no breasts.", 0, [](int idx) {});
            renderFrequencyRow("Mannequin", SDL_Color{ 100, 160, 255, 255 }, "Mannequins have no vagina, no penis, and breasts.", 0, [](int idx) {});
            renderFrequencyRow("Mannequin", SDL_Color{ 100, 160, 255, 255 }, "Mannequins have no vagina, no penis, and no breasts.", 0, [](int idx) {});

            renderDistributionBar({ { 45.0f, SDL_Color{ 100, 160, 255, 255 } }, { 10.0f, SDL_Color{ 140, 90, 120, 255 } }, { 5.0f, SDL_Color{ 190, 130, 160, 255 } }, { 40.0f, SDL_Color{ 255, 170, 215, 255 } } });

            UIWidget::drawText(renderer, "Androgynous", centerX - (45.0f * uiScale), curY, SDL_Color{ 180, 130, 255, 255 }, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            renderFrequencyRow("Hermaphrodite", SDL_Color{ 180, 130, 255, 255 }, "Hermaphrodites have a vagina, a penis, and breasts.", 0, [](int idx) {});
            renderFrequencyRow("Hermaphrodite", SDL_Color{ 180, 130, 255, 255 }, "Hermaphrodites have a vagina, a penis, and no breasts.", 0, [](int idx) {});
        }
        else if (opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS)
        {
            renderInfoDropdown();

            renderFrequencyRow("Androphilic", SDL_Color{ 100, 160, 255, 255 }, "", 3, [](int idx) {});
            renderFrequencyRow("Ambiphilic", SDL_Color{ 180, 130, 255, 255 }, "", 3, [](int idx) {});
            renderFrequencyRow("Gynephilic", SDL_Color{ 255, 110, 180, 255 }, "", 3, [](int idx) {});

            curY += (8.0f * uiScale);
            renderDistributionBar({ { 33.3f, SDL_Color{ 100, 160, 255, 255 } }, { 33.3f, SDL_Color{ 180, 130, 255, 255 } }, { 33.4f, SDL_Color{ 255, 170, 215, 255 } } });
        }
        else if (opt->contentCategory == ContentOptionsCategory::AGE_PREFS)
        {
            renderInfoDropdown();

            UIWidget::drawText(renderer, "Masculine", centerX - (38.0f * uiScale), curY, SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            renderFrequencyRow("Late teens", Theme::colors.textPrimary, "", 4, [](int idx) {});
            renderFrequencyRow("Early twenties", Theme::colors.textPrimary, "", 5, [](int idx) {});
            renderFrequencyRow("Mid-twenties", Theme::colors.textPrimary, "", 5, [](int idx) {});
            renderFrequencyRow("Late twenties", Theme::colors.textPrimary, "", 4, [](int idx) {});
            renderFrequencyRow("Early thirties", Theme::colors.textPrimary, "", 3, [](int idx) {});
            renderFrequencyRow("Mid-thirties", Theme::colors.textPrimary, "", 3, [](int idx) {});
            renderFrequencyRow("Late thirties", Theme::colors.textPrimary, "", 2, [](int idx) {});
            renderFrequencyRow("Early forties", Theme::colors.textPrimary, "", 2, [](int idx) {});
            renderFrequencyRow("Mid-forties", Theme::colors.textPrimary, "", 1, [](int idx) {});
        }
        else if (opt->contentCategory == ContentOptionsCategory::FURRY_PREFS)
        {
            renderInfoDropdown();

            // 3 Rate Cards in a row: Human Spawn Rate, Taur Spawn Rate, Half-Demon Spawn Rate
            float rateCardW = (availableW - (16.0f * uiScale)) / 3.0f;
            float rateCardH = 48.0f * uiScale;
            struct RateInfo { std::string title; SDL_Color col; int val; };
            RateInfo rates[3] = {
                { "Human Spawn Rate", Theme::colors.textPrimary, 5 },
                { "Taur Spawn Rate", SDL_Color{ 200, 160, 120, 255 }, 5 },
                { "Half-Demon Spawn Rate", SDL_Color{ 180, 130, 255, 255 }, 5 }
            };

            for (int r = 0; r < 3; ++r)
            {
                SDL_FRect rRect = { padX + (r * (rateCardW + 8.0f * uiScale)), curY, rateCardW, rateCardH };
                UIWidget::drawPanel(renderer, rRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                float titleW = rates[r].title.size() * (6.0f * uiScale);
                UIWidget::drawText(renderer, rates[r].title, rRect.x + ((rRect.w - titleW) / 2.0f), rRect.y + (5.0f * uiScale), rates[r].col, uiScale * 0.82f);

                float btnW = 16.0f * uiScale;
                float btnH = 16.0f * uiScale;
                float stepY = rRect.y + (24.0f * uiScale);
                float midX = rRect.x + (rRect.w / 2.0f);

                SDL_FRect m1 = { midX - (45.0f * uiScale), stepY, btnW, btnH };
                SDL_FRect m2 = { midX - (25.0f * uiScale), stepY, btnW, btnH };
                SDL_FRect p1 = { midX + (12.0f * uiScale), stepY, btnW, btnH };
                SDL_FRect p2 = { midX + (32.0f * uiScale), stepY, btnW, btnH };

                UIWidget::drawColoredButton(renderer, m1, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
                UIWidget::drawColoredButton(renderer, m2, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
                UIWidget::drawText(renderer, std::format("{}%", rates[r].val), midX - (8.0f * uiScale), stepY + (2.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.8f);
                UIWidget::drawColoredButton(renderer, p1, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
                UIWidget::drawColoredButton(renderer, p2, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
            }
            curY += rateCardH + (12.0f * uiScale);

            // Tauric Upper-body Furriness
            float taurH = 54.0f * uiScale;
            SDL_FRect taurRect = { padX, curY, availableW, taurH };
            UIWidget::drawPanel(renderer, taurRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Tauric Upper-body Furriness:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), SDL_Color{ 200, 160, 120, 255 }, uiScale * 0.85f);
            UIWidget::drawTextWrapped(renderer, "Set how furry you prefer the upper bodies of taurs to be.", padX + (10.0f * uiScale), curY + (24.0f * uiScale), availableW * 0.48f, Theme::colors.textSecondary, uiScale * 0.8f);

            static const char* taurOpts[6] = { "Untouched", "Human", "Minimum", "Lesser", "Greater", "Maximum" };
            float optBtnW = 60.0f * uiScale;
            float optBtnH = 20.0f * uiScale;
            float optStartX = padX + availableW - (3 * (optBtnW + 4.0f * uiScale)) - (8.0f * uiScale);

            for (int i = 0; i < 6; ++i)
            {
                int r = i / 3;
                int c = i % 3;
                SDL_FRect bRect = { optStartX + (c * (optBtnW + 4.0f * uiScale)), curY + (6.0f * uiScale) + (r * (optBtnH + 4.0f * uiScale)), optBtnW, optBtnH };
                bool isMin = (i == 2);
                SDL_Color bCol = isMin ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                SDL_Color tCol = isMin ? SDL_Color{ 255, 220, 245, 255 } : Theme::colors.textMuted;
                UIWidget::drawColoredButton(renderer, bRect, taurOpts[i], bCol, tCol, isMin, uiScale * 0.72f);
            }
            curY += taurH + (12.0f * uiScale);

            // Bulk Setters
            auto renderBulkRow = [&](const std::string& label, const std::vector<std::string>& pills) {
                float bH = 28.0f * uiScale;
                SDL_FRect bRect = { padX, curY, availableW, bH };
                UIWidget::drawPanel(renderer, bRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, label, padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
                float pW = 52.0f * uiScale;
                float pStartX = padX + availableW - (pills.size() * (pW + 4.0f * uiScale)) - (8.0f * uiScale);
                for (size_t i = 0; i < pills.size(); ++i)
                {
                    SDL_FRect pr = { pStartX + (i * (pW + 4.0f * uiScale)), curY + (4.0f * uiScale), pW, 20.0f * uiScale };
                    UIWidget::drawColoredButton(renderer, pr, pills[i], Theme::colors.bgButton, Theme::colors.textMuted, false, uiScale * 0.7f);
                }
                curY += bH + (6.0f * uiScale);
            };

            renderBulkRow("Set all furry preferences:", { "Disabled", "Minimum", "Lesser", "Greater", "Maximum" });
            renderBulkRow("Set all spawn frequencies:", { "Off", "Low", "Average", "High", "Abundant" });
            curY += (8.0f * uiScale);

            // Furry Race Table Header
            float headerColsX = padX + (availableW * 0.45f);
            UIWidget::drawText(renderer, "Furry Preference", headerColsX, curY, SDL_Color{ 140, 220, 110, 255 }, uiScale * 0.88f);
            UIWidget::drawText(renderer, "Spawn frequency", headerColsX + (150.0f * uiScale), curY, SDL_Color{ 230, 210, 100, 255 }, uiScale * 0.88f);
            curY += (20.0f * uiScale);

            // Furry Race Rows
            struct FurryRace { std::string femaleName; std::string maleName; };
            static const FurryRace races[8] = {
                { "Alligator-girl", "Alligator-boy" },
                { "Bat-girl", "Bat-boy" },
                { "Cat-girl", "Cat-boy" },
                { "Dog-girl", "Dog-boy" },
                { "Fox-girl", "Fox-boy" },
                { "Horse-girl", "Horse-boy" },
                { "Rabbit-girl", "Rabbit-boy" },
                { "Wolf-girl", "Wolf-boy" }
            };

            for (int i = 0; i < 8; ++i)
            {
                float rowH = 44.0f * uiScale;
                SDL_FRect rowRect = { padX, curY, availableW, rowH };
                UIWidget::drawPanel(renderer, rowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, "ⓘ", padX + (8.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                UIWidget::drawText(renderer, races[i].femaleName, padX + (28.0f * uiScale), curY + (5.0f * uiScale), SDL_Color{ 255, 120, 180, 255 }, uiScale * 0.85f);
                UIWidget::drawText(renderer, races[i].maleName, padX + (28.0f * uiScale), curY + (23.0f * uiScale), SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.85f);

                float pW = 20.0f * uiScale;
                float pH = 16.0f * uiScale;
                float group1X = headerColsX;
                float group2X = headerColsX + (150.0f * uiScale);

                // Row 1 (Girl):
                for (int p = 0; p < 5; ++p)
                {
                    SDL_FRect pr1 = { group1X + (p * (pW + 2.0f * uiScale)), curY + (4.0f * uiScale), pW, pH };
                    bool sel1 = (p == 3);
                    SDL_Color bCol = sel1 ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                    UIWidget::drawColoredButton(renderer, pr1, std::to_string(p), bCol, sel1 ? Theme::colors.textPrimary : Theme::colors.textMuted, sel1, uiScale * 0.65f);

                    SDL_FRect pr2 = { group2X + (p * (pW + 2.0f * uiScale)), curY + (4.0f * uiScale), pW, pH };
                    bool sel2 = (p == 4);
                    SDL_Color bCol2 = sel2 ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                    UIWidget::drawColoredButton(renderer, pr2, std::to_string(p), bCol2, sel2 ? Theme::colors.textPrimary : Theme::colors.textMuted, sel2, uiScale * 0.65f);
                }

                // Row 2 (Boy):
                for (int p = 0; p < 5; ++p)
                {
                    SDL_FRect pr1 = { group1X + (p * (pW + 2.0f * uiScale)), curY + (22.0f * uiScale), pW, pH };
                    bool sel1 = (p == 3);
                    SDL_Color bCol = sel1 ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                    UIWidget::drawColoredButton(renderer, pr1, std::to_string(p), bCol, sel1 ? Theme::colors.textPrimary : Theme::colors.textMuted, sel1, uiScale * 0.65f);

                    SDL_FRect pr2 = { group2X + (p * (pW + 2.0f * uiScale)), curY + (22.0f * uiScale), pW, pH };
                    bool sel2 = (p == 4);
                    SDL_Color bCol2 = sel2 ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                    UIWidget::drawColoredButton(renderer, pr2, std::to_string(p), bCol2, sel2 ? Theme::colors.textPrimary : Theme::colors.textMuted, sel2, uiScale * 0.65f);
                }

                curY += rowH + (6.0f * uiScale);
            }
        }
        else if (opt->contentCategory == ContentOptionsCategory::FETISH_PREFS)
        {
            renderInfoDropdown();

            static const char* fetishes[10] = {
                "Anal", "Buttslut", "Vaginal", "Pussy slut", "Oral",
                "Oral performer", "Breasts lover", "Breasts", "Milk lover", "Lactation"
            };

            static const std::vector<std::string> fetPills = {
                "Disabled", "Hate", "Dislike", "Neutral", "Like", "Love", "Always"
            };

            for (int i = 0; i < 10; ++i)
            {
                float cardWidth = availableW;
                float rowMinH = 34.0f * uiScale;
                SDL_FRect rowRect = { padX, curY, cardWidth, rowMinH };
                UIWidget::drawPanel(renderer, rowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, "ⓘ", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                UIWidget::drawText(renderer, fetishes[i], padX + (28.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);

                float pillTotalW = 340.0f * uiScale;
                float pillH = 22.0f * uiScale;
                float pillItemW = (pillTotalW - (6.0f * 4.0f * uiScale)) / 7.0f;
                float pillStartX = padX + cardWidth - pillTotalW - (10.0f * uiScale);
                float pillY = curY + (6.0f * uiScale);

                for (size_t p = 0; p < fetPills.size(); ++p)
                {
                    SDL_FRect pRect = { pillStartX + (p * (pillItemW + 4.0f * uiScale)), pillY, pillItemW, pillH };
                    bool isNeutral = (p == 3);

                    SDL_Color bgCol = isNeutral ? SDL_Color{ 75, 24, 55, 255 } : Theme::colors.bgButton;
                    SDL_Color borderCol = isNeutral ? SDL_Color{ 245, 80, 175, 255 } : Theme::colors.borderButton;
                    SDL_Color pTextCol = isNeutral ? SDL_Color{ 255, 220, 245, 255 } : Theme::colors.textMuted;

                    SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                    SDL_RenderFillRect(renderer, &pRect);
                    SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                    SDL_RenderRect(renderer, &pRect);

                    float labelW = fetPills[p].size() * (5.5f * uiScale);
                    UIWidget::drawText(renderer, fetPills[p], pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), pTextCol, uiScale * 0.72f);
                }

                curY += rowMinH + (6.0f * uiScale);
            }
        }
        else
        {
            renderFrequencyRow("Sub-category Tuning", Theme::colors.textGold, "Fine-tune generation frequencies and preference distributions.", 3, [](int idx) {});
        }

        return (curY - startY);
    }
}

float uiRenderer::renderCharacterCreationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    auto cc = dynamic_cast<characterCreationState*>(gameContext->getActiveState());
    if (!cc) return 0.0f;

    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);
    float centerX = rect.x + (rect.w / 2.0f);

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    // Scene Title: "A Night Out"
    std::string sceneTitle = "A Night Out";
    float titleW = sceneTitle.size() * (10.0f * uiScale);
    UIWidget::drawText(renderer, sceneTitle, centerX - (titleW / 2.0f), curY, SDL_Color{ 255, 240, 200, 255 }, uiScale * 1.15f);
    curY += (28.0f * uiScale);

    if (cc->step == 0) // Part 1: Start Date, Gender, Femininity
    {
        std::string story1 = "By the time the taxi finally pulls up to the British Museum, you're already almost five minutes late. The whole reason you're visiting London is to attend your aunt Lily's opening evening for her new exhibition, and as you hurriedly pay the driver his fare and step out of the car, you hope that she hasn't started her speech yet.";
        float h1 = UIWidget::drawTextWrapped(renderer, story1, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += h1 + (14.0f * uiScale);

        std::string story2 = "The street lights flicker into life as you rush over to the entrance, illuminating your surroundings with a dull orange glow. It only takes a moment before you're standing at the museum's front doors, where, much to your dismay, you see that a small queue has formed. Having no choice but to step in line and wait your turn, you briefly glance over at the large glass windows of the building's modern facade to see your blurry reflection in the glass...";
        float h2 = UIWidget::drawTextWrapped(renderer, story2, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += h2 + (20.0f * uiScale);

        // Section 1: Start Date
        UIWidget::drawText(renderer, "Start Date", centerX - (35.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (18.0f * uiScale);
        UIWidget::drawText(renderer, "Select the month in which the game starts.", centerX - (115.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.85f);
        curY += (20.0f * uiScale);

        static const char* months[12] = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };

        float monthBtnW = (availableW - (5 * 8.0f * uiScale)) / 6.0f;
        float monthBtnH = 22.0f * uiScale;

        for (int m = 0; m < 12; ++m)
        {
            int r = m / 6;
            int c = m % 6;
            SDL_FRect mRect = { padX + (c * (monthBtnW + 8.0f * uiScale)), curY + (r * (monthBtnH + 6.0f * uiScale)), monthBtnW, monthBtnH };
            bool isSel = (cc->startMonth == months[m]);
            bool mHover = (mousePos.x >= mRect.x && mousePos.x <= mRect.x + mRect.w &&
                           mousePos.y >= mRect.y && mousePos.y <= mRect.y + mRect.h);

            SDL_Color bg = isSel ? SDL_Color{ 45, 55, 65, 255 } : (mHover ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgButton);
            SDL_Color border = isSel ? Theme::colors.companion : (mHover ? Theme::colors.textGold : Theme::colors.borderButton);
            SDL_Color txt = isSel ? Theme::colors.companion : (mHover ? Theme::colors.textGold : Theme::colors.textMuted);

            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            SDL_RenderFillRect(renderer, &mRect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &mRect);

            float labelW = std::string(months[m]).size() * (6.0f * uiScale);
            UIWidget::drawText(renderer, months[m], mRect.x + ((mRect.w - labelW) / 2.0f), mRect.y + (3.0f * uiScale), txt, uiScale * 0.78f);

            if (mHover && clicked)
            {
                cc->startMonth = months[m];
                cc->startMonthIdx = m;
                gameContext->input.consumeMouseClick();
            }
        }
        curY += (2 * (monthBtnH + 6.0f * uiScale)) + (16.0f * uiScale);

        // Split Row: Gender (Left) & Femininity (Right)
        float halfW = (availableW - (16.0f * uiScale)) / 2.0f;
        float splitY = curY;

        // Gender Card (Left)
        SDL_FRect genRect = { padX, splitY, halfW, 80.0f * uiScale };
        UIWidget::drawPanel(renderer, genRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Gender", genRect.x + ((genRect.w - (25.0f * uiScale)) / 2.0f), splitY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);
        UIWidget::drawTextWrapped(renderer, "Your gender is used to determine what genitals you start the game with.", genRect.x + (8.0f * uiScale), splitY + (22.0f * uiScale), halfW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

        static const char* genders[2] = { "Male", "Female" };
        float gBtnW = (halfW - (24.0f * uiScale)) / 2.0f;
        for (int g = 0; g < 2; ++g)
        {
            SDL_FRect gbRect = { genRect.x + (8.0f * uiScale) + (g * (gBtnW + 8.0f * uiScale)), splitY + (50.0f * uiScale), gBtnW, 22.0f * uiScale };
            bool isSel = (cc->gender == genders[g]);
            bool gHover = (mousePos.x >= gbRect.x && mousePos.x <= gbRect.x + gbRect.w &&
                           mousePos.y >= gbRect.y && mousePos.y <= gbRect.y + gbRect.h);

            SDL_Color bg = isSel ? SDL_Color{ 45, 55, 65, 255 } : (gHover ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgButton);
            SDL_Color border = isSel ? (g == 0 ? SDL_Color{ 100, 160, 255, 255 } : SDL_Color{ 255, 120, 180, 255 }) : (gHover ? Theme::colors.textGold : Theme::colors.borderButton);
            SDL_Color txt = isSel ? Theme::colors.textPrimary : Theme::colors.textMuted;

            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            SDL_RenderFillRect(renderer, &gbRect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &gbRect);

            float labelW = std::string(genders[g]).size() * (6.5f * uiScale);
            UIWidget::drawText(renderer, genders[g], gbRect.x + ((gbRect.w - labelW) / 2.0f), gbRect.y + (3.0f * uiScale), txt, uiScale * 0.8f);

            if (gHover && clicked)
            {
                cc->gender = genders[g];
                gameContext->input.consumeMouseClick();
            }
        }

        // Femininity Card (Right)
        SDL_FRect femRect = { padX + halfW + (16.0f * uiScale), splitY, halfW, 80.0f * uiScale };
        UIWidget::drawPanel(renderer, femRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Femininity", femRect.x + ((femRect.w - (35.0f * uiScale)) / 2.0f), splitY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);
        UIWidget::drawTextWrapped(renderer, "Femininity is a measure of how masculine or feminine your face and body are.", femRect.x + (8.0f * uiScale), splitY + (22.0f * uiScale), halfW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

        static const char* femOpts[3] = { "Androgynous", "Masculine", "Very Masculine" };
        float fBtnW = (halfW - (28.0f * uiScale)) / 3.0f;
        for (int f = 0; f < 3; ++f)
        {
            SDL_FRect fbRect = { femRect.x + (8.0f * uiScale) + (f * (fBtnW + 6.0f * uiScale)), splitY + (50.0f * uiScale), fBtnW, 22.0f * uiScale };
            bool isSel = (cc->femininity == femOpts[f]);
            bool fHover = (mousePos.x >= fbRect.x && mousePos.x <= fbRect.x + fbRect.w &&
                           mousePos.y >= fbRect.y && mousePos.y <= fbRect.y + fbRect.h);

            SDL_Color bg = isSel ? SDL_Color{ 45, 55, 65, 255 } : (fHover ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgButton);
            SDL_Color border = isSel ? SDL_Color{ 100, 160, 255, 255 } : (fHover ? Theme::colors.textGold : Theme::colors.borderButton);
            SDL_Color txt = isSel ? (f == 0 ? SDL_Color{ 190, 130, 255, 255 } : SDL_Color{ 100, 160, 255, 255 }) : Theme::colors.textMuted;

            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            SDL_RenderFillRect(renderer, &fbRect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &fbRect);

            float labelW = std::string(femOpts[f]).size() * (5.0f * uiScale);
            UIWidget::drawText(renderer, femOpts[f], fbRect.x + ((fbRect.w - labelW) / 2.0f), fbRect.y + (3.0f * uiScale), txt, uiScale * 0.72f);

            if (fHover && clicked)
            {
                cc->femininity = femOpts[f];
                gameContext->input.consumeMouseClick();
            }
        }
        curY += (80.0f * uiScale) + (16.0f * uiScale);
    }
    else if (cc->step == 1) // Part 2: Birthday, Sexual Orientation, Personality
    {
        std::string refText = std::format("You will be referred to as a {}.", (cc->gender == "Female" ? "female" : "male"));
        float refW = refText.size() * (7.0f * uiScale);
        UIWidget::drawText(renderer, refText, centerX - (refW / 2.0f), curY, SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.9f);
        curY += (18.0f * uiScale);

        std::string optNote = "You can change all gender names in the options menu.";
        float noteW = optNote.size() * (6.0f * uiScale);
        UIWidget::drawText(renderer, optNote, centerX - (noteW / 2.0f), curY, Theme::colors.textMuted, uiScale * 0.8f);
        curY += (24.0f * uiScale);

        // Card 1: Birthday
        float bdayH = 75.0f * uiScale;
        SDL_FRect bdayRect = { padX, curY, availableW, bdayH };
        UIWidget::drawPanel(renderer, bdayRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Birthday", bdayRect.x + ((bdayRect.w - (40.0f * uiScale)) / 2.0f), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);

        std::string bdayDesc = std::format("You were born on the {}th {} {}, making you {} years old.", cc->birthDay, cc->birthMonth, 2019 - cc->birthAge, cc->birthAge);
        float bDescW = bdayDesc.size() * (6.5f * uiScale);
        UIWidget::drawText(renderer, bdayDesc, bdayRect.x + ((bdayRect.w - bDescW) / 2.0f), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

        float colW = availableW / 3.0f;
        float stepY = curY + (46.0f * uiScale);

        // Day Stepper
        UIWidget::drawText(renderer, "Day", padX + (colW * 0.1f), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        SDL_FRect dM2 = { padX + (colW * 0.3f), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect dM1 = { padX + (colW * 0.3f) + (22.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect dP1 = { padX + (colW * 0.3f) + (70.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect dP2 = { padX + (colW * 0.3f) + (92.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };

        UIWidget::drawColoredButton(renderer, dM2, "--", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, dM1, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawText(renderer, std::to_string(cc->birthDay), padX + (colW * 0.3f) + (46.0f * uiScale), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        UIWidget::drawColoredButton(renderer, dP1, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, dP2, "++", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);

        if (clicked)
        {
            auto checkClick = [&](const SDL_FRect& r) {
                return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h);
            };
            if (checkClick(dM2)) { cc->birthDay = std::max(1, cc->birthDay - 5); gameContext->input.consumeMouseClick(); }
            else if (checkClick(dM1)) { cc->birthDay = std::max(1, cc->birthDay - 1); gameContext->input.consumeMouseClick(); }
            else if (checkClick(dP1)) { cc->birthDay = std::min(31, cc->birthDay + 1); gameContext->input.consumeMouseClick(); }
            else if (checkClick(dP2)) { cc->birthDay = std::min(31, cc->birthDay + 5); gameContext->input.consumeMouseClick(); }
        }

        // Month Stepper
        static const char* allMonths[12] = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        UIWidget::drawText(renderer, "Month", padX + colW + (colW * 0.05f), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        SDL_FRect mM2 = { padX + colW + (colW * 0.32f), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect mM1 = { padX + colW + (colW * 0.32f) + (22.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect mP1 = { padX + colW + (colW * 0.32f) + (90.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect mP2 = { padX + colW + (colW * 0.32f) + (112.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };

        UIWidget::drawColoredButton(renderer, mM2, "--", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, mM1, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawText(renderer, cc->birthMonth, padX + colW + (colW * 0.32f) + (46.0f * uiScale), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        UIWidget::drawColoredButton(renderer, mP1, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, mP2, "++", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);

        if (clicked)
        {
            auto checkClick = [&](const SDL_FRect& r) {
                return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h);
            };
            if (checkClick(mM2)) { cc->birthMonthIdx = (cc->birthMonthIdx + 10) % 12; cc->birthMonth = allMonths[cc->birthMonthIdx]; gameContext->input.consumeMouseClick(); }
            else if (checkClick(mM1)) { cc->birthMonthIdx = (cc->birthMonthIdx + 11) % 12; cc->birthMonth = allMonths[cc->birthMonthIdx]; gameContext->input.consumeMouseClick(); }
            else if (checkClick(mP1)) { cc->birthMonthIdx = (cc->birthMonthIdx + 1) % 12; cc->birthMonth = allMonths[cc->birthMonthIdx]; gameContext->input.consumeMouseClick(); }
            else if (checkClick(mP2)) { cc->birthMonthIdx = (cc->birthMonthIdx + 2) % 12; cc->birthMonth = allMonths[cc->birthMonthIdx]; gameContext->input.consumeMouseClick(); }
        }

        // Age Stepper
        UIWidget::drawText(renderer, "Age", padX + (colW * 2.0f) + (colW * 0.1f), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        SDL_FRect aM2 = { padX + (colW * 2.0f) + (colW * 0.3f), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect aM1 = { padX + (colW * 2.0f) + (colW * 0.3f) + (22.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect aP1 = { padX + (colW * 2.0f) + (colW * 0.3f) + (70.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };
        SDL_FRect aP2 = { padX + (colW * 2.0f) + (colW * 0.3f) + (92.0f * uiScale), stepY, 18.0f * uiScale, 18.0f * uiScale };

        UIWidget::drawColoredButton(renderer, aM2, "--", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, aM1, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawText(renderer, std::to_string(cc->birthAge), padX + (colW * 2.0f) + (colW * 0.3f) + (46.0f * uiScale), stepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        UIWidget::drawColoredButton(renderer, aP1, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);
        UIWidget::drawColoredButton(renderer, aP2, "++", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.7f);

        if (clicked)
        {
            auto checkClick = [&](const SDL_FRect& r) {
                return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h);
            };
            if (checkClick(aM2)) { cc->birthAge = std::max(18, cc->birthAge - 5); gameContext->input.consumeMouseClick(); }
            else if (checkClick(aM1)) { cc->birthAge = std::max(18, cc->birthAge - 1); gameContext->input.consumeMouseClick(); }
            else if (checkClick(aP1)) { cc->birthAge = std::min(99, cc->birthAge + 1); gameContext->input.consumeMouseClick(); }
            else if (checkClick(aP2)) { cc->birthAge = std::min(99, cc->birthAge + 5); gameContext->input.consumeMouseClick(); }
        }

        curY += bdayH + (14.0f * uiScale);

        // Card 2: Sexual Orientation
        float oriH = 75.0f * uiScale;
        SDL_FRect oriRect = { padX, curY, availableW, oriH };
        UIWidget::drawPanel(renderer, oriRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Sexual Orientation", oriRect.x + ((oriRect.w - (75.0f * uiScale)) / 2.0f), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);
        UIWidget::drawText(renderer, "Sexual orientation is determined by your attraction towards femininity or masculinity.", oriRect.x + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);
        UIWidget::drawText(renderer, "Hover over the orientation icon in your character's status effects panel to see the effects.", oriRect.x + (12.0f * uiScale), curY + (36.0f * uiScale), Theme::colors.textMuted, uiScale * 0.78f);

        static const char* orientations[3] = { "Androphilic", "Ambiphilic", "Gynephilic" };
        float oBtnW = 75.0f * uiScale;
        float oStartX = padX + ((availableW - (3 * (oBtnW + 8.0f * uiScale))) / 2.0f);
        for (int o = 0; o < 3; ++o)
        {
            SDL_FRect obRect = { oStartX + (o * (oBtnW + 8.0f * uiScale)), curY + (50.0f * uiScale), oBtnW, 20.0f * uiScale };
            bool isSel = (cc->orientation == orientations[o]);
            bool oHover = (mousePos.x >= obRect.x && mousePos.x <= obRect.x + obRect.w &&
                           mousePos.y >= obRect.y && mousePos.y <= obRect.y + obRect.h);

            SDL_Color bg = isSel ? SDL_Color{ 45, 55, 65, 255 } : (oHover ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgButton);
            SDL_Color border = isSel ? SDL_Color{ 190, 130, 255, 255 } : (oHover ? Theme::colors.textGold : Theme::colors.borderButton);
            SDL_Color txt = isSel ? Theme::colors.textPrimary : Theme::colors.textMuted;

            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            SDL_RenderFillRect(renderer, &obRect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &obRect);

            float labelW = std::string(orientations[o]).size() * (6.0f * uiScale);
            UIWidget::drawText(renderer, orientations[o], obRect.x + ((obRect.w - labelW) / 2.0f), obRect.y + (3.0f * uiScale), txt, uiScale * 0.75f);

            if (oHover && clicked)
            {
                cc->orientation = orientations[o];
                gameContext->input.consumeMouseClick();
            }
        }
        curY += oriH + (14.0f * uiScale);

        // Card 3: Personality
        float persH = 96.0f * uiScale;
        SDL_FRect persRect = { padX, curY, availableW, persH };
        UIWidget::drawPanel(renderer, persRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Personality", persRect.x + ((persRect.w - (50.0f * uiScale)) / 2.0f), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);
        UIWidget::drawTextWrapped(renderer, "Your personality will have a minor influence in some situations. It will not lock out any options during the game, and is more for roleplaying purposes.", persRect.x + (12.0f * uiScale), curY + (22.0f * uiScale), availableW - (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);

        static const std::vector<std::string> traits = {
            "Confident", "Shy", "Kind", "Selfish", "Naive", "Cynical",
            "Brave", "Cowardly", "Lewd", "Innocent", "Prude", "Lisp",
            "Stutter", "Slovenly"
        };

        float tBtnW = (availableW - (7 * 6.0f * uiScale)) / 6.0f;
        float tBtnH = 18.0f * uiScale;
        float tGridY = curY + (44.0f * uiScale);

        for (size_t t = 0; t < traits.size(); ++t)
        {
            int r = t / 6;
            int c = t % 6;
            if (r == 2) c += 2;
            SDL_FRect tbRect = { padX + (6.0f * uiScale) + (c * (tBtnW + 5.0f * uiScale)), tGridY + (r * (tBtnH + 4.0f * uiScale)), tBtnW, tBtnH };
            bool isSel = (cc->personalityTraits.find(traits[t]) != cc->personalityTraits.end());
            bool tHover = (mousePos.x >= tbRect.x && mousePos.x <= tbRect.x + tbRect.w &&
                           mousePos.y >= tbRect.y && mousePos.y <= tbRect.y + tbRect.h);

            SDL_Color bg = isSel ? SDL_Color{ 45, 55, 65, 255 } : (tHover ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgButton);
            SDL_Color border = isSel ? Theme::colors.companion : (tHover ? Theme::colors.textGold : Theme::colors.borderButton);
            SDL_Color txt = isSel ? Theme::colors.companion : Theme::colors.textMuted;

            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            SDL_RenderFillRect(renderer, &tbRect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &tbRect);

            float labelW = traits[t].size() * (5.5f * uiScale);
            UIWidget::drawText(renderer, traits[t], tbRect.x + ((tbRect.w - labelW) / 2.0f), tbRect.y + (2.0f * uiScale), txt, uiScale * 0.72f);

            if (tHover && clicked)
            {
                if (isSel) cc->personalityTraits.erase(traits[t]);
                else cc->personalityTraits.insert(traits[t]);
                gameContext->input.consumeMouseClick();
            }
        }
        curY += persH + (16.0f * uiScale);
    }
    else if (cc->step == 2) // Part 3: Names, Surname
    {
        std::string line1 = "\"Sir,\" the doorman calls out to you, evidently having finished with the other people in the queue, \"do you have an invitation?\"";
        float l1 = UIWidget::drawTextWrapped(renderer, line1, padX, curY, availableW, SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.9f);
        curY += l1 + (12.0f * uiScale);

        std::string line2 = "You turn away from the glass and step forwards, smiling. \"Yes, I have it right here... erm... hold on...\"";
        float l2 = UIWidget::drawTextWrapped(renderer, line2, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += l2 + (12.0f * uiScale);

        std::string line3 = "Reaching into your pocket, you feel your heart start to race as you discover that the invitation isn't in there. \"No, no, no! I must have left it in the taxi!\"";
        float l3 = UIWidget::drawTextWrapped(renderer, line3, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += l3 + (12.0f * uiScale);

        std::string line4 = "\"Well, don't worry,\" the man replies, \"if you give me your name, I can check to make sure that you're on the list.\"";
        float l4 = UIWidget::drawTextWrapped(renderer, line4, padX, curY, availableW, SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.9f);
        curY += l4 + (12.0f * uiScale);

        std::string line5 = "Breathing a sigh of relief, you tell the man your name...";
        float l5 = UIWidget::drawTextWrapped(renderer, line5, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
        curY += l5 + (18.0f * uiScale);

        // Name Entry Card
        float cardH = 110.0f * uiScale;
        SDL_FRect nameCardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, nameCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        std::string expl = "Your first name can be set as three values; your masculine name, androgynous name, and feminine name. Your name will automatically switch to the one which corresponds to your body femininity.";
        float eH = UIWidget::drawTextWrapped(renderer, expl, padX + (12.0f * uiScale), curY + (6.0f * uiScale), availableW - (24.0f * uiScale), Theme::colors.textMuted, uiScale * 0.78f);

        float fieldY = curY + eH + (10.0f * uiScale);

        UIWidget::drawText(renderer, "First name:", padX + (12.0f * uiScale), fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);

        float inputW = (availableW - (110.0f * uiScale)) / 3.0f;
        float inputH = 22.0f * uiScale;
        float inStartX = padX + (90.0f * uiScale);

        struct NameBox { std::string val; SDL_Color col; int id; };
        NameBox boxes[3] = {
            { cc->masculineName, SDL_Color{ 100, 160, 255, 255 }, 0 },
            { cc->androgynousName, SDL_Color{ 190, 130, 255, 255 }, 1 },
            { cc->feminineName, SDL_Color{ 255, 120, 180, 255 }, 2 }
        };

        for (int b = 0; b < 3; ++b)
        {
            SDL_FRect boxRect = { inStartX + (b * (inputW + 6.0f * uiScale)), fieldY, inputW, inputH };
            bool isFocused = (cc->activeNameField == boxes[b].id);
            bool bHover = (mousePos.x >= boxRect.x && mousePos.x <= boxRect.x + boxRect.w &&
                           mousePos.y >= boxRect.y && mousePos.y <= boxRect.y + boxRect.h);

            SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
            SDL_RenderFillRect(renderer, &boxRect);

            SDL_Color border = isFocused ? Theme::colors.textGold : (bHover ? Theme::colors.borderButton : Theme::colors.borderNormal);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderRect(renderer, &boxRect);

            UIWidget::drawText(renderer, boxes[b].val, boxRect.x + (6.0f * uiScale), boxRect.y + (3.0f * uiScale), boxes[b].col, uiScale * 0.82f);

            if (bHover && clicked)
            {
                cc->activeNameField = boxes[b].id;
                gameContext->input.consumeMouseClick();
            }
        }

        fieldY += inputH + (8.0f * uiScale);

        UIWidget::drawText(renderer, "Surname:", padX + (12.0f * uiScale), fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
        SDL_FRect surRect = { inStartX, fieldY, inputW, inputH };
        bool surFocused = (cc->activeNameField == 3);
        bool surHover = (mousePos.x >= surRect.x && mousePos.x <= surRect.x + surRect.w &&
                         mousePos.y >= surRect.y && mousePos.y <= surRect.y + surRect.h);

        SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
        SDL_RenderFillRect(renderer, &surRect);
        SDL_Color surBorder = surFocused ? Theme::colors.textGold : (surHover ? Theme::colors.borderButton : Theme::colors.borderNormal);
        SDL_SetRenderDrawColor(renderer, surBorder.r, surBorder.g, surBorder.b, surBorder.a);
        SDL_RenderRect(renderer, &surRect);

        UIWidget::drawText(renderer, cc->surname, surRect.x + (6.0f * uiScale), surRect.y + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
        if (surHover && clicked)
        {
            cc->activeNameField = 3;
            gameContext->input.consumeMouseClick();
        }

        curY += cardH + (12.0f * uiScale);

        std::string footNote = "Your name must be between 2 and 32 characters long. You cannot use the square bracket characters or full stops. (Surname may be left blank.)";
        UIWidget::drawTextWrapped(renderer, footNote, padX, curY, availableW, Theme::colors.textMuted, uiScale * 0.78f);
        curY += (20.0f * uiScale);
    }
    else if (cc->step == 3) // Part 4: Customization ("In the Museum" / "Core Body Appearance")
    {
        if (cc->subView == 0) // In the Museum (Overview)
        {
            std::string dispName = cc->masculineName;
            if (cc->femininity == "Androgynous") dispName = cc->androgynousName;
            else if (cc->femininity == "Feminine" || cc->femininity == "Very Feminine") dispName = cc->feminineName;
            if (dispName.empty() || dispName == "Unknown") dispName = "Rudy";

            // Overview Section
            UIWidget::drawText(renderer, "Overview:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string ovText = std::format("You are {}, a boyish male human. Your shaft is concealed, so, due to your masculine appearance, everyone assumes that you're a male on first glance. Standing at full height, you measure {:.1f} metres. You appear to be in your early twenties.", dispName, cc->heightCm / 100.0f);
            float ovH = UIWidget::drawTextWrapped(renderer, ovText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += ovH + (14.0f * uiScale);

            // Face Section
            UIWidget::drawText(renderer, "Face:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string faceText = std::format("You have a {}, human face, covered in light, smooth skin. You have a head of {}, {}, human hair, which has been curled and left loose. You have a pair of normal, human eyes, with round, {} irises, round, black pupils, and white sclerae. You have a pair of normal, human ears, which are covered in light, smooth skin. You don't have any trace of facial hair.", cc->femininity == "Androgynous" ? "androgynous" : "masculine", cc->hairLength, cc->hairColor, cc->eyeColor);
            float faceH = UIWidget::drawTextWrapped(renderer, faceText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += faceH + (14.0f * uiScale);

            // Mouth Section
            UIWidget::drawText(renderer, "Mouth:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string mouthText = "You have average-sized, light lips. Your throat is flesh in colour. Your mouth holds a normal-sized, flesh tongue. You've never given head before, so are unsure of how much you could fit down your throat.";
            float mouthH = UIWidget::drawTextWrapped(renderer, mouthText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += mouthH + (14.0f * uiScale);

            // Torso Section
            UIWidget::drawText(renderer, "Torso:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string torsoText = std::format("Your torso has a boyish appearance, and is covered in light, smooth skin. You have an {}, {} body, giving you an average body shape.", cc->bodySize, cc->muscleDefinition);
            float torsoH = UIWidget::drawTextWrapped(renderer, torsoText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += torsoH + (14.0f * uiScale);

            // Chest Section
            UIWidget::drawText(renderer, "Chest:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string chestText = "You have a completely flat chest, with a single pair of pecs. On each of your pecs, you have one tiny, light nipple, with tiny, circular areolae. You are not producing any milk.";
            float chestH = UIWidget::drawTextWrapped(renderer, chestText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += chestH + (14.0f * uiScale);

            // Arms Section
            UIWidget::drawText(renderer, "Arms:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);
            std::string armsText = "You have a pair of normal human arms and hands, which are covered in light, smooth skin. You have a natural amount of brown, coarse hair in each of your armpits. Your arms are slightly muscled.";
            float armsH = UIWidget::drawTextWrapped(renderer, armsText, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += armsH + (16.0f * uiScale);
        }
        else if (cc->subView == 1) // Core Body Appearance (Image 2)
        {
            std::string sub = "All of these options can be influenced later on in the game.";
            float subW = sub.size() * (6.5f * uiScale);
            UIWidget::drawText(renderer, sub, centerX - (subW / 2.0f), curY, Theme::colors.textSecondary, uiScale * 0.85f);
            curY += (24.0f * uiScale);

            // Card 1: Height
            float hCardH = 48.0f * uiScale;
            SDL_FRect hRect = { padX, curY, availableW, hCardH };
            UIWidget::drawPanel(renderer, hRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Height", hRect.x + ((hRect.w - (35.0f * uiScale)) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "ⓘ", hRect.x + hRect.w - (22.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);

            float hBtnW = 42.0f * uiScale;
            float hBtnH = 20.0f * uiScale;
            float hMidX = hRect.x + (hRect.w / 2.0f);
            float hStepY = curY + (22.0f * uiScale);

            SDL_FRect hm5 = { hMidX - (110.0f * uiScale), hStepY, hBtnW, hBtnH };
            SDL_FRect hm1 = { hMidX - (60.0f * uiScale), hStepY, hBtnW, hBtnH };
            SDL_FRect hp1 = { hMidX + (20.0f * uiScale), hStepY, hBtnW, hBtnH };
            SDL_FRect hp5 = { hMidX + (70.0f * uiScale), hStepY, hBtnW, hBtnH };

            UIWidget::drawColoredButton(renderer, hm5, "-5cm", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, hm1, "-1cm", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);
            UIWidget::drawText(renderer, std::format("{}cm", cc->heightCm), hMidX - (16.0f * uiScale), hStepY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
            UIWidget::drawColoredButton(renderer, hp1, "+1cm", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, hp5, "+5cm", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);

            if (clicked)
            {
                auto checkClick = [&](const SDL_FRect& r) {
                    return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h);
                };
                if (checkClick(hm5)) { cc->heightCm = std::max(120, cc->heightCm - 5); gameContext->input.consumeMouseClick(); }
                else if (checkClick(hm1)) { cc->heightCm = std::max(120, cc->heightCm - 1); gameContext->input.consumeMouseClick(); }
                else if (checkClick(hp1)) { cc->heightCm = std::min(240, cc->heightCm + 1); gameContext->input.consumeMouseClick(); }
                else if (checkClick(hp5)) { cc->heightCm = std::min(240, cc->heightCm + 5); gameContext->input.consumeMouseClick(); }
            }
            curY += hCardH + (12.0f * uiScale);

            // Card 2: Skin Colour - Human
            float skinCardH = 118.0f * uiScale;
            SDL_FRect skinRect = { padX, curY, availableW, skinCardH };
            UIWidget::drawPanel(renderer, skinRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Skin Colour - Human", skinRect.x + ((skinRect.w - (110.0f * uiScale)) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "Light, smooth skin", skinRect.x + ((skinRect.w - (95.0f * uiScale)) / 2.0f), curY + (20.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);

            float sColW = (availableW - (16.0f * uiScale)) / 2.0f;
            float leftPanelX = padX + (8.0f * uiScale);
            float rightPanelX = padX + sColW + (8.0f * uiScale);
            float sContentY = curY + (38.0f * uiScale);

            // Left: Pattern & Modifiers
            UIWidget::drawText(renderer, "Pattern:", leftPanelX + (4.0f * uiScale), sContentY, Theme::colors.textPrimary, uiScale * 0.8f);
            static const char* patterns[3] = { "Plain", "Freckled (face)", "Freckled" };
            float patBtnW = 68.0f * uiScale;
            for (int p = 0; p < 3; ++p)
            {
                SDL_FRect pr = { leftPanelX + (55.0f * uiScale) + (p * (patBtnW + 4.0f * uiScale)), sContentY - (2.0f * uiScale), patBtnW, 18.0f * uiScale };
                bool isSel = (cc->skinPattern == patterns[p]);
                SDL_Color bg = isSel ? SDL_Color{ 45, 65, 55, 255 } : Theme::colors.bgButton;
                SDL_Color txt = isSel ? Theme::colors.companion : Theme::colors.textMuted;
                UIWidget::drawColoredButton(renderer, pr, patterns[p], bg, txt, isSel, uiScale * 0.68f);
                if (clicked && mousePos.x >= pr.x && mousePos.x <= pr.x + pr.w && mousePos.y >= pr.y && mousePos.y <= pr.y + pr.h)
                {
                    cc->skinPattern = patterns[p];
                    gameContext->input.consumeMouseClick();
                }
            }

            UIWidget::drawText(renderer, "Modifiers:", leftPanelX + (4.0f * uiScale), sContentY + (24.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.8f);
            UIWidget::drawText(renderer, "None Available", leftPanelX + (60.0f * uiScale), sContentY + (24.0f * uiScale), Theme::colors.textMuted, uiScale * 0.78f);

            // Right: Primary Colour | Light & Swatches
            UIWidget::drawText(renderer, "Primary Colour | Light", rightPanelX + (4.0f * uiScale), sContentY, Theme::colors.textPrimary, uiScale * 0.8f);

            static const SDL_Color swatches[8] = {
                { 250, 245, 240, 255 }, // White
                { 220, 245, 230, 255 }, // Light Greenish
                { 245, 235, 220, 255 }, // Cream
                { 240, 215, 195, 255 }, // Light Beige
                { 225, 195, 170, 255 }, // Beige
                { 205, 165, 135, 255 }, // Tan
                { 150, 105, 80, 255 },  // Dark Brown
                { 110, 115, 125, 255 }  // Grey
            };

            float swW = 16.0f * uiScale;
            float swH = 16.0f * uiScale;
            float swStartX = rightPanelX + (4.0f * uiScale);
            float swY = sContentY + (16.0f * uiScale);

            for (int s = 0; s < 8; ++s)
            {
                SDL_FRect sr = { swStartX + (s * (swW + 4.0f * uiScale)), swY, swW, swH };
                bool isSel = (cc->skinColorIdx == s);

                SDL_SetRenderDrawColor(renderer, swatches[s].r, swatches[s].g, swatches[s].b, 255);
                SDL_RenderFillRect(renderer, &sr);
                SDL_SetRenderDrawColor(renderer, isSel ? 245 : 40, isSel ? 80 : 40, isSel ? 175 : 40, 255);
                SDL_RenderRect(renderer, &sr);

                if (clicked && mousePos.x >= sr.x && mousePos.x <= sr.x + sr.w && mousePos.y >= sr.y && mousePos.y <= sr.y + sr.h)
                {
                    cc->skinColorIdx = s;
                    gameContext->input.consumeMouseClick();
                }
            }

            UIWidget::drawText(renderer, "Secondary Colour", rightPanelX + (4.0f * uiScale), swY + (22.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.8f);
            UIWidget::drawText(renderer, "None Available", rightPanelX + (110.0f * uiScale), swY + (22.0f * uiScale), Theme::colors.textMuted, uiScale * 0.78f);

            // Card Footer Buttons: [ Reset Changes ] [ Apply Changes ]
            float cfBtnW = 110.0f * uiScale;
            float cfBtnH = 18.0f * uiScale;
            float cfY = curY + skinCardH - (24.0f * uiScale);
            SDL_FRect rstBtn = { centerX - cfBtnW - (10.0f * uiScale), cfY, cfBtnW, cfBtnH };
            SDL_FRect appBtn = { centerX + (10.0f * uiScale), cfY, cfBtnW, cfBtnH };
            UIWidget::drawColoredButton(renderer, rstBtn, "Reset Changes", Theme::colors.bgButton, Theme::colors.textMuted, false, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, appBtn, "Apply Changes", Theme::colors.bgButton, Theme::colors.textMuted, false, uiScale * 0.72f);

            curY += skinCardH + (12.0f * uiScale);

            // Split Cards: Body Size (Left) & Muscle Definition (Right)
            float splitH = 75.0f * uiScale;
            SDL_FRect bsRect = { padX, curY, sColW, splitH };
            SDL_FRect mdRect = { padX + sColW + (8.0f * uiScale), curY, sColW, splitH };

            UIWidget::drawPanel(renderer, bsRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Body Size", bsRect.x + ((bsRect.w - (50.0f * uiScale)) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "ⓘ", bsRect.x + bsRect.w - (20.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);

            static const char* bodySizes[5] = { "Skinny", "Slender", "Average", "Large", "Huge" };
            float bsBtnW = (sColW - (24.0f * uiScale)) / 3.0f;
            float bsBtnH = 18.0f * uiScale;
            for (int i = 0; i < 5; ++i)
            {
                int r = i / 3;
                int c = i % 3;
                if (r == 1) c += 1;
                SDL_FRect bsr = { bsRect.x + (8.0f * uiScale) + (c * (bsBtnW + 4.0f * uiScale)), curY + (22.0f * uiScale) + (r * (bsBtnH + 4.0f * uiScale)), bsBtnW, bsBtnH };
                bool isSel = (cc->bodySize == bodySizes[i]);
                SDL_Color bg = isSel ? SDL_Color{ 50, 45, 35, 255 } : Theme::colors.bgButton;
                SDL_Color txt = isSel ? Theme::colors.textGold : Theme::colors.textMuted;
                UIWidget::drawColoredButton(renderer, bsr, bodySizes[i], bg, txt, isSel, uiScale * 0.7f);
                if (clicked && mousePos.x >= bsr.x && mousePos.x <= bsr.x + bsr.w && mousePos.y >= bsr.y && mousePos.y <= bsr.y + bsr.h)
                {
                    cc->bodySize = bodySizes[i];
                    gameContext->input.consumeMouseClick();
                }
            }

            UIWidget::drawPanel(renderer, mdRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Muscle Definition", mdRect.x + ((mdRect.w - (95.0f * uiScale)) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "ⓘ", mdRect.x + mdRect.w - (20.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);

            static const char* muscles[5] = { "Soft", "Lightly muscled", "Toned", "Muscular", "Ripped" };
            float mdBtnW = (sColW - (24.0f * uiScale)) / 3.0f;
            for (int i = 0; i < 5; ++i)
            {
                int r = i / 3;
                int c = i % 3;
                if (r == 1) c += 1;
                SDL_FRect mdr = { mdRect.x + (8.0f * uiScale) + (c * (mdBtnW + 4.0f * uiScale)), curY + (22.0f * uiScale) + (r * (bsBtnH + 4.0f * uiScale)), mdBtnW, bsBtnH };
                bool isSel = (cc->muscleDefinition == muscles[i]);
                SDL_Color bg = isSel ? SDL_Color{ 45, 60, 65, 255 } : Theme::colors.bgButton;
                SDL_Color txt = isSel ? Theme::colors.companion : Theme::colors.textMuted;
                UIWidget::drawColoredButton(renderer, mdr, muscles[i], bg, txt, isSel, uiScale * 0.65f);
                if (clicked && mousePos.x >= mdr.x && mousePos.x <= mdr.x + mdr.w && mousePos.y >= mdr.y && mousePos.y <= mdr.y + mdr.h)
                {
                    cc->muscleDefinition = muscles[i];
                    gameContext->input.consumeMouseClick();
                }
            }

            curY += splitH + (14.0f * uiScale);

            // Summary Text
            std::string sumTitle = "Your muscle and body size values result in your appearance being:";
            float sumTW = sumTitle.size() * (6.5f * uiScale);
            UIWidget::drawText(renderer, sumTitle, centerX - (sumTW / 2.0f), curY, Theme::colors.textSecondary, uiScale * 0.82f);
            curY += (18.0f * uiScale);

            std::string sumVal = "Average";
            float sumVW = sumVal.size() * (8.0f * uiScale);
            UIWidget::drawText(renderer, sumVal, centerX - (sumVW / 2.0f), curY, Theme::colors.textGold, uiScale * 1.0f);
            curY += (24.0f * uiScale);
        }
        else // Other sub-views
        {
            UIWidget::drawText(renderer, "Appearance Sub-Options", centerX - (80.0f * uiScale), curY, Theme::colors.textGold, uiScale * 1.0f);
            curY += (24.0f * uiScale);
            UIWidget::drawTextWrapped(renderer, "Select individual anatomical preferences or fine-tune cosmetic features before continuing with the prologue.", padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += (30.0f * uiScale);
        }
    }
    else if (cc->step == 4) // Part 5: "Evening's Attire" (Image 3 media_1788047076983.png)
    {
        // Dual Grid Panels: Your Inventory | Page 1 & Your wardrobe | Page 1
        float gridSectionH = 148.0f * uiScale;
        float halfGridW = (availableW - (12.0f * uiScale)) / 2.0f;

        // Left Container: Your Inventory
        SDL_FRect invRect = { padX, curY, halfGridW, gridSectionH };
        UIWidget::drawPanel(renderer, invRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Your Inventory | Page 1", invRect.x + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.companion, uiScale * 0.85f);

        // 5 Vertical bag tabs
        float bagTabW = 20.0f * uiScale;
        float bagTabH = 18.0f * uiScale;
        for (int b = 0; b < 5; ++b)
        {
            SDL_FRect btr = { padX + (8.0f * uiScale), curY + (24.0f * uiScale) + (b * (bagTabH + 3.0f * uiScale)), bagTabW, bagTabH };
            bool isBagSel = (cc->activeBagSlot == b);
            SDL_SetRenderDrawColor(renderer, 24, 26, 32, 255);
            SDL_RenderFillRect(renderer, &btr);
            SDL_SetRenderDrawColor(renderer, isBagSel ? 245 : 55, isBagSel ? 80 : 60, isBagSel ? 175 : 72, 255);
            SDL_RenderRect(renderer, &btr);
            UIWidget::drawText(renderer, "🎒", btr.x + (2.0f * uiScale), btr.y + (1.0f * uiScale), isBagSel ? Theme::colors.textPrimary : Theme::colors.textMuted, uiScale * 0.7f);
        }

        // 5x5 Inventory Grid Slots
        float slotW = (halfGridW - (44.0f * uiScale)) / 5.0f;
        float slotH = 18.0f * uiScale;
        float invGridStartX = padX + (34.0f * uiScale);

        for (int r = 0; r < 5; ++r)
        {
            for (int c = 0; c < 5; ++c)
            {
                SDL_FRect isr = { invGridStartX + (c * (slotW + 2.0f * uiScale)), curY + (24.0f * uiScale) + (r * (slotH + 3.0f * uiScale)), slotW, slotH };
                SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
                SDL_RenderFillRect(renderer, &isr);
                SDL_SetRenderDrawColor(renderer, 45, 48, 58, 255);
                SDL_RenderRect(renderer, &isr);
            }
        }

        // Left Footer: ★0 ¤0 > >>
        float fY = curY + gridSectionH - (18.0f * uiScale);
        UIWidget::drawText(renderer, "★0", padX + (12.0f * uiScale), fY, Theme::colors.arcane, uiScale * 0.78f);
        UIWidget::drawText(renderer, "¤0", padX + (60.0f * uiScale), fY, Theme::colors.textGold, uiScale * 0.78f);
        UIWidget::drawText(renderer, "> >>", padX + halfGridW - (35.0f * uiScale), fY, Theme::colors.textMuted, uiScale * 0.75f);

        // Right Container: Your wardrobe
        SDL_FRect wardRect = { padX + halfGridW + (12.0f * uiScale), curY, halfGridW, gridSectionH };
        UIWidget::drawPanel(renderer, wardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Your wardrobe | Page 1", wardRect.x + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        static const char* wardrobeIcons[18] = {
            "👕", "👓", "👖", "🧥", "🧣", "🥾", "🧥",
            "🧢", "👖", "🕶", "👟", "🧥", "👔", "👔",
            "🧤", "🩲", "👔", "👟"
        };

        float wSlotW = (halfGridW - (18.0f * uiScale)) / 7.0f;
        float wGridStartX = wardRect.x + (6.0f * uiScale);

        for (int i = 0; i < 18; ++i)
        {
            int r = i / 7;
            int c = i % 7;
            SDL_FRect wsr = { wGridStartX + (c * (wSlotW + 2.0f * uiScale)), curY + (24.0f * uiScale) + (r * (slotH + 4.0f * uiScale)), wSlotW, slotH + 2.0f * uiScale };
            SDL_SetRenderDrawColor(renderer, 24, 26, 34, 255);
            SDL_RenderFillRect(renderer, &wsr);
            SDL_SetRenderDrawColor(renderer, 55, 60, 72, 255);
            SDL_RenderRect(renderer, &wsr);
            UIWidget::drawText(renderer, wardrobeIcons[i], wsr.x + (2.0f * uiScale), wsr.y + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.72f);
        }

        // Right Footer: << < Page 1(of 1) > >>  ¤0
        UIWidget::drawText(renderer, "<< <", wardRect.x + (8.0f * uiScale), fY, Theme::colors.textMuted, uiScale * 0.75f);
        UIWidget::drawText(renderer, "Page 1(of 1)", wardRect.x + (50.0f * uiScale), fY, Theme::colors.textPrimary, uiScale * 0.78f);
        UIWidget::drawText(renderer, "> >>", wardRect.x + (130.0f * uiScale), fY, Theme::colors.textMuted, uiScale * 0.75f);
        UIWidget::drawText(renderer, "¤0", wardRect.x + halfGridW - (30.0f * uiScale), fY, Theme::colors.textGold, uiScale * 0.78f);

        curY += gridSectionH + (14.0f * uiScale);

        // Middle Narrative
        std::string n1 = "There doesn't seem to be any sign of activity on the main stage, so, afforded a few more minutes, you decide to smarten up your clothes a little. After all, this is a big evening for Lily, and you want her to see that you've put some effort into your appearance.";
        float nh1 = UIWidget::drawTextWrapped(renderer, n1, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += nh1 + (12.0f * uiScale);

        std::string n2 = "Turning this way and that to get a better look at yourself in the mirror, you begin to notice just how handsome you're looking tonight...";
        float nh2 = UIWidget::drawTextWrapped(renderer, n2, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += nh2 + (12.0f * uiScale);

        std::string n3 = "Why am I feeling so horny all of a sudden?";
        float nh3 = UIWidget::drawTextWrapped(renderer, n3, padX, curY, availableW, SDL_Color{ 100, 160, 255, 255 }, uiScale * 0.88f);
        curY += nh3 + (14.0f * uiScale);

        std::string n4 = "Choose what you decided to wear to the museum.";
        float n4W = n4.size() * (6.5f * uiScale);
        UIWidget::drawText(renderer, n4, centerX - (n4W / 2.0f), curY, Theme::colors.textSecondary, uiScale * 0.85f);
        curY += (24.0f * uiScale);
    }

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
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float availableW = innerW - (20.0f * uiScale);

    bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
    if (inPrologue)
    {
        UIWidget::drawText(renderer, "Land", padX + (availableW * 0.35f), curY, Theme::colors.textGold, uiScale * 1.05f);
        curY += (18.0f * uiScale);
        UIWidget::drawText(renderer, "Safe", padX + (availableW * 0.35f), curY, Theme::colors.companion, uiScale * 0.9f);
        curY += (20.0f * uiScale);

        UIWidget::drawText(renderer, "Characters Present", padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += (18.0f * uiScale);

        SDL_FRect cardRect = { padX, curY, availableW, 24.0f * uiScale };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "✋ Few people", padX + (6.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.85f);
        curY += (28.0f * uiScale);
        return (curY - startY);
    }

    UIWidget::drawText(renderer, "Land", padX + (availableW * 0.35f), curY, Theme::colors.textGold, uiScale * 1.05f);
    curY += (18.0f * uiScale);
    UIWidget::drawText(renderer, "Safe", padX + (availableW * 0.35f), curY, Theme::colors.companion, uiScale * 0.9f);
    curY += (20.0f * uiScale);

    UIWidget::drawText(renderer, "Characters Present", padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
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
        UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
        curY += (16.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float availableW = innerW - (20.0f * uiScale);

    bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
    if (inPrologue)
    {
        UIWidget::drawText(renderer, "Items Present", padX + (16.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += (18.0f * uiScale);
        UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
        curY += (16.0f * uiScale);
        return (curY - startY);
    }

    UIWidget::drawText(renderer, "Items Present", padX + (16.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
    curY += (18.0f * uiScale);

    auto ground = gameContext->getTileInventoryStacked();
    if (ground.empty())
    {
        UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
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
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (10.0f * uiScale);
    float availableW = innerW - (20.0f * uiScale);

    bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
    if (inPrologue)
    {
        UIWidget::drawText(renderer, "Event Log", padX + (availableW * 0.3f), curY, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += (18.0f * uiScale);

        static const std::vector<std::string> startingEquipLog = {
            "Equipped: Silver masculine watch",
            "Equipped: Silver ring",
            "Equipped: Black men's shoes",
            "Equipped: Black socks",
            "Equipped: Black trousers",
            "Equipped: White short-sleeved shirt",
            "Equipped: Black boxer shorts"
        };

        for (const auto& entry : startingEquipLog)
        {
            float descH = UIWidget::drawTextWrapped(renderer, entry, padX, curY, availableW, Theme::colors.textSecondary, uiScale * 0.8f);
            curY += descH + (6.0f * uiScale);
        }
        return (curY - startY);
    }

    UIWidget::drawText(renderer, "EVENT LOG", padX, curY, Theme::colors.textGold, uiScale);
    curY += (18.0f * uiScale);

    static const std::vector<std::pair<std::string, SDL_Color>> logEntries = {
        { "Entered: Lilaya's Home F1", Theme::colors.friendly },
        { "Discovered: Lilaya's Home F1", Theme::colors.textGold },
        { "Encyclopedia: Cat-morph", Theme::colors.textAccent },
        { "Encyclopedia: Half-demon", Theme::colors.arcane },
        { "Equipped: opaque demonstone", SDL_Color{ 100, 180, 255, 255 } },
        { "Encyclopedia: Opaque demonstone", Theme::colors.textGold },
        { "Gained: ¤ 5,000", Theme::colors.friendly },
        { "New Task: Lilaya's Tests", SDL_Color{ 100, 220, 255, 255 } }
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
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
    std::string dateStr = inPrologue ? "29th August" : "Unknown date";
    std::string timeStr = inPrologue ? "20:37" : "21:47";

    // Calendar icon + Date + Watch icon + Time
    UIWidget::drawText(renderer, "📅", padX, curY + (2.0f * uiScale), inPrologue ? Theme::colors.textGold : SDL_Color{ 255, 110, 120, 255 }, uiScale * 0.85f);
    UIWidget::drawText(renderer, dateStr, padX + (16.0f * uiScale), curY + (2.0f * uiScale), inPrologue ? Theme::colors.textPrimary : SDL_Color{ 255, 110, 120, 255 }, uiScale * 0.82f);
    UIWidget::drawText(renderer, "⌚", padX + availableW - (60.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
    UIWidget::drawText(renderer, timeStr, padX + availableW - (45.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
    curY += (18.0f * uiScale);

    // Weekdays tracker: M T W T [F] S S
    static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
    float dayW = (availableW - (6 * 3.0f * uiScale)) / 7.0f;
    for (int d = 0; d < 7; ++d)
    {
        SDL_FRect dRect = { padX + (d * (dayW + 3.0f * uiScale)), curY, dayW, 14.0f * uiScale };
        bool isActiveDay = inPrologue && (d == 4); // Friday selected during prologue
        if (isActiveDay)
        {
            SDL_SetRenderDrawColor(renderer, 45, 55, 65, 255);
            SDL_RenderFillRect(renderer, &dRect);
            SDL_SetRenderDrawColor(renderer, 100, 160, 255, 255);
            SDL_RenderRect(renderer, &dRect);
        }
        UIWidget::drawText(renderer, days[d], dRect.x + ((dRect.w - (5.0f * uiScale)) / 2.0f), dRect.y + (1.0f * uiScale), isActiveDay ? Theme::colors.textPrimary : Theme::colors.textMuted, uiScale * 0.68f);
    }

    curY += (18.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    static const std::vector<std::pair<std::string, CommandType>> tools = {
        { "⚙", CommandType::OPEN_SETTINGS },
        { "📱", CommandType::OPEN_PHONE },
        { "🛡", CommandType::OPEN_INVENTORY },
        { "👥", CommandType::OPEN_INVENTORY },
        { "🔍", CommandType::OPEN_TRANSFORMATION }
    };

    float btnGap = 4.0f * uiScale;
    float btnW = (availableW - (btnGap * (tools.size() - 1))) / tools.size();
    float btnH = 22.0f * uiScale;

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    for (size_t i = 0; i < tools.size(); ++i)
    {
        SDL_FRect btnRect = { padX + (i * (btnW + btnGap)), curY, btnW, btnH };
        bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                        mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

        bool isActive = (i == 1 && gameContext->isPhoneMenuOpen);
        UIWidget::drawButton(renderer, btnRect, tools[i].first, hovered, true, isActive, uiScale * 0.8f);

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
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    entity* p = gameContext->getPlayer();

    float startY = curY;
    float padX = curX + (8.0f * uiScale);
    float availableW = innerW - (16.0f * uiScale);

    bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
    if (inPrologue)
    {
        UIWidget::drawText(renderer, "Museum", padX + (availableW * 0.25f), curY, SDL_Color{ 255, 120, 140, 255 }, uiScale * 1.05f);
        curY += (17.0f * uiScale);
        UIWidget::drawText(renderer, "Lobby", padX + (availableW * 0.28f), curY, SDL_Color{ 255, 105, 180, 255 }, uiScale * 0.9f);
        curY += (15.0f * uiScale);
    }
    else
    {
        UIWidget::drawText(renderer, "Lilaya's Home F1", padX + (availableW * 0.15f), curY, SDL_Color{ 208, 112, 255, 255 }, uiScale * 1.05f);
        curY += (17.0f * uiScale);
        UIWidget::drawText(renderer, "Corridor", padX + (availableW * 0.28f), curY, SDL_Color{ 96, 208, 255, 255 }, uiScale * 0.9f);
        curY += (15.0f * uiScale);
    }

    // 2. Character Card Frame
    SDL_FRect cardRect = { padX, curY, availableW, 148.0f * uiScale };
    UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgDark, Theme::colors.borderButton);

    float innerPadX = padX + (8.0f * uiScale);
    float cardCurY = curY + (6.0f * uiScale);
    float cardInnerW = availableW - (16.0f * uiScale);

    // Row A: Avatar & Name/Level
    SDL_FRect avatarRect = { innerPadX, cardCurY, 22.0f * uiScale, 22.0f * uiScale };
    UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgHeader, Theme::colors.borderButton);
    UIWidget::drawText(renderer, "👤", avatarRect.x + (3.0f * uiScale), avatarRect.y + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

    std::string dispName = p->name.empty() ? "Rudy" : p->name;
    std::string nameLvl = std::format("{} - Level {}", dispName, p->stats.level);
    UIWidget::drawText(renderer, nameLvl, innerPadX + (28.0f * uiScale), cardCurY + (3.0f * uiScale), SDL_Color{ 100, 180, 255, 255 }, uiScale * 0.9f);
    cardCurY += (25.0f * uiScale);

    // Row B: Currency (Gold ¤ and Arcane Essence)
    float curVal = p->getStat("currency");
    float goldVal = inPrologue ? 0.0f : (curVal > 0.0f ? curVal : 5000.0f);
    UIWidget::drawText(renderer, std::format("¤ {:.0f}", goldVal), innerPadX, cardCurY, Theme::colors.textGold, uiScale * 0.85f);
    UIWidget::drawText(renderer, "★ 0", innerPadX + (cardInnerW * 0.52f), cardCurY, Theme::colors.lust, uiScale * 0.85f);
    cardCurY += (16.0f * uiScale);

    // Row C: Core stats numbers
    float arcVal = inPrologue ? 0.0f : 20.0f;
    UIWidget::drawText(renderer, "♥ 12", innerPadX, cardCurY, Theme::colors.enemy, uiScale * 0.8f);
    UIWidget::drawText(renderer, std::format("★ {:.0f}", arcVal), innerPadX + (cardInnerW * 0.35f), cardCurY, Theme::colors.arcane, uiScale * 0.8f);
    UIWidget::drawText(renderer, "💧 0", innerPadX + (cardInnerW * 0.68f), cardCurY, Theme::colors.corruption, uiScale * 0.8f);
    cardCurY += (16.0f * uiScale);

    // Row D: 3 Vitals Bars (Coral/Pink Health, Purple Mana, Lust)
    float barH = 12.0f * uiScale;
    float barW = cardInnerW - (75.0f * uiScale);

    // Health (40 / 40)
    UIWidget::drawText(renderer, "♥", innerPadX, cardCurY, Theme::colors.health, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, 40.0f, 40.0f, Theme::colors.health, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, "40", innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 4.0f * uiScale);

    // Mana (108 / 108 if in gameplay, 0 / 0 in prologue)
    float manaVal = inPrologue ? 0.0f : 108.0f;
    float manaMax = inPrologue ? 0.0f : 108.0f;
    UIWidget::drawText(renderer, "★", innerPadX, cardCurY, Theme::colors.mana, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, manaVal, manaMax > 0.0f ? manaMax : 1.0f, SDL_Color{ 190, 110, 240, 255 }, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, std::format("{:.0f}", manaVal), innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 4.0f * uiScale);

    // Lust (0 / 100)
    UIWidget::drawText(renderer, "💧", innerPadX, cardCurY, Theme::colors.lust, uiScale * 0.75f);
    UIWidget::drawProgressBar(renderer, { innerPadX + (22.0f * uiScale), cardCurY, barW, barH }, 0.0f, 100.0f, Theme::colors.lust, Theme::colors.bgSlot, "", uiScale);
    UIWidget::drawText(renderer, "0", innerPadX + (26.0f * uiScale) + barW, cardCurY, Theme::colors.textPrimary, uiScale * 0.75f);
    cardCurY += (barH + 6.0f * uiScale);

    // Row E: Status trait badges (hand, shield/cloud, gender, potion)
    static const std::vector<std::string> statusBadges = { "✋", "🛡", "⚥", "🧪" };
    float badgeW = (cardInnerW - (3.0f * 4.0f * uiScale)) / 4.0f;
    float badgeH = 16.0f * uiScale;
    for (size_t i = 0; i < statusBadges.size(); ++i)
    {
        SDL_FRect bRect = { innerPadX + (i * (badgeW + 4.0f * uiScale)), cardCurY, badgeW, badgeH };
        UIWidget::drawPanel(renderer, bRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, statusBadges[i], bRect.x + ((bRect.w - (10.0f * uiScale)) / 2.0f), bRect.y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.75f);
    }

    curY += cardRect.h + (8.0f * uiScale);
    return (curY - startY);
}

float uiRenderer::renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
{
    if (!gameContext->getPlayer())
    {
        return 0.0f;
    }

    float startY = curY;
    curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
    curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 120.0f * uiScale }, curY, uiScale);
    curY += renderWidgetOptionsToolbar(renderer, gameContext, curX, curY, innerW, uiScale);
    return (curY - startY);
}