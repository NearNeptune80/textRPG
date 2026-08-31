#include "ui/views/characterCreationView.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "state/characterCreationState.h"
#include <format>
#include <vector>
#include <string>
#include <cctype>
#include <functional>
#include <algorithm>

namespace CharacterCreationView
{
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

    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
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

            // Check if this option represents a color palette (skin, hair, eyes, etc.)
            bool allHaveColors = true;
            for (const auto& l : labels) {
                if (getChoiceColor(l).a == 0) { allHaveColors = false; break; }
            }

            if (allHaveColors && labels.size() >= 5)
            {
                // Pure small-square color swatch palette
                float tileSize = 24.0f * uiScale;
                float gap = 6.0f * uiScale;
                float padLeft = padX + (10.0f * uiScale);
                float maxGridW = availableW - (20.0f * uiScale);
                int maxCols = std::max(1, static_cast<int>((maxGridW + gap) / (tileSize + gap)));
                int cols = std::min(static_cast<int>(labels.size()), std::min(maxCols, 24));
                int rows = (static_cast<int>(labels.size()) + cols - 1) / cols;

                std::string selName = (selectedIndex >= 0 && selectedIndex < static_cast<int>(labels.size())) ? labels[selectedIndex] : "None";
                SDL_Color selColor = getChoiceColor(selName);

                float descH = description.empty() ? (26.0f * uiScale) : (44.0f * uiScale);
                float cardH = descH + (rows * (tileSize + gap)) + (10.0f * uiScale);

                SDL_FRect cardRect = { padX, curY, availableW, cardH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                // Header with color swatch preview
                SDL_FRect previewBox = { padX + (10.0f * uiScale), curY + (6.0f * uiScale), 16.0f * uiScale, 16.0f * uiScale };
                UIWidget::drawPanel(renderer, previewBox, selColor, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, std::format("{}: {}", title, selName), padX + (32.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);

                if (!description.empty())
                {
                    UIWidget::drawText(renderer, description, padX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.72f);
                }

                float gridStartY = curY + descH;

                for (size_t i = 0; i < labels.size(); ++i)
                {
                    int r = static_cast<int>(i / cols);
                    int c = static_cast<int>(i % cols);
                    SDL_FRect sRect = { padLeft + (c * (tileSize + gap)), gridStartY + (r * (tileSize + gap)), tileSize, tileSize };

                    bool isSel = (static_cast<int>(i) == selectedIndex);
                    bool pHover = (mousePos.x >= sRect.x && mousePos.x <= sRect.x + sRect.w &&
                                   mousePos.y >= sRect.y && mousePos.y <= sRect.y + sRect.h);

                    SDL_Color col = getChoiceColor(labels[i]);
                    UIWidget::drawColorSwatch(renderer, sRect, col, isSel, pHover, uiScale);

                    if (pHover && clicked)
                    {
                        onSelect(static_cast<int>(i));
                        gameContext->input.consumeMouseClick();
                    }
                }
                curY += cardH + (8.0f * uiScale);
            }
            else if (useSingleRowRight)
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

            float bBtnW = 22.0f * uiScale;
            float bBtnH = 22.0f * uiScale;
            float controlsTotalW = 270.0f * uiScale;
            float bdayControlsX = padX + availableW - controlsTotalW - (10.0f * uiScale);
            
            // Day Stepper: [<] [ Day 29 ] [>]
            SDL_FRect dayM = { bdayControlsX, curY + (12.0f * uiScale), bBtnW, bBtnH };
            SDL_FRect dayVal = { bdayControlsX + bBtnW + (3.0f * uiScale), curY + (12.0f * uiScale), 70.0f * uiScale, bBtnH };
            SDL_FRect dayP = { bdayControlsX + bBtnW + (76.0f * uiScale), curY + (12.0f * uiScale), bBtnW, bBtnH };
            
            bool dHovM = (mousePos.x >= dayM.x && mousePos.x <= dayM.x + dayM.w && mousePos.y >= dayM.y && mousePos.y <= dayM.y + dayM.h);
            bool dHovP = (mousePos.x >= dayP.x && mousePos.x <= dayP.x + dayP.w && mousePos.y >= dayP.y && mousePos.y <= dayP.y + dayP.h);
            UIWidget::drawButton(renderer, dayM, "<", dHovM, true, false, uiScale * 0.75f);
            UIWidget::drawPanel(renderer, dayVal, Theme::colors.bgSlot, Theme::colors.borderNormal);
            std::string dStr = std::format("Day {:02d}", cc->birthDay);
            float dStrW = UIWidget::getTextWidth(dStr, uiScale * 0.72f);
            UIWidget::drawText(renderer, dStr, dayVal.x + std::max(2.0f, (dayVal.w - dStrW) / 2.0f), curY + (15.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
            UIWidget::drawButton(renderer, dayP, ">", dHovP, true, false, uiScale * 0.75f);

            // Age Stepper: [<] [ 22 yrs ] [>]
            float ageControlsX = bdayControlsX + (140.0f * uiScale);
            SDL_FRect ageM = { ageControlsX, curY + (12.0f * uiScale), bBtnW, bBtnH };
            SDL_FRect ageVal = { ageControlsX + bBtnW + (3.0f * uiScale), curY + (12.0f * uiScale), 70.0f * uiScale, bBtnH };
            SDL_FRect ageP = { ageControlsX + bBtnW + (76.0f * uiScale), curY + (12.0f * uiScale), bBtnW, bBtnH };

            bool aHovM = (mousePos.x >= ageM.x && mousePos.x <= ageM.x + ageM.w && mousePos.y >= ageM.y && mousePos.y <= ageM.y + ageM.h);
            bool aHovP = (mousePos.x >= ageP.x && mousePos.x <= ageP.x + ageP.w && mousePos.y >= ageP.y && mousePos.y <= ageP.y + ageP.h);
            UIWidget::drawButton(renderer, ageM, "<", aHovM, true, false, uiScale * 0.75f);
            UIWidget::drawPanel(renderer, ageVal, Theme::colors.bgSlot, Theme::colors.borderNormal);
            std::string aStr = std::format("{} yrs", cc->birthAge);
            float aStrW = UIWidget::getTextWidth(aStr, uiScale * 0.72f);
            UIWidget::drawText(renderer, aStr, ageVal.x + std::max(2.0f, (ageVal.w - aStrW) / 2.0f), curY + (15.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
            UIWidget::drawButton(renderer, ageP, ">", aHovP, true, false, uiScale * 0.75f);

            if (clicked)
            {
                if (dHovM) { cc->birthDay = (cc->birthDay <= 1) ? 31 : (cc->birthDay - 1); gameContext->input.consumeMouseClick(); }
                else if (dHovP) { cc->birthDay = (cc->birthDay >= 31) ? 1 : (cc->birthDay + 1); gameContext->input.consumeMouseClick(); }
                else if (aHovM) { cc->birthAge = std::max(18, cc->birthAge - 1); gameContext->input.consumeMouseClick(); }
                else if (aHovP) { cc->birthAge = std::min(99, cc->birthAge + 1); gameContext->input.consumeMouseClick(); }
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
}
