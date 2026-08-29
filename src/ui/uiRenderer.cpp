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

    std::string stateKey = "";
    iGameState* curState = gameContext->getActiveState();
    if (dynamic_cast<mainMenuState*>(curState)) stateKey = "MAIN_MENU";
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
    float spaceX = 8.0f * uiScale;
    float spaceY = 6.0f * uiScale;
    int cols = 5;
    int rows = 3;
    float btnWidth = (availableW - (spaceX * (cols - 1))) / cols;
    float btnHeight = (rect.h - headerH - (10.0f * uiScale) - (spaceY * (rows - 1))) / rows;

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    for (int slotIdx = 0; slotIdx < BUTTONS_PER_PAGE; ++slotIdx)
    {
        int col = slotIdx % cols;
        int row = slotIdx / cols;
        SDL_FRect btnRect = { padX + col * (btnWidth + spaceX), startY + row * (btnHeight + spaceY), btnWidth, btnHeight };

        int buttonIdx = startIndex + slotIdx;
        if (buttonIdx < endIndex)
        {
            bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                            mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

            UIWidget::drawButton(renderer, btnRect, buttons[buttonIdx].label, hovered, buttons[buttonIdx].isEnabled, false, uiScale);

            if (hovered && clicked && buttons[buttonIdx].isEnabled && buttons[buttonIdx].onClick)
            {
                buttons[buttonIdx].onClick();
                gameContext->input.consumeMouseClick();
            }
        }
        else
        {
            // Empty inactive slot panel
            UIWidget::drawPanel(renderer, btnRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    float padX = rect.x + (20.0f * uiScale);
    float availableW = rect.w - (40.0f * uiScale);

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, "CHRONICLES OF LILITH • MAIN MENU", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (16.0f * uiScale);

    UIWidget::drawText(renderer, "CHRONICLES OF LILITH", padX, curY, Theme::colors.textGold, uiScale * 1.5f);
    curY += (28.0f * uiScale);

    UIWidget::drawText(renderer, "A Transformative Text-Based Fantasy RPG Engine", padX, curY, Theme::colors.textAccent, uiScale * 1.0f);
    curY += (22.0f * uiScale);

    float descH = UIWidget::drawTextWrapped(renderer,
        "Welcome to Dominion. Deep mutation pipelines, CYOA encounter resolution, and dynamic anatomy simulation await. Select an action command from the grid below to start a new adventure, continue your journey, or configure settings.",
        padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += descH + (20.0f * uiScale);

    UIWidget::drawText(renderer, "SAVE PROFILES & CHECKPOINTS:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
    curY += (20.0f * uiScale);

    static const std::vector<std::pair<std::string, std::string>> profiles = {
        { "QuickSave Profile", "QuickSave.json • Latest fast checkpoint" },
        { "Profile Slot 1", "Profile_1.json • Standard campaign slot" },
        { "Profile Slot 2", "Profile_2.json • Secondary campaign slot" }
    };

    for (size_t i = 0; i < profiles.size(); ++i)
    {
        SDL_FRect slotRect = { padX, curY, availableW, 30.0f * uiScale };
        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, profiles[i].first, padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textAccent, uiScale * 0.85f);
        UIWidget::drawText(renderer, profiles[i].second, padX + (150.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
        curY += (36.0f * uiScale);
    }

    return (curY - startY);
}

float uiRenderer::renderOptionsView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    optionsState* opt = dynamic_cast<optionsState*>(gameContext->getActiveState());
    if (!opt) return 0.0f;

    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float availableW = rect.w - (32.0f * uiScale);

    auto mousePos = gameContext->input.getMousePosition();
    bool clicked = gameContext->input.isLeftMouseJustClicked();

    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };

    std::string catTitle = "GAME SETTINGS & CONFIGURATION";
    if (opt->currentCategory == OptionsCategory::GAMEPLAY) catTitle = "SETTINGS: GAMEPLAY MECHANICS";
    else if (opt->currentCategory == OptionsCategory::CONTENT) catTitle = "SETTINGS: CONTENT & MUTATIONS";
    else if (opt->currentCategory == OptionsCategory::DEMOGRAPHICS) catTitle = "SETTINGS: WORLD DEMOGRAPHICS";
    else if (opt->currentCategory == OptionsCategory::DISPLAY) catTitle = "SETTINGS: DISPLAY & INTERFACE";

    UIWidget::drawHeader(renderer, headerRect, catTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (14.0f * uiScale);

    auto renderSettingSection = [&](const std::string& name, const std::string& description, const std::vector<std::pair<std::string, bool>>& options, std::function<void(int)> onSelect) {
        UIWidget::drawText(renderer, name, padX, curY, Theme::colors.textAccent, uiScale * 0.95f);
        curY += (18.0f * uiScale);

        float descH = UIWidget::drawTextWrapped(renderer, description, padX, curY, availableW, Theme::colors.textSecondary, uiScale * 0.85f);
        curY += descH + (8.0f * uiScale);

        float btnW = std::min(140.0f * uiScale, (availableW - ((options.size() - 1) * 8.0f * uiScale)) / options.size());
        float btnH = 24.0f * uiScale;

        for (size_t i = 0; i < options.size(); ++i)
        {
            SDL_FRect bRect = { padX + (i * (btnW + 8.0f * uiScale)), curY, btnW, btnH };
            bool hovered = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                            mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);
            bool isSelected = options[i].second;

            UIWidget::drawButton(renderer, bRect, options[i].first, hovered, true, isSelected, uiScale * 0.75f);

            if (hovered && clicked)
            {
                onSelect(static_cast<int>(i));
                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->input.consumeMouseClick();
            }
        }
        curY += btnH + (16.0f * uiScale);
    };

    if (opt->currentCategory == OptionsCategory::GAMEPLAY)
    {
        float diff = gameContext->settings.gameplay.difficultyMultiplier;
        renderSettingSection(
            "Combat Difficulty Multiplier",
            "Scales enemy maximum vitality, damage output, and offensive spellcasting frequency.",
            { { "Easy (0.8x)", diff <= 0.85f }, { "Normal (1.0x)", diff > 0.85f && diff <= 1.2f }, { "Hard (1.5x)", diff > 1.2f } },
            [&](int idx) {
                if (idx == 0) gameContext->settings.gameplay.difficultyMultiplier = 0.8f;
                else if (idx == 1) gameContext->settings.gameplay.difficultyMultiplier = 1.0f;
                else gameContext->settings.gameplay.difficultyMultiplier = 1.5f;
            }
        );

        float loss = gameContext->settings.gameplay.currencyLossOnDefeatPercent;
        renderSettingSection(
            "Defeat Currency Penalty",
            "The percentage of held gold and essence confiscated by opponents when submitted or defeated.",
            { { "0% (Safe)", loss <= 0.01f }, { "15% (Standard)", loss > 0.01f && loss <= 0.2f }, { "35% (Harsh)", loss > 0.2f && loss <= 0.4f }, { "50% (Hardcore)", loss > 0.4f } },
            [&](int idx) {
                if (idx == 0) gameContext->settings.gameplay.currencyLossOnDefeatPercent = 0.0f;
                else if (idx == 1) gameContext->settings.gameplay.currencyLossOnDefeatPercent = 0.15f;
                else if (idx == 2) gameContext->settings.gameplay.currencyLossOnDefeatPercent = 0.35f;
                else gameContext->settings.gameplay.currencyLossOnDefeatPercent = 0.50f;
            }
        );

        bool autoMap = gameContext->settings.gameplay.autoSaveOnMapChange;
        renderSettingSection(
            "Auto-Save on Room & Map Transitions",
            "Automatically writes a quicksave snapshot whenever entering new rooms, shops, or dungeons.",
            { { "ENABLED", autoMap }, { "DISABLED", !autoMap } },
            [&](int idx) { gameContext->settings.gameplay.autoSaveOnMapChange = (idx == 0); }
        );
    }
    else if (opt->currentCategory == OptionsCategory::CONTENT)
    {
        bool preg = gameContext->settings.content.pregnancyEnabled;
        renderSettingSection(
            "Biological Pregnancy & Gestation Pipeline",
            "Simulates multi-stage pregnancy, womb expansion, gestation timers, and offspring traits.",
            { { "ENABLED", preg }, { "DISABLED", !preg } },
            [&](int idx) { gameContext->settings.content.pregnancyEnabled = (idx == 0); }
        );

        bool lact = gameContext->settings.content.lactationEnabled;
        renderSettingSection(
            "Lactation & Milk Generation Mechanics",
            "Enables breast stimulation, milk accumulation, chest expansion, and nursery production.",
            { { "ENABLED", lact }, { "DISABLED", !lact } },
            [&](int idx) { gameContext->settings.content.lactationEnabled = (idx == 0); }
        );

        float fluid = gameContext->settings.content.fluidMultiplier;
        renderSettingSection(
            "Fluid Volume Capacity Multiplier",
            "Scales maximum bodily fluid storage (cum, milk, fluids) across all entities in the game.",
            { { "0.5x (Subtle)", fluid <= 0.6f }, { "1.0x (Normal)", fluid > 0.6f && fluid <= 1.5f }, { "2.5x (Abundant)", fluid > 1.5f && fluid <= 3.5f }, { "5.0x (Hyper)", fluid > 3.5f } },
            [&](int idx) {
                if (idx == 0) gameContext->settings.content.fluidMultiplier = 0.5f;
                else if (idx == 1) gameContext->settings.content.fluidMultiplier = 1.0f;
                else if (idx == 2) gameContext->settings.content.fluidMultiplier = 2.5f;
                else gameContext->settings.content.fluidMultiplier = 5.0f;
            }
        );

        float tfSpeed = gameContext->settings.content.transformationSpeedMultiplier;
        renderSettingSection(
            "Transformation Mutation Progression Speed",
            "Speed at which alchemical potions, morphic draughts, and essences mutate the body.",
            { { "Instantaneous", tfSpeed >= 5.0f }, { "Standard (1.0x)", tfSpeed > 0.7f && tfSpeed < 5.0f }, { "Gradual (0.5x)", tfSpeed <= 0.7f } },
            [&](int idx) {
                if (idx == 0) gameContext->settings.content.transformationSpeedMultiplier = 10.0f;
                else if (idx == 1) gameContext->settings.content.transformationSpeedMultiplier = 1.0f;
                else gameContext->settings.content.transformationSpeedMultiplier = 0.5f;
            }
        );
    }
    else if (opt->currentCategory == OptionsCategory::DEMOGRAPHICS)
    {
        renderSettingSection(
            "World NPC Orientation Distribution",
            "Statistical bias for generating heterosexuality, bisexuality, and homosexuality among NPCs.",
            { { "Balanced Mix", true }, { "Bi/Pan Heavy", false }, { "Hetero Bias", false } },
            [&](int idx) {
                if (idx == 0) { gameContext->settings.demographics.percentHetero = 40.0f; gameContext->settings.demographics.percentBi = 35.0f; gameContext->settings.demographics.percentHomo = 25.0f; }
                else if (idx == 1) { gameContext->settings.demographics.percentHetero = 20.0f; gameContext->settings.demographics.percentBi = 60.0f; gameContext->settings.demographics.percentHomo = 20.0f; }
                else { gameContext->settings.demographics.percentHetero = 70.0f; gameContext->settings.demographics.percentBi = 20.0f; gameContext->settings.demographics.percentHomo = 10.0f; }
            }
        );

        renderSettingSection(
            "World Gender & Morph Distribution",
            "Frequency of encounterable Male, Female, Hermaphrodite, and Morph individuals in Dominion.",
            { { "Standard Fantasy", true }, { "Herm / Futa Surge", false }, { "Matriarchal", false } },
            [&](int idx) {
                if (idx == 0) { gameContext->settings.demographics.percentMale = 35.0f; gameContext->settings.demographics.percentFemale = 45.0f; gameContext->settings.demographics.percentHermaphrodite = 20.0f; }
                else if (idx == 1) { gameContext->settings.demographics.percentMale = 20.0f; gameContext->settings.demographics.percentFemale = 30.0f; gameContext->settings.demographics.percentHermaphrodite = 50.0f; }
                else { gameContext->settings.demographics.percentMale = 10.0f; gameContext->settings.demographics.percentFemale = 70.0f; gameContext->settings.demographics.percentHermaphrodite = 20.0f; }
            }
        );
    }
    else if (opt->currentCategory == OptionsCategory::DISPLAY)
    {
        int verb = gameContext->settings.display.descriptionVerbosity;
        renderSettingSection(
            "Narrative Prose & Description Detail",
            "Controls the length and richness of descriptive prose during encounters and inspections.",
            { { "Full & Immersive", verb == 0 }, { "Condensed Summary", verb == 1 }, { "Minimal Speed", verb == 2 } },
            [&](int idx) { gameContext->settings.display.descriptionVerbosity = idx; }
        );

        std::string curTheme = gameContext->settings.display.activeTheme;
        renderSettingSection(
            "Color Palette & Aesthetic Theme",
            "Selects the primary color scheme and visual styling for panels, headers, and buttons.",
            { { "Dark Fantasy", curTheme == "theme_dark_fantasy" || curTheme == "dark_fantasy" },
              { "Cyber Neon", curTheme == "theme_cyber_neon" || curTheme == "cyber_neon" },
              { "Arcane Parchment", curTheme == "theme_parchment" || curTheme == "parchment" },
              { "Lilith Midnight", curTheme == "default" || curTheme == "theme.json" || curTheme.empty() } },
            [&](int idx) {
                if (idx == 0) gameContext->settings.display.activeTheme = "theme_dark_fantasy";
                else if (idx == 1) gameContext->settings.display.activeTheme = "theme_cyber_neon";
                else if (idx == 2) gameContext->settings.display.activeTheme = "theme_parchment";
                else gameContext->settings.display.activeTheme = "default";

                Theme::applyTheme(gameContext->settings.display.activeTheme);
            }
        );
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
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
    if (dynamic_cast<mainMenuState*>(gameContext->getActiveState()) || dynamic_cast<optionsState*>(gameContext->getActiveState()))
    {
        return 0.0f;
    }

    float startY = curY;
    curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
    curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 120.0f * uiScale }, curY, uiScale);
    curY += renderWidgetOptionsToolbar(renderer, gameContext, curX, curY, innerW, uiScale);
    return (curY - startY);
}