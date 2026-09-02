#include "ui/views/mainMenuView.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include <format>
#include <string>
#include <string_view>
#include <algorithm>

namespace MainMenuView
{
    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
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

        bool inGame = (gameContext && gameContext->getPlayer());

        // Subtitle
        std::string subTitle = inGame ? "Game Paused — Main Menu" : "Studio Edition";
        float subTextW = UIWidget::getTextWidth(subTitle, uiScale * 1.15f);
        UIWidget::drawText(renderer, subTitle, centerX - (subTextW / 2.0f), curY, SDL_Color{ 200, 140, 235, 255 }, uiScale * 1.15f);
        curY += (34.0f * uiScale);

        if (inGame)
        {
            entity* p = gameContext->getPlayer();
            std::string raceTitle = p->anatomy.getRacialTitle().empty() ? "Human" : p->anatomy.getRacialTitle();
            std::string statusLine = std::format("Active Character: {} • Level {} {}", p->name, p->stats.level, raceTitle);
            float sW = UIWidget::getTextWidth(statusLine, uiScale * 0.95f);
            UIWidget::drawText(renderer, statusLine, centerX - (sW / 2.0f), curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (28.0f * uiScale);

            float p1H = UIWidget::drawTextWrapped(renderer,
                "Your current adventure is active. Select 'Continue' in the bottom right of the action commands grid below (or press ESC) to return directly to exploration.",
                textX, curY, textW, Theme::colors.textPrimary, uiScale * 0.92f);
            curY += p1H + (18.0f * uiScale);

            float p2H = UIWidget::drawTextWrapped(renderer,
                "Use 'Save/Load' to write a manual save file or load a profile. Adjust 'Options' and 'Content Options' to configure gameplay mechanics, difficulty, and demographics.",
                textX, curY, textW, Theme::colors.textSecondary, uiScale * 0.9f);
            curY += p2H + (18.0f * uiScale);

            float p3H = UIWidget::drawTextWrapped(renderer,
                "Selecting 'New Game' will begin a new character creation sequence. Select 'Quit' to exit the game.",
                textX, curY, textW, Theme::colors.textMuted, uiScale * 0.88f);
            curY += p3H + (24.0f * uiScale);
        }
        else
        {
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
        }

        // Engine version
        std::string_view verText = "Engine Version: 0.5.0 Alpha (C++ Edition)";
        float verW = UIWidget::getTextWidth(verText, uiScale * 0.85f);
        UIWidget::drawText(renderer, verText, centerX - (verW / 2.0f), curY, Theme::colors.textMuted, uiScale * 0.85f);
        curY += (24.0f * uiScale);

        return (curY - startY);
    }
}
