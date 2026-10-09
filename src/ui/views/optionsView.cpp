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

        auto isInView = [&](const SDL_FRect& r) {
            return (mousePos.x >= rect.x && mousePos.x <= rect.x + rect.w &&
                    mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h &&
                    r.y + r.h >= rect.y && r.y <= rect.y + rect.h);
        };

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

            curY += panelH + (60.0f * uiScale);
            return (curY - startY);
        }

        if (opt->screenMode == OptionsScreenMode::GENERAL_OPTIONS)
        {
            float cardW = std::min(availableW, 360.0f * uiScale);
            float cardH = 32.0f * uiScale;
            UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Options", Theme::colors.textPrimary, uiScale);
            curY += cardH + (14.0f * uiScale);

            float textW = availableW;
            float textX = padX;

            SDL_FRect themeCardRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, themeCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Visual Theme:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);
            UIWidget::drawText(renderer, "Select application color palette and UI theme styling.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

            struct ThemeChoice { const char* id; const char* label; };
            static constexpr ThemeChoice themeChoices[] = {
                { "default", "Midnight" },
                { "theme_cyber_neon", "Cyber" },
                { "theme_dark_fantasy", "Fantasy" },
                { "theme_parchment", "Parchment" }
            };

            float themePillTotalW = 280.0f * uiScale;
            float themePillH = 24.0f * uiScale;
            float themePillItemW = (themePillTotalW - (3.0f * 4.0f * uiScale)) / 4.0f;
            float themePillStartX = textX + textW - themePillTotalW - (12.0f * uiScale);
            float themePillY = curY + (10.0f * uiScale);

            std::string curTheme = gameContext->settings.display.activeTheme;
            if (curTheme.empty()) curTheme = "default";

            for (size_t t = 0; t < 4; ++t)
            {
                SDL_FRect tRect = { themePillStartX + (t * (themePillItemW + 4.0f * uiScale)), themePillY, themePillItemW, themePillH };
                bool tHovered = isInView(tRect) && (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w && mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);
                bool isSelected = (curTheme == themeChoices[t].id || (curTheme == "default" && t == 0));

                SDL_Color bgCol = isSelected ? Theme::colors.bgSlotSelected : (tHovered ? Theme::colors.bgButtonHover : Theme::colors.bgButton);
                SDL_Color borderCol = isSelected ? Theme::colors.borderSelected : (tHovered ? Theme::colors.borderButtonHover : Theme::colors.borderButton);

                SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                SDL_RenderFillRect(renderer, &tRect);
                SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                SDL_RenderRect(renderer, &tRect);

                SDL_Color tTextCol = isSelected ? Theme::colors.textGold : (tHovered ? Theme::colors.textPrimary : Theme::colors.textMuted);
                float labelW = UIWidget::getTextWidth(themeChoices[t].label, uiScale * 0.76f);
                UIWidget::drawText(renderer, themeChoices[t].label, tRect.x + ((tRect.w - labelW) / 2.0f), tRect.y + (4.0f * uiScale), tTextCol, uiScale * 0.76f);

                if (tHovered && clicked)
                {
                    gameContext->settings.display.activeTheme = themeChoices[t].id;
                    Theme::applyTheme(themeChoices[t].id);
                    gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
            }
            curY += themeCardRect.h + (10.0f * uiScale);

            SDL_FRect fontCardRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, fontCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Font-size:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

            std::string fontDesc = std::format("Adjusts UI base font size (Min 12, Max 36). Current: {}pt.", gameContext->settings.display.fontSize);
            UIWidget::drawText(renderer, fontDesc, textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

            float btnW = 32.0f * uiScale;
            float btnH = 22.0f * uiScale;
            SDL_FRect minusBtn = { textX + textW - (btnW * 2.0f) - (14.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };
            SDL_FRect plusBtn = { textX + textW - btnW - (8.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };

            bool minusHovered = isInView(minusBtn) && (mousePos.x >= minusBtn.x && mousePos.x <= minusBtn.x + minusBtn.w && mousePos.y >= minusBtn.y && mousePos.y <= minusBtn.y + minusBtn.h);
            bool plusHovered = isInView(plusBtn) && (mousePos.x >= plusBtn.x && mousePos.x <= plusBtn.x + plusBtn.w && mousePos.y >= plusBtn.y && mousePos.y <= plusBtn.y + plusBtn.h);

            UIWidget::drawButton(renderer, minusBtn, "-", minusHovered, true, false, uiScale * 0.8f);
            UIWidget::drawButton(renderer, plusBtn, "+", plusHovered, true, false, uiScale * 0.8f);

            if (minusHovered && clicked)
            {
                if (gameContext->settings.display.fontSize > 12)
                {
                    gameContext->settings.display.fontSize -= 2;
                    opt->fontSize = gameContext->settings.display.fontSize;
                    gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
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
                    gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                    gameContext->refreshActionGrid();
                }
                gameContext->input.consumeMouseClick();
            }
            curY += fontCardRect.h + (10.0f * uiScale);

            SDL_FRect pronounRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, pronounRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Gender Pronouns:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

            float proW = 68.0f * uiScale;
            float proStartX = textX + textW - (proW * 2.0f) - (12.0f * uiScale);
            float proDescW = proStartX - textX - (20.0f * uiScale);
            UIWidget::drawTextWrapped(renderer, "Set pronoun style for narrative text (Normal vs Custom gender neutral).", textX + (10.0f * uiScale), curY + (24.0f * uiScale), proDescW, Theme::colors.textSecondary, uiScale * 0.80f);

            SDL_FRect normalPill = { proStartX, curY + (10.0f * uiScale), proW, 24.0f * uiScale };
            SDL_FRect customPill = { proStartX + proW, curY + (10.0f * uiScale), proW, 24.0f * uiScale };

            bool normHovered = isInView(normalPill) && (mousePos.x >= normalPill.x && mousePos.x <= normalPill.x + normalPill.w && mousePos.y >= normalPill.y && mousePos.y <= normalPill.y + normalPill.h);
            bool custHovered = isInView(customPill) && (mousePos.x >= customPill.x && mousePos.x <= customPill.x + customPill.w && mousePos.y >= customPill.y && mousePos.y <= customPill.y + customPill.h);

            bool isNorm = (gameContext->settings.gameplay.genderPronounMode == "Normal");
            UIWidget::drawColoredButton(renderer, normalPill, "Normal", isNorm ? Theme::colors.toggleOn : Theme::colors.bgButton, isNorm ? Theme::colors.textPrimary : Theme::colors.textMuted, isNorm, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, customPill, "Custom", !isNorm ? Theme::colors.toggleOn : Theme::colors.bgButton, !isNorm ? Theme::colors.textPrimary : Theme::colors.textMuted, !isNorm, uiScale * 0.72f);

            if (normHovered && clicked)
            {
                gameContext->settings.gameplay.genderPronounMode = "Normal";
                opt->genderPronounMode = "Normal";
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            else if (custHovered && clicked)
            {
                gameContext->settings.gameplay.genderPronounMode = "Custom";
                opt->genderPronounMode = "Custom";
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            curY += pronounRect.h + (10.0f * uiScale);

            SDL_FRect unitsRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, unitsRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Measurement Units:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);

            float unitW = 68.0f * uiScale;
            float unitStartX = textX + textW - (unitW * 2.0f) - (12.0f * uiScale);
            float unitDescW = unitStartX - textX - (20.0f * uiScale);
            UIWidget::drawTextWrapped(renderer, "Set units for height, length, and volume measurements.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), unitDescW, Theme::colors.textSecondary, uiScale * 0.80f);

            SDL_FRect metricPill = { unitStartX, curY + (10.0f * uiScale), unitW, 24.0f * uiScale };
            SDL_FRect imperialPill = { unitStartX + unitW, curY + (10.0f * uiScale), unitW, 24.0f * uiScale };

            bool metricHovered = isInView(metricPill) && (mousePos.x >= metricPill.x && mousePos.x <= metricPill.x + metricPill.w && mousePos.y >= metricPill.y && mousePos.y <= metricPill.y + metricPill.h);
            bool impHovered = isInView(imperialPill) && (mousePos.x >= imperialPill.x && mousePos.x <= imperialPill.x + imperialPill.w && mousePos.y >= imperialPill.y && mousePos.y <= imperialPill.y + imperialPill.h);

            bool isMetric = (gameContext->settings.gameplay.unitPreference == "Metric");
            UIWidget::drawColoredButton(renderer, metricPill, "Metric", isMetric ? Theme::colors.toggleOn : Theme::colors.bgButton, isMetric ? Theme::colors.textPrimary : Theme::colors.textMuted, isMetric, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, imperialPill, "Imperial", !isMetric ? Theme::colors.toggleOn : Theme::colors.bgButton, !isMetric ? Theme::colors.textPrimary : Theme::colors.textMuted, !isMetric, uiScale * 0.72f);

            if (metricHovered && clicked)
            {
                gameContext->settings.gameplay.unitPreference = "Metric";
                opt->unitPreference = "Metric";
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            else if (impHovered && clicked)
            {
                gameContext->settings.gameplay.unitPreference = "Imperial";
                opt->unitPreference = "Imperial";
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            curY += unitsRect.h + (10.0f * uiScale);

            SDL_FRect artRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, artRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Artwork & Portraits:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);
            UIWidget::drawText(renderer, "Enables character portraits and illustrations in information panels.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

            float pillW = 55.0f * uiScale;
            float pillStartX = textX + textW - (pillW * 2.0f) - (12.0f * uiScale);
            SDL_FRect artOff = { pillStartX, curY + (10.0f * uiScale), pillW, 24.0f * uiScale };
            SDL_FRect artOn = { pillStartX + pillW, curY + (10.0f * uiScale), pillW, 24.0f * uiScale };

            bool artOffHov = isInView(artOff) && (mousePos.x >= artOff.x && mousePos.x <= artOff.x + artOff.w && mousePos.y >= artOff.y && mousePos.y <= artOff.y + artOff.h);
            bool artOnHov = isInView(artOn) && (mousePos.x >= artOn.x && mousePos.x <= artOn.x + artOn.w && mousePos.y >= artOn.y && mousePos.y <= artOn.y + artOn.h);
            bool isArtOn = gameContext->settings.display.showArtwork;

            UIWidget::drawColoredButton(renderer, artOff, "OFF", !isArtOn ? Theme::colors.toggleOff : Theme::colors.bgButton, !isArtOn ? Theme::colors.textPrimary : Theme::colors.textMuted, !isArtOn, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, artOn, "ON", isArtOn ? Theme::colors.toggleOn : Theme::colors.bgButton, isArtOn ? Theme::colors.textPrimary : Theme::colors.textMuted, isArtOn, uiScale * 0.72f);

            if (artOffHov && clicked)
            {
                gameContext->settings.display.showArtwork = false;
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            else if (artOnHov && clicked)
            {
                gameContext->settings.display.showArtwork = true;
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            curY += artRect.h + (10.0f * uiScale);

            SDL_FRect thumbRect = { textX, curY, textW, 44.0f * uiScale };
            UIWidget::drawPanel(renderer, thumbRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Hover Thumbnails:", textX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.92f);
            UIWidget::drawText(renderer, "Enables tooltip popup thumbnails when inspecting characters or items.", textX + (10.0f * uiScale), curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

            SDL_FRect thOff = { pillStartX, curY + (10.0f * uiScale), pillW, 24.0f * uiScale };
            SDL_FRect thOn = { pillStartX + pillW, curY + (10.0f * uiScale), pillW, 24.0f * uiScale };

            bool thOffHov = isInView(thOff) && (mousePos.x >= thOff.x && mousePos.x <= thOff.x + thOff.w && mousePos.y >= thOff.y && mousePos.y <= thOff.y + thOff.h);
            bool thOnHov = isInView(thOn) && (mousePos.x >= thOn.x && mousePos.x <= thOn.x + thOn.w && mousePos.y >= thOn.y && mousePos.y <= thOn.y + thOn.h);
            bool isThOn = gameContext->settings.display.showThumbnails;

            UIWidget::drawColoredButton(renderer, thOff, "OFF", !isThOn ? Theme::colors.toggleOff : Theme::colors.bgButton, !isThOn ? Theme::colors.textPrimary : Theme::colors.textMuted, !isThOn, uiScale * 0.72f);
            UIWidget::drawColoredButton(renderer, thOn, "ON", isThOn ? Theme::colors.toggleOn : Theme::colors.bgButton, isThOn ? Theme::colors.textPrimary : Theme::colors.textMuted, isThOn, uiScale * 0.72f);

            if (thOffHov && clicked)
            {
                gameContext->settings.display.showThumbnails = false;
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            else if (thOnHov && clicked)
            {
                gameContext->settings.display.showThumbnails = true;
                gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            curY += thumbRect.h + (10.0f * uiScale);

            SDL_FRect diffHeaderRect = { textX, curY, textW, 24.0f * uiScale };
            UIWidget::drawText(renderer, "Gameplay Difficulty Tiers:", textX, curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.95f);
            curY += (26.0f * uiScale);

            struct DiffTier { std::string name; std::string desc; SDL_Color col; };
            const DiffTier tiers[5] = {
                { "Very Easy", "Relaxed narrative focus; enemies deal 75% damage and have reduced resilience.", Theme::colors.friendly },
                { "Easy", "Mild combat challenge with reduced enemy damage (85%).", Theme::colors.companion },
                { "Normal", "The standard balanced adventure experience (1.0x baseline stats).", Theme::colors.textPrimary },
                { "Hard", "Enemies level up aggressively and deal 150% damage. Tactical skill required.", Theme::colors.textAccent },
                { "Nightmare", "Brutal, uncompromising challenge; enemies deal 250% damage and fight ruthlessly.", Theme::colors.enemy }
            };

            for (int i = 0; i < 5; ++i)
            {
                bool isCurrentDiff = (gameContext->settings.gameplay.difficultyLevel == i);
                float nameOffset = UIWidget::getTextWidth(tiers[i].name, uiScale * 0.88f) + (16.0f * uiScale);
                float descW = textW - nameOffset - (14.0f * uiScale);

                float cardH2 = 30.0f * uiScale;

                SDL_FRect tierRect = { textX, curY, textW, cardH2 };
                bool tierHovered = isInView(tierRect) && (mousePos.x >= tierRect.x && mousePos.x <= tierRect.x + tierRect.w && mousePos.y >= tierRect.y && mousePos.y <= tierRect.y + tierRect.h);

                SDL_Color tierBg = isCurrentDiff ? Theme::colors.bgSlotSelected : (tierHovered ? Theme::colors.bgButtonHover : Theme::colors.bgSlot);
                SDL_Color tierBorder = isCurrentDiff ? Theme::colors.borderSelected : (tierHovered ? Theme::colors.borderButtonHover : Theme::colors.borderNormal);
                UIWidget::drawPanel(renderer, tierRect, tierBg, tierBorder);

                UIWidget::drawText(renderer, tiers[i].name, textX + (10.0f * uiScale), curY + (6.0f * uiScale), tiers[i].col, uiScale * 0.88f);
                UIWidget::drawTextWrapped(renderer, tiers[i].desc, textX + nameOffset, curY + (6.0f * uiScale), descW, isCurrentDiff ? Theme::colors.textPrimary : Theme::colors.textSecondary, uiScale * 0.80f);

                if (tierHovered && clicked)
                {
                    gameContext->settings.gameplay.difficultyLevel = i;
                    opt->difficultyLevel = i;
                    static constexpr float diffMults[] = { 0.75f, 0.85f, 1.0f, 1.5f, 2.5f };
                    gameContext->settings.gameplay.difficultyMultiplier = diffMults[i];
                    gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }

                curY += cardH2 + (5.0f * uiScale);
            }

            curY += (60.0f * uiScale);
            return (curY - startY);
        }
        else
        {
            std::string catName = "Misc.";
            if (opt->contentCategory == ContentOptionsCategory::GAMEPLAY) catName = "Gameplay";
            else if (opt->contentCategory == ContentOptionsCategory::FETISH_PREFS) catName = "Fetish preferences";
            else if (opt->contentCategory == ContentOptionsCategory::BODIES) catName = "Bodies";
            else if (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS) catName = "Gender preferences";
            else if (opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS) catName = "Orientation preferences";
            else if (opt->contentCategory == ContentOptionsCategory::AGE_PREFS) catName = "Age preferences";
            else if (opt->contentCategory == ContentOptionsCategory::FURRY_PREFS) catName = "Furry preferences";

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

            auto renderInfoDropdown = [&]() {
                float dropW = availableW;
                float dropH = 26.0f * uiScale;
                SDL_FRect dropRect = { padX, curY, dropW, dropH };
                UIWidget::drawPanel(renderer, dropRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                std::string infoStr = "> Click for more info.";
                float strW = UIWidget::getTextWidth(infoStr, uiScale * 0.86f);
                UIWidget::drawText(renderer, infoStr, padX + ((dropW - strW) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textAccent, uiScale * 0.86f);
                curY += dropH + (12.0f * uiScale);
            };

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

            auto renderOptionCard = [&](const std::string& title, SDL_Color titleCol, const std::string& description, const std::vector<std::string>& pillLabels, int selectedIndex, std::function<void(int)> onSelect) {
                float cardWidth = availableW;
                float maxItemTextW = 0.0f;
                for (const auto& l : pillLabels)
                {
                    maxItemTextW = std::max(maxItemTextW, UIWidget::getTextWidth(l, uiScale * 0.78f));
                }
                float pillItemW = std::max(56.0f * uiScale, maxItemTextW + (18.0f * uiScale));
                float pillTotalW = pillItemW * pillLabels.size();
                float leftW = cardWidth - pillTotalW - (20.0f * uiScale);
                float cardMinH = 48.0f * uiScale;

                SDL_FRect cardRect = { padX, curY, cardWidth, cardMinH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, title + ":", padX + (10.0f * uiScale), curY + (7.0f * uiScale), titleCol, uiScale * 0.88f);
                float titleW = UIWidget::getTextWidth(title + ":", uiScale * 0.88f);

                float descH = UIWidget::drawTextWrapped(renderer, description, padX + (10.0f * uiScale) + titleW + (8.0f * uiScale), curY + (7.0f * uiScale), leftW - titleW - (18.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);

                float pillH = 24.0f * uiScale;
                float pillStartX = padX + cardWidth - pillTotalW - (12.0f * uiScale);
                float pillY = curY + ((cardMinH - pillH) / 2.0f);

                for (size_t p = 0; p < pillLabels.size(); ++p)
                {
                    SDL_FRect pRect = { pillStartX + (p * pillItemW), pillY, pillItemW, pillH };
                    bool pHovered = isInView(pRect) && (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                     mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                    bool isSelected = (static_cast<int>(p) == selectedIndex);

                    SDL_Color bgCol = Theme::colors.bgButton;
                    SDL_Color borderCol = Theme::colors.borderButton;
                    if (isSelected)
                    {
                        if (pillLabels[p] == "OFF")
                        {
                            bgCol = Theme::colors.toggleOff;
                            borderCol = Theme::colors.enemy;
                        }
                        else if (pillLabels[p] == "WARN")
                        {
                            bgCol = SDL_Color{ 180, 110, 20, 255 };
                            borderCol = Theme::colors.textGold;
                        }
                        else if (pillLabels[p] == "ON")
                        {
                            bgCol = Theme::colors.toggleOn;
                            borderCol = Theme::colors.friendly;
                        }
                        else
                        {
                            bgCol = Theme::colors.bgSlotSelected;
                            borderCol = Theme::colors.borderSelected;
                        }
                    }
                    else if (pHovered)
                    {
                        bgCol = Theme::colors.bgButtonHover;
                        borderCol = Theme::colors.borderButtonHover;
                    }

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
                    bool pHovered = isInView(pRect) && (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                     mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                    bool isSelected = (static_cast<int>(p) == selectedIndex);

                    SDL_Color bgCol = isSelected ? Theme::colors.bgSlotSelected : (pHovered ? Theme::colors.bgButtonHover : Theme::colors.bgSlot);
                    SDL_Color borderCol = isSelected ? Theme::colors.borderSelected : (pHovered ? Theme::colors.borderButtonHover : Theme::colors.borderMuted);

                    SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                    SDL_RenderFillRect(renderer, &pRect);
                    SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                    SDL_RenderRect(renderer, &pRect);

                    SDL_Color pTextCol = isSelected ? Theme::colors.textPrimary : (pHovered ? Theme::colors.textGold : Theme::colors.textMuted);
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

            struct ToggleDef {
                std::string title;
                SDL_Color color;
                std::string description;
                bool* pVal;
            };

            auto renderToggles = [&](const std::vector<ToggleDef>& defs) {
                for (const auto& def : defs)
                {
                    renderOptionCard(def.title, def.color, def.description, { "OFF", "ON" }, *def.pVal ? 1 : 0,
                                     [&, p = def.pVal](int idx) {
                                         *p = (idx == 1);
                                         gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                     });
                }
            };


            auto renderDemographicCategory = [&](const std::vector<std::tuple<std::string, SDL_Color, std::string, float*>>& items) {
                renderInfoDropdown();
                std::vector<std::pair<float, SDL_Color>> segments;
                for (const auto& [title, col, desc, pVal] : items)
                {
                    renderFrequencyRow(title, col, desc, floatToFreqIdx(*pVal),
                                       [&, p = pVal](int idx) {
                                           *p = freqIdxToFloat(idx);
                                           gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                       });
                    segments.push_back({ *pVal, col });
                }
                renderDistributionBar(segments);
            };

            if (opt->contentCategory == ContentOptionsCategory::MISC)
            {
                renderToggles({
                    { "Autosave on Map Transition", Theme::colors.companion, "Automatically saves the game when travelling between world zones.", &gameContext->settings.gameplay.autoSaveOnMapChange },
                    { "Autosave on Scene Exit", Theme::colors.companion, "Automatically saves when finishing an intimate scene, combat, or dialogue.", &gameContext->settings.gameplay.autoSaveOnSceneExit },
                    { "Shared Encyclopedia", Theme::colors.textGold, "When enabled, your character will use the shared Encyclopedia across playthroughs.", &gameContext->settings.gameplay.sharedEncyclopedia }
                });

                int maxSlotsIdx = (gameContext->settings.gameplay.maxAutoSaves <= 1) ? 0 :
                                  (gameContext->settings.gameplay.maxAutoSaves <= 3 ? 1 : 2);
                renderOptionCard("Max Autosave Slots", Theme::colors.friendly, "Number of rotating autosave files before older ones are overwritten.",
                                 { "1 Slot", "3 Slots", "5 Slots" },
                                 maxSlotsIdx,
                                 [&](int idx) {
                                     static constexpr int slotCounts[] = { 1, 3, 5 };
                                     gameContext->settings.gameplay.maxAutoSaves = slotCounts[idx];
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });
            }
            else if (opt->contentCategory == ContentOptionsCategory::GAMEPLAY)
            {
                renderToggles({
                    { "Bad Ends", Theme::colors.enemy, "Toggle the ability to trigger 'bad ends', which end the game when encountered.", &gameContext->settings.gameplay.badEndsEnabled },
                    { "Level Drain", Theme::colors.enemy, "Toggle the use of the 'orgasmic level drain' perk by unique NPCs.", &gameContext->settings.gameplay.levelDrainEnabled },
                    { "Opportunistic attackers", Theme::colors.enemy, "Makes random attacks more likely when you're high on lust, low health, or exposed.", &gameContext->settings.gameplay.opportunisticAttackers },
                    { "Auto-Loot Defeated Enemies", Theme::colors.friendly, "Automatically collects dropped coin and essentials upon combat victory.", &gameContext->settings.gameplay.autoLoot }
                });

                int curLossIdx = (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.05f) ? 0 :
                                 (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.20f ? 1 :
                                 (gameContext->settings.gameplay.currencyLossOnDefeatPercent < 0.40f ? 2 : 3));
                renderOptionCard("Currency Loss on Defeat", Theme::colors.currency, "Percentage of carried gold dropped when suffering a defeat.",
                                 { "0%", "10%", "25%", "50%" },
                                 curLossIdx,
                                 [&](int idx) {
                                     static constexpr float lossPcts[] = { 0.0f, 0.10f, 0.25f, 0.50f };
                                     gameContext->settings.gameplay.currencyLossOnDefeatPercent = lossPcts[idx];
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });
            }
            else if (opt->contentCategory == ContentOptionsCategory::BODIES)
            {

                int styleIdx = (gameContext->settings.content.furryGenitalStyle == "Animalistic") ? 1 : 0;
                renderOptionCard("Furry Genitalia Style", Theme::colors.arcane, "Toggles between standard humanoid anatomy and animalistic traits (knots, sheaths, barbs).",
                                 { "Humanoid", "Animalistic" },
                                 styleIdx,
                                 [&](int idx) {
                                     gameContext->settings.content.furryGenitalStyle = (idx == 1) ? "Animalistic" : "Humanoid";
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });

                int tfIdx = (gameContext->settings.content.transformationSpeedMultiplier >= 5.0f) ? 0 :
                            (gameContext->settings.content.transformationSpeedMultiplier >= 1.5f ? 1 :
                            (gameContext->settings.content.transformationSpeedMultiplier >= 0.8f ? 2 :
                            (gameContext->settings.content.transformationSpeedMultiplier > 0.0f ? 3 : 4)));
                renderOptionCard("Transformation Speed", Theme::colors.currency, "Governs speed of anatomical mutations and bodily reshaping.",
                                 { "Instant", "Fast", "Normal", "Slow", "Off" },
                                 tfIdx,
                                 [&](int idx) {
                                     static constexpr float tfVals[] = { 10.0f, 2.0f, 1.0f, 0.5f, 0.0f };
                                     gameContext->settings.content.transformationSpeedMultiplier = tfVals[idx];
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });
            }
            else if (opt->contentCategory == ContentOptionsCategory::GENDER_PREFS)
            {
                auto& d = gameContext->settings.demographics;
                renderDemographicCategory({
                    { "Male / Masculine", Theme::colors.friendly, "Masculine bodies with male anatomy.", &d.percentMale },
                    { "Female / Feminine", Theme::colors.lust, "Feminine bodies with female anatomy.", &d.percentFemale },
                    { "Hermaphrodite", Theme::colors.arcane, "Dual sex anatomy with breasts and penis.", &d.percentHermaphrodite },
                    { "Gynomorph", Theme::colors.textAccent, "Feminine frame with penis and breasts.", &d.percentGynomorph },
                    { "Andromorph", Theme::colors.friendly, "Masculine frame with vagina and flat chest.", &d.percentAndromorph },
                    { "Asexual / Null", Theme::colors.textMuted, "Neutral form with smooth groin.", &d.percentNull }
                });
            }
            else if (opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS)
            {
                auto& d = gameContext->settings.demographics;
                renderDemographicCategory({
                    { "Heterosexual", Theme::colors.friendly, "Attracted to opposite sex.", &d.percentHetero },
                    { "Bisexual", Theme::colors.arcane, "Attracted to both sexes.", &d.percentBi },
                    { "Homosexual", Theme::colors.lust, "Attracted to same sex.", &d.percentHomo },
                    { "Asexual", Theme::colors.textMuted, "Low or no sexual interest.", &d.percentAsexual }
                });
            }
            else if (opt->contentCategory == ContentOptionsCategory::AGE_PREFS)
            {
                auto& d = gameContext->settings.demographics;
                renderDemographicCategory({
                    { "Young Adult (18-25)", Theme::colors.companion, "", &d.percentYoungAdult },
                    { "Adult (26-40)", Theme::colors.friendly, "", &d.percentAdult },
                    { "Mature (41-60)", Theme::colors.currency, "", &d.percentMature },
                    { "Elder (60+)", Theme::colors.enemy, "", &d.percentElder }
                });
            }
            else if (opt->contentCategory == ContentOptionsCategory::FURRY_PREFS)
            {
                auto& d = gameContext->settings.demographics;
                renderDemographicCategory({
                    { "Human / Pureblood", Theme::colors.textPrimary, "Regular human bodies without morph features.", &d.percentHuman },
                    { "Partial Morph (Ears/Tail)", Theme::colors.currency, "Humanoid bodies with animal ears, tails, or horns.", &d.percentPartial },
                    { "Anthropomorphic", Theme::colors.arcane, "Full fur, muzzle, and digitigrade anatomy on bipedal form.", &d.percentAnthro },
                    { "Feral / Bestial", Theme::colors.enemy, "Quadrupedal / animalistic body structures.", &d.percentFeral }
                });
            }
            else if (opt->contentCategory == ContentOptionsCategory::FETISH_PREFS)
            {
                int filterIdx = static_cast<int>(gameContext->settings.content.contentFilterMode);
                renderOptionCard("Content Filter Handling", Theme::colors.textGold, "Governs how content set to 'Never' is handled in narrative scenes and dialogue choices.",
                                 { "Dropdown", "Warn & Confirm", "Block & Skip" },
                                 filterIdx,
                                 [&](int idx) {
                                     gameContext->settings.content.contentFilterMode = static_cast<ContentFilterMode>(idx);
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });

                int fluidIdx = (gameContext->settings.content.fluidMultiplier <= 0.3f) ? 0 :
                               (gameContext->settings.content.fluidMultiplier <= 0.7f ? 1 :
                               (gameContext->settings.content.fluidMultiplier <= 1.5f ? 2 :
                               (gameContext->settings.content.fluidMultiplier <= 3.0f ? 3 : 4)));
                renderOptionCard("Fluid Multiplier", Theme::colors.friendly, "Scales fluid volume generated during climax and bodily transformations.",
                                 { "0.25x", "0.5x", "1.0x", "2.0x", "4.0x" },
                                 fluidIdx,
                                 [&](int idx) {
                                     static constexpr float flVals[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
                                     gameContext->settings.content.fluidMultiplier = flVals[idx];
                                     gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                                 });

                curY += 8.0f * uiScale;
                float subW = std::min(availableW, 400.0f * uiScale);
                float subH = 30.0f * uiScale;
                UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, subW, subH, "Fetish Desires & Frequency", Theme::colors.textGold, uiScale);
                curY += subH + (10.0f * uiScale);

                renderInfoDropdown();

                static const char* fetishes[] = {
                    "Anal", "Aphrodisiacs & Drugs", "BDSM & Restraints", "Bondage",
                    "Breasts", "Breasts lover", "Buttslut", "Crossdressing",
                    "Cum Inflation & Expansion", "Defloration", "Denial & Edging", "Dominance",
                    "Exhibitionism / Public Sex", "Feet", "Foot worship", "Feral / Bestiality",
                    "Forced Transformation", "Hypnosis & Mind Control", "Incest", "Insemination",
                    "Lactation", "Masochism", "Milk lover", "Non-Consent",
                    "Oral", "Oral performer", "Penetration", "Pregnancy",
                    "Prolapse", "Pussy slut", "Sadistic sex / Extreme", "Size Difference",
                    "Spitting", "Submission", "Tentacles & Monsters", "Transformations",
                    "Vaginal", "Voyeurism", "Watersports"
                };

                static constexpr std::string_view fetPills[] = {
                    "Never", "V.Rare", "Rare", "Average", "Common", "V.Common", "Always"
                };

                for (size_t i = 0; i < std::size(fetishes); ++i)
                {
                    float cardWidth = availableW;
                    float rowMinH = 34.0f * uiScale;
                    SDL_FRect rowRect = { padX, curY, cardWidth, rowMinH };
                    UIWidget::drawPanel(renderer, rowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    UIWidget::drawText(renderer, "[i]", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                    UIWidget::drawText(renderer, fetishes[i], padX + (36.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);

                    float pillTotalW = 390.0f * uiScale;
                    float pillH = 22.0f * uiScale;
                    float pillItemW = (pillTotalW - (6.0f * 4.0f * uiScale)) / 7.0f;
                    float pillStartX = padX + cardWidth - pillTotalW - (10.0f * uiScale);
                    float pillY = curY + (6.0f * uiScale);

                    int curRating = gameContext->settings.content.getFetishRating(fetishes[i]);

                    for (size_t p = 0; p < std::size(fetPills); ++p)
                    {
                        SDL_FRect pRect = { pillStartX + (p * (pillItemW + 4.0f * uiScale)), pillY, pillItemW, pillH };
                        bool pHovered = isInView(pRect) && (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                                         mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);
                        bool isSelected = (static_cast<int>(p) == curRating);

                        SDL_Color bgCol = Theme::colors.bgButton;
                        SDL_Color borderCol = Theme::colors.borderButton;
                        SDL_Color pTextCol = Theme::colors.textMuted;

                        if (isSelected)
                        {
                            if (p == 0) { bgCol = Theme::colors.bgButtonDisabled; borderCol = Theme::colors.borderButtonDisabled; pTextCol = Theme::colors.textDisabled; }
                            else if (p == 1) { bgCol = Theme::colors.toggleOff; borderCol = Theme::colors.enemy; pTextCol = Theme::colors.textPrimary; }
                            else if (p == 2) { bgCol = Theme::colors.bgSlotOccupied; borderCol = Theme::colors.physique; pTextCol = Theme::colors.textPrimary; }
                            else if (p == 3) { bgCol = Theme::colors.bgSlotSelected; borderCol = Theme::colors.borderSelected; pTextCol = Theme::colors.textGold; }
                            else if (p == 4) { bgCol = Theme::colors.bgSlotOccupied; borderCol = Theme::colors.companion; pTextCol = Theme::colors.textPrimary; }
                            else if (p == 5) { bgCol = Theme::colors.bgButton; borderCol = Theme::colors.friendly; pTextCol = Theme::colors.textPrimary; }
                            else { bgCol = Theme::colors.badgeBackground; borderCol = Theme::colors.textGold; pTextCol = Theme::colors.textGold; }
                        }
                        else if (pHovered)
                        {
                            bgCol = Theme::colors.bgButtonHover;
                            borderCol = Theme::colors.borderButtonHover;
                            pTextCol = Theme::colors.textGold;
                        }

                        SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
                        SDL_RenderFillRect(renderer, &pRect);
                        SDL_SetRenderDrawColor(renderer, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
                        SDL_RenderRect(renderer, &pRect);

                        float labelW = UIWidget::getTextWidth(std::string(fetPills[p]), uiScale * 0.70f);
                        UIWidget::drawText(renderer, std::string(fetPills[p]), pRect.x + ((pRect.w - labelW) / 2.0f), pRect.y + (3.0f * uiScale), pTextCol, uiScale * 0.70f);

                        if (pHovered && clicked)
                        {
                            gameContext->settings.content.setFetishPreference(fetishes[i], static_cast<int>(p));
                            gameContext->handleCommand({ CommandType::SAVE_SETTINGS });
                            gameContext->input.consumeMouseClick();
                        }
                    }

                    curY += rowMinH + (6.0f * uiScale);
                }
            }

            curY += (60.0f * uiScale);
            return (curY - startY);
        }
    }
}
