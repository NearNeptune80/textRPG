#include "ui/widgets/characterCardWidget.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "entities/statusEffect.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"

namespace CharacterCardWidget
{
    void renderStatusEffectsGrid(SDL_Renderer* renderer, const std::vector<StatusEffect>& effects, float s2ContentX, float s2Y, float s2ContentW, float chipSize, float chipGap, int chipsPerRow, float uiScale, const TooltipPoint& mousePos)
    {
        if (effects.empty()) return;

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
            SDL_Color bdCol = isHov ? Theme::colors.borderSelected : (eff.isDebuff ? Theme::colors.health : Theme::colors.companion);

            // 1. Base dark background panel
            UIWidget::drawPanel(renderer, chipRect, Theme::colors.bgDark, bdCol);

            // 2. Interior horizontal shading (0hrs on left to 24hrs on right)
            int remMins = eff.getRemainingMinutes();
            float fillRatio = std::clamp(static_cast<float>(remMins) / 1440.0f, 0.0f, 1.0f);
            if (fillRatio > 0.0f)
            {
                float borderInset = 1.0f * uiScale;
                float shadeW = (chipSize - (borderInset * 2.0f)) * fillRatio;
                float shadeH = chipSize - (borderInset * 2.0f);
                SDL_FRect shadeRect = { chipX + borderInset, chipY + borderInset, shadeW, shadeH };

                // Color gradient transitioning from rich green (24h) to bright red (0h)
                uint8_t r, g, b;
                if (fillRatio >= 0.5f)
                {
                    float t = (fillRatio - 0.5f) / 0.5f;
                    r = static_cast<uint8_t>(std::lerp(230.0f, 45.0f, t));
                    g = static_cast<uint8_t>(std::lerp(190.0f, 190.0f, t));
                    b = static_cast<uint8_t>(std::lerp(40.0f, 75.0f, t));
                }
                else
                {
                    float t = fillRatio / 0.5f;
                    r = static_cast<uint8_t>(std::lerp(230.0f, 230.0f, t));
                    g = static_cast<uint8_t>(std::lerp(50.0f, 190.0f, t));
                    b = static_cast<uint8_t>(std::lerp(50.0f, 40.0f, t));
                }
                uint8_t a = 180;

                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, r, g, b, a);
                SDL_RenderFillRect(renderer, &shadeRect);
            }

            // 3. Centered icon / glyph abbreviation
            std::string code;
            if (!eff.iconId.empty() && eff.iconId.length() <= 3)
            {
                code = eff.iconId;
            }
            else if (eff.id == "buff_str") code = "STR";
            else if (eff.id == "buff_arc") code = "ARC";
            else if (eff.id == "debuff_pois") code = "POI";
            else if (eff.id == "buff_haste") code = "HST";
            else if (eff.id == "buff_shield") code = "SHD";
            else if (eff.id == "debuff_lust") code = "LST";
            else if (eff.id == "buff_regen") code = "REG";
            else
            {
                code = eff.name.substr(0, std::min<size_t>(3, eff.name.length()));
            }
            for (auto& c : code) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            float textScale = uiScale * 0.54f;
            float cW = UIWidget::getTextWidth(code, textScale);
            float textH = UIWidget::getLineHeight(textScale);
            float tX = chipX + ((chipSize - cW) / 2.0f);
            float tY = chipY + ((chipSize - textH) / 2.0f) - (1.0f * uiScale);

            // Drop shadow for legibility over shaded background
            UIWidget::drawText(renderer, code, tX + (1.0f * uiScale), tY + (1.0f * uiScale), SDL_Color{ 10, 10, 15, 240 }, textScale);
            UIWidget::drawText(renderer, code, tX, tY, Theme::colors.textPrimary, textScale);

            // 4. Hover Tooltip
            std::string timeStr;
            if (eff.durationMinutes > 0)
            {
                int h = eff.durationMinutes / 60;
                int m = eff.durationMinutes % 60;
                if (h > 0 && m > 0) timeStr = std::format("{}h {}m remaining", h, m);
                else if (h > 0) timeStr = std::format("{}h remaining", h);
                else timeStr = std::format("{}m remaining", m);
            }
            else if (eff.durationTurns > 0)
            {
                timeStr = std::format("{} turns remaining", eff.durationTurns);
            }
            else
            {
                timeStr = "Permanent";
            }

            std::string sub = std::format("{} • {}", eff.isDebuff ? "Debuff" : "Buff", timeStr);
            TooltipManager::setHoverTooltip(chipRect, mousePos, eff.name, eff.description, sub);
        }
    }

    static float renderSingleCompanionCard(SDL_Renderer* renderer, entity* companion, float padX, float curY, float availableW, float uiScale, const TooltipPoint& mousePos)
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

        TooltipManager::setHoverTooltip(cardRect, mousePos, companion->name,
                                        "Active party companion fighting alongside you.",
                                        std::format("Level {} • {}", companion->stats.level, companion->anatomy.getRacialTitle()));

        curY += avatarSize + (2.0f * uiScale);

        // Compact Health & Lust Vitals
        float barH = 7.0f * uiScale;
        float barGap = 2.0f * uiScale;
        float labelW = 36.0f * uiScale;
        float valW = 42.0f * uiScale;
        float progressW = cW - labelW - valW - (4.0f * uiScale);

        // Health
        float curHp = companion->getStat("health");
        float maxHp = companion->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = std::max(curHp, 100.0f);
        curHp = std::clamp(curHp, 0.0f, maxHp);

        UIWidget::drawText(renderer, "Health", cX, curY, Theme::colors.health, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);

        TooltipManager::setHoverTooltip({ cX, curY, cW, barH }, mousePos,
                                        std::format("{}'s Health", companion->name),
                                        "Companion life force. If health drops to 0, they will be knocked unconscious.",
                                        std::format("{:.0f} / {:.0f} HP", curHp, maxHp));

        curY += barH + barGap;

        // Lust
        float curLust = std::clamp(companion->getStat("lust"), 0.0f, 100.0f);
        float maxLust = companion->getStat("max_lust");
        if (maxLust <= 0.0f) maxLust = 100.0f;
        UIWidget::drawText(renderer, "Lust", cX, curY, Theme::colors.lust, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", (curLust / maxLust) * 100.0f);
        UIWidget::drawText(renderer, lustStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);

        TooltipManager::setHoverTooltip({ cX, curY, cW, barH }, mousePos,
                                        std::format("{}'s Lust", companion->name),
                                        "Companion sexual desire and susceptibility to lust-based seduction.",
                                        std::format("{:.0f}% Lust", (curLust / maxLust) * 100.0f));

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

        const float chipSize = 25.0f * uiScale;
        const float chipGap = 4.0f * uiScale;
        int chipsPerRow = std::max(1, static_cast<int>(std::floor((s2ContentW + chipGap) / (chipSize + chipGap))));

        const auto& effects = player->statusEffects;
        int numEffectRows = effects.empty() ? 0 : static_cast<int>(std::ceil(effects.size() / static_cast<float>(chipsPerRow)));
        float statusSectionH = (numEffectRows == 0) ? 0.0f : ((numEffectRows * chipSize) + ((numEffectRows - 1) * chipGap) + (6.0f * uiScale));

        // Calculate heights dynamically with generous vertical headroom
        float sub1H = 58.0f * uiScale;
        float vitalsTopH = 80.0f * uiScale; // "VITALS & STATUS" header + 4 progress bars
        float sub2H = vitalsTopH + statusSectionH + (4.0f * uiScale);
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

        TooltipManager::setHoverTooltip(avatarRect, mousePos, player->name,
                                        "Player protagonist exploring the realm.",
                                        std::format("Level {} • {}", player->stats.level, player->anatomy.getRacialTitle()));

        // Name & Level / Species
        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.84f);
        std::string lvlStr = std::format("Lvl {} • {}", player->stats.level, player->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (15.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

        // Gold & Essence
        float goldVal = player->getStat("currency");
        std::string goldText = std::format("Gold: {:.0f} ¤", goldVal);
        UIWidget::drawText(renderer, goldText, s1ContentX, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.currency, uiScale * 0.72f);
        TooltipManager::setHoverTooltip({ s1ContentX, s1Y + avatarSize + (2.0f * uiScale), 70.0f * uiScale, 16.0f * uiScale },
                                        mousePos, "Gold (Currency)",
                                        "Universal coins used to trade with merchants, purchase equipment, and pay for town services.",
                                        std::format("{:.0f} ¤ Available", goldVal));

        float essenceVal = player->getStat("arcaneEssence");
        if (essenceVal <= 0.0f) essenceVal = player->getStat("essence");
        std::string essenceText = std::format("Essence: {:.0f}", essenceVal);
        float essW = UIWidget::getTextWidth(essenceText, uiScale * 0.72f);
        UIWidget::drawText(renderer, essenceText, s1ContentX + s1ContentW - essW, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.arcane, uiScale * 0.72f);
        TooltipManager::setHoverTooltip({ s1ContentX + s1ContentW - essW - (4.0f * uiScale), s1Y + avatarSize + (2.0f * uiScale), essW + (8.0f * uiScale), 16.0f * uiScale },
                                        mousePos, "Arcane Essence",
                                        "Purified demonic essence harvested from transformations and rituals, used to unlock body mutations.",
                                        std::format("{:.0f} Essence Available", essenceVal));

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
        float maxHp = player->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = std::max(curHp, 100.0f);
        curHp = std::clamp(curHp, 0.0f, maxHp);

        UIWidget::drawText(renderer, "Health", s2ContentX, s2Y, Theme::colors.health, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Health Points (HP)",
                                        "Physical endurance and vitality. Reaching 0 results in collapse, defeat, or capture.",
                                        std::format("{:.0f} / {:.0f} HP", curHp, maxHp));
        s2Y += barH + barGap;

        // 2. Mana Bar
        float curMp = player->getStat("mana");
        float maxMp = player->getStat("max_mana");
        if (maxMp <= 0.0f) maxMp = std::max(curMp, 100.0f);
        curMp = std::clamp(curMp, 0.0f, maxMp);

        UIWidget::drawText(renderer, "Mana", s2ContentX, s2Y, Theme::colors.mana, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgHeader, "", uiScale);
        std::string mpStr = std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Arcane Mana (MP)",
                                        "Magical reserves consumed by spells, abilities, enchantments, and transformations.",
                                        std::format("{:.0f} / {:.0f} MP", curMp, maxMp));
        s2Y += barH + barGap;

        // 3. Lust Bar
        float curLust = std::clamp(player->getStat("lust"), 0.0f, 100.0f);
        float maxLust = player->getStat("max_lust");
        if (maxLust <= 0.0f) maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", s2ContentX, s2Y, Theme::colors.lust, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", (curLust / maxLust) * 100.0f);
        UIWidget::drawText(renderer, lustStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Lust Level",
                                        "Sexual desire and psychological vulnerability. High lust weakens resistance against seductive influences.",
                                        std::format("{:.0f}% Lust", (curLust / maxLust) * 100.0f));
        s2Y += barH + barGap;

        // 4. Arousal Bar
        float curArousal = std::clamp(player->getStat("arousal"), 0.0f, 100.0f);
        float maxArousal = player->getStat("max_arousal");
        if (maxArousal <= 0.0f) maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgHeader, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", (curArousal / maxArousal) * 100.0f);
        UIWidget::drawText(renderer, arousalStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Physical Arousal",
                                        "Active physiological excitement during intimate encounters. Reaching 100% triggers orgasmic climax.",
                                        std::format("{:.0f}% Arousal", (curArousal / maxArousal) * 100.0f));
        s2Y += barH + (6.0f * uiScale);

        // -------------------------------------------------------------------------
        // DYNAMIC STATUS EFFECT SQUARE CHIP ROWS
        // -------------------------------------------------------------------------
        if (!effects.empty())
        {
            renderStatusEffectsGrid(renderer, effects, s2ContentX, s2Y, s2ContentW, chipSize, chipGap, chipsPerRow, uiScale, mousePos);
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
                curY += renderSingleCompanionCard(renderer, comp.get(), padX, curY, availableW, uiScale, mousePos);
            }
        }

        return (curY - startY);
    }
}
