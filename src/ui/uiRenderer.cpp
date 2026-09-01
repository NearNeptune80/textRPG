#include "ui/uiRenderer.h"

#include <filesystem>
#include <format>
#include <iostream>
#include <algorithm>

#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "settings/settingsManager.h"
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

#include "ui/actionGridManager.h"
#include "ui/fontManager.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"
#include "ui/views/characterCreationView.h"
#include "ui/views/gameplayViews.h"
#include "ui/views/loadGameView.h"
#include "ui/views/mainMenuView.h"
#include "ui/views/optionsView.h"
#include "ui/widgets/characterCardWidget.h"
#include "ui/widgets/entityListWidgets.h"
#include "ui/widgets/paperdollWidgets.h"
#include "ui/widgets/radarWidget.h"
#include "ui/widgets/statusWidgets.h"

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

    // Load active theme
    std::string themeName = settings.display.activeTheme.empty() ? "default" : settings.display.activeTheme;
    Theme::applyTheme(themeName);

    // Attempt loading primary TTF font with fallback to embedded font
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
    else if (dynamic_cast<characterCreationState*>(curState)) stateKey = "CHARACTER_CREATION";
    else if (dynamic_cast<loadGameState*>(curState)) stateKey = "LOAD_GAME";
    else if (dynamic_cast<optionsState*>(curState)) stateKey = "SETTINGS";
    else if (dynamic_cast<CombatState*>(curState)) stateKey = "COMBAT";
    else if (dynamic_cast<inventoryState*>(curState)) stateKey = "INVENTORY";
    else if (dynamic_cast<sexState*>(curState)) stateKey = "SEX";
    else if (dynamic_cast<shopState*>(curState)) stateKey = "SHOP";
    else if (dynamic_cast<transformationState*>(curState)) stateKey = "TRANSFORMATION";
    else if (dynamic_cast<phoneAppsState*>(curState)) stateKey = "PHONE_APP";

    auto panels = m_layoutEngine.computeLayout(static_cast<float>(winW), static_cast<float>(winH), uiScale, stateKey);

    // Clear Screen with Dark Theme Background
    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, Theme::colors.bgDark.a);
    SDL_RenderClear(renderer);
    TooltipManager::clear();

    auto hasWidgetTag = [](const std::vector<std::string>& widgets, const std::string& target) {
        return std::ranges::find(widgets, target) != widgets.end();
    };

    // Handle Mouse Wheel Scrolling on Hovered Panel
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
            UIWidget::drawPanel(renderer, p.rect);

            float scrollY = m_panelScrollY.contains(p.id) ? m_panelScrollY[p.id] : 0.0f;
            float curY = p.rect.y + (6.0f * uiScale) - scrollY;
            bool renderedCenterViewInPanel = false;

            for (const auto& wId : p.widgets)
            {
                if (wId == "widget_char_overview")
                    curY += StatusWidgets::renderWidgetCharOverview(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_vitals_gauges")
                    curY += StatusWidgets::renderWidgetVitals(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_attributes_table")
                    curY += StatusWidgets::renderWidgetAttributes(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_anatomy_fluids")
                    curY += StatusWidgets::renderWidgetAnatomy(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_paperdoll_equipment")
                    curY += PaperdollWidgets::renderWidgetPaperdoll(renderer, gameContext, p.rect, curY, uiScale, gameContext->getPlayer());
                else if (wId == "widget_partner_paperdoll")
                    curY += PaperdollWidgets::renderWidgetPaperdoll(renderer, gameContext, p.rect, curY, uiScale, gameContext->getActiveTargetNPC());
                else if (wId == "widget_item_details_inspector")
                    curY += PaperdollWidgets::renderWidgetItemInspector(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_minimap_radar" || wId == "MINIMAP_RADAR" || wId == "widget_lt_radar_map" || wId == "RADAR_MAP_5X5")
                    curY += RadarWidgets::renderWidgetRadar(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_target_inspector" || wId == "TARGET_INSPECTOR")
                    curY += StatusWidgets::renderWidgetTarget(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_inventory_dual" || wId == "BACKPACK_INVENTORY")
                    curY += GameplayViews::renderInventoryView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_tactical_combat" || wId == "COMBAT_VIEW")
                    curY += GameplayViews::renderCombatView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_merchant_dialog" || wId == "widget_merchant_catalog" || wId == "widget_player_sell_grid" || wId == "widget_transaction_cart")
                    curY += GameplayViews::renderShopView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_lt_character_card")
                    curY += CharacterCardWidget::renderWidgetCharacterCard(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_time_bar" || wId == "TIME_CALENDAR_BAR")
                    curY += RadarWidgets::renderWidgetTimeBar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_options_toolbar" || wId == "OPTIONS_TOOLBAR_5")
                    curY += RadarWidgets::renderWidgetOptionsToolbar(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_dpad_radar")
                    curY += RadarWidgets::renderWidgetDpadRadar(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_lt_characters_present")
                    curY += EntityListWidgets::renderWidgetCharactersPresent(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_items_present")
                    curY += EntityListWidgets::renderWidgetItemsPresent(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_event_log")
                    curY += EntityListWidgets::renderWidgetEventLog(renderer, gameContext, p.rect.x, curY, p.rect.w, uiScale);
                else if (wId == "widget_lt_enchanting_screen" || wId == "widget_enchanting_altar")
                    curY += GameplayViews::renderEnchantingView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_body_mutations_tree" || wId == "widget_transformation_suite")
                    curY += GameplayViews::renderTransformationView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_character_creation")
                    curY += CharacterCreationView::render(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_phone_menu")
                    curY += GameplayViews::renderPhoneAppView(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_load_game")
                    curY += LoadGameView::render(renderer, gameContext, p.rect, curY, uiScale);
                else if (wId == "widget_narrative_story" || wId == "SCENE_NARRATIVE" ||
                         wId == "widget_main_menu_hero" || wId == "widget_main_menu_actions" || wId == "widget_save_slot_list" ||
                         wId == "widget_options_content" || wId == "widget_options_demographics" || wId == "widget_options_display_audio")
                {
                    if (!renderedCenterViewInPanel)
                    {
                        renderedCenterViewInPanel = true;
                        curY += renderCenterPane(renderer, gameContext, p.rect, curY, uiScale);
                    }
                }
            }

            float totalContentH = (curY + scrollY) - p.rect.y;
            float calculatedMaxScroll = std::max(0.0f, totalContentH - p.rect.h);
            m_panelMaxScrollY[p.id] = calculatedMaxScroll;
            m_panelScrollY[p.id] = std::clamp(m_panelScrollY[p.id], 0.0f, calculatedMaxScroll);
            drawScrollbar(renderer, p.rect, totalContentH, m_panelScrollY[p.id], uiScale);
        }

        SDL_SetRenderClipRect(renderer, nullptr);
    }

    // Render universal topmost tooltip overlay before presenting
    TooltipManager::render(renderer, uiScale, static_cast<float>(winW), static_cast<float>(winH), mousePos);

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
    UIWidget::drawText(renderer, "textRPG v0.5.0", rect.x + (12.0f * uiScale), rect.y + ((rect.h - (10.0f * uiScale)) / 2.0f), Theme::colors.textGold, uiScale);

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
    TooltipManager::setHoverTooltip(menuBtnRect, mousePos, inMenu ? "Close Options Menu" : "Game Settings & Options",
                                    "Access options, content filters, graphics, audio, and keybinding configuration.",
                                    "System Menu", "[ ESC ]");

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
    TooltipManager::setHoverTooltip(leftMidRect, mousePos, "Previous Action Page", "Switch to earlier action commands.", "Pagination", "[ Q / < ]");

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
    TooltipManager::setHoverTooltip(rightMidRect, mousePos, "Next Action Page", "Switch to additional action commands.", "Pagination", "[ E / > ]");

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

            if (hovered)
            {
                std::string desc = buttons[buttonIdx].description;
                if (desc.empty())
                {
                    desc = std::format("Execute command: {}", buttons[buttonIdx].label);
                }
                std::string sub = buttons[buttonIdx].isEnabled ? "Action Command" : "Unavailable in Current Context";
                TooltipManager::setHoverTooltip(btnRect, mousePos, buttons[buttonIdx].label, desc, sub, std::format("[ {} ]", hotkeys[slotIdx]));
            }

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
            UIWidget::drawLTActionButton(renderer, btnRect, "", hotkeys[slotIdx], false, false, false, uiScale);
        }
    }
}

float uiRenderer::renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
{
    iGameState* state = gameContext->getActiveState();
    if (!state) return 0.0f;

    if (dynamic_cast<mainMenuState*>(state))
    {
        return MainMenuView::render(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<characterCreationState*>(state))
    {
        return CharacterCreationView::render(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<loadGameState*>(state))
    {
        return LoadGameView::render(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<optionsState*>(state))
    {
        return OptionsView::render(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<shopState*>(state))
    {
        return GameplayViews::renderShopView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<transformationState*>(state))
    {
        return GameplayViews::renderTransformationView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<phoneAppsState*>(state))
    {
        return GameplayViews::renderPhoneAppView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<sexState*>(state))
    {
        return GameplayViews::renderSexView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<CombatState*>(state))
    {
        return GameplayViews::renderCombatView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<encounterResolutionState*>(state))
    {
        return GameplayViews::renderResolutionView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<inventoryState*>(state))
    {
        return GameplayViews::renderInventoryView(renderer, gameContext, rect, curY, uiScale);
    }
    else if (dynamic_cast<eventState*>(state))
    {
        return GameplayViews::renderSceneView(renderer, gameContext, rect, curY, uiScale);
    }
    else
    {
        return GameplayViews::renderExplorationView(renderer, gameContext, rect, curY, uiScale);
    }
}