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
#include "state/phoneAppsState.h"
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
    fontManager::getInstance().setPointSize(static_cast<float>(gameContext->settings.display.fontSize));
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
    else if (dynamic_cast<phoneAppsState*>(state))
    {
        return renderPhoneAppView(renderer, gameContext, rect, curY, uiScale);
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

    if (sex->isSolo())
    {
        UIWidget::drawHeader(renderer, headerRect, "SOLO INTIMATE ENCOUNTER", Theme::colors.bgHeader, Theme::colors.lust, uiScale);
        curY += headerH + (10.0f * uiScale);

        float padX = rect.x + (12.0f * uiScale);
        float innerW = rect.w - (24.0f * uiScale);
        float barH = 18.0f * uiScale;

        UIWidget::drawText(renderer, std::format("Position: {} | Private Solitude", sexStanceToString(sex->getStance())), padX, curY, Theme::colors.textGold, uiScale);
        curY += (22.0f * uiScale);

        UIWidget::drawProgressBar(renderer, { padX, curY, innerW, barH }, sex->getPlayerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Your Arousal: {:.0f}/100", sex->getPlayerArousal()), uiScale);
        curY += (barH + 12.0f * uiScale);

        UIWidget::drawText(renderer, "INTIMATE LOG:", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);
        float textH = UIWidget::drawTextWrapped(renderer, sex->getNarrativeLog(), padX, curY, innerW, Theme::colors.textSecondary, uiScale);
        curY += textH + (6.0f * uiScale);

        return (curY - startY);
    }

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

float uiRenderer::renderPhoneAppView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    phoneAppsState* app = dynamic_cast<phoneAppsState*>(gameContext->getActiveState());
    if (!app) return 0.0f;

    float startY = curY;
    float padX = rect.x + (16.0f * uiScale);
    float innerW = rect.w - (32.0f * uiScale);

    std::string appTitle = phoneAppModeToString(app->getAppMode());
    float headerH = 26.0f * uiScale;
    SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
    UIWidget::drawHeader(renderer, headerRect, std::format("📱 PHONE APP - {}", appTitle), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (12.0f * uiScale);

    const auto& data = app->getAppData();

    if (app->getAppMode() == PhoneAppMode::QUESTS)
    {
        // Quest Journal
        UIWidget::drawText(renderer, "Lilaya's Tests (Active Main Quest)", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
        curY += (20.0f * uiScale);
        UIWidget::drawText(renderer, "Chapter 1: The New World", padX, curY, Theme::colors.textAccent, uiScale * 0.85f);
        curY += (18.0f * uiScale);

        std::string qDesc = "Having arrived at Lilaya's residence, you must explore your surroundings and undergo initial arcane resonance testing in her laboratory.";
        float qH = UIWidget::drawTextWrapped(renderer, qDesc, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
        curY += qH + (14.0f * uiScale);

        UIWidget::drawText(renderer, "Objectives:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
        curY += (18.0f * uiScale);

        static const std::vector<std::pair<std::string, bool>> objectives = {
            { "[✓] Awaken in Lilaya's Home F1", true },
            { "[ ] Explore the first floor corridors", false },
            { "[ ] Speak with Lilaya in her laboratory", false }
        };
        for (const auto& obj : objectives)
        {
            UIWidget::drawText(renderer, obj.first, padX + (8.0f * uiScale), curY, obj.second ? Theme::colors.friendly : Theme::colors.textSecondary, uiScale * 0.85f);
            curY += (16.0f * uiScale);
        }
        curY += (10.0f * uiScale);

        UIWidget::drawText(renderer, "Rewards: ¤ 5,000 | 150 XP | Arcane Essence x5", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
        curY += (20.0f * uiScale);
    }
    else if (app->getAppMode() == PhoneAppMode::ENCYCLOPEDIA)
    {
        // Encyclopedia
        if (data.contains("categories") && data["categories"].is_array())
        {
            for (const auto& cat : data["categories"])
            {
                std::string catName = cat.value("name", "Category");
                UIWidget::drawText(renderer, catName, padX, curY, Theme::colors.textGold, uiScale * 0.95f);
                curY += (18.0f * uiScale);

                if (cat.contains("entries") && cat["entries"].is_array())
                {
                    for (const auto& ent : cat["entries"])
                    {
                        std::string title = ent.value("title", "Unknown");
                        std::string subtitle = ent.value("subtitle", "");
                        std::string desc = ent.value("description", "");
                        bool unlocked = ent.value("unlocked", true);

                        SDL_FRect cardRect = { padX, curY, innerW, 42.0f * uiScale };
                        UIWidget::drawPanel(renderer, cardRect);

                        if (unlocked)
                        {
                            UIWidget::drawText(renderer, title, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                            if (!subtitle.empty())
                            {
                                float titleW = UIWidget::getTextWidth(title, uiScale * 0.85f);
                                UIWidget::drawText(renderer, subtitle, padX + (12.0f * uiScale) + titleW, curY + (5.0f * uiScale), Theme::colors.textAccent, uiScale * 0.72f);
                            }
                            UIWidget::drawTextWrapped(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), innerW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                        }
                        else
                        {
                            UIWidget::drawText(renderer, "??? [Locked Entry]", padX + (8.0f * uiScale), curY + (12.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                        }
                        curY += cardRect.h + (6.0f * uiScale);
                    }
                }
                curY += (8.0f * uiScale);
            }
        }
    }
    else if (app->getAppMode() == PhoneAppMode::SPELLS)
    {
        // Spellbook
        UIWidget::drawText(renderer, "Unlocked Arcane Spells & Abilities:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
        curY += (20.0f * uiScale);

        if (data.contains("spells") && data["spells"].is_array())
        {
            for (const auto& sp : data["spells"])
            {
                std::string sName = sp.value("name", "Spell");
                std::string school = sp.value("school", "Arcane");
                int mp = sp.value("mpCost", 10);
                std::string desc = sp.value("description", "");
                bool unlocked = sp.value("unlocked", true);

                SDL_FRect spRect = { padX, curY, innerW, 40.0f * uiScale };
                UIWidget::drawPanel(renderer, spRect);

                if (unlocked)
                {
                    UIWidget::drawText(renderer, sName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                    std::string meta = std::format("{} | Cost: {} MP", school, mp);
                    float metaW = UIWidget::getTextWidth(meta, uiScale * 0.75f);
                    UIWidget::drawText(renderer, meta, padX + innerW - metaW - (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.arcane, uiScale * 0.75f);
                    UIWidget::drawTextWrapped(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), innerW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                }
                else
                {
                    UIWidget::drawText(renderer, std::format("{} [Locked]", sName), padX + (8.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                }
                curY += spRect.h + (6.0f * uiScale);
            }
        }
    }
    else if (app->getAppMode() == PhoneAppMode::PERKS)
    {
        // Perk Tree
        UIWidget::drawText(renderer, "Available Perk Trees & Upgrades:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
        curY += (20.0f * uiScale);

        if (data.contains("perks") && data["perks"].is_array())
        {
            for (const auto& pk : data["perks"])
            {
                std::string pName = pk.value("name", "Perk");
                std::string category = pk.value("category", "General");
                int cost = pk.value("cost", 1);
                std::string desc = pk.value("description", "");
                bool unlocked = pk.value("unlocked", true);

                SDL_FRect pkRect = { padX, curY, innerW, 40.0f * uiScale };
                UIWidget::drawPanel(renderer, pkRect);

                UIWidget::drawText(renderer, pName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), unlocked ? Theme::colors.textGold : Theme::colors.textPrimary, uiScale * 0.85f);
                std::string status = unlocked ? "UNLOCKED" : std::format("Cost: {} Point", cost);
                float statusW = UIWidget::getTextWidth(status, uiScale * 0.75f);
                UIWidget::drawText(renderer, status, padX + innerW - statusW - (8.0f * uiScale), curY + (4.0f * uiScale), unlocked ? Theme::colors.friendly : Theme::colors.textAccent, uiScale * 0.75f);
                UIWidget::drawTextWrapped(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), innerW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);

                curY += pkRect.h + (6.0f * uiScale);
            }
        }
    }
    else if (app->getAppMode() == PhoneAppMode::CONTACTS)
    {
        // Contacts
        UIWidget::drawText(renderer, "Address Book & Contact List:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
        curY += (20.0f * uiScale);

        if (data.contains("contacts") && data["contacts"].is_array())
        {
            for (const auto& ct : data["contacts"])
            {
                std::string cName = ct.value("name", "Contact");
                std::string title = ct.value("title", "");
                std::string loc = ct.value("location", "");
                std::string status = ct.value("status", "Offline");
                int aff = ct.value("affinity", 0);
                std::string desc = ct.value("description", "");

                SDL_FRect ctRect = { padX, curY, innerW, 46.0f * uiScale };
                UIWidget::drawPanel(renderer, ctRect);

                UIWidget::drawText(renderer, cName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                float cNameW = UIWidget::getTextWidth(cName, uiScale * 0.85f);
                UIWidget::drawText(renderer, title, padX + (12.0f * uiScale) + cNameW, curY + (5.0f * uiScale), Theme::colors.textAccent, uiScale * 0.72f);
                
                std::string statusMeta = std::format("{} | Aff: {}", status, aff);
                float statusMetaW = UIWidget::getTextWidth(statusMeta, uiScale * 0.75f);
                UIWidget::drawText(renderer, statusMeta, padX + innerW - statusMetaW - (8.0f * uiScale), curY + (4.0f * uiScale), (status == "Available") ? Theme::colors.friendly : Theme::colors.textSecondary, uiScale * 0.75f);
                
                UIWidget::drawText(renderer, std::format("Location: {}", loc), padX + (8.0f * uiScale), curY + (18.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.72f);
                UIWidget::drawTextWrapped(renderer, desc, padX + (8.0f * uiScale), curY + (30.0f * uiScale), innerW - (16.0f * uiScale), Theme::colors.textMuted, uiScale * 0.72f);

                curY += ctRect.h + (6.0f * uiScale);
            }
        }
    }
    else if (app->getAppMode() == PhoneAppMode::STATS)
    {
        // Comprehensive Character Stats
        entity* p = gameContext->getPlayer();
        if (p)
        {
            UIWidget::drawText(renderer, std::format("Full Diagnostic: {} (Level 1)", p->name), padX, curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            float hp = p->getStat("health");
            float mana = p->getStat("mana");
            float lust = p->getStat("lust");
            float arc = p->getStat("arcaneEssence");

            UIWidget::drawText(renderer, std::format("Core Attributes: Health {:.0f}/40 | Mana {:.0f}/108 | Lust {:.0f}/100 | Arcane Essence {:.0f}", hp, mana, lust, arc), padX, curY, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += (18.0f * uiScale);

            UIWidget::drawText(renderer, std::format("Anatomy: Height {:.2f}m | Gender: {} | Race: {}", p->anatomy.heightMeters, genderArchetypeToString(p->anatomy.getGenderArchetype()), p->anatomy.getRacialTitle()), padX, curY, Theme::colors.textSecondary, uiScale * 0.85f);
            curY += (18.0f * uiScale);

            if (p->anatomy.hasPenis())
            {
                bodyPart* penis = p->anatomy.getPart(bodySlot::GROIN);
                float cum = penis ? penis->currentFluidMl : 0.0f;
                float maxCum = penis ? penis->maxFluidMl : 15.0f;
                UIWidget::drawText(renderer, std::format("Genital Vitals: Penis ({:.1f}cm x {:.1f}cm) | Cum Capacity: {:.0f}/{:.0f}ml", penis ? penis->length : 15.0f, penis ? penis->diameter : 3.5f, cum, maxCum), padX, curY, Theme::colors.textSecondary, uiScale * 0.82f);
                curY += (16.0f * uiScale);
            }

            if (p->anatomy.hasBreasts())
            {
                bodyPart* breasts = p->anatomy.getPart(bodySlot::BREASTS);
                float milk = breasts ? breasts->currentFluidMl : 0.0f;
                float maxMilk = breasts ? breasts->maxFluidMl : 100.0f;
                UIWidget::drawText(renderer, std::format("Chest Vitals: {}-Cup | Milk Production: {:.0f}/{:.0f}ml", bodyPart::getCupSizeName(breasts ? breasts->cupSize : 0), milk, maxMilk), padX, curY, Theme::colors.textSecondary, uiScale * 0.82f);
                curY += (16.0f * uiScale);
            }
        }
    }
    else if (app->getAppMode() == PhoneAppMode::MAPS)
    {
        // Area & World Map
        UIWidget::drawText(renderer, "Lilaya's Home & Dominion Area Map:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
        curY += (20.0f * uiScale);

        SDL_FRect mapBox = { padX, curY, innerW, 160.0f * uiScale };
        UIWidget::drawPanel(renderer, mapBox);

        UIWidget::drawText(renderer, "🗺 LILAYA'S HOME F1 - CORRIDOR & LABS", mapBox.x + (16.0f * uiScale), mapBox.y + (16.0f * uiScale), Theme::colors.textGold, uiScale * 0.9f);
        UIWidget::drawText(renderer, "Current Location: Corridor [📍 (X: 1, Y: 1)]", mapBox.x + (16.0f * uiScale), mapBox.y + (38.0f * uiScale), Theme::colors.friendly, uiScale * 0.82f);
        UIWidget::drawText(renderer, "Surrounding Wings: Guest Quarters (North), Laboratory (East), Entrance (South)", mapBox.x + (16.0f * uiScale), mapBox.y + (58.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);
        UIWidget::drawText(renderer, "Danger Level: Safe Sanctuary", mapBox.x + (16.0f * uiScale), mapBox.y + (78.0f * uiScale), Theme::colors.companion, uiScale * 0.82f);

        curY += mapBox.h + (10.0f * uiScale);
    }

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
        float titleW = UIWidget::getTextWidth(phoneTitle, uiScale * 1.15f);
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

    float titleW = UIWidget::getTextWidth(locTitle, uiScale * 1.15f);
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

    // Title: TextRPG Engine in glowing purple/pink
    std::string_view mainTitle = "TextRPG Engine";
    float titleTextW = UIWidget::getTextWidth(mainTitle, uiScale * 1.8f);
    UIWidget::drawText(renderer, mainTitle, centerX - (titleTextW / 2.0f), curY, SDL_Color{ 235, 145, 255, 255 }, uiScale * 1.8f);
    curY += (38.0f * uiScale);

    // Subtitle: Studio Edition
    std::string_view subTitle = "Studio Edition";
    float subTextW = UIWidget::getTextWidth(subTitle, uiScale * 1.15f);
    UIWidget::drawText(renderer, subTitle, centerX - (subTextW / 2.0f), curY, SDL_Color{ 200, 140, 235, 255 }, uiScale * 1.15f);
    curY += (34.0f * uiScale);

    // Paragraph 1: Welcome
    float p1H = UIWidget::drawTextWrapped(renderer,
        "Welcome to TextRPG. Explore dynamic text-driven adventures, manage inventory and clothing displacement, customize character anatomy, and engage in interactive encounters.",
        textX, curY, textW, Theme::colors.textPrimary, uiScale * 0.92f);
    curY += p1H + (20.0f * uiScale);

    // Paragraph 2: Config note
    float p2H = UIWidget::drawTextWrapped(renderer,
        "Use the Options and Content Options commands in the action grid below to customize gameplay mechanics, difficulty, content toggles, themes, and demographics.",
        textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.9f);
    curY += p2H + (20.0f * uiScale);

    // Paragraph 3: Saves note
    float p3H = UIWidget::drawTextWrapped(renderer,
        "All characters, items, maps, and dialogue scenes are data-driven and fully editable in the Studio web app.",
        textX, curY, textW, Theme::colors.textPrimary, uiScale * 0.9f);
    curY += p3H + (24.0f * uiScale);

    // Engine version
    std::string_view verText = "Engine Version: 0.5.0 Alpha (C++ Edition)";
    float verW = UIWidget::getTextWidth(verText, uiScale * 0.85f);
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

    // New Save Input Row (when in Save mode or Player entity active)
    if (isSaveMode || player != nullptr)
    {
        float inputRowH = 32.0f * uiScale;
        SDL_FRect inputRowRect = { padX, curY, availableW, inputRowH };
        UIWidget::drawPanel(renderer, inputRowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "New save name:", padX + (10.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);

        float saveBtnW = 100.0f * uiScale;
        float btnH = 24.0f * uiScale;
        SDL_FRect saveBtnRect = { padX + availableW - saveBtnW - (6.0f * uiScale), curY + (4.0f * uiScale), saveBtnW, btnH };
        bool saveHovered = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                            mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

        float nameBoxX = padX + (140.0f * uiScale);
        float nameBoxW = availableW - (150.0f * uiScale) - saveBtnW - (12.0f * uiScale);
        SDL_FRect nameBoxRect = { nameBoxX, curY + (4.0f * uiScale), nameBoxW, btnH };
        bool boxHovered = (mousePos.x >= nameBoxRect.x && mousePos.x <= nameBoxRect.x + nameBoxRect.w &&
                           mousePos.y >= nameBoxRect.y && mousePos.y <= nameBoxRect.y + nameBoxRect.h);

        SDL_Color boxBorder = (loadState && loadState->isEditingSaveName) ? Theme::colors.textGold : (boxHovered ? Theme::colors.borderSelected : Theme::colors.borderButton);
        UIWidget::drawPanel(renderer, nameBoxRect, Theme::colors.bgPanel, boxBorder);

        std::string dispInput = loadState ? loadState->newSaveNameInput : "Manual_Save";
        if (loadState && loadState->isEditingSaveName) dispInput += "_";
        UIWidget::drawText(renderer, dispInput, nameBoxX + (8.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

        if (boxHovered && clicked && loadState)
        {
            loadState->isEditingSaveName = true;
            gameContext->input.consumeMouseClick();
        }

        UIWidget::drawButton(renderer, saveBtnRect, "Save Game", saveHovered, true, false, uiScale * 0.78f);
        if (saveHovered && clicked && loadState)
        {
            saveManager::saveNamedGame(gameContext, loadState->newSaveNameInput);
            loadState->isEditingSaveName = false;
            gameContext->input.consumeMouseClick();
        }

        curY += inputRowH + (12.0f * uiScale);
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

    if (loadState && loadState->sortMode == 1)
    {
        // Alphabetical sort
        std::sort(characterGroups.begin(), characterGroups.end(), [](const CharacterSaveGroup& a, const CharacterSaveGroup& b) {
            return a.characterName < b.characterName;
        });
        for (auto& grp : characterGroups)
        {
            std::sort(grp.saves.begin(), grp.saves.end(), [](const SaveMetaData& a, const SaveMetaData& b) {
                return a.saveName < b.saveName;
            });
        }
    }

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
                        if (loadState->confirmationsEnabled)
                        {
                            loadState->pendingDeleteFileName = save.fileName;
                        }
                        else
                        {
                            saveManager::deleteSave(save.fileName);
                        }
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
                            if (loadState->confirmationsEnabled)
                            {
                                loadState->pendingOverwriteSaveName = save.saveName.empty() ? save.fileName : save.saveName;
                            }
                            else
                            {
                                saveManager::saveNamedGame(gameContext, save.saveName.empty() ? save.fileName : save.saveName);
                            }
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

    if (opt->isKeybindsOpen)
    {
        float cardW = std::min(availableW, 600.0f * uiScale);
        float cardH = 34.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Controls & Keybindings", Theme::colors.textPrimary, uiScale);
        curY += cardH + (20.0f * uiScale);

        float panelW = std::min(availableW, 720.0f * uiScale);
        float panelX = centerX - (panelW / 2.0f);
        float panelH = 280.0f * uiScale;
        SDL_FRect panelRect = { panelX, curY, panelW, panelH };
        UIWidget::drawPanel(renderer, panelRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float kbRowY = curY + (14.0f * uiScale);
        auto drawKbRow = [&](const std::string& section, const std::string& keys, const std::string& desc) {
            UIWidget::drawText(renderer, section, panelX + (16.0f * uiScale), kbRowY, Theme::colors.textGold, uiScale * 0.85f);
            UIWidget::drawText(renderer, keys, panelX + (160.0f * uiScale), kbRowY, Theme::colors.textPrimary, uiScale * 0.85f);
            UIWidget::drawText(renderer, desc, panelX + (360.0f * uiScale), kbRowY, Theme::colors.textSecondary, uiScale * 0.85f);
            kbRowY += (24.0f * uiScale);
        };

        drawKbRow("Grid Row 1", "1, 2, 3, 4, 5", "Execute slots 0 through 4");
        drawKbRow("Grid Row 2", "SHIFT + 1 .. 5", "Execute slots 5 through 9");
        drawKbRow("Grid Row 3", "CTRL + 1 .. 5", "Execute slots 10 through 14");
        drawKbRow("Exploration", "W, A, S, D / Arrows", "Move player on grid map");
        drawKbRow("Pages", "Q / E", "Previous / Next action grid page");
        drawKbRow("Quick Menus", "I (Inventory)", "Toggle inventory / equipment");
        drawKbRow("Save / Load", "F5 (QuickSave) / F9 (Load)", "Quick save / Quick load");
        drawKbRow("Back / Close", "ESC / Backspace", "Return to previous screen");

        curY += panelH + (20.0f * uiScale);
        return (curY - startY);
    }

    if (opt->screenMode == OptionsScreenMode::GENERAL_OPTIONS)
    {
        // 1. Centered Header Card: Options
        float cardW = std::min(availableW, 360.0f * uiScale);
        float cardH = 32.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Options", Theme::colors.textPrimary, uiScale);
        curY += cardH + (14.0f * uiScale);

        float textW = availableW;
        float textX = padX;

        // Section: Active Theme
        std::string curThemeName = gameContext->settings.display.activeTheme;
        if (curThemeName.empty() || curThemeName == "default") curThemeName = "Lilith Midnight";
        else if (curThemeName == "theme_cyber_neon") curThemeName = "Cyber Neon";
        else if (curThemeName == "theme_dark_fantasy") curThemeName = "Dark Fantasy";
        else if (curThemeName == "theme_parchment") curThemeName = "Arcane Parchment";

        SDL_FRect themeCardRect = { textX, curY, textW, 44.0f * uiScale };
        bool themeHovered = (mousePos.x >= themeCardRect.x && mousePos.x <= themeCardRect.x + themeCardRect.w &&
                             mousePos.y >= themeCardRect.y && mousePos.y <= themeCardRect.y + themeCardRect.h);
        UIWidget::drawPanel(renderer, themeCardRect, Theme::colors.bgSlot, themeHovered ? Theme::colors.borderSelected : Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "Visual Theme:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);
        std::string themeDesc = std::format("Currently Active Theme: {} (Click anywhere to cycle themes).", curThemeName);
        UIWidget::drawText(renderer, themeDesc, textX + (10.0f * uiScale), curY + (24.0f * uiScale), themeHovered ? Theme::colors.textGold : Theme::colors.textSecondary, uiScale * 0.82f);

        if (themeHovered && clicked)
        {
            auto& cur = gameContext->settings.display.activeTheme;
            if (cur == "theme_dark_fantasy" || cur == "dark_fantasy") cur = "theme_cyber_neon";
            else if (cur == "theme_cyber_neon" || cur == "cyber_neon") cur = "theme_parchment";
            else if (cur == "theme_parchment" || cur == "parchment") cur = "default";
            else cur = "theme_dark_fantasy";

            Theme::applyTheme(cur);
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        curY += themeCardRect.h + (10.0f * uiScale);

        // Section: Font-size
        SDL_FRect fontCardRect = { textX, curY, textW, 44.0f * uiScale };
        UIWidget::drawPanel(renderer, fontCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Font-size:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

        std::string fontDesc = std::format("Adjusts UI base font size (Min 12, Max 36). Current: {}pt.", gameContext->settings.display.fontSize);
        UIWidget::drawText(renderer, fontDesc, textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

        float btnW = 32.0f * uiScale;
        float btnH = 22.0f * uiScale;
        SDL_FRect minusBtn = { textX + textW - (btnW * 2.0f) - (14.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };
        SDL_FRect plusBtn = { textX + textW - btnW - (8.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };

        bool minusHovered = (mousePos.x >= minusBtn.x && mousePos.x <= minusBtn.x + minusBtn.w && mousePos.y >= minusBtn.y && mousePos.y <= minusBtn.y + minusBtn.h);
        bool plusHovered = (mousePos.x >= plusBtn.x && mousePos.x <= plusBtn.x + plusBtn.w && mousePos.y >= plusBtn.y && mousePos.y <= plusBtn.y + plusBtn.h);

        UIWidget::drawButton(renderer, minusBtn, "-", minusHovered, true, false, uiScale * 0.8f);
        UIWidget::drawButton(renderer, plusBtn, "+", plusHovered, true, false, uiScale * 0.8f);

        if (minusHovered && clicked)
        {
            if (gameContext->settings.display.fontSize > 12)
            {
                gameContext->settings.display.fontSize -= 2;
                opt->fontSize = gameContext->settings.display.fontSize;
                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->refreshActionGrid();
            }
            gameContext->input.consumeMouseClick();
        }
        else if (plusHovered && clicked)
        {
            if (gameContext->settings.display.fontSize < 36)
            {
                gameContext->settings.display.fontSize += 2;
                opt->fontSize = gameContext->settings.display.fontSize;
                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->refreshActionGrid();
            }
            gameContext->input.consumeMouseClick();
        }
        curY += fontCardRect.h + (10.0f * uiScale);

        // Section: Fade-in
        SDL_FRect fadeInRect = { textX, curY, textW, 44.0f * uiScale };
        UIWidget::drawPanel(renderer, fadeInRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Fade-in:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);
        UIWidget::drawText(renderer, "Fades in main narrative text upon entering new scenes.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

        float fadeW = 55.0f * uiScale;
        float fadeStartX = textX + textW - (fadeW * 2.0f) - (12.0f * uiScale);
        SDL_FRect offPill = { fadeStartX, curY + (10.0f * uiScale), fadeW, 24.0f * uiScale };
        SDL_FRect onPill = { fadeStartX + fadeW, curY + (10.0f * uiScale), fadeW, 24.0f * uiScale };

        bool offHovered = (mousePos.x >= offPill.x && mousePos.x <= offPill.x + offPill.w && mousePos.y >= offPill.y && mousePos.y <= offPill.y + offPill.h);
        bool onHovered = (mousePos.x >= onPill.x && mousePos.x <= onPill.x + onPill.w && mousePos.y >= onPill.y && mousePos.y <= onPill.y + onPill.h);

        bool isFadeOn = gameContext->settings.display.fadeInEnabled;
        UIWidget::drawColoredButton(renderer, offPill, "OFF", !isFadeOn ? SDL_Color{ 160, 45, 55, 240 } : Theme::colors.bgButton, !isFadeOn ? Theme::colors.textPrimary : Theme::colors.textMuted, !isFadeOn, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, onPill, "ON", isFadeOn ? SDL_Color{ 45, 120, 65, 240 } : Theme::colors.bgButton, isFadeOn ? Theme::colors.textPrimary : Theme::colors.textMuted, isFadeOn, uiScale * 0.72f);

        if (offHovered && clicked)
        {
            gameContext->settings.display.fadeInEnabled = false;
            opt->fadeInEnabled = false;
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        else if (onHovered && clicked)
        {
            gameContext->settings.display.fadeInEnabled = true;
            opt->fadeInEnabled = true;
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        curY += fadeInRect.h + (10.0f * uiScale);

        // Section: Pronouns
        SDL_FRect pronounRect = { textX, curY, textW, 44.0f * uiScale };
        UIWidget::drawPanel(renderer, pronounRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Gender Pronouns:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

        float proW = 68.0f * uiScale;
        float proStartX = textX + textW - (proW * 2.0f) - (12.0f * uiScale);
        float proDescW = proStartX - textX - (20.0f * uiScale);
        UIWidget::drawTextWrapped(renderer, "Set pronoun style for narrative text (Normal vs Custom gender neutral).", textX + (10.0f * uiScale), curY + (24.0f * uiScale), proDescW, Theme::colors.textSecondary, uiScale * 0.80f);

        SDL_FRect normalPill = { proStartX, curY + (10.0f * uiScale), proW, 24.0f * uiScale };
        SDL_FRect customPill = { proStartX + proW, curY + (10.0f * uiScale), proW, 24.0f * uiScale };

        bool normHovered = (mousePos.x >= normalPill.x && mousePos.x <= normalPill.x + normalPill.w && mousePos.y >= normalPill.y && mousePos.y <= normalPill.y + normalPill.h);
        bool custHovered = (mousePos.x >= customPill.x && mousePos.x <= customPill.x + customPill.w && mousePos.y >= customPill.y && mousePos.y <= customPill.y + customPill.h);

        bool isNorm = (gameContext->settings.gameplay.genderPronounMode == "Normal");
        UIWidget::drawColoredButton(renderer, normalPill, "Normal", isNorm ? SDL_Color{ 45, 120, 65, 240 } : Theme::colors.bgButton, isNorm ? Theme::colors.textPrimary : Theme::colors.textMuted, isNorm, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, customPill, "Custom", !isNorm ? SDL_Color{ 45, 120, 65, 240 } : Theme::colors.bgButton, !isNorm ? Theme::colors.textPrimary : Theme::colors.textMuted, !isNorm, uiScale * 0.72f);

        if (normHovered && clicked)
        {
            gameContext->settings.gameplay.genderPronounMode = "Normal";
            opt->genderPronounMode = "Normal";
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        else if (custHovered && clicked)
        {
            gameContext->settings.gameplay.genderPronounMode = "Custom";
            opt->genderPronounMode = "Custom";
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        curY += pronounRect.h + (10.0f * uiScale);

        // Section: Measurement Units
        SDL_FRect unitsRect = { textX, curY, textW, 44.0f * uiScale };
        UIWidget::drawPanel(renderer, unitsRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Measurement Units:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

        float unitW = 68.0f * uiScale;
        float unitStartX = textX + textW - (unitW * 2.0f) - (12.0f * uiScale);
        float unitDescW = unitStartX - textX - (20.0f * uiScale);
        UIWidget::drawTextWrapped(renderer, "Set units for height, length, and volume measurements.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), unitDescW, Theme::colors.textSecondary, uiScale * 0.80f);

        SDL_FRect metricPill = { unitStartX, curY + (10.0f * uiScale), unitW, 24.0f * uiScale };
        SDL_FRect imperialPill = { unitStartX + unitW, curY + (10.0f * uiScale), unitW, 24.0f * uiScale };

        bool metricHovered = (mousePos.x >= metricPill.x && mousePos.x <= metricPill.x + metricPill.w && mousePos.y >= metricPill.y && mousePos.y <= metricPill.y + metricPill.h);
        bool impHovered = (mousePos.x >= imperialPill.x && mousePos.x <= imperialPill.x + imperialPill.w && mousePos.y >= imperialPill.y && mousePos.y <= imperialPill.y + imperialPill.h);

        bool isMetric = (gameContext->settings.gameplay.unitPreference == "Metric");
        UIWidget::drawColoredButton(renderer, metricPill, "Metric", isMetric ? SDL_Color{ 45, 120, 65, 240 } : Theme::colors.bgButton, isMetric ? Theme::colors.textPrimary : Theme::colors.textMuted, isMetric, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, imperialPill, "Imperial", !isMetric ? SDL_Color{ 45, 120, 65, 240 } : Theme::colors.bgButton, !isMetric ? Theme::colors.textPrimary : Theme::colors.textMuted, !isMetric, uiScale * 0.72f);

        if (metricHovered && clicked)
        {
            gameContext->settings.gameplay.unitPreference = "Metric";
            opt->unitPreference = "Metric";
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        else if (impHovered && clicked)
        {
            gameContext->settings.gameplay.unitPreference = "Imperial";
            opt->unitPreference = "Imperial";
            settingsManager::saveToFile(gameContext->settings, "data/settings.json");
            gameContext->refreshActionGrid();
            gameContext->input.consumeMouseClick();
        }
        curY += unitsRect.h + (12.0f * uiScale);

        // Section: Difficulty
        static const char* diffNames[] = { "Human", "Morph", "Demon", "Lilin", "Lilith" };
        std::string diffHeader = std::format("Difficulty (Currently set to {}):", diffNames[gameContext->settings.gameplay.difficultyLevel % 5]);
        UIWidget::drawText(renderer, diffHeader, textX, curY, Theme::colors.textPrimary, uiScale * 0.92f);
        curY += (18.0f * uiScale);

        // Colored difficulty tiers (interactive cards)
        struct DiffTier { std::string name; std::string desc; SDL_Color col; };
        static const DiffTier tiers[5] = {
            { "Human", "The standard gameplay experience. Balanced level progression and baseline enemy stats.", Theme::colors.textPrimary },
            { "Morph", "Enemies level up alongside your character, but do normal damage.", SDL_Color{ 180, 140, 200, 255 } },
            { "Demon", "Enemies level up alongside your character and do 200% damage.", SDL_Color{ 190, 120, 220, 255 } },
            { "Lilin", "Enemies level up alongside your character, do 200% damage, and take only 50% damage from all sources.", SDL_Color{ 210, 110, 240, 255 } },
            { "Lilith", "Enemies are always 2x your character's level, do 400% damage, and take only 25% damage from all sources. Prepare for intense challenge.", SDL_Color{ 240, 90, 110, 255 } }
        };

        for (int i = 0; i < 5; ++i)
        {
            bool isCurrentDiff = (gameContext->settings.gameplay.difficultyLevel == i);
            float nameOffset = UIWidget::getTextWidth(tiers[i].name, uiScale * 0.88f) + (16.0f * uiScale);
            float descW = textW - nameOffset - (14.0f * uiScale);
            
            float descH = UIWidget::drawTextWrapped(renderer, "", textX + nameOffset, curY + (6.0f * uiScale), descW, Theme::colors.textSecondary, uiScale * 0.80f);
            float cardH2 = 30.0f * uiScale;

            SDL_FRect tierRect = { textX, curY, textW, cardH2 };
            bool tierHovered = (mousePos.x >= tierRect.x && mousePos.x <= tierRect.x + tierRect.w && mousePos.y >= tierRect.y && mousePos.y <= tierRect.y + tierRect.h);

            SDL_Color tierBg = isCurrentDiff ? SDL_Color{ 50, 20, 40, 255 } : (tierHovered ? SDL_Color{ 45, 48, 56, 255 } : Theme::colors.bgSlot);
            SDL_Color tierBorder = isCurrentDiff ? Theme::colors.borderSelected : (tierHovered ? Theme::colors.textGold : Theme::colors.borderNormal);
            UIWidget::drawPanel(renderer, tierRect, tierBg, tierBorder);

            UIWidget::drawText(renderer, tiers[i].name, textX + (10.0f * uiScale), curY + (6.0f * uiScale), tiers[i].col, uiScale * 0.88f);
            UIWidget::drawTextWrapped(renderer, tiers[i].desc, textX + nameOffset, curY + (6.0f * uiScale), descW, isCurrentDiff ? Theme::colors.textPrimary : Theme::colors.textSecondary, uiScale * 0.80f);

            if (tierHovered && clicked)
            {
                gameContext->settings.gameplay.difficultyLevel = i;
                opt->difficultyLevel = i;
                static constexpr float diffMults[] = { 1.0f, 1.25f, 2.0f, 2.5f, 4.0f };
                gameContext->settings.gameplay.difficultyMultiplier = diffMults[i];
                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }

            curY += cardH2 + (5.0f * uiScale);
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
            std::string infoStr = "> Click for more info.";
            float strW = UIWidget::getTextWidth(infoStr, uiScale * 0.86f);
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

            float curBarX = padX;
            for (const auto& seg : segments)
            {
                float segW = barW * (seg.first / total);
                SDL_FRect segRect = { curBarX, curY, segW, barH };
                SDL_SetRenderDrawColor(renderer, seg.second.r, seg.second.g, seg.second.b, seg.second.a);
                SDL_RenderFillRect(renderer, &segRect);
                curBarX += segW;
            }
            curY += barH + (12.0f * uiScale);
        };

        // Helper lambda: Setting Row with Segments (e.g. OFF / ON)
        auto renderOptionCard = [&](const std::string& title, SDL_Color titleCol, const std::string& description, const std::vector<std::string>& pillLabels, int selectedIndex, std::function<void(int)> onSelect) {
            float cardWidth = availableW;
            float leftW = cardWidth - (180.0f * uiScale);
            float cardMinH = 48.0f * uiScale;

            SDL_FRect cardRect = { padX, curY, cardWidth, cardMinH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (7.0f * uiScale), titleCol, uiScale * 0.88f);
            float titleW = UIWidget::getTextWidth(title + ":", uiScale * 0.88f);

            float descH = UIWidget::drawTextWrapped(renderer, description, padX + (10.0f * uiScale) + titleW + (8.0f * uiScale), curY + (7.0f * uiScale), leftW - titleW - (18.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

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
                float labelW = UIWidget::getTextWidth(pillLabels[p], uiScale * 0.78f);
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

            static constexpr std::string_view freqLabels[] = { "Off", "Minimal", "Low", "Average", "High", "Abundant" };
            float pillTotalW = 280.0f * uiScale;
            float pillH = 22.0f * uiScale;
            float pillItemW = (pillTotalW - (5.0f * 4.0f * uiScale)) / 6.0f;
            float pillStartX = padX + cardWidth - pillTotalW - (12.0f * uiScale);
            float pillY = curY + (5.0f * uiScale);

            for (size_t p = 0; p < std::size(freqLabels); ++p)
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
                float labelW = UIWidget::getTextWidth(std::string(freqLabels[p]), uiScale * 0.75f);
                UIWidget::drawText(renderer, std::string(freqLabels[p]), pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), pTextCol, uiScale * 0.75f);

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

        auto freqIdxToFloat = [](int idx) -> float {
            static constexpr float vals[] = { 0.0f, 5.0f, 15.0f, 30.0f, 60.0f, 100.0f };
            return (idx >= 0 && idx < 6) ? vals[idx] : 30.0f;
        };

        auto floatToFreqIdx = [](float val) -> int {
            if (val < 2.5f) return 0;
            if (val < 10.0f) return 1;
            if (val < 22.5f) return 2;
            if (val < 45.0f) return 3;
            if (val < 80.0f) return 4;
            return 5;
        };

        if (opt->contentCategory == ContentOptionsCategory::MISC)
        {
            renderOptionCard("Autosave Frequency", Theme::colors.companion, "Choose how often want the game to autosave when you transition from one map to another.",
                             { "Always", "Daily", "Weekly", "Off" },
                             gameContext->settings.gameplay.autoSaveFrequency,
                             [&](int idx) {
                                 gameContext->settings.gameplay.autoSaveFrequency = idx;
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Artwork", SDL_Color{ 100, 200, 255, 255 }, "Enables artwork to be displayed in characters' information screens.",
                             { "OFF", "ON" },
                             gameContext->settings.display.showArtwork ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.display.showArtwork = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Thumbnails", SDL_Color{ 100, 200, 255, 255 }, "Enables tooltips containing thumbnail images of the character.",
                             { "OFF", "ON" },
                             gameContext->settings.display.showThumbnails ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.display.showThumbnails = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Shared Encyclopedia", Theme::colors.textGold, "When enabled, your character will use the shared Encyclopedia across playthroughs.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.sharedEncyclopedia ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.sharedEncyclopedia = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Storm interruptions", SDL_Color{ 235, 140, 255, 255 }, "When enabled, arcane storms will interrupt dialogue to let you know that they've started.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.stormInterruptions ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.stormInterruptions = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });
        }
        else if (opt->contentCategory == ContentOptionsCategory::GAMEPLAY)
        {
            renderOptionCard("Enchantment Instability", SDL_Color{ 235, 140, 255, 255 }, "Toggle the 'enchantment instability' mechanic, restricting enchanted items.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.enchantmentInstability ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.enchantmentInstability = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Bad Ends", SDL_Color{ 255, 110, 120, 255 }, "Toggle the ability to trigger 'bad ends', which end the game when encountered.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.badEndsEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.badEndsEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Level Drain", SDL_Color{ 255, 90, 100, 255 }, "Toggle the use of the 'orgasmic level drain' perk by unique NPCs.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.levelDrainEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.levelDrainEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Opportunistic attackers", SDL_Color{ 255, 110, 120, 255 }, "Makes random attacks more likely when you're high on lust, low health, or exposed.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.opportunisticAttackers ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.opportunisticAttackers = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Auto-Loot Defeated Enemies", SDL_Color{ 100, 200, 255, 255 }, "Automatically collects dropped coin and essentials upon combat victory.",
                             { "OFF", "ON" },
                             gameContext->settings.gameplay.autoLoot ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.gameplay.autoLoot = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            int curLossIdx = (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.05f) ? 0 :
                             (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.20f ? 1 :
                             (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.40f ? 2 : 3));
            renderOptionCard("Currency Loss on Defeat", SDL_Color{ 255, 180, 80, 255 }, "Percentage of carried gold dropped when suffering a defeat.",
                             { "0%", "10%", "25%", "50%" },
                             curLossIdx,
                             [&](int idx) {
                                 static constexpr float lossPcts[] = { 0.0f, 0.10f, 0.25f, 0.50f };
                                 gameContext->settings.gameplay.currencyLossOnDefeatPercent = lossPcts[idx];
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });
        }
        else if (opt->contentCategory == ContentOptionsCategory::SEX_AND_FETISHES)
        {
            renderOptionCard("Non-consent", SDL_Color{ 255, 95, 120, 255 }, "This enables the 'resist' pace in sex scenes, which contains more extreme non-consensual descriptions.",
                             { "OFF", "ON" },
                             gameContext->settings.content.nonConEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.content.nonConEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Sadistic sex / Extreme", SDL_Color{ 255, 95, 120, 255 }, "This unlocks 'sadistic' sex actions such as rough treatment and heavy restraints.",
                             { "OFF", "ON" },
                             gameContext->settings.content.extremeContentEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.content.extremeContentEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Public Sex Exposure", SDL_Color{ 255, 105, 180, 255 }, "Allows public exhibitionism and onlookers during intimate encounters in open zones.",
                             { "OFF", "ON" },
                             gameContext->settings.content.publicSexEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.content.publicSexEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            int fluidIdx = (gameContext->settings.content.fluidMultiplier <= 0.3f) ? 0 :
                           (gameContext->settings.content.fluidMultiplier <= 0.7f ? 1 :
                           (gameContext->settings.content.fluidMultiplier <= 1.5f ? 2 :
                           (gameContext->settings.content.fluidMultiplier <= 3.0f ? 3 : 4)));
            renderOptionCard("Fluid Multiplier", SDL_Color{ 100, 210, 255, 255 }, "Scales fluid volume generated during climax and bodily transformations.",
                             { "0.25x", "0.5x", "1.0x", "2.0x", "4.0x" },
                             fluidIdx,
                             [&](int idx) {
                                 static constexpr float flVals[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
                                 gameContext->settings.content.fluidMultiplier = flVals[idx];
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });
        }
        else if (opt->contentCategory == ContentOptionsCategory::BODIES)
        {
            renderOptionCard("Pregnancy", SDL_Color{ 185, 230, 110, 255 }, "Enables insemination, gestation progression, and progeny generation mechanics.",
                             { "OFF", "ON" },
                             gameContext->settings.content.pregnancyEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.content.pregnancyEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            renderOptionCard("Lactation", SDL_Color{ 100, 210, 255, 255 }, "Enables breast engorgement, milk production, and related dialogue / feeding actions.",
                             { "OFF", "ON" },
                             gameContext->settings.content.lactationEnabled ? 1 : 0,
                             [&](int idx) {
                                 gameContext->settings.content.lactationEnabled = (idx == 1);
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });

            int tfIdx = (gameContext->settings.content.transformationSpeedMultiplier >= 5.0f) ? 0 :
                        (gameContext->settings.content.transformationSpeedMultiplier >= 1.5f ? 1 :
                        (gameContext->settings.content.transformationSpeedMultiplier >= 0.8f ? 2 :
                        (gameContext->settings.content.transformationSpeedMultiplier > 0.0f ? 3 : 4)));
            renderOptionCard("Transformation Speed", SDL_Color{ 240, 180, 80, 255 }, "Governs speed of anatomical mutations and bodily reshaping.",
                             { "Instant", "Fast", "Normal", "Slow", "Off" },
                             tfIdx,
                             [&](int idx) {
                                 static constexpr float tfVals[] = { 10.0f, 2.0f, 1.0f, 0.5f, 0.0f };
                                 gameContext->settings.content.transformationSpeedMultiplier = tfVals[idx];
                                 settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                             });
        }
        else if (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS)
        {
            renderInfoDropdown();

            auto& d = gameContext->settings.demographics;
            renderFrequencyRow("Male / Masculine", SDL_Color{ 100, 160, 255, 255 }, "Masculine bodies with male anatomy.", floatToFreqIdx(d.percentMale),
                               [&](int idx) { d.percentMale = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Female / Feminine", SDL_Color{ 255, 120, 180, 255 }, "Feminine bodies with female anatomy.", floatToFreqIdx(d.percentFemale),
                               [&](int idx) { d.percentFemale = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Hermaphrodite", SDL_Color{ 180, 130, 255, 255 }, "Dual sex anatomy with breasts and penis.", floatToFreqIdx(d.percentHermaphrodite),
                               [&](int idx) { d.percentHermaphrodite = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Gynomorph", SDL_Color{ 255, 140, 220, 255 }, "Feminine frame with penis and breasts.", floatToFreqIdx(d.percentGynomorph),
                               [&](int idx) { d.percentGynomorph = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Andromorph", SDL_Color{ 120, 190, 255, 255 }, "Masculine frame with vagina and flat chest.", floatToFreqIdx(d.percentAndromorph),
                               [&](int idx) { d.percentAndromorph = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Asexual / Null", SDL_Color{ 180, 180, 190, 255 }, "Neutral form with smooth groin.", floatToFreqIdx(d.percentNull),
                               [&](int idx) { d.percentNull = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderDistributionBar({
                { d.percentMale, SDL_Color{ 100, 160, 255, 255 } },
                { d.percentFemale, SDL_Color{ 255, 120, 180, 255 } },
                { d.percentHermaphrodite, SDL_Color{ 180, 130, 255, 255 } },
                { d.percentGynomorph, SDL_Color{ 255, 140, 220, 255 } },
                { d.percentAndromorph, SDL_Color{ 120, 190, 255, 255 } },
                { d.percentNull, SDL_Color{ 180, 180, 190, 255 } }
            });
        }
        else if (opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS)
        {
            renderInfoDropdown();

            auto& d = gameContext->settings.demographics;
            renderFrequencyRow("Heterosexual", SDL_Color{ 100, 160, 255, 255 }, "Attracted to opposite sex.", floatToFreqIdx(d.percentHetero),
                               [&](int idx) { d.percentHetero = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Bisexual", SDL_Color{ 180, 130, 255, 255 }, "Attracted to both sexes.", floatToFreqIdx(d.percentBi),
                               [&](int idx) { d.percentBi = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Homosexual", SDL_Color{ 255, 110, 180, 255 }, "Attracted to same sex.", floatToFreqIdx(d.percentHomo),
                               [&](int idx) { d.percentHomo = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Asexual", SDL_Color{ 180, 180, 190, 255 }, "Low or no sexual interest.", floatToFreqIdx(d.percentAsexual),
                               [&](int idx) { d.percentAsexual = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderDistributionBar({
                { d.percentHetero, SDL_Color{ 100, 160, 255, 255 } },
                { d.percentBi, SDL_Color{ 180, 130, 255, 255 } },
                { d.percentHomo, SDL_Color{ 255, 110, 180, 255 } },
                { d.percentAsexual, SDL_Color{ 180, 180, 190, 255 } }
            });
        }
        else if (opt->contentCategory == ContentOptionsCategory::AGE_PREFS)
        {
            renderInfoDropdown();

            auto& d = gameContext->settings.demographics;
            renderFrequencyRow("Young Adult (18-25)", SDL_Color{ 140, 220, 110, 255 }, "", floatToFreqIdx(d.percentYoungAdult),
                               [&](int idx) { d.percentYoungAdult = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Adult (26-40)", SDL_Color{ 100, 200, 255, 255 }, "", floatToFreqIdx(d.percentAdult),
                               [&](int idx) { d.percentAdult = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Mature (41-60)", SDL_Color{ 220, 180, 90, 255 }, "", floatToFreqIdx(d.percentMature),
                               [&](int idx) { d.percentMature = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Elder (60+)", SDL_Color{ 200, 140, 140, 255 }, "", floatToFreqIdx(d.percentElder),
                               [&](int idx) { d.percentElder = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderDistributionBar({
                { d.percentYoungAdult, SDL_Color{ 140, 220, 110, 255 } },
                { d.percentAdult, SDL_Color{ 100, 200, 255, 255 } },
                { d.percentMature, SDL_Color{ 220, 180, 90, 255 } },
                { d.percentElder, SDL_Color{ 200, 140, 140, 255 } }
            });
        }
        else if (opt->contentCategory == ContentOptionsCategory::FURRY_PREFS)
        {
            renderInfoDropdown();

            auto& d = gameContext->settings.demographics;
            renderFrequencyRow("Human / Pureblood", Theme::colors.textPrimary, "Regular human bodies without morph features.", floatToFreqIdx(d.percentHuman),
                               [&](int idx) { d.percentHuman = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Partial Morph (Ears/Tail)", SDL_Color{ 200, 160, 120, 255 }, "Humanoid bodies with animal ears, tails, or horns.", floatToFreqIdx(d.percentPartial),
                               [&](int idx) { d.percentPartial = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Anthropomorphic", SDL_Color{ 180, 130, 255, 255 }, "Full fur, muzzle, and digitigrade anatomy on bipedal form.", floatToFreqIdx(d.percentAnthro),
                               [&](int idx) { d.percentAnthro = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderFrequencyRow("Feral / Bestial", SDL_Color{ 240, 110, 110, 255 }, "Quadrupedal / animalistic body structures.", floatToFreqIdx(d.percentFeral),
                               [&](int idx) { d.percentFeral = freqIdxToFloat(idx); settingsManager::saveToFile(gameContext->settings, "data/settings.json"); });

            renderDistributionBar({
                { d.percentHuman, Theme::colors.textPrimary },
                { d.percentPartial, SDL_Color{ 200, 160, 120, 255 } },
                { d.percentAnthro, SDL_Color{ 180, 130, 255, 255 } },
                { d.percentFeral, SDL_Color{ 240, 110, 110, 255 } }
            });
        }
        else if (opt->contentCategory == ContentOptionsCategory::FETISH_PREFS)
        {
            renderInfoDropdown();

            static const char* fetishes[10] = {
                "Anal", "Buttslut", "Vaginal", "Pussy slut", "Oral",
                "Oral performer", "Breasts lover", "Breasts", "Milk lover", "Lactation"
            };

            static constexpr std::string_view fetPills[] = {
                "Disabled", "Hate", "Dislike", "Neutral", "Like", "Love", "Always"
            };

            for (int i = 0; i < 10; ++i)
            {
                float cardWidth = availableW;
                float rowMinH = 34.0f * uiScale;
                SDL_FRect rowRect = { padX, curY, cardWidth, rowMinH };
                UIWidget::drawPanel(renderer, rowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, "[i]", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                UIWidget::drawText(renderer, fetishes[i], padX + (36.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);

                float pillTotalW = 340.0f * uiScale;
                float pillH = 22.0f * uiScale;
                float pillItemW = (pillTotalW - (6.0f * 4.0f * uiScale)) / 7.0f;
                float pillStartX = padX + cardWidth - pillTotalW - (10.0f * uiScale);
                float pillY = curY + (6.0f * uiScale);

                int curRating = 3;
                auto it = gameContext->settings.content.fetishPreferences.find(fetishes[i]);
                if (it != gameContext->settings.content.fetishPreferences.end())
                {
                    curRating = it->second;
                }

                for (size_t p = 0; p < std::size(fetPills); ++p)
                {
                    SDL_FRect pRect = { pillStartX + (p * (pillItemW + 4.0f * uiScale)), pillY, pillItemW, pillH };
                    bool pHovered = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                     mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                    bool isSelected = (static_cast<int>(p) == curRating);

                    SDL_Color bgCol = Theme::colors.bgButton;
                    SDL_Color borderCol = Theme::colors.borderButton;
                    SDL_Color pTextCol = Theme::colors.textMuted;

                    if (isSelected)
                    {
                        if (p == 0) { bgCol = SDL_Color{ 45, 45, 52, 255 }; borderCol = SDL_Color{ 90, 90, 100, 255 }; pTextCol = Theme::colors.textMuted; }
                        else if (p == 1) { bgCol = SDL_Color{ 100, 20, 25, 255 }; borderCol = SDL_Color{ 220, 50, 60, 255 }; pTextCol = Theme::colors.textPrimary; }
                        else if (p == 2) { bgCol = SDL_Color{ 90, 45, 25, 255 }; borderCol = SDL_Color{ 200, 90, 50, 255 }; pTextCol = Theme::colors.textPrimary; }
                        else if (p == 3) { bgCol = SDL_Color{ 75, 24, 55, 255 }; borderCol = SDL_Color{ 245, 80, 175, 255 }; pTextCol = SDL_Color{ 255, 220, 245, 255 }; }
                        else if (p == 4) { bgCol = SDL_Color{ 24, 75, 45, 255 }; borderCol = SDL_Color{ 60, 200, 110, 255 }; pTextCol = SDL_Color{ 220, 255, 230, 255 }; }
                        else if (p == 5) { bgCol = SDL_Color{ 20, 85, 100, 255 }; borderCol = SDL_Color{ 50, 220, 240, 255 }; pTextCol = SDL_Color{ 210, 255, 255, 255 }; }
                        else { bgCol = SDL_Color{ 95, 75, 20, 255 }; borderCol = SDL_Color{ 255, 215, 60, 255 }; pTextCol = SDL_Color{ 255, 245, 200, 255 }; }
                    }
                    else if (pHovered)
                    {
                        bgCol = SDL_Color{ 50, 54, 62, 255 };
                        borderCol = Theme::colors.textGold;
                        pTextCol = Theme::colors.textGold;
                    }

                    SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                    SDL_RenderFillRect(renderer, &pRect);
                    SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                    SDL_RenderRect(renderer, &pRect);

                    float labelW = UIWidget::getTextWidth(std::string(fetPills[p]), uiScale * 0.72f);
                    UIWidget::drawText(renderer, std::string(fetPills[p]), pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), pTextCol, uiScale * 0.72f);

                    if (pHovered && clicked)
                    {
                        gameContext->settings.content.fetishPreferences[fetishes[i]] = static_cast<int>(p);
                        settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                        gameContext->input.consumeMouseClick();
                    }
                }

                curY += rowMinH + (6.0f * uiScale);
            }
        }

        return (curY - startY);
    }
}

static SDL_Color getChoiceColor(const std::string& choice)
{
    auto toLower = [](std::string str) {
        for (char& c : str) c = std::tolower(static_cast<unsigned char>(c));
        return str;
    };
    std::string s = toLower(choice);

    // Hair Colors
    if (s == "black") return SDL_Color{ 145, 150, 155, 255 };
    if (s == "dark brown") return SDL_Color{ 140, 85, 55, 255 };
    if (s == "brown") return SDL_Color{ 175, 115, 65, 255 };
    if (s == "auburn") return SDL_Color{ 185, 80, 50, 255 };
    if (s == "ginger") return SDL_Color{ 240, 125, 45, 255 };
    if (s == "blonde") return SDL_Color{ 245, 215, 95, 255 };
    if (s == "platinum") return SDL_Color{ 230, 235, 240, 255 };
    if (s == "grey" || s == "gray") return SDL_Color{ 180, 185, 190, 255 };
    if (s == "white") return SDL_Color{ 245, 245, 250, 255 };
    if (s == "pink") return SDL_Color{ 255, 115, 185, 255 };
    if (s == "light pink") return SDL_Color{ 255, 180, 205, 255 };
    if (s == "red") return SDL_Color{ 235, 60, 60, 255 };
    if (s == "dark red") return SDL_Color{ 170, 35, 35, 255 };
    if (s == "light red") return SDL_Color{ 255, 125, 125, 255 };
    if (s == "purple") return SDL_Color{ 190, 95, 240, 255 };
    if (s == "light purple") return SDL_Color{ 220, 170, 250, 255 };
    if (s == "blue") return SDL_Color{ 75, 155, 255, 255 };
    if (s == "light blue") return SDL_Color{ 140, 210, 255, 255 };
    if (s == "gold") return SDL_Color{ 255, 215, 30, 255 };
    if (s == "silver") return SDL_Color{ 210, 215, 225, 255 };
    if (s == "clear") return SDL_Color{ 165, 225, 240, 255 };
    if (s == "hazel") return SDL_Color{ 190, 160, 70, 255 };
    if (s == "green") return SDL_Color{ 75, 200, 105, 255 };
    if (s == "amber") return SDL_Color{ 255, 185, 40, 255 };
    if (s == "violet") return SDL_Color{ 200, 115, 250, 255 };

    // Skin Tones
    if (s == "pale") return SDL_Color{ 252, 228, 214, 255 };
    if (s == "light") return SDL_Color{ 245, 212, 186, 255 };
    if (s == "porcelain") return SDL_Color{ 255, 243, 236, 255 };
    if (s == "rosy") return SDL_Color{ 245, 192, 182, 255 };
    if (s == "olive") return SDL_Color{ 200, 185, 130, 255 };
    if (s == "tanned") return SDL_Color{ 215, 155, 105, 255 };
    if (s == "dark") return SDL_Color{ 155, 95, 60, 255 };
    if (s == "ebony") return SDL_Color{ 110, 70, 50, 255 };

    return SDL_Color{ 0, 0, 0, 0 };
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

    auto activeTabs = cc->getActiveTabs();
    int tabCount = static_cast<int>(activeTabs.size());
    if (cc->step >= tabCount) cc->step = std::max(0, tabCount - 1);
    EditorTabId currentTab = activeTabs[cc->step];

    // 1. Top Header Banner with Tab Title
    float headerH = 28.0f * uiScale;
    SDL_FRect headerRect = { centerX - (240.0f * uiScale), curY, 480.0f * uiScale, headerH };
    std::string headerTitle = std::format("{} - {}", cc->config.title, cc->getTabName(currentTab));
    UIWidget::drawHeader(renderer, headerRect, headerTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
    curY += headerH + (10.0f * uiScale);

    // 2. Character Live Preview Summary Banner
    float prevH = 44.0f * uiScale;
    SDL_FRect prevRect = { padX, curY, availableW, prevH };
    UIWidget::drawPanel(renderer, prevRect, Theme::colors.bgSlot, Theme::colors.borderSelected);

    std::string chosenFirst = (cc->gender == "Female") ? cc->feminineName : cc->masculineName;
    std::string fullName = cc->surname.empty() ? chosenFirst : (chosenFirst + " " + cc->surname);
    std::string previewLine1 = std::format("Hero: {}  |  {} ({})  |  Orientation: {}", fullName, cc->gender, cc->femininity, cc->orientation);
    std::string previewLine2 = std::format("Body: {}cm, {}, {}  |  Hair: {} {}  |  Eyes: {}", cc->heightCm, cc->bodySize, cc->muscleDefinition, cc->hairColor, cc->hairStyle, cc->eyeColor);

    UIWidget::drawText(renderer, previewLine1, padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
    UIWidget::drawText(renderer, previewLine2, padX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.80f);
    curY += prevH + (12.0f * uiScale);

    // Helper: Option Row with adaptive non-clipping layout and color-coded labels
    auto renderChoiceCard = [&](const std::string& title, const std::string& description, const std::vector<std::string>& labels, int selectedIndex, std::function<void(int)> onSelect) {
        if (labels.empty()) return;

        // Measure maximum label width across all options
        float maxLabelW = 0.0f;
        for (const auto& l : labels) {
            float lw = UIWidget::getTextWidth(l, uiScale * 0.70f);
            if (lw > maxLabelW) maxLabelW = lw;
        }

        // Determine if choices fit in a single right-hand row (<= 4 items with compact labels)
        bool useSingleRowRight = (labels.size() <= 4 && ((maxLabelW + (18.0f * uiScale)) * labels.size()) < (availableW * 0.48f));

        if (useSingleRowRight)
        {
            float pillW = std::max(maxLabelW + (18.0f * uiScale), 58.0f * uiScale);
            float pillTotalW = pillW * labels.size();
            float pillStartX = padX + availableW - pillTotalW - (10.0f * uiScale);
            float pillH = 24.0f * uiScale;

            float descW = pillStartX - padX - (16.0f * uiScale);
            float cardH = (description.length() > 50) ? (52.0f * uiScale) : (44.0f * uiScale);
            float pillY = curY + ((cardH - pillH) / 2.0f);

            SDL_FRect cardRect = { padX, curY, availableW, cardH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawTextWrapped(renderer, description, padX + (10.0f * uiScale), curY + (24.0f * uiScale), descW, Theme::colors.textSecondary, uiScale * 0.74f);

            for (size_t i = 0; i < labels.size(); ++i)
            {
                SDL_FRect pRect = { pillStartX + (i * pillW), pillY, pillW, pillH };
                bool isSel = (static_cast<int>(i) == selectedIndex);
                bool pHover = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                               mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);

                SDL_Color customCol = getChoiceColor(labels[i]);
                bool hasCustomCol = (customCol.a > 0);

                SDL_Color bg = isSel ? SDL_Color{ 45, 55, 68, 255 } : (pHover ? SDL_Color{ 48, 52, 60, 255 } : Theme::colors.bgButton);
                SDL_Color border = isSel ? Theme::colors.borderSelected : (pHover ? Theme::colors.textGold : Theme::colors.borderButton);
                SDL_Color txt = hasCustomCol ? customCol : (isSel ? Theme::colors.companion : (pHover ? Theme::colors.textGold : Theme::colors.textMuted));

                SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
                SDL_RenderFillRect(renderer, &pRect);
                SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
                SDL_RenderRect(renderer, &pRect);

                float labelW = UIWidget::getTextWidth(labels[i], uiScale * 0.70f);
                UIWidget::drawText(renderer, labels[i], pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (4.0f * uiScale), txt, uiScale * 0.70f);

                if (pHover && clicked)
                {
                    onSelect(static_cast<int>(i));
                    gameContext->input.consumeMouseClick();
                }
            }
            curY += cardH + (8.0f * uiScale);
        }
        else
        {
            // Responsive Multi-Column Grid below Title & Description (zero clipping guaranteed)
            float neededPillW = maxLabelW + (18.0f * uiScale);
            int maxColsPossible = std::max(1, static_cast<int>((availableW - (20.0f * uiScale)) / neededPillW));
            int cols = std::clamp(static_cast<int>(labels.size()), 1, std::min(maxColsPossible, 8));
            int rows = (static_cast<int>(labels.size()) + cols - 1) / cols;

            float pillW = (availableW - (20.0f * uiScale) - ((cols - 1) * 4.0f * uiScale)) / static_cast<float>(cols);
            float pillH = 22.0f * uiScale;
            float descH = description.empty() ? (26.0f * uiScale) : (44.0f * uiScale);
            float cardH = descH + (rows * (pillH + 4.0f * uiScale)) + (8.0f * uiScale);

            SDL_FRect cardRect = { padX, curY, availableW, cardH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            if (!description.empty())
            {
                UIWidget::drawText(renderer, description, padX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.72f);
            }

            float gridStartY = curY + descH;
            float pillFontScale = (cols >= 7 ? (uiScale * 0.64f) : (uiScale * 0.70f));

            for (size_t i = 0; i < labels.size(); ++i)
            {
                int r = static_cast<int>(i / cols);
                int c = static_cast<int>(i % cols);
                SDL_FRect pRect = { padX + (10.0f * uiScale) + (c * (pillW + 4.0f * uiScale)), gridStartY + (r * (pillH + 4.0f * uiScale)), pillW, pillH };

                bool isSel = (static_cast<int>(i) == selectedIndex);
                bool pHover = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                               mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);

                SDL_Color customCol = getChoiceColor(labels[i]);
                bool hasCustomCol = (customCol.a > 0);

                SDL_Color bg = isSel ? SDL_Color{ 45, 55, 68, 255 } : (pHover ? SDL_Color{ 48, 52, 60, 255 } : Theme::colors.bgButton);
                SDL_Color border = isSel ? Theme::colors.borderSelected : (pHover ? Theme::colors.textGold : Theme::colors.borderButton);
                SDL_Color txt = hasCustomCol ? customCol : (isSel ? Theme::colors.companion : (pHover ? Theme::colors.textGold : Theme::colors.textMuted));

                SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
                SDL_RenderFillRect(renderer, &pRect);
                SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
                SDL_RenderRect(renderer, &pRect);

                float labelW = UIWidget::getTextWidth(labels[i], pillFontScale);
                UIWidget::drawText(renderer, labels[i], pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), txt, pillFontScale);

                if (pHover && clicked)
                {
                    onSelect(static_cast<int>(i));
                    gameContext->input.consumeMouseClick();
                }
            }
            curY += cardH + (8.0f * uiScale);
        }
    };

    // Helper: Stepper Row
    auto renderStepperCard = [&](const std::string& title, const std::string& valueStr, std::function<void(int)> onStep) {
        float cardH = 44.0f * uiScale;
        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (12.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.90f);

        float btnW = 34.0f * uiScale;
        float btnH = 22.0f * uiScale;
        float stepStartX = padX + availableW - (btnW * 4.0f) - (80.0f * uiScale) - (10.0f * uiScale);
        float stepY = curY + (11.0f * uiScale);

        SDL_FRect m2 = { stepStartX, stepY, btnW, btnH };
        SDL_FRect m1 = { stepStartX + btnW + (4.0f * uiScale), stepY, btnW, btnH };
        SDL_FRect p1 = { stepStartX + (btnW * 2.0f) + (80.0f * uiScale) + (4.0f * uiScale), stepY, btnW, btnH };
        SDL_FRect p2 = { stepStartX + (btnW * 3.0f) + (80.0f * uiScale) + (8.0f * uiScale), stepY, btnW, btnH };

        UIWidget::drawColoredButton(renderer, m2, "--", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, m1, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);

        float valW = UIWidget::getTextWidth(valueStr, uiScale * 0.85f);
        UIWidget::drawText(renderer, valueStr, stepStartX + (btnW * 2.0f) + (((80.0f * uiScale) - valW) / 2.0f), stepY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        UIWidget::drawColoredButton(renderer, p1, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, p2, "++", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.72f);

        if (clicked)
        {
            auto check = [&](const SDL_FRect& r) { return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h); };
            if (check(m2)) { onStep(-5); gameContext->input.consumeMouseClick(); }
            else if (check(m1)) { onStep(-1); gameContext->input.consumeMouseClick(); }
            else if (check(p1)) { onStep(1); gameContext->input.consumeMouseClick(); }
            else if (check(p2)) { onStep(5); gameContext->input.consumeMouseClick(); }
        }
        curY += cardH + (8.0f * uiScale);
    };

    if (currentTab == EditorTabId::IDENTITY)
    {
        // 1. Gender
        if (cc->config.isOptionEnabled("gender"))
        {
            auto genderOpts = cc->config.filterChoices("gender", { "Male", "Female" });
            int genIdx = (cc->gender == "Female") ? 1 : 0;
            if (genIdx >= (int)genderOpts.size()) genIdx = 0;
            renderChoiceCard("Biological Sex", "Determines initial physical anatomy and starting bodily equipment.", genderOpts, genIdx, [&](int i) {
                cc->gender = genderOpts[i];
            });
        }

        // 2. Femininity
        if (cc->config.isOptionEnabled("femininity"))
        {
            static const std::vector<std::string> allFem = { "Very Masculine", "Masculine", "Androgynous", "Feminine", "Very Feminine" };
            auto femOpts = cc->config.filterChoices("femininity", allFem);
            int femIdx = 1;
            for (size_t i = 0; i < femOpts.size(); ++i) if (cc->femininity == femOpts[i]) femIdx = static_cast<int>(i);
            renderChoiceCard("Femininity", "How feminine or masculine your overall facial and bodily presentation is.", femOpts, femIdx, [&](int i) {
                cc->femininity = femOpts[i];
            });
        }

        // 3. Orientation
        if (cc->config.isOptionEnabled("orientation"))
        {
            static const std::vector<std::string> allOri = { "Androphilic", "Ambiphilic", "Gynephilic" };
            auto oriOpts = cc->config.filterChoices("orientation", allOri);
            int oriIdx = 1;
            for (size_t i = 0; i < oriOpts.size(); ++i) if (cc->orientation == oriOpts[i]) oriIdx = static_cast<int>(i);
            renderChoiceCard("Sexual Orientation", "Attraction preference towards masculinity, femininity, or both.", oriOpts, oriIdx, [&](int i) {
                cc->orientation = oriOpts[i];
            });
        }

        // 4. Starting Month
        if (cc->config.isOptionEnabled("start_month"))
        {
            float monthCardH = 72.0f * uiScale;
            SDL_FRect monthRect = { padX, curY, availableW, monthCardH };
            UIWidget::drawPanel(renderer, monthRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Starting Month:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "Select the calendar month in which your adventure begins.", padX + (10.0f * uiScale), curY + (20.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

            static const char* months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            static const char* fullMonths[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

            float mBtnW = (availableW - (7 * 6.0f * uiScale)) / 6.0f;
            float mBtnH = 18.0f * uiScale;
            float mGridY = curY + (38.0f * uiScale);

            for (int m = 0; m < 12; ++m)
            {
                int r = m / 6;
                int c = m % 6;
                SDL_FRect mr = { padX + (6.0f * uiScale) + (c * (mBtnW + 5.0f * uiScale)), mGridY + (r * (mBtnH + 4.0f * uiScale)), mBtnW, mBtnH };
                bool isSel = (cc->startMonth == fullMonths[m] || cc->startMonth == months[m]);
                bool mHover = (mousePos.x >= mr.x && mousePos.x <= mr.x + mr.w && mousePos.y >= mr.y && mousePos.y <= mr.y + mr.h);

                SDL_Color bg = isSel ? SDL_Color{ 45, 55, 68, 255 } : (mHover ? SDL_Color{ 48, 52, 60, 255 } : Theme::colors.bgButton);
                SDL_Color border = isSel ? Theme::colors.borderSelected : (mHover ? Theme::colors.textGold : Theme::colors.borderButton);
                SDL_Color txt = isSel ? Theme::colors.companion : (mHover ? Theme::colors.textGold : Theme::colors.textMuted);

                SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
                SDL_RenderFillRect(renderer, &mr);
                SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
                SDL_RenderRect(renderer, &mr);

                float labelW = UIWidget::getTextWidth(months[m], uiScale * 0.70f);
                UIWidget::drawText(renderer, months[m], mr.x + ((mr.w - labelW) / 2.0f), mr.y + (2.0f * uiScale), txt, uiScale * 0.70f);

                if (mHover && clicked)
                {
                    cc->startMonth = fullMonths[m];
                    cc->startMonthIdx = m;
                    gameContext->input.consumeMouseClick();
                }
            }
            curY += monthCardH + (8.0f * uiScale);
        }

        // 5. Birthday (Day, Month, Age)
        float bdayCardH = 46.0f * uiScale;
        SDL_FRect bdayRect = { padX, curY, availableW, bdayCardH };
        UIWidget::drawPanel(renderer, bdayRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Birthday & Age:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
        std::string bdayDesc = std::format("Born on the {} of {}, making you {} years old.", cc->birthDay, cc->birthMonth, cc->birthAge);
        UIWidget::drawText(renderer, bdayDesc, padX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

        float bBtnW = 24.0f * uiScale;
        float bBtnH = 20.0f * uiScale;
        float controlsTotalW = 260.0f * uiScale;
        float bdayControlsX = padX + availableW - controlsTotalW - (10.0f * uiScale);
        
        // Day Stepper: [-] D: 29 [+]
        SDL_FRect dayM = { bdayControlsX, curY + (13.0f * uiScale), bBtnW, bBtnH };
        SDL_FRect dayP = { bdayControlsX + bBtnW + (48.0f * uiScale), curY + (13.0f * uiScale), bBtnW, bBtnH };
        UIWidget::drawColoredButton(renderer, dayM, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.70f);
        std::string dStr = std::format("D: {:02d}", cc->birthDay);
        float dStrW = UIWidget::getTextWidth(dStr, uiScale * 0.72f);
        UIWidget::drawText(renderer, dStr, bdayControlsX + bBtnW + (((48.0f * uiScale) - dStrW) / 2.0f), curY + (15.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, dayP, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.70f);

        // Age Stepper: [-] Age: 22 [+]
        float ageControlsX = bdayControlsX + (135.0f * uiScale);
        SDL_FRect ageM = { ageControlsX, curY + (13.0f * uiScale), bBtnW, bBtnH };
        SDL_FRect ageP = { ageControlsX + bBtnW + (65.0f * uiScale), curY + (13.0f * uiScale), bBtnW, bBtnH };
        UIWidget::drawColoredButton(renderer, ageM, "-", SDL_Color{ 160, 45, 55, 240 }, Theme::colors.textPrimary, false, uiScale * 0.70f);
        std::string aStr = std::format("Age: {}", cc->birthAge);
        float aStrW = UIWidget::getTextWidth(aStr, uiScale * 0.72f);
        UIWidget::drawText(renderer, aStr, ageControlsX + bBtnW + (((65.0f * uiScale) - aStrW) / 2.0f), curY + (15.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
        UIWidget::drawColoredButton(renderer, ageP, "+", SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, false, uiScale * 0.70f);

        if (clicked)
        {
            auto check = [&](const SDL_FRect& r) { return (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h); };
            if (check(dayM)) { cc->birthDay = (cc->birthDay <= 1) ? 31 : (cc->birthDay - 1); gameContext->input.consumeMouseClick(); }
            else if (check(dayP)) { cc->birthDay = (cc->birthDay >= 31) ? 1 : (cc->birthDay + 1); gameContext->input.consumeMouseClick(); }
            else if (check(ageM)) { cc->birthAge = std::max(18, cc->birthAge - 1); gameContext->input.consumeMouseClick(); }
            else if (check(ageP)) { cc->birthAge = std::min(99, cc->birthAge + 1); gameContext->input.consumeMouseClick(); }
        }
        curY += bdayCardH + (8.0f * uiScale);

        // 6. Personality Traits Chips
        if (cc->config.isOptionEnabled("personality_traits"))
        {
            static const std::vector<std::string> allP = { "Confident", "Shy", "Kind", "Selfish", "Naive", "Cynical", "Brave", "Cowardly", "Lewd", "Innocent", "Prude" };
            float pCardH = 68.0f * uiScale;
            SDL_FRect pRect = { padX, curY, availableW, pCardH };
            UIWidget::drawPanel(renderer, pRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Personality Traits:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);
            UIWidget::drawText(renderer, "Click traits to toggle them. Influences roleplaying choices.", padX + (10.0f * uiScale), curY + (20.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

            float tBtnW = (availableW - (7 * 4.0f * uiScale)) / 6.0f;
            float tBtnH = 18.0f * uiScale;
            float tGridY = curY + (38.0f * uiScale);

            for (size_t t = 0; t < allP.size(); ++t)
            {
                int r = static_cast<int>(t / 6);
                int c = static_cast<int>(t % 6);
                SDL_FRect tr = { padX + (6.0f * uiScale) + (c * (tBtnW + 4.0f * uiScale)), tGridY + (r * (tBtnH + 4.0f * uiScale)), tBtnW, tBtnH };
                bool isSel = (cc->personalityTraits.contains(allP[t]));
                bool tHover = (mousePos.x >= tr.x && mousePos.x <= tr.x + tr.w && mousePos.y >= tr.y && mousePos.y <= tr.y + tr.h);

                SDL_Color bg = isSel ? SDL_Color{ 45, 65, 55, 255 } : (tHover ? SDL_Color{ 48, 52, 60, 255 } : Theme::colors.bgButton);
                SDL_Color border = isSel ? Theme::colors.companion : (tHover ? Theme::colors.textGold : Theme::colors.borderButton);
                SDL_Color txt = isSel ? Theme::colors.companion : (tHover ? Theme::colors.textGold : Theme::colors.textMuted);

                SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
                SDL_RenderFillRect(renderer, &tr);
                SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
                SDL_RenderRect(renderer, &tr);

                float labelW = UIWidget::getTextWidth(allP[t], uiScale * 0.65f);
                UIWidget::drawText(renderer, allP[t], tr.x + ((tr.w - labelW) / 2.0f), tr.y + (2.0f * uiScale), txt, uiScale * 0.65f);

                if (tHover && clicked)
                {
                    if (isSel) cc->personalityTraits.erase(allP[t]);
                    else cc->personalityTraits.insert(allP[t]);
                    gameContext->input.consumeMouseClick();
                }
            }
            curY += pCardH + (8.0f * uiScale);
        }
    }
    else if (currentTab == EditorTabId::BODY)
    {
        // 1. Height
        if (cc->config.isOptionEnabled("height"))
        {
            auto* rule = cc->config.getRule("height");
            int minH = rule ? static_cast<int>(rule->minRange) : 130;
            int maxH = rule ? static_cast<int>(rule->maxRange) : 230;
            renderStepperCard("Height", std::format("{} cm", cc->heightCm), [&](int delta) {
                cc->heightCm = std::clamp(cc->heightCm + delta, minH, maxH);
            });
        }

        // 2. Skin Tone
        if (cc->config.isOptionEnabled("skin_tone"))
        {
            static const std::vector<std::string> allSkins = { "pale", "light", "porcelain", "rosy", "olive", "tanned", "dark", "ebony" };
            auto skinOpts = cc->config.filterChoices("skin_tone", allSkins);
            int skinIdx = 1;
            for (size_t i = 0; i < skinOpts.size(); ++i) if (cc->skinPrimaryColor == skinOpts[i]) skinIdx = static_cast<int>(i);
            renderChoiceCard("Skin Colour", "The colour of the skin that's covering your body.", skinOpts, skinIdx, [&](int i) {
                cc->skinPrimaryColor = skinOpts[i];
            });
        }

        // 3. Body Size
        if (cc->config.isOptionEnabled("body_size"))
        {
            static const std::vector<std::string> allSizes = { "skinny", "slender", "average", "large", "huge" };
            auto sizeOpts = cc->config.filterChoices("body_size", allSizes);
            int sizeIdx = 2;
            for (size_t i = 0; i < sizeOpts.size(); ++i) if (cc->bodySize == sizeOpts[i]) sizeIdx = static_cast<int>(i);
            renderChoiceCard("Body Size", "How much fat your body carries.", sizeOpts, sizeIdx, [&](int i) {
                cc->bodySize = sizeOpts[i];
            });
        }

        // 4. Muscle Definition
        if (cc->config.isOptionEnabled("muscle"))
        {
            static const std::vector<std::string> allMusc = { "soft", "lightly muscled", "toned", "muscular", "ripped" };
            auto muscOpts = cc->config.filterChoices("muscle", allMusc);
            int muscIdx = 1;
            for (size_t i = 0; i < muscOpts.size(); ++i) if (cc->muscleDefinition == muscOpts[i]) muscIdx = static_cast<int>(i);
            renderChoiceCard("Muscle", "How muscular your body is.", muscOpts, muscIdx, [&](int i) {
                cc->muscleDefinition = muscOpts[i];
            });
        }

        // 5. Ass Size
        static const std::vector<std::string> allAssSizes = { "tiny", "small", "average-sized", "large", "huge" };
        int assIdx = std::clamp(cc->assSize, 0, 4);
        renderChoiceCard("Ass Size", "How large your ass is.", allAssSizes, assIdx, [&](int i) {
            cc->assSize = i;
        });

        // 6. Hip Size
        static const std::vector<std::string> allHipSizes = { "tiny", "small", "average-sized", "large", "huge" };
        int hipIdx = std::clamp(cc->hipSize, 0, 4);
        renderChoiceCard("Hip Size", "How wide your hips are.", allHipSizes, hipIdx, [&](int i) {
            cc->hipSize = i;
        });

        // 7. Bleached Anus
        static const std::vector<std::string> bleachOpts = { "Bleached", "Natural" };
        int bleachIdx = cc->anusBleached ? 0 : 1;
        renderChoiceCard("Bleached Anus", "Whether you've bleached around your anus.", bleachOpts, bleachIdx, [&](int i) {
            cc->anusBleached = (i == 0);
        });

        // 8. Composite Body Shape Indicator
        std::string shapeRating = EditorConfig::calculateBodyShape(cc->muscleDefinition, cc->bodySize);
        SDL_FRect shapeRect = { padX, curY, availableW, 34.0f * uiScale };
        UIWidget::drawPanel(renderer, shapeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Composite Body Shape:", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);
        float ratingW = UIWidget::getTextWidth(shapeRating, uiScale * 0.88f);
        UIWidget::drawText(renderer, shapeRating, padX + availableW - ratingW - (12.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        curY += (42.0f * uiScale);
    }
    else if (currentTab == EditorTabId::FACE_HAIR)
    {
        // 1. Lip Size
        static const std::vector<std::string> allLips = { "thin", "average-sized", "full", "plump", "huge" };
        int lipIdx = std::clamp(cc->lipSize, 0, 4);
        renderChoiceCard("Lip Size", "How large your lips are.", allLips, lipIdx, [&](int i) {
            cc->lipSize = i;
        });

        // 2. Puffy Lips
        static const std::vector<std::string> puffyLipOpts = { "Puffy", "Natural" };
        int pLipIdx = cc->puffyLips ? 0 : 1;
        renderChoiceCard("Puffy Lips", "Whether your lips are extra puffy.", puffyLipOpts, pLipIdx, [&](int i) {
            cc->puffyLips = (i == 0);
        });

        // 3. Iris Colour
        if (cc->config.isOptionEnabled("eye_color"))
        {
            static const std::vector<std::string> allEyes = { "brown", "hazel", "green", "blue", "grey", "amber" };
            auto eyeOpts = cc->config.filterChoices("eye_color", allEyes);
            int eyeIdx = 0;
            for (size_t i = 0; i < eyeOpts.size(); ++i) if (cc->eyeColor == eyeOpts[i]) eyeIdx = static_cast<int>(i);
            renderChoiceCard("Iris Colour", "The colour of your eye's irises.", eyeOpts, eyeIdx, [&](int i) {
                cc->eyeColor = eyeOpts[i];
            });
        }

        // 4. Hair Length
        if (cc->config.isOptionEnabled("hair_length"))
        {
            static const std::vector<std::string> hairLengths = { "bald", "very short", "short", "shoulder-length", "long", "very long", "incredibly long" };
            int hLenIdx = 2;
            if (cc->hairLengthCm <= 0) hLenIdx = 0;
            else if (cc->hairLengthCm <= 5) hLenIdx = 1;
            else if (cc->hairLengthCm <= 15) hLenIdx = 2;
            else if (cc->hairLengthCm <= 30) hLenIdx = 3;
            else if (cc->hairLengthCm <= 60) hLenIdx = 4;
            else if (cc->hairLengthCm <= 90) hLenIdx = 5;
            else hLenIdx = 6;

            renderChoiceCard("Hair Length", "Choose how long your hair is.", hairLengths, hLenIdx, [&](int i) {
                static const int lenVals[7] = { 0, 5, 15, 30, 60, 90, 120 };
                cc->hairLengthCm = lenVals[i];
            });
        }

        // 5. Hair Style (Filtered by Length Requirement)
        if (cc->config.isOptionEnabled("hair_style"))
        {
            auto validStyles = EditorConfig::getValidHairstyles(cc->hairLengthCm);
            auto styleOpts = cc->config.filterChoices("hair_style", validStyles);
            int styleIdx = 0;
            bool foundStyle = false;
            for (size_t i = 0; i < styleOpts.size(); ++i) {
                if (cc->hairStyle == styleOpts[i]) {
                    styleIdx = static_cast<int>(i);
                    foundStyle = true;
                    break;
                }
            }
            if (!foundStyle && !styleOpts.empty()) {
                cc->hairStyle = styleOpts[0];
                styleIdx = 0;
            }
            renderChoiceCard("Hair Style", "Choose your hair style. Certain hair styles are unavailable at shorter hair lengths.", styleOpts, styleIdx, [&](int i) {
                cc->hairStyle = styleOpts[i];
            });
        }

        // 6. Hair Color
        if (cc->config.isOptionEnabled("hair_color"))
        {
            static const std::vector<std::string> allHColors = { "black", "dark brown", "brown", "auburn", "ginger", "blonde", "platinum", "grey", "white", "pink" };
            auto colorOpts = cc->config.filterChoices("hair_color", allHColors);
            int colorIdx = 2;
            for (size_t i = 0; i < colorOpts.size(); ++i) if (cc->hairColor == colorOpts[i]) colorIdx = static_cast<int>(i);
            renderChoiceCard("Hair Colour", "The colour of your hair.", colorOpts, colorIdx, [&](int i) {
                cc->hairColor = colorOpts[i];
            });
        }

        // 7. Ear Type
        if (cc->config.isOptionEnabled("ear_type"))
        {
            static const std::vector<std::string> allEars = { "Human", "Cat", "Dog", "Elf", "Demon", "Cow", "Rabbit", "Dragon" };
            auto earOpts = cc->config.filterChoices("ear_type", allEars);
            int earIdx = 0;
            for (size_t i = 0; i < earOpts.size(); ++i) if (cc->earType == earOpts[i]) earIdx = static_cast<int>(i);
            renderChoiceCard("Ear Type", "Species morphology of ears.", earOpts, earIdx, [&](int i) {
                cc->earType = earOpts[i];
            });
        }
    }
    else if (currentTab == EditorTabId::BREASTS)
    {
        // 1. Breast Size
        static const std::vector<std::string> allCups = { "flat", "AA-cup", "A-cup", "B-cup", "C-cup", "D-cup", "DD-cup", "E-cup", "F-cup", "FF-cup", "G-cup", "GG-cup", "H-cup" };
        auto cupOpts = cc->config.filterChoices("chest_size", allCups);
        int cupIdx = std::clamp(cc->breastCupSize, 0, (int)cupOpts.size() - 1);
        renderChoiceCard("Breast Size", "How large your breasts are.", cupOpts, cupIdx, [&](int i) {
            cc->breastCupSize = i;
        });

        // 2. Breast Shape
        static const std::vector<std::string> allBShapes = { "round", "pointy", "perky", "side-set", "wide", "narrow" };
        int bShapeIdx = 0;
        for (size_t i = 0; i < allBShapes.size(); ++i) if (cc->breastShape == allBShapes[i]) bShapeIdx = static_cast<int>(i);
        renderChoiceCard("Breast Shape", "The shape of your breasts.", allBShapes, bShapeIdx, [&](int i) {
            cc->breastShape = allBShapes[i];
        });

        // 3. Nipple Size
        static const std::vector<std::string> allNipSizes = { "tiny", "small", "average-sized", "large", "huge" };
        int nipIdx = std::clamp(cc->nippleSize, 0, 4);
        renderChoiceCard("Nipple Size", "How large your nipples are.", allNipSizes, nipIdx, [&](int i) {
            cc->nippleSize = i;
        });

        // 4. Areolae Size
        static const std::vector<std::string> allAreSizes = { "tiny", "small", "average-sized", "large", "huge" };
        int areIdx = std::clamp(cc->areolaeSize, 0, 4);
        renderChoiceCard("Areolae Size", "How large your areolae are.", allAreSizes, areIdx, [&](int i) {
            cc->areolaeSize = i;
        });

        // 5. Puffy Nipples
        static const std::vector<std::string> pNipOpts = { "Puffy", "Natural" };
        int pNipIdx = cc->puffyNipples ? 0 : 1;
        renderChoiceCard("Puffy Nipples", "Whether your nipples are puffy.", pNipOpts, pNipIdx, [&](int i) {
            cc->puffyNipples = (i == 0);
        });

        // 6. Lactation
        static const std::vector<std::string> lactTiers = { "none", "a trickle", "a small amount", "a decent amount", "a large amount", "a huge amount", "an extreme amount", "a monstrous amount" };
        int lactIdx = std::clamp(cc->lactationTier, 0, 7);
        renderChoiceCard("Lactation", "How much milk your breasts produce.", lactTiers, lactIdx, [&](int i) {
            cc->lactationTier = i;
            cc->isLactating = (i > 0);
        });
    }
    else if (currentTab == EditorTabId::GENITALIA)
    {
        if (cc->gender == "Female")
        {
            // Capacity
            static const std::vector<std::string> allSizes5 = { "tiny", "small", "average-sized", "large", "huge" };
            int capIdx = std::clamp(cc->vaginaCapacity, 0, 4);
            renderChoiceCard("Capacity", "How accommodating your vagina is.", allSizes5, capIdx, [&](int i) {
                cc->vaginaCapacity = i;
            });

            // Labia Size
            int labIdx = std::clamp(cc->labiaSize, 0, 4);
            renderChoiceCard("Labia Size", "How large your labia are.", allSizes5, labIdx, [&](int i) {
                cc->labiaSize = i;
            });

            // Clitoris Size
            int clitIdx = std::clamp(cc->clitorisSize, 0, 4);
            renderChoiceCard("Clitoris Size", "How large your clitoris is.", allSizes5, clitIdx, [&](int i) {
                cc->clitorisSize = i;
            });
        }
        else
        {
            // Penis Length
            auto* rule = cc->config.getRule("genitals");
            float minL = rule ? rule->minRange : 5.0f;
            float maxL = rule ? rule->maxRange : 40.0f;
            renderStepperCard("Penis Length", std::format("{:.1f} cm", cc->penisLengthCm), [&](int delta) {
                cc->penisLengthCm = std::clamp(cc->penisLengthCm + static_cast<float>(delta), minL, maxL);
            });

            // Testicle Size
            static const std::vector<std::string> allSizes5 = { "tiny", "small", "average-sized", "large", "huge" };
            int testIdx = std::clamp(cc->testicleSize, 0, 4);
            renderChoiceCard("Testicle Size", "How large your testicles are.", allSizes5, testIdx, [&](int i) {
                cc->testicleSize = i;
            });

            // Cum Production
            static const std::vector<std::string> cumTiers = { "none", "a trickle", "a small amount", "a decent amount", "a large amount", "a huge amount", "an extreme amount", "a monstrous amount" };
            int cumIdx = std::clamp(cc->cumProductionTier, 0, 7);
            renderChoiceCard("Cum Production", "How much cum your body produces.", cumTiers, cumIdx, [&](int i) {
                cc->cumProductionTier = i;
            });
        }
    }
    else if (currentTab == EditorTabId::COSMETICS)
    {
        static const std::vector<std::string> makeColors = { "none", "black", "white", "red", "dark red", "light red", "pink", "light pink", "purple", "light purple", "blue", "light blue", "gold", "silver", "clear" };

        // 1. Blusher
        int blushIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->blusher == makeColors[i]) blushIdx = static_cast<int>(i);
        renderChoiceCard("Blusher", "Blusher (also called rouge) is used to colour the cheeks.", makeColors, blushIdx, [&](int i) {
            cc->blusher = makeColors[i];
        });

        // 2. Lipstick
        int lipstIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->lipstick == makeColors[i]) lipstIdx = static_cast<int>(i);
        renderChoiceCard("Lipstick", "Lipstick is used to provide colour, texture, and protection to lips.", makeColors, lipstIdx, [&](int i) {
            cc->lipstick = makeColors[i];
        });

        // 3. Eyeliner
        int eyelnIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->eyeliner == makeColors[i]) eyelnIdx = static_cast<int>(i);
        renderChoiceCard("Eyeliner", "Eyeliner is applied around the contours of the eyes.", makeColors, eyelnIdx, [&](int i) {
            cc->eyeliner = makeColors[i];
        });

        // 4. Eye Shadow
        int eyeshIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->eyeshadow == makeColors[i]) eyeshIdx = static_cast<int>(i);
        renderChoiceCard("Eye shadow", "Eye shadow is used to make the wearer's eyes stand out.", makeColors, eyeshIdx, [&](int i) {
            cc->eyeshadow = makeColors[i];
        });

        // 5. Nail Polish
        int nailIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->nailPolish == makeColors[i]) nailIdx = static_cast<int>(i);
        renderChoiceCard("Nail polish", "Nail polish is used to colour and protect fingernails.", makeColors, nailIdx, [&](int i) {
            cc->nailPolish = makeColors[i];
        });

        // 6. Toenail Polish
        int toenaIdx = 0;
        for (size_t i = 0; i < makeColors.size(); ++i) if (cc->toenailPolish == makeColors[i]) toenaIdx = static_cast<int>(i);
        renderChoiceCard("Toenail polish", "Toenail polish is used to colour and protect toenails.", makeColors, toenaIdx, [&](int i) {
            cc->toenailPolish = makeColors[i];
        });

        // 7. Piercings Section
        static const std::vector<std::string> pSockets = { "ear", "nose", "lip", "tongue", "navel", "nipple" };
        for (const auto& s : pSockets)
        {
            static const std::vector<std::string> piercStates = { "Unpierced", "Pierced" };
            int pState = cc->piercings[s] ? 1 : 0;
            std::string sTitle = s + " piercing";
            sTitle[0] = std::toupper(sTitle[0]);
            renderChoiceCard(sTitle, "Piercing through " + s + ".", piercStates, pState, [&cc, s](int i) {
                cc->piercings[s] = (i == 1);
            });
        }

        // 8. Body Hair
        static const std::vector<std::string> bhGroom = { "none", "stubble", "manicured", "trimmed", "natural", "unkempt", "bushy", "wild" };
        int pubIdx = 4;
        for (size_t i = 0; i < bhGroom.size(); ++i) if (cc->pubicHair == bhGroom[i]) pubIdx = static_cast<int>(i);
        renderChoiceCard("Pubic hair", "The body hair found in the genital area.", bhGroom, pubIdx, [&](int i) {
            cc->pubicHair = bhGroom[i];
        });

        int armIdx = 0;
        for (size_t i = 0; i < bhGroom.size(); ++i) if (cc->underarmHair == bhGroom[i]) armIdx = static_cast<int>(i);
        renderChoiceCard("Underarm hair", "The body hair found in your armpits.", bhGroom, armIdx, [&](int i) {
            cc->underarmHair = bhGroom[i];
        });

        int aHairIdx = 0;
        for (size_t i = 0; i < bhGroom.size(); ++i) if (cc->assHair == bhGroom[i]) aHairIdx = static_cast<int>(i);
        renderChoiceCard("Ass hair", "The body hair found around your asshole.", bhGroom, aHairIdx, [&](int i) {
            cc->assHair = bhGroom[i];
        });
    }
    else if (currentTab == EditorTabId::WARDROBE)
    {
        if (!cc->wardrobeInitialized) cc->initializeWardrobe();

        // 1. Wardrobe Header Card & Decency Status
        bool decent = cc->isClothedEnough();
        std::string decencyStatus = cc->getDecencyStatus();

        float bannerH = 48.0f * uiScale;
        SDL_FRect bannerRect = { padX, curY, availableW, bannerH };
        SDL_Color bannerBg = decent ? SDL_Color{ 25, 45, 32, 255 } : SDL_Color{ 50, 25, 25, 255 };
        SDL_Color bannerBorder = decent ? SDL_Color{ 60, 160, 80, 255 } : SDL_Color{ 200, 60, 60, 255 };
        UIWidget::drawPanel(renderer, bannerRect, bannerBg, bannerBorder);

        UIWidget::drawText(renderer, "Evening Wardrobe & Attire:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        UIWidget::drawText(renderer, decencyStatus, padX + (10.0f * uiScale), curY + (24.0f * uiScale), decent ? Theme::colors.companion : SDL_Color{ 240, 100, 100, 255 }, uiScale * 0.78f);
        curY += bannerH + (10.0f * uiScale);

        // 2. Equipped Slots (Worn Items)
        float wornSectionH = 120.0f * uiScale;
        SDL_FRect wornRect = { padX, curY, availableW, wornSectionH };
        UIWidget::drawPanel(renderer, wornRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Currently Worn Garments (Click 'Strip' to remove):", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

        struct SlotDisplay { equipSlot slot; std::string name; };
        static const SlotDisplay s_displaySlots[6] = {
            { equipSlot::TORSO_OVER, "Overgarment" },
            { equipSlot::TORSO_UNDER, "Top / Shirt" },
            { equipSlot::CHEST_WEAR, "Bra / Underwear" },
            { equipSlot::LEGS_OUTER, "Pants / Skirt" },
            { equipSlot::GROIN_OVER, "Underwear" },
            { equipSlot::FEET, "Footwear" }
        };

        float slotW = (availableW - (4.0f * 10.0f * uiScale)) / 3.0f;
        float slotH = 38.0f * uiScale;

        for (int i = 0; i < 6; ++i)
        {
            int r = i / 3;
            int c = i % 3;
            float sx = padX + (10.0f * uiScale) + (c * (slotW + 10.0f * uiScale));
            float sy = curY + (26.0f * uiScale) + (r * (slotH + 8.0f * uiScale));
            SDL_FRect sBox = { sx, sy, slotW, slotH };

            auto itemPtr = cc->wardrobeEquipped[static_cast<size_t>(s_displaySlots[i].slot)];
            UIWidget::drawPanel(renderer, sBox, itemPtr ? SDL_Color{ 32, 38, 48, 255 } : SDL_Color{ 20, 22, 26, 255 }, itemPtr ? Theme::colors.borderSelected : Theme::colors.borderNormal);

            UIWidget::drawText(renderer, s_displaySlots[i].name + ":", sx + (6.0f * uiScale), sy + (3.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.68f);
            std::string itemName = itemPtr ? itemPtr->name : "(Empty)";
            UIWidget::drawText(renderer, itemName, sx + (6.0f * uiScale), sy + (18.0f * uiScale), itemPtr ? Theme::colors.textPrimary : Theme::colors.textMuted, uiScale * 0.74f);

            if (itemPtr)
            {
                float uBtnW = 44.0f * uiScale;
                float uBtnH = 18.0f * uiScale;
                SDL_FRect uBtn = { sx + slotW - uBtnW - (4.0f * uiScale), sy + (10.0f * uiScale), uBtnW, uBtnH };
                bool uHover = (mousePos.x >= uBtn.x && mousePos.x <= uBtn.x + uBtn.w && mousePos.y >= uBtn.y && mousePos.y <= uBtn.y + uBtn.h);
                UIWidget::drawColoredButton(renderer, uBtn, "Strip", Theme::colors.bgButton, uHover ? Theme::colors.textGold : Theme::colors.textMuted, false, uiScale * 0.64f);
                if (uHover && clicked)
                {
                    cc->unequipWardrobeItem(s_displaySlots[i].slot);
                    gameContext->input.consumeMouseClick();
                }
            }
        }
        curY += wornSectionH + (10.0f * uiScale);

        // 3. Available Wardrobe Pile
        float pileH = 110.0f * uiScale;
        SDL_FRect pileRect = { padX, curY, availableW, pileH };
        UIWidget::drawPanel(renderer, pileRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Wardrobe Pile (Click 'Equip' to put on):", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

        if (cc->availableWardrobe.empty())
        {
            UIWidget::drawText(renderer, "Nothing left in the wardrobe pile.", padX + (10.0f * uiScale), curY + (30.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
        }
        else
        {
            float pItemW = (availableW - (3.0f * 10.0f * uiScale)) / 2.0f;
            float pItemH = 32.0f * uiScale;
            for (size_t j = 0; j < std::min((size_t)4, cc->availableWardrobe.size()); ++j)
            {
                int r = static_cast<int>(j / 2);
                int c = static_cast<int>(j % 2);
                float px = padX + (10.0f * uiScale) + (c * (pItemW + 10.0f * uiScale));
                float py = curY + (26.0f * uiScale) + (r * (pItemH + 8.0f * uiScale));
                SDL_FRect piBox = { px, py, pItemW, pItemH };

                auto itPtr = cc->availableWardrobe[j];
                UIWidget::drawPanel(renderer, piBox, SDL_Color{ 25, 28, 34, 255 }, Theme::colors.borderButton);
                UIWidget::drawText(renderer, itPtr->name, px + (6.0f * uiScale), py + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);
                UIWidget::drawText(renderer, itPtr->description.substr(0, 35) + "...", px + (6.0f * uiScale), py + (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.62f);

                float eqBtnW = 44.0f * uiScale;
                float eqBtnH = 18.0f * uiScale;
                SDL_FRect eqBtn = { px + pItemW - eqBtnW - (4.0f * uiScale), py + (7.0f * uiScale), eqBtnW, eqBtnH };
                bool eqHover = (mousePos.x >= eqBtn.x && mousePos.x <= eqBtn.x + eqBtn.w && mousePos.y >= eqBtn.y && mousePos.y <= eqBtn.y + eqBtn.h);
                UIWidget::drawColoredButton(renderer, eqBtn, "Equip", Theme::colors.bgButton, eqHover ? Theme::colors.companion : Theme::colors.textMuted, false, uiScale * 0.64f);
                if (eqHover && clicked)
                {
                    cc->equipWardrobeItem(j);
                    gameContext->input.consumeMouseClick();
                    break;
                }
            }
        }
        curY += pileH + (10.0f * uiScale);
    }
    else if (currentTab == EditorTabId::NAME_FINISH)
    {
        // 1. Name Input Box Card
        if (cc->config.isOptionEnabled("first_name") || cc->config.isOptionEnabled("surname"))
        {
            float nameCardH = 92.0f * uiScale;
            SDL_FRect ncRect = { padX, curY, availableW, nameCardH };
            UIWidget::drawPanel(renderer, ncRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Character Identity & Names:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.90f);
            UIWidget::drawText(renderer, "Type to enter your name, or click Randomize to roll from the name pool.", padX + (10.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

            float fieldY = curY + (40.0f * uiScale);
            float boxH = 22.0f * uiScale;
            float rBtnW = 54.0f * uiScale;
            float labelW = 32.0f * uiScale;
            float inputW = 140.0f * uiScale;
            float colW = labelW + inputW + (6.0f * uiScale) + rBtnW;

            // First Name (Left Column)
            float firstX = padX + (12.0f * uiScale);
            UIWidget::drawText(renderer, "First:", firstX, fieldY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
            SDL_FRect fnBox = { firstX + labelW, fieldY, inputW, boxH };
            bool fnHover = (mousePos.x >= fnBox.x && mousePos.x <= fnBox.x + fnBox.w && mousePos.y >= fnBox.y && mousePos.y <= fnBox.y + fnBox.h);
            SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
            SDL_RenderFillRect(renderer, &fnBox);
            SDL_Color fnBorder = fnHover ? Theme::colors.textGold : Theme::colors.borderSelected;
            SDL_SetRenderDrawColor(renderer, fnBorder.r, fnBorder.g, fnBorder.b, fnBorder.a);
            SDL_RenderRect(renderer, &fnBox);
            UIWidget::drawText(renderer, chosenFirst, fnBox.x + (6.0f * uiScale), fnBox.y + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

            SDL_FRect rFirstBtn = { firstX + labelW + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
            bool rfHover = (mousePos.x >= rFirstBtn.x && mousePos.x <= rFirstBtn.x + rFirstBtn.w && mousePos.y >= rFirstBtn.y && mousePos.y <= rFirstBtn.y + rFirstBtn.h);
            UIWidget::drawColoredButton(renderer, rFirstBtn, "Random", Theme::colors.bgButton, rfHover ? Theme::colors.textGold : Theme::colors.textMuted, false, uiScale * 0.65f);
            if (rfHover && clicked) { cc->randomizeFirstNames(); gameContext->input.consumeMouseClick(); }

            // Surname (Right Column)
            float surStartX = padX + availableW - colW - (12.0f * uiScale);
            UIWidget::drawText(renderer, "Last:", surStartX, fieldY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
            SDL_FRect lnBox = { surStartX + labelW, fieldY, inputW, boxH };
            bool lnHover = (mousePos.x >= lnBox.x && mousePos.x <= lnBox.x + lnBox.w && mousePos.y >= lnBox.y && mousePos.y <= lnBox.y + lnBox.h);
            SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
            SDL_RenderFillRect(renderer, &lnBox);
            SDL_Color lnBorder = lnHover ? Theme::colors.textGold : Theme::colors.borderNormal;
            SDL_SetRenderDrawColor(renderer, lnBorder.r, lnBorder.g, lnBorder.b, lnBorder.a);
            SDL_RenderRect(renderer, &lnBox);
            UIWidget::drawText(renderer, cc->surname.empty() ? "(None)" : cc->surname, lnBox.x + (6.0f * uiScale), lnBox.y + (3.0f * uiScale), cc->surname.empty() ? Theme::colors.textMuted : Theme::colors.textPrimary, uiScale * 0.85f);

            SDL_FRect rSurBtn = { surStartX + labelW + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
            bool rsHover = (mousePos.x >= rSurBtn.x && mousePos.x <= rSurBtn.x + rSurBtn.w && mousePos.y >= rSurBtn.y && mousePos.y <= rSurBtn.y + rSurBtn.h);
            UIWidget::drawColoredButton(renderer, rSurBtn, "Random", Theme::colors.bgButton, rsHover ? Theme::colors.textGold : Theme::colors.textMuted, false, uiScale * 0.65f);
            if (rsHover && clicked) { cc->randomizeSurname(); gameContext->input.consumeMouseClick(); }

            UIWidget::drawText(renderer, "Surname may be left blank. First name dynamically adapts to chosen gender & femininity.", padX + (10.0f * uiScale), curY + (68.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
            curY += nameCardH + (12.0f * uiScale);
        }

        // 2. Full Character Overview Stat Card
        float sumCardH = 100.0f * uiScale;
        SDL_FRect sumCardRect = { padX, curY, availableW, sumCardH };
        UIWidget::drawPanel(renderer, sumCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Character Profile Sheet:", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.90f);

        std::string s1 = std::format("• Identity: {} | {} ({}) | Orientation: {} | Starts in {}", fullName, cc->gender, cc->femininity, cc->orientation, cc->startMonth);
        std::string s2 = std::format("• Body: {}cm height | {} frame | {} muscle | {} skin", cc->heightCm, cc->bodySize, cc->muscleDefinition, cc->skinPrimaryColor);
        std::string s3 = std::format("• Appearance: {} {} hair ({}cm) | {} eyes | {} ears", cc->hairColor, cc->hairStyle, cc->hairLengthCm, cc->eyeColor, cc->earType);

        std::string traitList = "";
        for (const auto& tr : cc->personalityTraits) {
            if (!traitList.empty()) traitList += ", ";
            traitList += tr;
        }
        if (traitList.empty()) traitList = "None";
        std::string s4 = "• Traits: " + traitList;

        UIWidget::drawText(renderer, s1, padX + (12.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);
        UIWidget::drawText(renderer, s2, padX + (12.0f * uiScale), curY + (40.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);
        UIWidget::drawText(renderer, s3, padX + (12.0f * uiScale), curY + (56.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);
        UIWidget::drawText(renderer, s4, padX + (12.0f * uiScale), curY + (72.0f * uiScale), Theme::colors.companion, uiScale * 0.78f);

        curY += sumCardH + (14.0f * uiScale);

        // 3. Final Confirmation Action Button
        float startBtnW = std::min(availableW, 320.0f * uiScale);
        float startBtnH = 34.0f * uiScale;
        SDL_FRect startBtn = { centerX - (startBtnW / 2.0f), curY, startBtnW, startBtnH };
        bool sbHover = (mousePos.x >= startBtn.x && mousePos.x <= startBtn.x + startBtn.w && mousePos.y >= startBtn.y && mousePos.y <= startBtn.y + startBtn.h);

        UIWidget::drawColoredButton(renderer, startBtn, cc->config.finishButtonText, sbHover ? SDL_Color{ 60, 150, 80, 255 } : SDL_Color{ 45, 120, 65, 240 }, Theme::colors.textPrimary, true, uiScale * 0.90f);

        if (sbHover && clicked)
        {
            cc->finalizeCharacter(gameContext);
            gameContext->input.consumeMouseClick();
        }
        curY += startBtnH + (12.0f * uiScale);
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
    static constexpr std::string_view tiers[] = { "Major Drain", "Drain", "Minor Drain", "Minor Boost", "Boost", "Major Boost" };
    float tierW = (availableW - (gap * 5)) / 6.0f;
    float tierH = 22.0f * uiScale;

    for (size_t i = 0; i < std::size(tiers); ++i)
    {
        SDL_FRect tRect = { padX + (i * (tierW + gap)), curY, tierW, tierH };
        bool isSelected = (i == 5); // Major Boost
        UIWidget::drawButton(renderer, tRect, std::string(tiers[i]), isSelected, true, isSelected, uiScale * 0.8f);
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

    static constexpr std::string_view appliedEffects[] = {
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

        static constexpr std::string_view startingEquipLog[] = {
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
            float descH = UIWidget::drawTextWrapped(renderer, std::string(entry), padX, curY, availableW, Theme::colors.textSecondary, uiScale * 0.8f);
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
    static constexpr std::string_view statusBadges[] = { "✋", "🛡", "⚥", "🧪" };
    float badgeW = (cardInnerW - (3.0f * 4.0f * uiScale)) / 4.0f;
    float badgeH = 16.0f * uiScale;
    for (size_t i = 0; i < std::size(statusBadges); ++i)
    {
        SDL_FRect bRect = { innerPadX + (i * (badgeW + 4.0f * uiScale)), cardCurY, badgeW, badgeH };
        UIWidget::drawPanel(renderer, bRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, std::string(statusBadges[i]), bRect.x + ((bRect.w - (10.0f * uiScale)) / 2.0f), bRect.y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.75f);
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