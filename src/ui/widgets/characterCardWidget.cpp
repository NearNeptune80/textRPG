#include "ui/widgets/characterCardWidget.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include "state/characterCreationState.h"
#include <format>
#include <vector>
#include <string_view>

namespace CharacterCardWidget
{
    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
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
}
