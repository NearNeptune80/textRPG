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
    static float renderSingleCompanionCard(SDL_Renderer* renderer, entity* companion, float padX, float curY, float availableW, float uiScale)
    {
        if (!companion) return 0.0f;

        float startY = curY;
        float headerH = 20.0f * uiScale;
        float cardH = 82.0f * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Header: COMPANION: Name
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        std::string compHeader = std::format("COMPANION: {}", companion->name);
        UIWidget::drawHeader(renderer, headerRect, compHeader, Theme::colors.bgHeader, Theme::colors.companion, uiScale * 0.72f);
        curY += headerH + (4.0f * uiScale);

        float innerPad = 6.0f * uiScale;
        float cX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        // Avatar Badge
        float avatarSize = 26.0f * uiScale;
        SDL_FRect avatarRect = { cX, curY, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string initials = companion->name.empty() ? "C" : companion->name.substr(0, 1);
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.75f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (4.0f * uiScale), Theme::colors.companion, uiScale * 0.75f);

        // Level & Species
        std::string lvlStr = std::format("Lvl {} • {}", companion->stats.level, companion->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, cX + avatarSize + (6.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

        curY += avatarSize + (4.0f * uiScale);

        // Compact Health & Lust Vitals
        float barH = 9.0f * uiScale;
        float barGap = 3.0f * uiScale;
        float labelW = 40.0f * uiScale;
        float valW = 48.0f * uiScale;
        float progressW = cW - labelW - valW - (4.0f * uiScale);

        // Health
        float curHp = companion->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = companion->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "Health", cX, curY, Theme::colors.health, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.64f);
        curY += barH + barGap;

        // Lust
        float curLust = companion->getStat("lust");
        UIWidget::drawText(renderer, "Lust", cX, curY, Theme::colors.lust, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curLust, 100.0f, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.64f);

        return cardH + (6.0f * uiScale);
    }

    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* player = gameContext ? gameContext->getPlayer() : nullptr;
        if (!player) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);

        // =========================================================================
        // OVERARCHING CONTAINER: Player Overview Card
        // =========================================================================
        float innerPad = 6.0f * uiScale;
        float subW = availableW - (innerPad * 2.0f);

        float sub1H = 54.0f * uiScale;
        float sub2H = 120.0f * uiScale;
        float headerH = 20.0f * uiScale;
        float outerH = headerH + (4.0f * uiScale) + sub1H + (5.0f * uiScale) + sub2H + (5.0f * uiScale);

        SDL_FRect outerRect = { padX, curY, availableW, outerH };
        UIWidget::drawPanel(renderer, outerRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Overarching Header
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "PLAYER CHARACTER", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.74f);
        curY += headerH + (4.0f * uiScale);

        // -------------------------------------------------------------------------
        // SUB-BOX 1: Identity & Wealth Box
        // -------------------------------------------------------------------------
        float sub1X = padX + innerPad;
        float sub1Y = curY;
        SDL_FRect sub1Rect = { sub1X, sub1Y, subW, sub1H };
        UIWidget::drawPanel(renderer, sub1Rect, Theme::colors.bgDark, Theme::colors.borderButton);

        float s1Pad = 5.0f * uiScale;
        float s1ContentX = sub1X + s1Pad;
        float s1ContentW = subW - (s1Pad * 2.0f);
        float s1Y = sub1Y + (4.0f * uiScale);

        // Avatar Badge
        float avatarSize = 28.0f * uiScale;
        SDL_FRect avatarRect = { s1ContentX, s1Y, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgHeader, Theme::colors.borderButton);

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
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.80f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.80f);

        // Name & Level / Species
        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        std::string lvlStr = std::format("Level {} • {}", player->stats.level, player->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (15.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

        // Gold & Essence
        float goldVal = player->getStat("currency");
        if (goldVal <= 0.0f && !inPrologue) goldVal = 5000.0f;
        std::string goldText = std::format("Gold: {:.0f} ¤", goldVal);
        UIWidget::drawText(renderer, goldText, s1ContentX, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.currency, uiScale * 0.72f);

        std::string essenceText = "Essence: 0";
        float essW = UIWidget::getTextWidth(essenceText, uiScale * 0.72f);
        UIWidget::drawText(renderer, essenceText, s1ContentX + s1ContentW - essW, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.arcane, uiScale * 0.72f);

        curY += sub1H + (5.0f * uiScale);

        // -------------------------------------------------------------------------
        // SUB-BOX 2: Vitals & Status Gauges Box
        // -------------------------------------------------------------------------
        float sub2X = padX + innerPad;
        float sub2Y = curY;
        SDL_FRect sub2Rect = { sub2X, sub2Y, subW, sub2H };
        UIWidget::drawPanel(renderer, sub2Rect, Theme::colors.bgDark, Theme::colors.borderButton);

        float s2Pad = 5.0f * uiScale;
        float s2ContentX = sub2X + s2Pad;
        float s2ContentW = subW - (s2Pad * 2.0f);
        float s2Y = sub2Y + (4.0f * uiScale);

        UIWidget::drawText(renderer, "VITALS & STATUS", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.72f);
        s2Y += (15.0f * uiScale);

        float barH = 10.0f * uiScale;
        float barGap = 3.0f * uiScale;
        float labelW = 44.0f * uiScale;
        float valW = 50.0f * uiScale;
        float progressW = s2ContentW - labelW - valW - (4.0f * uiScale);

        // 1. Health Bar
        float curHp = player->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = player->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "Health", s2ContentX, s2Y, Theme::colors.health, uiScale * 0.68f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.66f);
        s2Y += barH + barGap;

        // 2. Mana Bar
        float curMp = inPrologue ? 0.0f : player->getStat("mana");
        if (curMp <= 0.0f && !inPrologue) curMp = 80.0f;
        float maxMp = inPrologue ? 0.0f : player->getStat("max_mana");
        if (maxMp <= 0.0f && !inPrologue) maxMp = 80.0f;

        UIWidget::drawText(renderer, "Mana", s2ContentX, s2Y, Theme::colors.mana, uiScale * 0.68f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgHeader, "", uiScale);
        std::string mpStr = inPrologue ? "0/0" : std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.66f);
        s2Y += barH + barGap;

        // 3. Lust Bar
        float curLust = player->getStat("lust");
        float maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", s2ContentX, s2Y, Theme::colors.lust, uiScale * 0.68f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.66f);
        s2Y += barH + barGap;

        // 4. Arousal Bar
        float curArousal = player->getStat("arousal");
        float maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.68f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgHeader, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", curArousal);
        UIWidget::drawText(renderer, arousalStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.66f);
        s2Y += barH + (5.0f * uiScale);

        // Status trait chips
        static const std::vector<std::pair<std::string, SDL_Color>> traits = {
            { "Phys", Theme::colors.health },
            { "Arc", Theme::colors.mana },
            { "Form", Theme::colors.companion },
            { "Buff", Theme::colors.textGold }
        };
        float tGap = 3.0f * uiScale;
        float tW = (s2ContentW - (tGap * (traits.size() - 1))) / static_cast<float>(traits.size());
        float tH = 16.0f * uiScale;

        for (size_t t = 0; t < traits.size(); ++t)
        {
            SDL_FRect tBox = { s2ContentX + (t * (tW + tGap)), s2Y, tW, tH };
            UIWidget::drawPanel(renderer, tBox, Theme::colors.bgHeader, Theme::colors.borderButton);
            float txtW = UIWidget::getTextWidth(traits[t].first, uiScale * 0.62f);
            UIWidget::drawText(renderer, traits[t].first, tBox.x + ((tW - txtW) / 2.0f), tBox.y + (1.0f * uiScale), traits[t].second, uiScale * 0.62f);
        }

        curY += sub2H + (5.0f * uiScale);
        curY += (8.0f * uiScale);

        // =========================================================================
        // COMPANION CARDS (If any active party companions)
        // =========================================================================
        const auto& companions = gameContext->getCompanions();
        for (const auto& comp : companions)
        {
            if (comp)
            {
                curY += renderSingleCompanionCard(renderer, comp.get(), padX, curY, availableW, uiScale);
            }
        }

        return (curY - startY);
    }
}
