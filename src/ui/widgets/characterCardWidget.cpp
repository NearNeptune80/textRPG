#include "ui/widgets/characterCardWidget.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

namespace CharacterCardWidget
{
    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* player = gameContext ? gameContext->getPlayer() : nullptr;
        if (!player) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);

        // ==========================================
        // CARD 1: Character Identity & Wealth Card
        // ==========================================
        float card1H = 68.0f * uiScale;
        SDL_FRect card1Rect = { padX, curY, availableW, card1H };
        UIWidget::drawPanel(renderer, card1Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerPad = 8.0f * uiScale;
        float c1X = padX + innerPad;
        float c1W = availableW - (innerPad * 2.0f);
        float c1Y = curY + (6.0f * uiScale);

        // Avatar Badge
        float avatarSize = 32.0f * uiScale;
        SDL_FRect avatarRect = { c1X, c1Y, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string initials = "AV";
        if (!player->name.empty())
        {
            initials = player->name.substr(0, 1);
            size_t spacePos = player->name.find(' ');
            if (spacePos != std::string::npos && spacePos + 1 < player->name.length())
            {
                initials += player->name[spacePos + 1];
            }
        }
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.85f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        // Name & Subtitle
        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, c1X + avatarSize + (8.0f * uiScale), c1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.95f);
        std::string lvlStr = std::format("Level {} • {}", player->stats.level, player->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, c1X + avatarSize + (8.0f * uiScale), c1Y + (18.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

        // Wealth & Essence Row
        float goldVal = player->getStat("currency");
        if (goldVal <= 0.0f && !inPrologue) goldVal = 5000.0f;
        std::string goldText = std::format("Gold: {:.0f} ¤", goldVal);
        UIWidget::drawText(renderer, goldText, c1X, c1Y + avatarSize + (6.0f * uiScale), Theme::colors.currency, uiScale * 0.80f);

        std::string essenceText = "Essence: 0";
        float essW = UIWidget::getTextWidth(essenceText, uiScale * 0.80f);
        UIWidget::drawText(renderer, essenceText, c1X + c1W - essW, c1Y + avatarSize + (6.0f * uiScale), Theme::colors.arcane, uiScale * 0.80f);

        curY += card1H + (10.0f * uiScale);

        // ==========================================
        // CARD 2: Vitals & Status Gauges Card
        // ==========================================
        float card2H = 138.0f * uiScale;
        SDL_FRect card2Rect = { padX, curY, availableW, card2H };
        UIWidget::drawPanel(renderer, card2Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float c2X = padX + innerPad;
        float c2W = availableW - (innerPad * 2.0f);
        float c2Y = curY + (6.0f * uiScale);

        UIWidget::drawText(renderer, "VITALS & STATUS", c2X, c2Y, Theme::colors.textGold, uiScale * 0.78f);
        c2Y += (18.0f * uiScale);

        float barH = 12.0f * uiScale;
        float barGap = 4.0f * uiScale;
        float labelW = 46.0f * uiScale;
        float valW = 54.0f * uiScale;
        float progressW = c2W - labelW - valW - (6.0f * uiScale);

        // 1. Health Bar
        float curHp = player->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = player->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "Health", c2X, c2Y + (1.0f * uiScale), Theme::colors.health, uiScale * 0.72f);
        UIWidget::drawProgressBar(renderer, { c2X + labelW, c2Y, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgDark, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, c2X + labelW + progressW + (6.0f * uiScale), c2Y + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.70f);
        c2Y += barH + barGap;

        // 2. Mana Bar
        float curMp = inPrologue ? 0.0f : player->getStat("mana");
        if (curMp <= 0.0f && !inPrologue) curMp = 80.0f;
        float maxMp = inPrologue ? 0.0f : player->getStat("max_mana");
        if (maxMp <= 0.0f && !inPrologue) maxMp = 80.0f;

        UIWidget::drawText(renderer, "Mana", c2X, c2Y + (1.0f * uiScale), Theme::colors.mana, uiScale * 0.72f);
        UIWidget::drawProgressBar(renderer, { c2X + labelW, c2Y, progressW, barH }, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgDark, "", uiScale);
        std::string mpStr = inPrologue ? "0/0" : std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, c2X + labelW + progressW + (6.0f * uiScale), c2Y + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.70f);
        c2Y += barH + barGap;

        // 3. Lust Bar
        float curLust = player->getStat("lust");
        float maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", c2X, c2Y + (1.0f * uiScale), Theme::colors.lust, uiScale * 0.72f);
        UIWidget::drawProgressBar(renderer, { c2X + labelW, c2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgDark, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, c2X + labelW + progressW + (6.0f * uiScale), c2Y + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.70f);
        c2Y += barH + barGap;

        // 4. Arousal Bar
        float curArousal = player->getStat("arousal");
        float maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", c2X, c2Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
        UIWidget::drawProgressBar(renderer, { c2X + labelW, c2Y, progressW, barH }, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgDark, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", curArousal);
        UIWidget::drawText(renderer, arousalStr, c2X + labelW + progressW + (6.0f * uiScale), c2Y + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.70f);
        c2Y += barH + (6.0f * uiScale);

        // Status trait chips
        static const std::vector<std::pair<std::string, SDL_Color>> traits = {
            { "Phys", Theme::colors.health },
            { "Arc", Theme::colors.mana },
            { "Form", Theme::colors.companion },
            { "Buff", Theme::colors.textGold }
        };
        float tGap = 4.0f * uiScale;
        float tW = (c2W - (tGap * (traits.size() - 1))) / static_cast<float>(traits.size());
        float tH = 18.0f * uiScale;

        for (size_t t = 0; t < traits.size(); ++t)
        {
            SDL_FRect tBox = { c2X + (t * (tW + tGap)), c2Y, tW, tH };
            UIWidget::drawPanel(renderer, tBox, Theme::colors.bgDark, Theme::colors.borderButton);
            float txtW = UIWidget::getTextWidth(traits[t].first, uiScale * 0.65f);
            UIWidget::drawText(renderer, traits[t].first, tBox.x + ((tW - txtW) / 2.0f), tBox.y + (2.0f * uiScale), traits[t].second, uiScale * 0.65f);
        }

        curY += card2H + (10.0f * uiScale);
        return (curY - startY);
    }
}
