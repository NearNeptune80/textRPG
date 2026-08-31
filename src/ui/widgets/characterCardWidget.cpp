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

        // 1. Location Header Banner Card
        float locHeaderH = 26.0f * uiScale;
        SDL_FRect locHeaderRect = { padX, curY, availableW, locHeaderH };
        std::string locName = inPrologue ? "Museum - Lobby" : "Lilaya's Home F1 - Corridor";
        if (const gameMap* m = gameContext->getActiveMap())
        {
            if (!m->getName().empty() && m->getName() != "District Map")
            {
                locName = m->getName();
            }
        }
        UIWidget::drawHeader(renderer, locHeaderRect, locName, Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.88f);
        curY += locHeaderH + (8.0f * uiScale);

        // 2. Main Character Bio Card
        float cardH = 175.0f * uiScale;
        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerPad = 8.0f * uiScale;
        float contentX = padX + innerPad;
        float contentW = availableW - (innerPad * 2.0f);
        float cardCurY = curY + (6.0f * uiScale);

        // Row A: Avatar Badge + Name & Level Title
        float avatarSize = 28.0f * uiScale;
        SDL_FRect avatarRect = { contentX, cardCurY, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgDark, Theme::colors.borderButton);

        // Initial letters in avatar box
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
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, contentX + avatarSize + (8.0f * uiScale), cardCurY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.92f);
        std::string lvlStr = std::format("Level {} ({})", player->stats.level, player->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, contentX + avatarSize + (8.0f * uiScale), cardCurY + (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        cardCurY += avatarSize + (8.0f * uiScale);

        // Row B: Currency & Arcane Essence
        float goldVal = player->getStat("currency");
        if (goldVal <= 0.0f && !inPrologue) goldVal = 5000.0f;
        std::string goldText = std::format("Gold: {:.0f} ¤", goldVal);
        UIWidget::drawText(renderer, goldText, contentX, cardCurY, Theme::colors.currency, uiScale * 0.82f);

        std::string essenceText = "Essence: 0";
        float essW = UIWidget::getTextWidth(essenceText, uiScale * 0.82f);
        UIWidget::drawText(renderer, essenceText, contentX + contentW - essW, cardCurY, Theme::colors.arcane, uiScale * 0.82f);
        cardCurY += (18.0f * uiScale);

        // Row C: 4 Core Vitals Progress Bars
        float barH = 13.0f * uiScale;
        float barGap = 4.0f * uiScale;
        float labelW = 54.0f * uiScale;
        float valW = 52.0f * uiScale;
        float progressW = contentW - labelW - valW - (6.0f * uiScale);

        // 1. Health Bar (Red/Coral)
        float curHp = player->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = player->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "HP", contentX, cardCurY + (1.0f * uiScale), Theme::colors.health, uiScale * 0.76f);
        SDL_FRect hpBarRect = { contentX + labelW, cardCurY, progressW, barH };
        UIWidget::drawProgressBar(renderer, hpBarRect, curHp, maxHp, Theme::colors.health, Theme::colors.bgDark, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, contentX + labelW + progressW + (6.0f * uiScale), cardCurY + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.72f);
        cardCurY += barH + barGap;

        // 2. Mana Bar (Arcane Purple/Blue)
        float curMp = inPrologue ? 0.0f : player->getStat("mana");
        if (curMp <= 0.0f && !inPrologue) curMp = 80.0f;
        float maxMp = inPrologue ? 0.0f : player->getStat("max_mana");
        if (maxMp <= 0.0f && !inPrologue) maxMp = 80.0f;

        UIWidget::drawText(renderer, "MP", contentX, cardCurY + (1.0f * uiScale), Theme::colors.mana, uiScale * 0.76f);
        SDL_FRect mpBarRect = { contentX + labelW, cardCurY, progressW, barH };
        UIWidget::drawProgressBar(renderer, mpBarRect, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgDark, "", uiScale);
        std::string mpStr = inPrologue ? "0/0" : std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, contentX + labelW + progressW + (6.0f * uiScale), cardCurY + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.72f);
        cardCurY += barH + barGap;

        // 3. Lust Bar (Pink/Magenta)
        float curLust = player->getStat("lust");
        float maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", contentX, cardCurY + (1.0f * uiScale), Theme::colors.lust, uiScale * 0.76f);
        SDL_FRect lustBarRect = { contentX + labelW, cardCurY, progressW, barH };
        UIWidget::drawProgressBar(renderer, lustBarRect, curLust, maxLust, Theme::colors.lust, Theme::colors.bgDark, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, contentX + labelW + progressW + (6.0f * uiScale), cardCurY + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.72f);
        cardCurY += barH + barGap;

        // 4. Arousal Bar (Gold/Orange)
        float curArousal = player->getStat("arousal");
        float maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", contentX, cardCurY + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.76f);
        SDL_FRect arousalBarRect = { contentX + labelW, cardCurY, progressW, barH };
        UIWidget::drawProgressBar(renderer, arousalBarRect, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgDark, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", curArousal);
        UIWidget::drawText(renderer, arousalStr, contentX + labelW + progressW + (6.0f * uiScale), cardCurY + (1.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.72f);
        cardCurY += barH + (8.0f * uiScale);

        // Row D: Trait Badges Row
        static const std::vector<std::pair<std::string, SDL_Color>> traits = {
            { "Phys", Theme::colors.health },
            { "Arc", Theme::colors.mana },
            { "Form", Theme::colors.companion },
            { "Buff", Theme::colors.textGold }
        };
        float tGap = 4.0f * uiScale;
        float tW = (contentW - (tGap * (traits.size() - 1))) / static_cast<float>(traits.size());
        float tH = 18.0f * uiScale;

        for (size_t t = 0; t < traits.size(); ++t)
        {
            SDL_FRect tBox = { contentX + (t * (tW + tGap)), cardCurY, tW, tH };
            UIWidget::drawPanel(renderer, tBox, Theme::colors.bgDark, Theme::colors.borderButton);
            float txtW = UIWidget::getTextWidth(traits[t].first, uiScale * 0.68f);
            UIWidget::drawText(renderer, traits[t].first, tBox.x + ((tW - txtW) / 2.0f), tBox.y + (2.0f * uiScale), traits[t].second, uiScale * 0.68f);
        }

        curY += cardH + (10.0f * uiScale);
        return (curY - startY);
    }
}
