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
#include "state/combatState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"

namespace CharacterCardWidget
{
    TooltipData createAnatomyTooltip(const entity* ent, const entity* observer)
    {
        TooltipData data;
        if (!ent) return data;

        data.title = ent->name.empty() ? "Character" : ent->name;
        std::string race = ent->anatomy.getRacialTitle().empty() ? "Human" : ent->anatomy.getRacialTitle();
        bool isSelf = (observer != nullptr && ent == observer) || (ent->id == "player" || ent->id == "hero");

        if (isSelf || ent->isSlotRevealed(bodySlot::GROIN))
        {
            std::string archStr = genderArchetypeToString(ent->genderArchetype);
            data.subtitle = std::format("Level {} • {} ({})", ent->stats.level, race, archStr);
        }
        else
        {
            GenderArchetype perceived = ent->getPerceivedGender(observer);
            std::string archStr = genderArchetypeToString(perceived);
            data.subtitle = std::format("Level {} • {} (Appears {})", ent->stats.level, race, archStr);
        }
        data.description = "Physical Anatomy & Body Overview";

        // 1. Core Physical Attributes (Important info not shown in main card!)
        if (isSelf || ent->isSlotRevealed(bodySlot::TORSO) || ent->isSlotRevealed(bodySlot::HEAD))
        {
            data.stats.emplace_back("Height", std::format("{:.2f}m", ent->anatomy.heightMeters), false);
            std::string buildStr = (ent->anatomy.bodySize.empty() ? "Average" : ent->anatomy.bodySize) +
                                   ", " +
                                   (ent->anatomy.muscleTone.empty() ? "Toned" : ent->anatomy.muscleTone);
            data.stats.emplace_back("Build", buildStr, false);
        }
        else
        {
            std::string estHeight;
            if (ent->anatomy.heightMeters >= 1.95f) estHeight = "Towering (~2.0m)";
            else if (ent->anatomy.heightMeters >= 1.83f) estHeight = "Tall (~1.85m)";
            else if (ent->anatomy.heightMeters >= 1.70f) estHeight = "Average (~1.75m)";
            else if (ent->anatomy.heightMeters >= 1.55f) estHeight = "Petite (~1.60m)";
            else estHeight = "Short (~1.50m)";
            data.stats.emplace_back("Height", estHeight + " (Estimated)", true);

            std::string estBuild = (ent->anatomy.bodySize.empty() ? "Average" : ent->anatomy.bodySize) +
                                   " Silhouette (Clothed)";
            data.stats.emplace_back("Build", estBuild, true);
        }

        if (!isSelf)
        {
            std::string affStatus = "Neutral";
            if (ent->affection >= 50.0f) affStatus = "Devoted";
            else if (ent->affection >= 20.0f) affStatus = "Friendly";
            else if (ent->affection <= -50.0f) affStatus = "Nemesis";
            else if (ent->affection <= -20.0f) affStatus = "Hostile";
            data.stats.emplace_back("Affection", std::format("{:+.0f} [{}]", ent->affection, affStatus), false);
        }

        std::string corrStatus = "Pure";
        if (ent->corruption >= 75.0f) corrStatus = "Depraved";
        else if (ent->corruption >= 50.0f) corrStatus = "Corrupted";
        else if (ent->corruption >= 25.0f) corrStatus = "Tainted";
        data.stats.emplace_back("Corruption", std::format("{:.0f}% [{}]", ent->corruption, corrStatus), false);

        struct SlotMeta {
            bodySlot slot;
            std::string label;
        };
        static const std::vector<SlotMeta> slotsToCheck = {
            { bodySlot::HEAD, "Face" },
            { bodySlot::HAIR, "Hair" },
            { bodySlot::EYES, "Eyes" },
            { bodySlot::EARS, "Ears" },
            { bodySlot::TORSO, "Torso" },
            { bodySlot::BREASTS, "Breasts" },
            { bodySlot::GROIN, "Groin" },
            { bodySlot::ASS, "Rear" },
            { bodySlot::ARMS, "Arms" },
            { bodySlot::LEGS, "Legs" },
            { bodySlot::FEET, "Feet" },
            { bodySlot::HORNS, "Horns" },
            { bodySlot::WINGS, "Wings" },
            { bodySlot::TAIL, "Tail" }
        };

        for (const auto& sm : slotsToCheck)
        {
            bool hasPart = ent->anatomy.hasPart(sm.slot);
            bool exposedNow = ent->inventory.isSlotExposed(sm.slot);
            if (exposedNow && !isSelf)
            {
                const_cast<entity*>(ent)->revealSlot(sm.slot);
            }
            bool known = isSelf || exposedNow || ent->revealedBodySlots.contains(bodySlotToString(sm.slot));

            // If not known (covered by clothing and never seen previously)
            // Use the "best guess" observation mechanic based on visible silhouette
            if (!known)
            {
                std::string bestGuess = "Concealed (Clothed)";
                if (sm.slot == bodySlot::BREASTS)
                {
                    const bodyPart* b = ent->anatomy.getPart(bodySlot::BREASTS);
                    int cup = b ? b->cupSize : 0;
                    if (cup == 0) bestGuess = "Appears Flat (Clothed)";
                    else if (cup == 1) bestGuess = "Est. Small / A-Cup (Clothed)";
                    else if (cup == 2) bestGuess = "Est. ~B-Cup (Clothed)";
                    else if (cup == 3) bestGuess = "Est. ~C-Cup (Clothed)";
                    else if (cup == 4) bestGuess = "Est. ~D-Cup (Clothed)";
                    else if (cup == 5) bestGuess = "Est. ~DD-Cup (Clothed)";
                    else bestGuess = "Est. Voluptuous / Heavy (Clothed)";
                }
                else if (sm.slot == bodySlot::GROIN)
                {
                    const bodyPart* g = ent->anatomy.getPart(bodySlot::GROIN);
                    if (g && g->length >= 20.0f) bestGuess = "Prominent Bulge (Est. Very Large)";
                    else if (g && g->length >= 14.0f) bestGuess = "Noticeable Bulge (Estimated)";
                    else if (g && g->length >= 8.0f) bestGuess = "Slight Bulge (Estimated)";
                    else if (g && g->orifice.exists) bestGuess = "Flat / Smooth Contour (Clothed)";
                    else bestGuess = "Concealed Contour (Clothed)";
                }
                else if (sm.slot == bodySlot::TORSO)
                {
                    bestGuess = "Attire: " + (ent->anatomy.bodySize.empty() ? "Average" : ent->anatomy.bodySize) + " Frame";
                }
                else if (sm.slot == bodySlot::ASS)
                {
                    const bodyPart* a = ent->anatomy.getPart(bodySlot::ASS);
                    bestGuess = "Clothed Contour (" + (a && !a->style.empty() ? a->style : "Average") + ")";
                }
                else if (sm.slot == bodySlot::ARMS || sm.slot == bodySlot::LEGS)
                {
                    bestGuess = "Covered (Est. " + (ent->anatomy.muscleTone.empty() ? "Toned" : ent->anatomy.muscleTone) + ")";
                }
                else if (sm.slot == bodySlot::HAIR)
                {
                    const bodyPart* h = ent->anatomy.getPart(bodySlot::HAIR);
                    bestGuess = h ? (h->style.empty() ? "Concealed under headwear" : h->style + " (Concealed)") : "None";
                }
                else if (sm.slot == bodySlot::HORNS || sm.slot == bodySlot::WINGS || sm.slot == bodySlot::TAIL)
                {
                    bestGuess = hasPart ? "Concealed under attire" : "None";
                }
                else if (sm.slot == bodySlot::FEET)
                {
                    bestGuess = "Covered by Footwear";
                }
                else if (sm.slot == bodySlot::HEAD || sm.slot == bodySlot::EYES || sm.slot == bodySlot::EARS)
                {
                    bestGuess = "Obscured / Veiled";
                }

                data.stats.emplace_back(sm.label, bestGuess, true);
                continue;
            }

            // If known, but character doesn't have this body part
            if (!hasPart)
            {
                data.stats.emplace_back(sm.label, "None", true);
                continue;
            }

            // The part is known and present!
            const bodyPart* p = ent->anatomy.getPart(sm.slot);
            if (!p)
            {
                data.stats.emplace_back(sm.label, "None", true);
                continue;
            }

            std::string color = p->primaryColor.empty() ? "Natural" : p->primaryColor;
            std::string pRace = p->race.empty() ? race : p->race;
            std::string detail;

            if (sm.slot == bodySlot::HAIR)
            {
                detail = p->style.empty() ? "Medium" : p->style;
            }
            else if (sm.slot == bodySlot::BREASTS)
            {
                detail = std::format("{}-Cup", bodyPart::getCupSizeName(p->cupSize));
                if (p->isLactating)
                {
                    detail += ", Lactating";
                    if (isSelf && p->currentFluidMl > 0.0f)
                    {
                        detail += std::format(" ({:.0f}ml)", p->currentFluidMl);
                    }
                }
            }
            else if (sm.slot == bodySlot::GROIN)
            {
                if (p->length > 0.0f)
                {
                    detail = std::format("{:.0f}cm {}", p->length, p->name);
                    if (isSelf && p->diameter > 0.0f)
                    {
                        detail += std::format(" ({:.1f}cm dia)", p->diameter);
                    }
                }
                else if (p->orifice.exists)
                {
                    detail = std::format("{}, Wetness: {}/5", p->name.empty() ? "Vagina" : p->name, p->orifice.wetnessLevel);
                }
                else
                {
                    detail = p->name.empty() ? "Genitals" : p->name;
                }
            }
            else if (sm.slot == bodySlot::TORSO)
            {
                detail = ent->anatomy.bodySize.empty() ? "Average" : ent->anatomy.bodySize;
            }
            else if (sm.slot == bodySlot::ASS)
            {
                detail = p->style.empty() ? "Average" : p->style;
            }
            else if (sm.slot == bodySlot::ARMS || sm.slot == bodySlot::LEGS)
            {
                detail = ent->anatomy.muscleTone.empty() ? "Toned" : ent->anatomy.muscleTone;
            }
            else if (sm.slot == bodySlot::HORNS || sm.slot == bodySlot::WINGS || sm.slot == bodySlot::TAIL)
            {
                detail = p->length > 0.0f ? std::format("{:.0f}cm", p->length) : "Present";
            }

            std::string summary = color + ", " + pRace;
            if (!detail.empty())
            {
                summary += ", " + detail;
            }

            data.stats.emplace_back(sm.label, summary, false);
        }

        // Pregnancy / Womb info (important info not in main card!)
        if (isSelf || ent->isSlotRevealed(bodySlot::GROIN))
        {
            if (ent->gestation.isPregnant)
            {
                int daysProgressed = std::max(1, ent->gestation.totalGestationDays - ent->gestation.gestationDaysRemaining);
                std::string pregStr = std::format("Day {} ({})", daysProgressed, ent->gestation.fatherRace);
                data.stats.emplace_back("Womb", pregStr, false);
            }
            else if (isSelf)
            {
                data.stats.emplace_back("Womb", "Empty", true);
            }
        }

        return data;
    }

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
            bool isSexuality = eff.id.rfind("status_sexuality_", 0) == 0;
            SDL_Color bdCol = isHov ? Theme::colors.borderSelected : (isSexuality ? Theme::colors.lust : (eff.isDebuff ? Theme::colors.health : Theme::colors.companion));

            UIWidget::drawPanel(renderer, chipRect, Theme::colors.bgDark, bdCol);

            int remMins = eff.getRemainingMinutes();
            float fillRatio = std::clamp(static_cast<float>(remMins) / 1440.0f, 0.0f, 1.0f);
            if (fillRatio > 0.0f)
            {
                float borderInset = 1.0f * uiScale;
                float shadeW = (chipSize - (borderInset * 2.0f)) * fillRatio;
                float shadeH = chipSize - (borderInset * 2.0f);
                SDL_FRect shadeRect = { chipX + borderInset, chipY + borderInset, shadeW, shadeH };

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

            UIWidget::drawText(renderer, code, tX + (1.0f * uiScale), tY + (1.0f * uiScale), SDL_Color{ 10, 10, 15, 240 }, textScale);
            UIWidget::drawText(renderer, code, tX, tY, Theme::colors.textPrimary, textScale);

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

            std::string typeLabel = eff.isDebuff ? "Debuff" : (eff.durationMinutes < 0 && eff.durationTurns < 0 ? (isSexuality ? "Sexuality / Orientation" : "Innate Trait") : "Buff");
            std::string sub = std::format("{} • {}", typeLabel, timeStr);
            TooltipManager::setHoverTooltip(chipRect, mousePos, eff.name, eff.description, sub);
        }
    }

    static float renderSingleCompanionCard(SDL_Renderer* renderer, game* gameContext, entity* companion, float padX, float curY, float availableW, float uiScale, const TooltipPoint& mousePos)
    {
        if (!companion) return 0.0f;
        companion->ensureSexualityStatusEffect();

        float startY = curY;
        float headerH = 17.0f * uiScale;
        float cardH = 95.0f * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        std::string compHeader = std::format("COMPANION: {}", companion->name);
        UIWidget::drawHeader(renderer, headerRect, compHeader, Theme::colors.bgHeader, Theme::colors.companion, uiScale * 0.68f);
        curY += headerH + (3.0f * uiScale);

        float innerPad = 4.0f * uiScale;
        float cX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        float avatarSize = 22.0f * uiScale;
        SDL_FRect avatarRect = { cX, curY, avatarSize, avatarSize };
        UIWidget::drawPanel(renderer, avatarRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string initials = companion->name.empty() ? "C" : companion->name.substr(0, 1);
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.68f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (2.0f * uiScale), Theme::colors.companion, uiScale * 0.68f);

        std::string lvlStr = std::format("Lvl {} • {}", companion->stats.level, companion->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, cX + avatarSize + (4.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.66f);

        entity* playerEnt = gameContext ? gameContext->getPlayer() : nullptr;
        TooltipManager::setHoverTooltip(avatarRect, mousePos, createAnatomyTooltip(companion, playerEnt));
        TooltipManager::setHoverTooltip(cardRect, mousePos, companion->name,
                                        "Active party companion fighting alongside you.",
                                        std::format("Level {} • {}", companion->stats.level, companion->anatomy.getRacialTitle()));

        curY += avatarSize + (2.0f * uiScale);

        float barH = 7.0f * uiScale;
        float barGap = 2.0f * uiScale;
        float labelW = 36.0f * uiScale;
        float valW = 42.0f * uiScale;
        float progressW = cW - labelW - valW - (4.0f * uiScale);

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

        float curSta = companion->getStat("stamina");
        float maxSta = companion->getStat("max_stamina");
        if (maxSta <= 0.0f) {
            float physique = companion->getStat("physique");
            float toughness = companion->getStat("toughness");
            maxSta = 80.0f + (physique * 1.5f) + (toughness * 1.0f);
        }
        if (gameContext)
        {
            if (auto* cs = dynamic_cast<CombatState*>(gameContext->getActiveState()))
            {
                for (const auto& p : cs->getEngine().getPlayerParty())
                {
                    if (p.character && p.character.get() == companion)
                    {
                        curSta = p.currentStamina;
                        maxSta = p.maxStamina;
                        break;
                    }
                }
            }
        }
        if (curSta <= 0.0f && (!gameContext || dynamic_cast<CombatState*>(gameContext->getActiveState()) == nullptr))
        {
            curSta = maxSta;
        }
        curSta = std::clamp(curSta, 0.0f, maxSta);

        UIWidget::drawText(renderer, "Stamina", cX, curY, Theme::colors.stamina, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, curSta, maxSta, Theme::colors.stamina, Theme::colors.bgHeader, "", uiScale);
        std::string staStr = std::format("{:.0f}/{:.0f}", curSta, maxSta);
        UIWidget::drawText(renderer, staStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);

        TooltipManager::setHoverTooltip({ cX, curY, cW, barH }, mousePos,
                                        std::format("{}'s Stamina", companion->name),
                                        "Action and endurance reserve consumed by attacks and techniques in battle.",
                                        std::format("{:.0f} / {:.0f} Stamina", curSta, maxSta));

        curY += barH + barGap;

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
        curY += barH + barGap;

        float aff = companion->affection;
        float affBarVal = std::clamp(aff + 100.0f, 0.0f, 200.0f);
        UIWidget::drawText(renderer, "Affect.", cX, curY, Theme::colors.affection, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, affBarVal, 200.0f, Theme::colors.affection, Theme::colors.bgHeader, "", uiScale);
        std::string affStr = std::format("{:+.0f}", aff);
        UIWidget::drawText(renderer, affStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);
        TooltipManager::setHoverTooltip({ cX, curY, cW, barH }, mousePos,
                                        std::format("{}'s Affection", companion->name),
                                        "Companion bond and loyalty towards the protagonist (-100 to +100).",
                                        std::format("{:+.0f} Affection", aff));
        curY += barH + barGap;

        float corr = companion->corruption;
        UIWidget::drawText(renderer, "Corrupt.", cX, curY, Theme::colors.corruption, uiScale * 0.60f);
        UIWidget::drawProgressBar(renderer, { cX + labelW, curY, progressW, barH }, corr, 100.0f, Theme::colors.corruption, Theme::colors.bgHeader, "", uiScale);
        std::string corrStr = std::format("{:.0f}%", corr);
        UIWidget::drawText(renderer, corrStr, cX + labelW + progressW + (4.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.58f);
        TooltipManager::setHoverTooltip({ cX, curY, cW, barH }, mousePos,
                                        std::format("{}'s Corruption", companion->name),
                                        "Spiritual depravity and degradation (0% to 100%).",
                                        std::format("{:.0f}% Corruption", corr));

        return cardH + (5.0f * uiScale);
    }

    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* player = gameContext ? gameContext->getPlayer() : nullptr;
        if (!player) return 0.0f;
        player->ensureSexualityStatusEffect();

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        auto mousePos = gameContext->input.getMousePosition();

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

        float sub1H = 58.0f * uiScale;
        float vitalsTopH = 80.0f * uiScale;;
        float sub2H = vitalsTopH + statusSectionH + (4.0f * uiScale);
        float headerH = 20.0f * uiScale;
        float outerH = headerH + (3.0f * uiScale) + sub1H + (5.0f * uiScale) + sub2H + (5.0f * uiScale);

        SDL_FRect outerRect = { padX, curY, availableW, outerH };
        UIWidget::drawPanel(renderer, outerRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "PLAYER CHARACTER", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.76f);
        curY += headerH + (3.0f * uiScale);

        float sub1X = padX + innerPad;
        float sub1Y = curY;
        SDL_FRect sub1Rect = { sub1X, sub1Y, subW, sub1H };
        UIWidget::drawPanel(renderer, sub1Rect, Theme::colors.bgDark, Theme::colors.borderButton);

        float s1Pad = 5.0f * uiScale;
        float s1ContentX = sub1X + s1Pad;
        float s1ContentW = subW - (s1Pad * 2.0f);
        float s1Y = sub1Y + (4.0f * uiScale);

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

        TooltipManager::setHoverTooltip(avatarRect, mousePos, createAnatomyTooltip(player, player));

        std::string dispName = player->name.empty() ? "Hero" : player->name;
        UIWidget::drawText(renderer, dispName, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.84f);
        std::string lvlStr = std::format("Lvl {} • {}", player->stats.level, player->anatomy.getRacialTitle());
        UIWidget::drawText(renderer, lvlStr, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (15.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

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

        float curSta = player->getStat("stamina");
        float maxSta = player->getStat("max_stamina");
        if (maxSta <= 0.0f) {
            float physique = player->getStat("physique");
            float toughness = player->getStat("toughness");
            maxSta = 80.0f + (physique * 1.5f) + (toughness * 1.0f);
        }
        if (gameContext)
        {
            if (auto* cs = dynamic_cast<CombatState*>(gameContext->getActiveState()))
            {
                for (const auto& p : cs->getEngine().getPlayerParty())
                {
                    if (p.character && p.character.get() == player)
                    {
                        curSta = p.currentStamina;
                        maxSta = p.maxStamina;
                        break;
                    }
                }
            }
        }
        if (curSta <= 0.0f && (!gameContext || dynamic_cast<CombatState*>(gameContext->getActiveState()) == nullptr))
        {
            curSta = maxSta;
        }
        curSta = std::clamp(curSta, 0.0f, maxSta);

        UIWidget::drawText(renderer, "Stamina", s2ContentX, s2Y, Theme::colors.stamina, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curSta, maxSta, Theme::colors.stamina, Theme::colors.bgHeader, "", uiScale);
        std::string staStr = std::format("{:.0f}/{:.0f}", curSta, maxSta);
        UIWidget::drawText(renderer, staStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Stamina (STA)",
                                        "Action and endurance reserve consumed by martial strikes, weapons, and defense maneuvers.",
                                        std::format("{:.0f} / {:.0f} Stamina", curSta, maxSta));
        s2Y += barH + barGap;

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

        float curLust = std::clamp(player->getStat("lust"), 0.0f, 100.0f);
        float maxLust = player->getStat("max_lust");
        if (maxLust <= 0.0f) maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", s2ContentX, s2Y, Theme::colors.lust, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", (curLust / maxLust) * 100.0f);
        UIWidget::drawText(renderer, lustStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Lust & Arousal",
                                        "Sexual desire, physical arousal, and psychological vulnerability. Reaching 100% triggers erotic submission or climax.",
                                        std::format("{:.0f}% Lust / Arousal", (curLust / maxLust) * 100.0f));
        s2Y += barH + (6.0f * uiScale);

        if (!effects.empty())
        {
            renderStatusEffectsGrid(renderer, effects, s2ContentX, s2Y, s2ContentW, chipSize, chipGap, chipsPerRow, uiScale, mousePos);
        }

        curY += sub2H + (5.0f * uiScale);
        curY += (6.0f * uiScale);

        const auto& companions = gameContext->getCompanions();
        for (const auto& comp : companions)
        {
            if (comp)
            {
                curY += renderSingleCompanionCard(renderer, gameContext, comp.get(), padX, curY, availableW, uiScale, mousePos);
            }
        }

        return (curY - startY);
    }
}
