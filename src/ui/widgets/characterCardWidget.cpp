#include "ui/widgets/characterCardWidget.h"

#include <algorithm>
#include <cmath>
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
        float headerH = 17.0f * uiScale;
        float cardH = 68.0f * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Header: COMPANION: Name
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        std::string compHeader = std::format("COMPANION: {}", companion->name);
        UIWidget::drawHeader(renderer, headerRect, compHeader, Theme::colors.bgHeader, Theme::colors.companion, uiScale * 0.68f);
        curY += headerH + (3.0f * uiScale);

        float innerPad = 4.0f * uiScale;
        float cX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        // Avatar Badge
        float avatarSize = 22.0f * uiScale;
        SDL_FRect avatarRect = { cX, curY, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string initials = companion->name.empty() ? "C" : companion->name.substr(0, 1);
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.68f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (2.0f * uiScale), Theme::colors.companion, uiScale * 0.68f);

        // Level & Species
        std::string lvlStr = std::format("Lvl {} • {}", companion->stats.level, companion->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, cX + avatarSize + (4.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.66f);

        curY += avatarSize + (2.0f * uiScale);

        // Compact Health & Lust Vitals
        float barH = 7.0f * uiScale;
        float barGap = 2.0f * uiScale;
        float labelW = 36.0f * uiScale;
        float valW = 42.0f * uiScale;
        float progressW = cW - labelW - valW - (4.0f * uiScale);

        // Health
        float curHp = companion->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = companion->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "Health", cX, curY, Theme::colors.health, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);
        curY += barH + barGap;

        // Lust
        float curLust = companion->getStat("lust");
        UIWidget::drawText(renderer, "Lust", cX, curY, Theme::colors.lust, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curLust, 100.0f, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);

        return cardH + (5.0f * uiScale);
    }

    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* player = gameContext ? gameContext->getPlayer() : nullptr;
        if (!player) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        auto mousePos = gameContext->input.getMousePosition();

        // =========================================================================
        // DYNAMIC STATUS EFFECTS ROWS CALCULATION
        // =========================================================================
        float innerPad = 4.0f * uiScale;
        float subW = availableW - (innerPad * 2.0f);
        float s2Pad = 5.0f * uiScale;
        float s2ContentW = subW - (s2Pad * 2.0f);

        const float chipSize = 22.0f * uiScale;
        const float chipGap = 3.0f * uiScale;
        int chipsPerRow = std::max(1, static_cast<int>(std::floor((s2ContentW + chipGap) / (chipSize + chipGap))));

        const auto& effects = player->statusEffects;
        int numEffectRows = effects.empty() ? 1 : static_cast<int>(std::ceil(effects.size() / static_cast<float>(chipsPerRow)));
        float statusSectionH = (numEffectRows * chipSize) + ((numEffectRows - 1) * chipGap);

        // Calculate heights dynamically with generous vertical headroom
        float sub1H = 58.0f * uiScale;
        float vitalsTopH = 80.0f * uiScale; // "VITALS & STATUS" header + 4 progress bars
        float sub2H = vitalsTopH + statusSectionH + (8.0f * uiScale);
        float headerH = 20.0f * uiScale;
        float outerH = headerH + (3.0f * uiScale) + sub1H + (5.0f * uiScale) + sub2H + (5.0f * uiScale);

        // =========================================================================
        // OVERARCHING CONTAINER: Player Overview Card
        // =========================================================================
        SDL_FRect outerRect = { padX, curY, availableW, outerH };
        UIWidget::drawPanel(renderer, outerRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Overarching Header
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "PLAYER CHARACTER", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.76f);
        curY += headerH + (3.0f * uiScale);

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
        float avatarSize = 30.0f * uiScale;
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
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.78f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

        // Name & Level / Species
        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.84f);
        std::string lvlStr = std::format("Lvl {} • {}", player->stats.level, player->anatomy.getRacialTitle());
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

        float s2ContentX = sub2X + s2Pad;
        float s2Y = sub2Y + (4.0f * uiScale);

        UIWidget::drawText(renderer, "VITALS & STATUS", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.72f);
        s2Y += (16.0f * uiScale);

        float barH = 10.0f * uiScale;
        float barGap = 4.0f * uiScale;
        float labelW = 40.0f * uiScale;
        float valW = 44.0f * uiScale;
        float progressW = s2ContentW - labelW - valW - (4.0f * uiScale);

        // 1. Health Bar
        float curHp = player->getStat("health");
        if (curHp <= 0.0f) curHp = 100.0f;
        float maxHp = player->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 100.0f;

        UIWidget::drawText(renderer, "Health", s2ContentX, s2Y, Theme::colors.health, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        s2Y += barH + barGap;

        // 2. Mana Bar
        float curMp = inPrologue ? 0.0f : player->getStat("mana");
        if (curMp <= 0.0f && !inPrologue) curMp = 80.0f;
        float maxMp = inPrologue ? 0.0f : player->getStat("max_mana");
        if (maxMp <= 0.0f && !inPrologue) maxMp = 80.0f;

        UIWidget::drawText(renderer, "Mana", s2ContentX, s2Y, Theme::colors.mana, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgHeader, "", uiScale);
        std::string mpStr = inPrologue ? "0/0" : std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        s2Y += barH + barGap;

        // 3. Lust Bar
        float curLust = player->getStat("lust");
        float maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", s2ContentX, s2Y, Theme::colors.lust, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", curLust);
        UIWidget::drawText(renderer, lustStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        s2Y += barH + barGap;

        // 4. Arousal Bar
        float curArousal = player->getStat("arousal");
        float maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgHeader, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", curArousal);
        UIWidget::drawText(renderer, arousalStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        s2Y += barH + (6.0f * uiScale);

        // -------------------------------------------------------------------------
        // DYNAMIC STATUS EFFECT SQUARE CHIP ROWS
        // -------------------------------------------------------------------------
        std::string hoveredTooltip = "";

        if (effects.empty())
        {
            // Baseline status chips when no active effects
            static const std::vector<std::pair<std::string, SDL_Color>> defaultChips = {
                { "Phys", Theme::colors.health },
                { "Arc", Theme::colors.mana },
                { "Form", Theme::colors.companion },
                { "Buff", Theme::colors.textGold }
            };

            float dGap = 3.0f * uiScale;
            float dW = (s2ContentW - (dGap * (defaultChips.size() - 1))) / static_cast<float>(defaultChips.size());
            float dH = chipSize;

            for (size_t t = 0; t < defaultChips.size(); ++t)
            {
                SDL_FRect tBox = { s2ContentX + (t * (dW + dGap)), s2Y, dW, dH };
                UIWidget::drawPanel(renderer, tBox, Theme::colors.bgHeader, Theme::colors.borderButton);
                float txtW = UIWidget::getTextWidth(defaultChips[t].first, uiScale * 0.62f);
                UIWidget::drawText(renderer, defaultChips[t].first, tBox.x + ((dW - txtW) / 2.0f), tBox.y + (3.0f * uiScale), defaultChips[t].second, uiScale * 0.62f);
            }
        }
        else
        {
            // Render square status effect icons in 1, 2, 3+ dynamic rows
            for (size_t i = 0; i < effects.size(); ++i)
            {
                int row = static_cast<int>(i / chipsPerRow);
                int col = static_cast<int>(i % chipsPerRow);

                float chipX = s2ContentX + (col * (chipSize + chipGap));
                float chipY = s2Y + (row * (chipSize + chipGap));
                SDL_FRect chipRect = { chipX, chipY, chipSize, chipSize };

                bool isHov = (mousePos.x >= chipRect.x && mousePos.x <= chipRect.x + chipRect.w &&
                              mousePos.y >= chipRect.y && mousePos.y <= chipRect.y + chipRect.h);

                const auto& eff = effects[i];
                SDL_Color fillCol = isHov ? Theme::colors.bgHeader : Theme::colors.bgHeader;
                SDL_Color bdCol = eff.isDebuff ? Theme::colors.health : Theme::colors.companion;
                SDL_Color textCol = eff.isDebuff ? Theme::colors.health : Theme::colors.textGold;

                UIWidget::drawPanel(renderer, chipRect, fillCol, bdCol);

                std::string code = eff.name.substr(0, std::min<size_t>(2, eff.name.length()));
                float cW = UIWidget::getTextWidth(code, uiScale * 0.58f);
                UIWidget::drawText(renderer, code, chipX + ((chipSize - cW) / 2.0f), chipY + (3.0f * uiScale), textCol, uiScale * 0.58f);

                if (isHov)
                {
                    hoveredTooltip = std::format("{}: {}", eff.name, eff.description);
                }
            }
        }

        curY += sub2H + (5.0f * uiScale);
        curY += (6.0f * uiScale);

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
