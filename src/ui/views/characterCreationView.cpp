#include "ui/views/characterCreationView.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <vector>

#include "core/game.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

namespace CharacterCreationView
{
    struct ColorOption
    {
        std::string name;
        SDL_Color color;
    };

    static const std::vector<ColorOption> s_skinTones = {
        { "Pale", { 252, 228, 214, 255 } },
        { "Light", { 245, 212, 186, 255 } },
        { "Porcelain", { 255, 243, 236, 255 } },
        { "Fair", { 248, 224, 204, 255 } },
        { "Rosy", { 245, 192, 182, 255 } },
        { "Olive", { 200, 185, 130, 255 } },
        { "Tanned", { 215, 155, 105, 255 } },
        { "Dark", { 155, 95, 60, 255 } },
        { "Ebony", { 110, 70, 50, 255 } }
    };

    static const std::vector<ColorOption> s_hairColors = {
        { "Black", { 35, 35, 35, 255 } },
        { "Dark Brown", { 90, 55, 35, 255 } },
        { "Brown", { 140, 85, 55, 255 } },
        { "Auburn", { 165, 75, 45, 255 } },
        { "Ginger", { 225, 115, 40, 255 } },
        { "Blonde", { 240, 210, 95, 255 } },
        { "Platinum", { 230, 235, 240, 255 } },
        { "Grey", { 170, 175, 180, 255 } },
        { "White", { 245, 245, 250, 255 } },
        { "Red", { 205, 45, 45, 255 } },
        { "Pink", { 245, 120, 180, 255 } },
        { "Blue", { 65, 140, 235, 255 } }
    };

    static const std::vector<ColorOption> s_eyeColors = {
        { "Brown", { 115, 70, 40, 255 } },
        { "Hazel", { 165, 135, 60, 255 } },
        { "Green", { 65, 160, 85, 255 } },
        { "Blue", { 55, 135, 225, 255 } },
        { "Amber", { 225, 160, 35, 255 } },
        { "Grey", { 160, 165, 170, 255 } },
        { "Violet", { 160, 90, 210, 255 } },
        { "Black", { 30, 30, 30, 255 } }
    };

    static const std::vector<ColorOption> s_makeupColors = {
        { "None", { 0, 0, 0, 0 } },
        { "Red", { 210, 35, 35, 255 } },
        { "Dark Red", { 145, 25, 25, 255 } },
        { "Light Red", { 240, 105, 105, 255 } },
        { "Pink", { 245, 110, 175, 255 } },
        { "Light Pink", { 250, 170, 195, 255 } },
        { "Purple", { 160, 75, 215, 255 } },
        { "Light Purple", { 205, 150, 240, 255 } },
        { "Black", { 25, 25, 25, 255 } },
        { "White", { 240, 240, 245, 255 } },
        { "Gold", { 230, 190, 40, 255 } },
        { "Silver", { 195, 200, 210, 255 } },
        { "Clear", { 150, 210, 230, 255 } }
    };

    static float drawPillCard(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float padX, float curY, float availableW, std::string_view title, std::string_view description, const std::vector<std::string>& options, const std::string& currentSelected, auto onSelect, float uiScale, int cols = 5)
    {
        float startY = curY;
        float gapX = 6.0f * uiScale;
        float gapY = 6.0f * uiScale;
        float innerPad = 12.0f * uiScale;
        float btnW = (availableW - (innerPad * 2.0f) - (gapX * (cols - 1))) / static_cast<float>(cols);
        float btnH = 25.0f * uiScale;

        int rows = static_cast<int>((options.size() + cols - 1) / cols);
        float descH = description.empty() ? (26.0f * uiScale) : (44.0f * uiScale);
        float cardH = descH + (rows * (btnH + gapY)) + (innerPad * 1.4f);

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Header Title & Subtitle
        UIWidget::drawText(renderer, title, padX + innerPad, curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (26.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        float gridStartY = curY + descH + (innerPad * 0.4f);
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        auto equalsIgnoreCase = [](std::string_view a, std::string_view b) {
            return std::ranges::equal(a, b, [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb));
            });
        };

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + innerPad + (c * (btnW + gapX)), gridStartY + (r * (btnH + gapY)), btnW, btnH };

            bool isSelected = equalsIgnoreCase(options[i], currentSelected);
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            bRect.y + bRect.h >= panelRect.y && bRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                       mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isSelected, uiScale * 0.76f);

            if (hovered && clicked)
            {
                onSelect(options[i]);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    static float drawTogglePillCard(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float padX, float curY, float availableW, std::string_view title, std::string_view description, const std::vector<std::string>& options, const std::set<std::string>& activeItems, auto onToggle, float uiScale, int cols = 5)
    {
        float startY = curY;
        float gapX = 6.0f * uiScale;
        float gapY = 6.0f * uiScale;
        float innerPad = 12.0f * uiScale;
        float btnW = (availableW - (innerPad * 2.0f) - (gapX * (cols - 1))) / static_cast<float>(cols);
        float btnH = 25.0f * uiScale;

        int rows = static_cast<int>((options.size() + cols - 1) / cols);
        float descH = description.empty() ? (26.0f * uiScale) : (44.0f * uiScale);
        float cardH = descH + (rows * (btnH + gapY)) + (innerPad * 1.4f);

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, title, padX + innerPad, curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (26.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        float gridStartY = curY + descH + (innerPad * 0.4f);
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + innerPad + (c * (btnW + gapX)), gridStartY + (r * (btnH + gapY)), btnW, btnH };

            bool isActive = (activeItems.find(options[i]) != activeItems.end());
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            bRect.y + bRect.h >= panelRect.y && bRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                       mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isActive, uiScale * 0.76f);

            if (hovered && clicked)
            {
                onToggle(options[i], !isActive);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    static float drawColorSwatchCard(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float padX, float curY, float availableW, std::string_view title, std::string_view description, const std::vector<ColorOption>& options, const std::string& currentSelected, auto onSelect, float uiScale)
    {
        float startY = curY;
        float innerPad = 12.0f * uiScale;
        float tileSize = 28.0f * uiScale;
        float gap = 8.0f * uiScale;

        auto equalsIgnoreCase = [](std::string_view a, std::string_view b) {
            return std::ranges::equal(a, b, [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb));
            });
        };

        std::string selectedName = options.empty() ? "None" : options[0].name;
        SDL_Color selectedColor = options.empty() ? Theme::colors.textGold : options[0].color;
        for (const auto& opt : options)
        {
            if (equalsIgnoreCase(opt.name, currentSelected))
            {
                selectedName = opt.name;
                selectedColor = opt.color;
                break;
            }
        }

        int cols = 12;
        int rows = static_cast<int>((options.size() + cols - 1) / cols);
        float descH = description.empty() ? (28.0f * uiScale) : (46.0f * uiScale);
        float cardH = descH + (rows * (tileSize + gap)) + (innerPad * 1.5f);

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Header Title with Preview Swatch
        if (selectedColor.a > 0)
        {
            SDL_FRect previewBox = { padX + innerPad, curY + (7.0f * uiScale), 18.0f * uiScale, 18.0f * uiScale };
            UIWidget::drawPanel(renderer, previewBox, selectedColor, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, std::format("{}: {}", title, selectedName), padX + innerPad + (26.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        }
        else
        {
            UIWidget::drawText(renderer, std::format("{}: None", title), padX + innerPad, curY + (7.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        }

        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (27.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        float gridStartY = curY + descH + (innerPad * 0.4f);
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect sRect = { padX + innerPad + (c * (tileSize + gap)), gridStartY + (r * (tileSize + gap)), tileSize, tileSize };

            bool isSelected = equalsIgnoreCase(options[i].name, selectedName);
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            sRect.y + sRect.h >= panelRect.y && sRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= sRect.x && mousePos.x <= sRect.x + sRect.w &&
                                       mousePos.y >= sRect.y && mousePos.y <= sRect.y + sRect.h);

            if (options[i].color.a == 0)
            {
                // 'None' swatch button
                UIWidget::drawButton(renderer, sRect, "X", hovered, true, isSelected, uiScale * 0.72f);
            }
            else
            {
                UIWidget::drawColorSwatch(renderer, sRect, options[i].color, isSelected, hovered, uiScale);
            }

            if (hovered && clicked)
            {
                onSelect(options[i].name);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    template <typename T = float>
    static float drawStepperCard(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float padX, float curY, float availableW, std::string_view title, std::string_view description, std::string_view displayVal, auto onStep, float uiScale, T stepSmall = 1, T stepMed = 0, T stepLarge = 0)
    {
        float startY = curY;
        float innerPad = 12.0f * uiScale;
        float cardH = (description.empty() ? 48.0f : 58.0f) * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, title, padX + innerPad, curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        bool hasMultiTier = (stepMed > 0);
        float btnW = (hasMultiTier ? 28.0f : 34.0f) * uiScale;
        float btnGap = 2.0f * uiScale;
        float valW = (hasMultiTier ? 120.0f : 140.0f) * uiScale;
        float h = 26.0f * uiScale;

        int btnCountPerSide = (stepLarge > 0) ? 3 : ((stepMed > 0) ? 2 : 1);
        float sideBtnsW = (btnCountPerSide * btnW) + ((btnCountPerSide - 1) * btnGap);
        float controlsTotalW = (sideBtnsW * 2.0f) + valW + (8.0f * uiScale);
        float curX = padX + availableW - innerPad - controlsTotalW;
        float controlY = curY + ((cardH - h) / 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        auto drawBtnHelper = [&](float x, const char* label, T delta) {
            SDL_FRect bRect = { x, controlY, btnW, h };
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            controlY + h >= panelRect.y && controlY <= panelRect.y + panelRect.h);
            bool hov = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                   mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);
            UIWidget::drawButton(renderer, bRect, label, hov, true, false, uiScale * 0.76f);
            if (hov && clicked)
            {
                onStep(delta);
                gameContext->input.consumeMouseClick();
            }
        };

        // Left Decrease Buttons
        if (btnCountPerSide == 3)
        {
            drawBtnHelper(curX, "<<<", -stepLarge); curX += btnW + btnGap;
            drawBtnHelper(curX, "<<", -stepMed); curX += btnW + btnGap;
            drawBtnHelper(curX, "<", -stepSmall); curX += btnW + (4.0f * uiScale);
        }
        else if (btnCountPerSide == 2)
        {
            drawBtnHelper(curX, "<<", -stepMed); curX += btnW + btnGap;
            drawBtnHelper(curX, "<", -stepSmall); curX += btnW + (4.0f * uiScale);
        }
        else
        {
            drawBtnHelper(curX, "<", -stepSmall); curX += btnW + (4.0f * uiScale);
        }

        // Center Value Box
        SDL_FRect valRect = { curX, controlY, valW, h };
        UIWidget::drawPanel(renderer, valRect, Theme::colors.bgDark, Theme::colors.borderNormal);
        float valTextW = UIWidget::getTextWidth(displayVal, uiScale * 0.82f);
        float valTextX = curX + std::max(4.0f * uiScale, (valW - valTextW) / 2.0f);
        UIWidget::drawText(renderer, displayVal, valTextX, controlY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
        curX += valW + (4.0f * uiScale);

        // Right Increase Buttons
        if (btnCountPerSide == 3)
        {
            drawBtnHelper(curX, ">", stepSmall); curX += btnW + btnGap;
            drawBtnHelper(curX, ">>", stepMed); curX += btnW + btnGap;
            drawBtnHelper(curX, ">>>", stepLarge);
        }
        else if (btnCountPerSide == 2)
        {
            drawBtnHelper(curX, ">", stepSmall); curX += btnW + btnGap;
            drawBtnHelper(curX, ">>", stepMed);
        }
        else
        {
            drawBtnHelper(curX, ">", stepSmall);
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    static float drawToggleCard(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float padX, float curY, float availableW, std::string_view title, std::string_view description, bool currentState, auto onToggle, float uiScale, std::string_view trueLabel = "Yes", std::string_view falseLabel = "No")
    {
        float startY = curY;
        float innerPad = 12.0f * uiScale;
        float cardH = (description.empty() ? 48.0f : 58.0f) * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, title, padX + innerPad, curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        float btnW = 70.0f * uiScale;
        float h = 26.0f * uiScale;
        float controlsTotalW = (btnW * 2.0f) + (6.0f * uiScale);
        float btn1X = padX + availableW - innerPad - controlsTotalW;
        float controlY = curY + ((cardH - h) / 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();
        bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                        controlY + h >= panelRect.y && controlY <= panelRect.y + panelRect.h);

        SDL_FRect r1 = { btn1X, controlY, btnW, h };
        bool hov1 = inPanel && (mousePos.x >= r1.x && mousePos.x <= r1.x + r1.w && mousePos.y >= r1.y && mousePos.y <= r1.y + r1.h);
        UIWidget::drawButton(renderer, r1, trueLabel, hov1, true, currentState, uiScale * 0.82f);
        if (hov1 && clicked)
        {
            onToggle(true);
            gameContext->input.consumeMouseClick();
        }

        float btn2X = btn1X + btnW + (6.0f * uiScale);
        SDL_FRect r2 = { btn2X, controlY, btnW, h };
        bool hov2 = inPanel && (mousePos.x >= r2.x && mousePos.x <= r2.x + r2.w && mousePos.y >= r2.y && mousePos.y <= r2.y + r2.h);
        UIWidget::drawButton(renderer, r2, falseLabel, hov2, true, !currentState, uiScale * 0.82f);
        if (hov2 && clicked)
        {
            onToggle(false);
            gameContext->input.consumeMouseClick();
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        auto cc = dynamic_cast<characterCreationState*>(gameContext->getActiveState());
        if (!cc) return 0.0f;

        float startY = curY;

        // Centered Content Column (comfortable width with generous side margins)
        float contentMaxW = std::min(rect.w - (48.0f * uiScale), 940.0f * uiScale);
        float padX = rect.x + (rect.w - contentMaxW) / 2.0f;
        float availableW = contentMaxW;

        auto activeTabs = cc->getActiveTabs();
        int tabCount = static_cast<int>(activeTabs.size());
        if (cc->step >= tabCount) cc->step = std::max(0, tabCount - 1);
        EditorTabId currentTab = activeTabs[cc->step];

        // 1. Top Header Banner
        float headerH = 28.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        std::string headerTitle = std::format("{} [{}]", cc->config.title, cc->getTabName(currentTab));
        UIWidget::drawHeader(renderer, headerRect, headerTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        // 2. Character Live Preview Summary Banner
        float prevH = 44.0f * uiScale;
        SDL_FRect prevRect = { padX, curY, availableW, prevH };
        UIWidget::drawPanel(renderer, prevRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string chosenFirst = (cc->gender == "Female") ? cc->feminineName : cc->masculineName;
        std::string fullName = cc->surname.empty() ? chosenFirst : (chosenFirst + " " + cc->surname);
        std::string previewLine1 = std::format("Hero: {}  |  {} ({})  |  Orientation: {}", fullName, cc->gender, cc->femininity, cc->orientation);
        std::string previewLine2 = std::format("Body: {}cm, {}, {}  |  Hair: {} {}  |  Eyes: {}", cc->heightCm, cc->bodySize, cc->muscleDefinition, cc->hairColor, cc->hairStyle, cc->eyeColor);

        UIWidget::drawText(renderer, previewLine1, padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        UIWidget::drawText(renderer, previewLine2, padX + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
        curY += prevH + (12.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        if (currentTab == EditorTabId::IDENTITY)
        {
            // 1. Biological Sex
            if (cc->config.isOptionEnabled("gender"))
            {
                static const std::vector<std::string> genderOpts = { "Male", "Female" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Biological Sex", "Determines initial physical anatomy and starting bodily equipment.", genderOpts, cc->gender, [&](const std::string& g) {
                    cc->gender = g;
                }, uiScale, 2);
            }

            // 2. Femininity
            if (cc->config.isOptionEnabled("femininity"))
            {
                static const std::vector<std::string> allFem = { "Very Masculine", "Masculine", "Androgynous", "Feminine", "Very Feminine" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Femininity", "How feminine or masculine your overall facial and bodily presentation is.", allFem, cc->femininity, [&](const std::string& f) {
                    cc->femininity = f;
                }, uiScale, 5);
            }

            // 3. Orientation
            if (cc->config.isOptionEnabled("orientation"))
            {
                static const std::vector<std::string> allOri = { "Androphilic", "Ambiphilic", "Gynephilic" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Sexual Orientation", "Attraction preference towards masculinity, femininity, or both.", allOri, cc->orientation, [&](const std::string& o) {
                    cc->orientation = o;
                }, uiScale, 3);
            }

            // 4. Starting Month
            if (cc->config.isOptionEnabled("start_month"))
            {
                static const std::vector<std::string> fullMonths = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Starting Calendar Month", "Select the calendar month in which your adventure begins.", fullMonths, cc->startMonth, [&](const std::string& m) {
                    cc->startMonth = m;
                    for (size_t i = 0; i < fullMonths.size(); ++i) if (fullMonths[i] == m) cc->startMonthIdx = static_cast<int>(i);
                }, uiScale, 6);
            }

            // 5. Birthday (Day & Age Steppers)
            curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Birth Day", "Day of birth within the month.", std::format("Day {:02d}", cc->birthDay), [&](int delta) {
                cc->birthDay = std::clamp(cc->birthDay + delta, 1, 31);
            }, uiScale, 1, 5, 0);

            curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Age", "Normal human starting age (18 to 65).", std::format("{} yrs", cc->birthAge), [&](int delta) {
                cc->birthAge = std::clamp(cc->birthAge + delta, 18, 65);
            }, uiScale, 1, 5, 10);

            // 6. Personality Traits
            if (cc->config.isOptionEnabled("personality_traits"))
            {
                static const std::vector<std::string> allP = { "Confident", "Shy", "Kind", "Selfish", "Naive", "Cynical", "Brave", "Cowardly", "Lewd", "Innocent", "Prude" };
                curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Personality Traits", "Click traits to toggle them. Influences roleplaying choices.", allP, cc->personalityTraits, [&](const std::string& t, bool act) {
                    if (act) cc->personalityTraits.insert(t);
                    else cc->personalityTraits.erase(t);
                }, uiScale, 6);
            }
        }
        else if (currentTab == EditorTabId::BODY)
        {
            // 1. Height (Human Range: 140cm to 210cm)
            if (cc->config.isOptionEnabled("height"))
            {
                auto* rule = cc->config.getRule("height");
                int minH = rule ? static_cast<int>(rule->minRange) : 140;
                int maxH = rule ? static_cast<int>(rule->maxRange) : 210;
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Height (cm)", "Normal human standing height (140 - 210 cm).", std::format("{} cm", cc->heightCm), [&](int delta) {
                    cc->heightCm = std::clamp(cc->heightCm + delta, minH, maxH);
                }, uiScale, 1, 5, 10);
            }

            // 2. Skin Tone
            if (cc->config.isOptionEnabled("skin_tone"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Skin Tone", "Natural human skin tone covering your body.", s_skinTones, cc->skinPrimaryColor, [&](const std::string& s) {
                    cc->skinPrimaryColor = s;
                }, uiScale);
            }

            // 3. Body Size
            if (cc->config.isOptionEnabled("body_size"))
            {
                static const std::vector<std::string> allSizes = { "Skinny", "Slender", "Average", "Muscular", "Chubby" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Body Frame Proportion", "Overall fat distribution and build frame.", allSizes, cc->bodySize, [&](const std::string& s) {
                    cc->bodySize = s;
                }, uiScale, 5);
            }

            // 4. Muscle Definition
            if (cc->config.isOptionEnabled("muscle"))
            {
                static const std::vector<std::string> allMusc = { "Soft", "Lightly muscled", "Toned", "Muscular", "Ripped" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Muscle Definition", "Muscularity, firmness, and tone.", allMusc, cc->muscleDefinition, [&](const std::string& m) {
                    cc->muscleDefinition = m;
                }, uiScale, 5);
            }

            // 5. Ass Size (Normal Human 0..4)
            static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
            int assIdx = std::clamp(cc->assSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ass Dimensions", "Rear cheek volume and fullness.", allSizes5, allSizes5[assIdx], [&](const std::string& a) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == a) cc->assSize = static_cast<int>(i);
            }, uiScale, 5);

            // 6. Hip Size (Normal Human 0..4)
            int hipIdx = std::clamp(cc->hipSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hip Breadth", "Pelvic width and hourglass curvature.", allSizes5, allSizes5[hipIdx], [&](const std::string& h) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == h) cc->hipSize = static_cast<int>(i);
            }, uiScale, 5);

            // 7. Bleached Anus Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Bleached Anus", "Cosmetic treatment around anal sphincter.", cc->anusBleached, [&](bool b) {
                cc->anusBleached = b;
            }, uiScale, "Bleached", "Natural");

            // 8. Composite Body Shape Card
            std::string shapeRating = EditorConfig::calculateBodyShape(cc->muscleDefinition, cc->bodySize);
            SDL_FRect shapeRect = { padX, curY, availableW, 38.0f * uiScale };
            UIWidget::drawPanel(renderer, shapeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Composite Human Physique Rating:", padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);
            float ratingW = UIWidget::getTextWidth(shapeRating, uiScale * 0.88f);
            UIWidget::drawText(renderer, shapeRating, padX + availableW - ratingW - (14.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
            curY += (48.0f * uiScale);
        }
        else if (currentTab == EditorTabId::FACE_HAIR)
        {
            // 1. Iris Colour
            if (cc->config.isOptionEnabled("eye_color"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Iris Colour", "Natural eye color.", s_eyeColors, cc->eyeColor, [&](const std::string& e) {
                    cc->eyeColor = e;
                }, uiScale);
            }

            // 2. Lip Size
            static const std::vector<std::string> allLips = { "Thin", "Average", "Full", "Plump", "Huge" };
            int lipIdx = std::clamp(cc->lipSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lip Dimensions", "Size and plumpness of lips.", allLips, allLips[lipIdx], [&](const std::string& l) {
                for (size_t i = 0; i < allLips.size(); ++i) if (allLips[i] == l) cc->lipSize = static_cast<int>(i);
            }, uiScale, 5);

            // 3. Puffy Lips Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Puffy Lips", "Extra softness and swelling.", cc->puffyLips, [&](bool p) {
                cc->puffyLips = p;
            }, uiScale, "Puffy", "Natural");

            // 4. Hair Length (Human Range: 0 to 100 cm)
            if (cc->config.isOptionEnabled("hair_length"))
            {
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Hair Length (cm)", "Normal human head hair length (0 - 100 cm).", std::format("{} cm", cc->hairLengthCm), [&](int delta) {
                    cc->hairLengthCm = std::clamp(cc->hairLengthCm + delta, 0, 100);
                }, uiScale, 1, 5, 15);
            }

            // 5. Hair Style (Filtered by Length Requirement)
            if (cc->config.isOptionEnabled("hair_style"))
            {
                auto validStyles = EditorConfig::getValidHairstyles(cc->hairLengthCm);
                auto styleOpts = cc->config.filterChoices("hair_style", validStyles);
                bool foundStyle = false;
                for (const auto& s : styleOpts) {
                    if (s == cc->hairStyle) { foundStyle = true; break; }
                }
                if (!foundStyle && !styleOpts.empty()) {
                    cc->hairStyle = styleOpts[0];
                }
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hairstyle", "Styling options available for your current hair length.", styleOpts, cc->hairStyle, [&](const std::string& s) {
                    cc->hairStyle = s;
                }, uiScale, 6);
            }

            // 6. Hair Color
            if (cc->config.isOptionEnabled("hair_color"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Hair Colour", "Select natural hair pigment.", s_hairColors, cc->hairColor, [&](const std::string& h) {
                    cc->hairColor = h;
                }, uiScale);
            }

            // 7. Ear Type (Human start)
            if (cc->config.isOptionEnabled("ear_type"))
            {
                static const std::vector<std::string> allEars = { "Human" };
                auto earOpts = cc->config.filterChoices("ear_type", allEars);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ear Morphology", "Human baseline ear structure.", earOpts, cc->earType, [&](const std::string& e) {
                    cc->earType = e;
                }, uiScale, 1);
            }
        }
        else if (currentTab == EditorTabId::BREASTS)
        {
            // 1. Human Breast Cup Size (Flat through G-cup)
            static const std::vector<std::string> humanCups = { "Flat", "AA", "A", "B", "C", "D", "DD", "E", "F", "FF", "G" };
            auto cupOpts = cc->config.filterChoices("chest_size", humanCups);
            int cupIdx = std::clamp(cc->breastCupSize, 0, static_cast<int>(cupOpts.size()) - 1);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Breast Cup Size", "Natural human breast cup size.", cupOpts, cupOpts[cupIdx], [&](const std::string& c) {
                for (size_t i = 0; i < cupOpts.size(); ++i) if (cupOpts[i] == c) cc->breastCupSize = static_cast<int>(i);
            }, uiScale, 6);

            // 2. Breast Shape
            static const std::vector<std::string> allBShapes = { "Round", "Pointy", "Perky", "Side-set", "Wide", "Narrow" };
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Breast Shape", "Contour and projection of breasts.", allBShapes, cc->breastShape, [&](const std::string& s) {
                cc->breastShape = s;
            }, uiScale, 6);

            // 3. Nipple & Areolae Sizes (Human 0..4)
            static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
            int nipIdx = std::clamp(cc->nippleSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Nipple Size", "Prominence of nipples.", allSizes5, allSizes5[nipIdx], [&](const std::string& n) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == n) cc->nippleSize = static_cast<int>(i);
            }, uiScale, 5);

            int areIdx = std::clamp(cc->areolaeSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Areolae Size", "Diameter of surrounding areolae.", allSizes5, allSizes5[areIdx], [&](const std::string& a) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == a) cc->areolaeSize = static_cast<int>(i);
            }, uiScale, 5);

            // 4. Puffy Nipples Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Puffy Nipples", "Extra softness and swelling.", cc->puffyNipples, [&](bool p) {
                cc->puffyNipples = p;
            }, uiScale, "Puffy", "Natural");

            // 5. Lactation Status
            static const std::vector<std::string> lactTiers = { "None", "Trickle", "Small Amount" };
            int lactIdx = std::clamp(cc->lactationTier, 0, static_cast<int>(lactTiers.size()) - 1);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lactation Production", "Mammary milk production level at start.", lactTiers, lactTiers[lactIdx], [&](const std::string& l) {
                for (size_t i = 0; i < lactTiers.size(); ++i) {
                    if (lactTiers[i] == l) {
                        cc->lactationTier = static_cast<int>(i);
                        cc->isLactating = (i > 0);
                    }
                }
            }, uiScale, 3);
        }
        else if (currentTab == EditorTabId::GENITALIA)
        {
            if (cc->gender == "Female")
            {
                static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
                int capIdx = std::clamp(cc->vaginaCapacity, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Vaginal Capacity", "Internal accommodation and depth.", allSizes5, allSizes5[capIdx], [&](const std::string& c) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == c) cc->vaginaCapacity = static_cast<int>(i);
                }, uiScale, 5);

                int labIdx = std::clamp(cc->labiaSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Labia Size", "Outer and inner labia prominence.", allSizes5, allSizes5[labIdx], [&](const std::string& l) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == l) cc->labiaSize = static_cast<int>(i);
                }, uiScale, 5);

                int clitIdx = std::clamp(cc->clitorisSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Clitoris Size", "Clitoral prominence and length.", allSizes5, allSizes5[clitIdx], [&](const std::string& cl) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == cl) cc->clitorisSize = static_cast<int>(i);
                }, uiScale, 5);
            }
            else
            {
                // Normal Human Penis Length: 8.0 cm to 24.0 cm
                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Penis Length (cm)", "Normal human erect length (8.0 - 24.0 cm).", std::format("{:.1f} cm", cc->penisLengthCm), [&](float delta) {
                    cc->penisLengthCm = std::clamp(cc->penisLengthCm + delta, 8.0f, 24.0f);
                }, uiScale, 0.5f, 1.0f, 2.5f);

                static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
                int testIdx = std::clamp(cc->testicleSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Testicle Size", "Testicular volume and scrotal size.", allSizes5, allSizes5[testIdx], [&](const std::string& t) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == t) cc->testicleSize = static_cast<int>(i);
                }, uiScale, 5);

                static const std::vector<std::string> cumTiers = { "Trickle", "Small Amount", "Decent Amount", "Large Amount" };
                int cumIdx = std::clamp(cc->cumProductionTier, 0, static_cast<int>(cumTiers.size()) - 1);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Cum Production", "Volume of ejaculatory output.", cumTiers, cumTiers[cumIdx], [&](const std::string& ct) {
                    for (size_t i = 0; i < cumTiers.size(); ++i) if (cumTiers[i] == ct) cc->cumProductionTier = static_cast<int>(i);
                }, uiScale, 4);
            }
        }
        else if (currentTab == EditorTabId::COSMETICS)
        {
            // Cosmetics Palettes
            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Lipstick", "Cosmetic pigment applied to lips.", s_makeupColors, cc->lipstick, [&](const std::string& c) {
                cc->lipstick = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Blusher / Rouge", "Cheek cosmetic coloring.", s_makeupColors, cc->blusher, [&](const std::string& c) {
                cc->blusher = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Eyeliner", "Cosmetic line around eyes.", s_makeupColors, cc->eyeliner, [&](const std::string& c) {
                cc->eyeliner = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Eye Shadow", "Pigment on eyelids.", s_makeupColors, cc->eyeshadow, [&](const std::string& c) {
                cc->eyeshadow = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Fingernail Polish", "Color on fingernails.", s_makeupColors, cc->nailPolish, [&](const std::string& c) {
                cc->nailPolish = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Toenail Polish", "Color on toenails.", s_makeupColors, cc->toenailPolish, [&](const std::string& c) {
                cc->toenailPolish = c;
            }, uiScale);

            // Piercings
            static const std::vector<std::string> pSockets = { "ear", "nose", "lip", "tongue", "navel", "nipple" };
            for (const auto& s : pSockets)
            {
                std::string sTitle = s + " piercing";
                sTitle[0] = static_cast<char>(std::toupper(sTitle[0]));
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, sTitle, "Piercing through " + s + ".", cc->piercings[s], [&cc, s](bool state) {
                    cc->piercings[s] = state;
                }, uiScale, "Pierced", "Unpierced");
            }

            // Body Hair
            static const std::vector<std::string> bhGroom = { "None", "Stubble", "Manicured", "Trimmed", "Natural", "Unkempt", "Bushy", "Wild" };
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Pubic Hair Grooming", "Hair grooming in the groin area.", bhGroom, cc->pubicHair, [&](const std::string& ph) {
                cc->pubicHair = ph;
            }, uiScale, 4);

            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Underarm Hair Grooming", "Hair grooming in the armpits.", bhGroom, cc->underarmHair, [&](const std::string& uh) {
                cc->underarmHair = uh;
            }, uiScale, 4);

            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ass Hair Grooming", "Hair grooming around the anus.", bhGroom, cc->assHair, [&](const std::string& ah) {
                cc->assHair = ah;
            }, uiScale, 4);
        }
        else if (currentTab == EditorTabId::WARDROBE)
        {
            if (!cc->wardrobeInitialized) cc->initializeWardrobe();

            // 1. Decency Status Card
            bool decent = cc->isClothedEnough();
            std::string decencyStatus = cc->getDecencyStatus();

            float bannerH = 48.0f * uiScale;
            SDL_FRect bannerRect = { padX, curY, availableW, bannerH };
            SDL_Color bannerBg = decent ? SDL_Color{ 25, 45, 32, 255 } : SDL_Color{ 50, 25, 25, 255 };
            SDL_Color bannerBorder = decent ? SDL_Color{ 60, 160, 80, 255 } : SDL_Color{ 200, 60, 60, 255 };
            UIWidget::drawPanel(renderer, bannerRect, bannerBg, bannerBorder);

            UIWidget::drawText(renderer, "Evening Wardrobe & Attire:", padX + (12.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
            UIWidget::drawText(renderer, decencyStatus, padX + (12.0f * uiScale), curY + (26.0f * uiScale), decent ? Theme::colors.companion : SDL_Color{ 240, 100, 100, 255 }, uiScale * 0.78f);
            curY += bannerH + (12.0f * uiScale);

            // 2. Equipped Slots (Worn Items)
            float wornSectionH = 120.0f * uiScale;
            SDL_FRect wornRect = { padX, curY, availableW, wornSectionH };
            UIWidget::drawPanel(renderer, wornRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Currently Worn Garments (Click 'Strip' to remove):", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

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
                    bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h &&
                                    uBtn.y + uBtnH >= rect.y && uBtn.y <= rect.y + rect.h);
                    bool uHover = inPanel && (mousePos.x >= uBtn.x && mousePos.x <= uBtn.x + uBtn.w && mousePos.y >= uBtn.y && mousePos.y <= uBtn.y + uBtn.h);
                    UIWidget::drawColoredButton(renderer, uBtn, "Strip", Theme::colors.bgButton, uHover ? Theme::colors.textGold : Theme::colors.textMuted, false, uiScale * 0.64f);
                    if (uHover && clicked)
                    {
                        cc->unequipWardrobeItem(s_displaySlots[i].slot);
                        gameContext->input.consumeMouseClick();
                    }
                }
            }
            curY += wornSectionH + (12.0f * uiScale);

            // 3. Available Wardrobe Pile
            float pileH = 110.0f * uiScale;
            SDL_FRect pileRect = { padX, curY, availableW, pileH };
            UIWidget::drawPanel(renderer, pileRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Wardrobe Pile (Click 'Equip' to put on):", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

            if (cc->availableWardrobe.empty())
            {
                UIWidget::drawText(renderer, "Nothing left in the wardrobe pile.", padX + (12.0f * uiScale), curY + (30.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
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
                    bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h &&
                                    eqBtn.y + eqBtnH >= rect.y && eqBtn.y <= rect.y + rect.h);
                    bool eqHover = inPanel && (mousePos.x >= eqBtn.x && mousePos.x <= eqBtn.x + eqBtn.w && mousePos.y >= eqBtn.y && mousePos.y <= eqBtn.y + eqBtn.h);
                    UIWidget::drawColoredButton(renderer, eqBtn, "Equip", Theme::colors.bgButton, eqHover ? Theme::colors.companion : Theme::colors.textMuted, false, uiScale * 0.64f);
                    if (eqHover && clicked)
                    {
                        cc->equipWardrobeItem(j);
                        gameContext->input.consumeMouseClick();
                        break;
                    }
                }
            }
            curY += pileH + (12.0f * uiScale);
        }
        else if (currentTab == EditorTabId::NAME_FINISH)
        {
            // 1. Name Input Box Card
            if (cc->config.isOptionEnabled("first_name") || cc->config.isOptionEnabled("surname"))
            {
                float nameCardH = 92.0f * uiScale;
                SDL_FRect ncRect = { padX, curY, availableW, nameCardH };
                UIWidget::drawPanel(renderer, ncRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, "Character Identity & Names:", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.90f);
                UIWidget::drawText(renderer, "Type to enter your name, or click Random to roll from the pool.", padX + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

                float fieldY = curY + (40.0f * uiScale);
                float boxH = 24.0f * uiScale;
                float rBtnW = 60.0f * uiScale;
                float labelW = 36.0f * uiScale;
                float inputW = 150.0f * uiScale;
                float colW = labelW + inputW + (6.0f * uiScale) + rBtnW;

                // First Name
                float firstX = padX + (12.0f * uiScale);
                UIWidget::drawText(renderer, "First:", firstX, fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
                SDL_FRect fnBox = { firstX + labelW, fieldY, inputW, boxH };
                bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h);
                bool fnHover = inPanel && (mousePos.x >= fnBox.x && mousePos.x <= fnBox.x + fnBox.w && mousePos.y >= fnBox.y && mousePos.y <= fnBox.y + fnBox.h);
                SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
                SDL_RenderFillRect(renderer, &fnBox);
                SDL_Color fnBorder = fnHover ? Theme::colors.textGold : Theme::colors.borderSelected;
                SDL_SetRenderDrawColor(renderer, fnBorder.r, fnBorder.g, fnBorder.b, fnBorder.a);
                SDL_RenderRect(renderer, &fnBox);
                UIWidget::drawText(renderer, chosenFirst, fnBox.x + (6.0f * uiScale), fnBox.y + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

                SDL_FRect rFirstBtn = { firstX + labelW + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
                bool rfHover = inPanel && (mousePos.x >= rFirstBtn.x && mousePos.x <= rFirstBtn.x + rFirstBtn.w && mousePos.y >= rFirstBtn.y && mousePos.y <= rFirstBtn.y + rFirstBtn.h);
                UIWidget::drawButton(renderer, rFirstBtn, "Random", rfHover, true, false, uiScale * 0.74f);
                if (rfHover && clicked) { cc->randomizeFirstNames(); gameContext->input.consumeMouseClick(); }

                // Surname
                float surStartX = padX + availableW - colW - (12.0f * uiScale);
                UIWidget::drawText(renderer, "Last:", surStartX, fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);
                SDL_FRect lnBox = { surStartX + labelW, fieldY, inputW, boxH };
                bool lnHover = inPanel && (mousePos.x >= lnBox.x && mousePos.x <= lnBox.x + lnBox.w && mousePos.y >= lnBox.y && mousePos.y <= lnBox.y + lnBox.h);
                SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
                SDL_RenderFillRect(renderer, &lnBox);
                SDL_Color lnBorder = lnHover ? Theme::colors.textGold : Theme::colors.borderNormal;
                SDL_SetRenderDrawColor(renderer, lnBorder.r, lnBorder.g, lnBorder.b, lnBorder.a);
                SDL_RenderRect(renderer, &lnBox);
                UIWidget::drawText(renderer, cc->surname.empty() ? "(None)" : cc->surname, lnBox.x + (6.0f * uiScale), lnBox.y + (4.0f * uiScale), cc->surname.empty() ? Theme::colors.textMuted : Theme::colors.textPrimary, uiScale * 0.85f);

                SDL_FRect rSurBtn = { surStartX + labelW + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
                bool rsHover = inPanel && (mousePos.x >= rSurBtn.x && mousePos.x <= rSurBtn.x + rSurBtn.w && mousePos.y >= rSurBtn.y && mousePos.y <= rSurBtn.y + rSurBtn.h);
                UIWidget::drawButton(renderer, rSurBtn, "Random", rsHover, true, false, uiScale * 0.74f);
                if (rsHover && clicked) { cc->randomizeSurname(); gameContext->input.consumeMouseClick(); }

                UIWidget::drawText(renderer, "Surname may be left blank. First name adapts to chosen gender & femininity.", padX + (12.0f * uiScale), curY + (70.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
                curY += nameCardH + (12.0f * uiScale);
            }

            // 2. Full Character Overview Stat Card
            float sumCardH = 96.0f * uiScale;
            SDL_FRect sumCardRect = { padX, curY, availableW, sumCardH };
            UIWidget::drawPanel(renderer, sumCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Character Profile Sheet:", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.90f);

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
            float startBtnX = padX + (availableW - startBtnW) / 2.0f;
            SDL_FRect startBtn = { startBtnX, curY, startBtnW, startBtnH };
            bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h);
            bool sbHover = inPanel && (mousePos.x >= startBtn.x && mousePos.x <= startBtn.x + startBtn.w && mousePos.y >= startBtn.y && mousePos.y <= startBtn.y + startBtn.h);

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
}
