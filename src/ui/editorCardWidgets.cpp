#include "ui/editorCardWidgets.h"
#include <cctype>

namespace EditorCardWidgets
{
    namespace
    {
        bool equalsIgnoreCase(std::string_view a, std::string_view b)
        {
            return std::ranges::equal(a, b, [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb));
            });
        }
    }

    const std::vector<ColorOption>& getSkinTones()
    {
        static const std::vector<ColorOption> s_skinTones = {
            { "PALE", "Pale", { 252, 228, 214, 255 } },
            { "LIGHT", "Light", { 245, 212, 186, 255 } },
            { "PORCELAIN", "Porcelain", { 255, 243, 236, 255 } },
            { "FAIR", "Fair", { 248, 224, 204, 255 } },
            { "ROSY", "Rosy", { 245, 192, 182, 255 } },
            { "OLIVE", "Olive", { 200, 185, 130, 255 } },
            { "TANNED", "Tanned", { 215, 155, 105, 255 } },
            { "DARK", "Dark", { 155, 95, 60, 255 } },
            { "EBONY", "Ebony", { 110, 70, 50, 255 } }
        };
        return s_skinTones;
    }

    const std::vector<ColorOption>& getHairColors()
    {
        static const std::vector<ColorOption> s_hairColors = {
            { "BLACK", "Black", { 35, 35, 35, 255 } },
            { "DARK_BROWN", "Dark Brown", { 90, 55, 35, 255 } },
            { "BROWN", "Brown", { 140, 85, 55, 255 } },
            { "AUBURN", "Auburn", { 165, 75, 45, 255 } },
            { "GINGER", "Ginger", { 225, 115, 40, 255 } },
            { "BLONDE", "Blonde", { 240, 210, 95, 255 } },
            { "PLATINUM", "Platinum", { 230, 235, 240, 255 } },
            { "GREY", "Grey", { 170, 175, 180, 255 } },
            { "WHITE", "White", { 245, 245, 250, 255 } },
            { "RED", "Red", { 205, 45, 45, 255 } },
            { "PINK", "Pink", { 245, 120, 180, 255 } },
            { "BLUE", "Blue", { 65, 140, 235, 255 } }
        };
        return s_hairColors;
    }

    const std::vector<ColorOption>& getEyeColors()
    {
        static const std::vector<ColorOption> s_eyeColors = {
            { "BROWN", "Brown", { 115, 70, 40, 255 } },
            { "HAZEL", "Hazel", { 165, 135, 60, 255 } },
            { "GREEN", "Green", { 65, 160, 85, 255 } },
            { "BLUE", "Blue", { 55, 135, 225, 255 } },
            { "AMBER", "Amber", { 225, 160, 35, 255 } },
            { "GREY", "Grey", { 160, 165, 170, 255 } },
            { "VIOLET", "Violet", { 160, 90, 210, 255 } },
            { "BLACK", "Black", { 30, 30, 30, 255 } }
        };
        return s_eyeColors;
    }

    const std::vector<ColorOption>& getMakeupColors()
    {
        static const std::vector<ColorOption> s_makeupColors = {
            { "NONE", "None", { 0, 0, 0, 0 } },
            { "RED", "Red", { 210, 35, 35, 255 } },
            { "DARK_RED", "Dark Red", { 145, 25, 25, 255 } },
            { "LIGHT_RED", "Light Red", { 240, 105, 105, 255 } },
            { "PINK", "Pink", { 245, 110, 175, 255 } },
            { "LIGHT_PINK", "Light Pink", { 250, 170, 195, 255 } },
            { "PURPLE", "Purple", { 160, 75, 215, 255 } },
            { "LIGHT_PURPLE", "Light Purple", { 205, 150, 240, 255 } },
            { "BLACK", "Black", { 25, 25, 25, 255 } },
            { "WHITE", "White", { 240, 240, 245, 255 } },
            { "GOLD", "Gold", { 230, 190, 40, 255 } },
            { "SILVER", "Silver", { 195, 200, 210, 255 } },
            { "CLEAR", "Clear", { 150, 210, 230, 255 } }
        };
        return s_makeupColors;
    }

    const std::vector<ColorOption>& getTransformationColors()
    {
        static const std::vector<ColorOption> tfColors = {
            { "PALE", "Pale", { 243, 215, 196, 255 } },
            { "LIGHT", "Light", { 232, 196, 168, 255 } },
            { "PORCELAIN", "Porcelain", { 247, 230, 216, 255 } },
            { "ROSY", "Rosy", { 232, 179, 154, 255 } },
            { "OLIVE", "Olive", { 196, 146, 98, 255 } },
            { "TANNED", "Tanned", { 198, 134, 66, 255 } },
            { "DARK", "Dark", { 141, 85, 36, 255 } },
            { "EBONY", "Ebony", { 59, 34, 19, 255 } },
            { "BLACK", "Raven Black", { 30, 30, 30, 255 } },
            { "WHITE", "Pure White", { 245, 245, 245, 255 } },
            { "GREY", "Silver Grey", { 160, 160, 160, 255 } },
            { "RED", "Ruby Red", { 220, 20, 60, 255 } },
            { "CRIMSON", "Crimson", { 139, 0, 0, 255 } },
            { "PURPLE", "Royal Purple", { 123, 63, 161, 255 } },
            { "VIOLET", "Violet", { 155, 89, 182, 255 } },
            { "LILAC", "Lilac", { 200, 162, 200, 255 } },
            { "PINK", "Pastel Pink", { 255, 107, 218, 255 } },
            { "HOT_PINK", "Hot Pink", { 255, 20, 147, 255 } },
            { "BLUE", "Azure Blue", { 59, 110, 165, 255 } },
            { "SKY_BLUE", "Sky Blue", { 135, 206, 235, 255 } },
            { "GREEN", "Emerald Green", { 61, 140, 64, 255 } },
            { "GOLD", "Radiant Gold", { 212, 175, 55, 255 } },
            { "AMBER", "Golden Amber", { 196, 123, 23, 255 } },
            { "BROWN", "Chestnut Brown", { 107, 63, 29, 255 } }
        };
        return tfColors;
    }

    float drawPillCard(SDL_Renderer* renderer,
                       game* gameContext,
                       const SDL_FRect& panelRect,
                       float padX,
                       float curY,
                       float availableW,
                       std::string_view title,
                       std::string_view description,
                       const std::vector<std::string>& options,
                       const std::string& currentSelected,
                       const std::function<void(const std::string&)>& onSelect,
                       float uiScale,
                       int cols)
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

            bool isSelected = equalsIgnoreCase(options[i], currentSelected);
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            bRect.y + bRect.h >= panelRect.y && bRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                       mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isSelected, uiScale * 0.76f);

            if (hovered && clicked && onSelect)
            {
                onSelect(options[i]);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float drawTogglePillCard(SDL_Renderer* renderer,
                             game* gameContext,
                             const SDL_FRect& panelRect,
                             float padX,
                             float curY,
                             float availableW,
                             std::string_view title,
                             std::string_view description,
                             const std::vector<std::string>& options,
                             const std::set<std::string>& activeItems,
                             const std::function<void(const std::string&, bool)>& onToggle,
                             float uiScale,
                             int cols)
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

            if (hovered && clicked && onToggle)
            {
                onToggle(options[i], !isActive);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float drawTogglePillCard(SDL_Renderer* renderer,
                             game* gameContext,
                             const SDL_FRect& panelRect,
                             float padX,
                             float curY,
                             float availableW,
                             std::string_view title,
                             std::string_view description,
                             const std::vector<std::string>& options,
                             const std::vector<std::string>& activeItems,
                             const std::function<void(const std::string&, bool)>& onToggle,
                             float uiScale,
                             int cols)
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

            bool isActive = (std::find(activeItems.begin(), activeItems.end(), options[i]) != activeItems.end());
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            bRect.y + bRect.h >= panelRect.y && bRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                       mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isActive, uiScale * 0.76f);

            if (hovered && clicked && onToggle)
            {
                onToggle(options[i], !isActive);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float drawColorSwatchCard(SDL_Renderer* renderer,
                              game* gameContext,
                              const SDL_FRect& panelRect,
                              float padX,
                              float curY,
                              float availableW,
                              std::string_view title,
                              std::string_view description,
                              const std::vector<ColorOption>& options,
                              const std::string& currentSelected,
                              const std::function<void(const std::string&)>& onSelect,
                              float uiScale)
    {
        float startY = curY;
        float innerPad = 12.0f * uiScale;
        float tileSize = 28.0f * uiScale;
        float gap = 8.0f * uiScale;

        std::string selectedName = options.empty() ? "None" : options[0].name;
        SDL_Color selectedColor = options.empty() ? Theme::colors.textGold : options[0].color;
        for (const auto& opt : options)
        {
            if (equalsIgnoreCase(opt.name, currentSelected) || equalsIgnoreCase(opt.id, currentSelected))
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

            bool isSelected = equalsIgnoreCase(options[i].name, selectedName) || equalsIgnoreCase(options[i].id, selectedName);
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            sRect.y + sRect.h >= panelRect.y && sRect.y <= panelRect.y + panelRect.h);
            bool hovered = inPanel && (mousePos.x >= sRect.x && mousePos.x <= sRect.x + sRect.w &&
                                       mousePos.y >= sRect.y && mousePos.y <= sRect.y + sRect.h);

            if (options[i].color.a == 0)
            {
                UIWidget::drawButton(renderer, sRect, "X", hovered, true, isSelected, uiScale * 0.72f);
            }
            else
            {
                UIWidget::drawColorSwatch(renderer, sRect, options[i].color, isSelected, hovered, uiScale);
            }

            if (hovered && clicked && onSelect)
            {
                onSelect(options[i].name);
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float drawCycleStepperCard(SDL_Renderer* renderer,
                               game* gameContext,
                               const SDL_FRect& panelRect,
                               float padX,
                               float curY,
                               float availableW,
                               std::string_view title,
                               std::string_view description,
                               const std::vector<std::string>& options,
                               const std::string& currentSelected,
                               const std::function<void(int)>& onCycle,
                               float uiScale)
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

        float btnW = 34.0f * uiScale;
        float valW = 160.0f * uiScale;
        float h = 26.0f * uiScale;
        float controlsTotalW = (btnW * 2.0f) + valW + (8.0f * uiScale);
        float curX = padX + availableW - innerPad - controlsTotalW;
        float controlY = curY + ((cardH - h) / 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Left Cycle Button (<)
        SDL_FRect leftBtn = { curX, controlY, btnW, h };
        bool leftIn = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                       controlY + h >= panelRect.y && controlY <= panelRect.y + panelRect.h);
        bool leftHov = leftIn && (mousePos.x >= leftBtn.x && mousePos.x <= leftBtn.x + leftBtn.w &&
                                  mousePos.y >= leftBtn.y && mousePos.y <= leftBtn.y + leftBtn.h);
        UIWidget::drawButton(renderer, leftBtn, "<", leftHov, true, false, uiScale * 0.76f);
        if (leftHov && clicked && onCycle)
        {
            onCycle(-1);
            gameContext->input.consumeMouseClick();
        }
        curX += btnW + (4.0f * uiScale);

        // Center Display Box
        SDL_FRect valRect = { curX, controlY, valW, h };
        UIWidget::drawPanel(renderer, valRect, Theme::colors.bgHeader, Theme::colors.borderNormal);
        std::string dispStr = currentSelected.empty() ? "None" : currentSelected;
        float textW = UIWidget::getTextWidth(dispStr, uiScale * 0.82f);
        UIWidget::drawText(renderer, dispStr, curX + ((valW - textW) / 2.0f), controlY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
        curX += valW + (4.0f * uiScale);

        // Right Cycle Button (>)
        SDL_FRect rightBtn = { curX, controlY, btnW, h };
        bool rightIn = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                        controlY + h >= panelRect.y && controlY <= panelRect.y + panelRect.h);
        bool rightHov = rightIn && (mousePos.x >= rightBtn.x && mousePos.x <= rightBtn.x + rightBtn.w &&
                                   mousePos.y >= rightBtn.y && mousePos.y <= rightBtn.y + rightBtn.h);
        UIWidget::drawButton(renderer, rightBtn, ">", rightHov, true, false, uiScale * 0.76f);
        if (rightHov && clicked && onCycle)
        {
            onCycle(1);
            gameContext->input.consumeMouseClick();
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }

    float drawToggleCard(SDL_Renderer* renderer,
                         game* gameContext,
                         const SDL_FRect& panelRect,
                         float padX,
                         float curY,
                         float availableW,
                         std::string_view title,
                         std::string_view description,
                         bool currentState,
                         const std::function<void(bool)>& onToggle,
                         float uiScale,
                         std::string_view trueLabel,
                         std::string_view falseLabel)
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
        float btnH = 26.0f * uiScale;
        float btnGap = 8.0f * uiScale;
        float controlY = curY + ((cardH - btnH) / 2.0f);
        float curX = padX + availableW - innerPad - (btnW * 2.0f) - btnGap;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // True Button
        SDL_FRect trueRect = { curX, controlY, btnW, btnH };
        bool trueIn = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                       controlY + btnH >= panelRect.y && controlY <= panelRect.y + panelRect.h);
        bool trueHov = trueIn && (mousePos.x >= trueRect.x && mousePos.x <= trueRect.x + trueRect.w &&
                                  mousePos.y >= trueRect.y && mousePos.y <= trueRect.y + trueRect.h);
        UIWidget::drawButton(renderer, trueRect, trueLabel, trueHov, true, currentState, uiScale * 0.76f);
        if (trueHov && clicked && onToggle)
        {
            onToggle(true);
            gameContext->input.consumeMouseClick();
        }

        curX += btnW + btnGap;

        // False Button
        SDL_FRect falseRect = { curX, controlY, btnW, btnH };
        bool falseIn = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                        controlY + btnH >= panelRect.y && controlY <= panelRect.y + panelRect.h);
        bool falseHov = falseIn && (mousePos.x >= falseRect.x && mousePos.x <= falseRect.x + falseRect.w &&
                                    mousePos.y >= falseRect.y && mousePos.y <= falseRect.y + falseRect.h);
        UIWidget::drawButton(renderer, falseRect, falseLabel, falseHov, true, !currentState, uiScale * 0.76f);
        if (falseHov && clicked && onToggle)
        {
            onToggle(false);
            gameContext->input.consumeMouseClick();
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }
}
