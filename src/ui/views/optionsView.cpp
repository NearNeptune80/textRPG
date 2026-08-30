#include "ui/views/optionsView.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "settings/settingsManager.h"
#include "state/optionsState.h"
#include <format>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <string_view>

namespace OptionsView
{
    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
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
}
