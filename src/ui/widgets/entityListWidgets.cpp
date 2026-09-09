#include "ui/widgets/entityListWidgets.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>

#include "core/game.h"
#include "entities/entity.h"
#include "entities/namedCharacter.h"
#include "map/gameMap.h"
#include "state/characterCreationState.h"
#include "state/combatState.h"
#include "state/eventState.h"
#include "common/enums.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"

namespace EntityListWidgets
{
    bool isInteractingWithNPC(game* gameContext)
    {
        if (!gameContext) return false;

        // 1. In CombatState with enemy party
        if (auto* cs = dynamic_cast<CombatState*>(gameContext->getActiveState()))
        {
            for (const auto& p : cs->getEngine().getEnemyParty())
            {
                if (p.character) return true;
            }
        }

        // 2. In dialogue scene (eventState) with an active target
        if (dynamic_cast<eventState*>(gameContext->getActiveState()) != nullptr)
        {
            if (gameContext->getActiveTargetNPC() != nullptr)
                return true;
        }

        // 3. In active combat encounter enemy mode (e.g. encounter resolution)
        if (gameContext->activeTargetMode == TargetMode::COMBAT_ENEMY)
            return true;

        return false;
    }

    std::vector<std::shared_ptr<entity>> getInteractingNPCs(game* gameContext)
    {
        std::vector<std::shared_ptr<entity>> npcs;
        if (!gameContext) return npcs;

        // 1. From CombatState if active
        if (auto* cs = dynamic_cast<CombatState*>(gameContext->getActiveState()))
        {
            for (const auto& p : cs->getEngine().getEnemyParty())
            {
                if (p.character && std::find(npcs.begin(), npcs.end(), p.character) == npcs.end())
                    npcs.push_back(p.character);
            }
        }

        // 2. Tile NPCs in stable order
        auto tileNPCs = gameContext->getTileNPCs();
        for (const auto& n : tileNPCs)
        {
            if (n && std::find(npcs.begin(), npcs.end(), n) == npcs.end())
                npcs.push_back(n);
        }

        // 3. Active target NPC if set and not already in list
        if (auto active = gameContext->getActiveTargetNPCShared())
        {
            if (std::find(npcs.begin(), npcs.end(), active) == npcs.end())
            {
                npcs.push_back(active);
            }
        }

        return npcs;
    }

    float renderWidgetNPCCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext) return 0.0f;

        auto npcs = getInteractingNPCs(gameContext);
        if (npcs.empty()) return 0.0f;

        // Determine currently selected character:
        // "If theres only one character, obviously they are selected. More than one should default to the top of the list."
        std::shared_ptr<entity> selectedShared = nullptr;
        if (gameContext->activeTargetNPC)
        {
            auto it = std::find(npcs.begin(), npcs.end(), gameContext->activeTargetNPC);
            if (it != npcs.end())
            {
                selectedShared = *it;
            }
        }
        if (!selectedShared)
        {
            selectedShared = npcs.front();
            gameContext->activeTargetNPC = selectedShared;
            gameContext->activeTargetMode = TargetMode::DIALOGUE;
        }

        entity* npc = selectedShared.get();
        if (!npc) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Dynamic Status Effects calculation
        float innerPad = 4.0f * uiScale;
        float subW = availableW - (innerPad * 2.0f);
        float s2Pad = 5.0f * uiScale;
        float s2ContentW = subW - (s2Pad * 2.0f);

        const float chipSize = 22.0f * uiScale;
        const float chipGap = 3.0f * uiScale;
        int chipsPerRow = std::max(1, static_cast<int>(std::floor((s2ContentW + chipGap) / (chipSize + chipGap))));

        const auto& effects = npc->statusEffects;
        int numEffectRows = effects.empty() ? 1 : static_cast<int>(std::ceil(effects.size() / static_cast<float>(chipsPerRow)));
        float statusSectionH = (numEffectRows * chipSize) + ((numEffectRows - 1) * chipGap);

        float sub1H = 58.0f * uiScale;
        float vitalsTopH = 80.0f * uiScale;
        float sub2H = vitalsTopH + statusSectionH + (8.0f * uiScale);
        float headerH = 20.0f * uiScale;
        float outerH = headerH + (3.0f * uiScale) + sub1H + (5.0f * uiScale) + sub2H + (5.0f * uiScale);

        // Outer Container: NPC Overview Card
        SDL_FRect outerRect = { padX, curY, availableW, outerH };
        UIWidget::drawPanel(renderer, outerRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Header (No close button: "You shouldn't be able to close out a character card")
        bool isHostile = (gameContext->activeTargetMode == TargetMode::COMBAT_ENEMY || dynamic_cast<CombatState*>(gameContext->getActiveState()) != nullptr);
        SDL_Color headerCol = isHostile ? Theme::colors.enemy : Theme::colors.textGold;
        std::string headerTitle = isHostile ? std::format("OPPONENT: {}", npc->name) : std::format("TARGET: {}", npc->name);

        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, headerTitle, Theme::colors.bgHeader, headerCol, uiScale * 0.74f);
        curY += headerH + (3.0f * uiScale);

        // Sub-Box 1: Identity & Wealth
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

        std::string initials = "NPC";
        if (!npc->name.empty())
        {
            initials = npc->name.substr(0, 1);
            size_t spacePos = npc->name.find(' ');
            if (spacePos != std::string::npos && spacePos + 1 < npc->name.length())
            {
                initials += npc->name[spacePos + 1];
            }
        }
        float initW = UIWidget::getTextWidth(initials, uiScale * 0.78f);
        UIWidget::drawText(renderer, initials, avatarRect.x + ((avatarSize - initW) / 2.0f), avatarRect.y + (5.0f * uiScale), headerCol, uiScale * 0.78f);

        std::string raceStr = npc->anatomy.getRacialTitle().empty() ? "Demon" : npc->anatomy.getRacialTitle();
        TooltipManager::setHoverTooltip(avatarRect, mousePos, npc->name,
                                        "Character engaged in active interaction or combat.",
                                        std::format("Level {} • {}", npc->stats.level, raceStr));

        // Name & Level / Species
        UIWidget::drawText(renderer, npc->name, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (1.0f * uiScale), Theme::colors.textGold, uiScale * 0.84f);
        std::string lvlStr = std::format("Lvl {} • {}", npc->stats.level, raceStr);
        UIWidget::drawText(renderer, lvlStr, s1ContentX + avatarSize + (6.0f * uiScale), s1Y + (15.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

        // Gold & Gender Archetype
        float goldVal = npc->getStat("currency");
        std::string goldText = std::format("Gold: {:.0f} ¤", goldVal);
        UIWidget::drawText(renderer, goldText, s1ContentX, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.currency, uiScale * 0.72f);
        TooltipManager::setHoverTooltip({ s1ContentX, s1Y + avatarSize + (2.0f * uiScale), 70.0f * uiScale, 16.0f * uiScale },
                                        mousePos, "Enemy Spoils", "Carried coin that can be acquired through victory, bribery, or trade.", goldText);

        std::string archStr = genderArchetypeToString(npc->genderArchetype);
        float archW = UIWidget::getTextWidth(archStr, uiScale * 0.70f);
        UIWidget::drawText(renderer, archStr, s1ContentX + s1ContentW - archW, s1Y + avatarSize + (4.0f * uiScale), Theme::colors.companion, uiScale * 0.70f);
        TooltipManager::setHoverTooltip({ s1ContentX + s1ContentW - archW - (4.0f * uiScale), s1Y + avatarSize + (2.0f * uiScale), archW + (8.0f * uiScale), 16.0f * uiScale },
                                        mousePos, "Gender & Archetype",
                                        std::format("Orientation: {}", sexualOrientationToString(npc->orientation)),
                                        archStr);

        curY += sub1H + (5.0f * uiScale);

        // Sub-Box 2: Vitals & Status Gauges
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
        float curHp = npc->getStat("health");
        float maxHp = npc->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = std::max(curHp, 100.0f);
        curHp = std::clamp(curHp, 0.0f, maxHp);

        UIWidget::drawText(renderer, "Health", s2ContentX, s2Y, Theme::colors.health, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curHp, maxHp, Theme::colors.health, Theme::colors.bgHeader, "", uiScale);
        std::string hpStr = std::format("{:.0f}/{:.0f}", curHp, maxHp);
        UIWidget::drawText(renderer, hpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Health Points (HP)",
                                        "Character vitality. Reaching 0 results in collapse, defeat, or surrender.",
                                        std::format("{:.0f} / {:.0f} HP", curHp, maxHp));
        s2Y += barH + barGap;

        // 2. Mana Bar
        float curMp = npc->getStat("mana");
        float maxMp = npc->getStat("max_mana");
        if (maxMp <= 0.0f) maxMp = std::max(curMp, 100.0f);
        curMp = std::clamp(curMp, 0.0f, maxMp);

        UIWidget::drawText(renderer, "Mana", s2ContentX, s2Y, Theme::colors.mana, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curMp, std::max(1.0f, maxMp), Theme::colors.mana, Theme::colors.bgHeader, "", uiScale);
        std::string mpStr = std::format("{:.0f}/{:.0f}", curMp, maxMp);
        UIWidget::drawText(renderer, mpStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Arcane Mana (MP)",
                                        "Magical reserves consumed by spells and special attacks.",
                                        std::format("{:.0f} / {:.0f} MP", curMp, maxMp));
        s2Y += barH + barGap;

        // 3. Lust Bar
        float curLust = std::clamp(npc->getStat("lust"), 0.0f, 100.0f);
        float maxLust = npc->getStat("max_lust");
        if (maxLust <= 0.0f) maxLust = 100.0f;

        UIWidget::drawText(renderer, "Lust", s2ContentX, s2Y, Theme::colors.lust, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curLust, maxLust, Theme::colors.lust, Theme::colors.bgHeader, "", uiScale);
        std::string lustStr = std::format("{:.0f}%", (curLust / maxLust) * 100.0f);
        UIWidget::drawText(renderer, lustStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Lust Level",
                                        "Susceptibility to flirtation, seduction, and submission.",
                                        std::format("{:.0f}% Lust", (curLust / maxLust) * 100.0f));
        s2Y += barH + barGap;

        // 4. Arousal Bar
        float curArousal = std::clamp(npc->getStat("arousal"), 0.0f, 100.0f);
        float maxArousal = npc->getStat("max_arousal");
        if (maxArousal <= 0.0f) maxArousal = 100.0f;

        UIWidget::drawText(renderer, "Arousal", s2ContentX, s2Y, Theme::colors.textGold, uiScale * 0.65f);
        UIWidget::drawProgressBar(renderer, { s2ContentX + labelW, s2Y, progressW, barH }, curArousal, maxArousal, Theme::colors.textGold, Theme::colors.bgHeader, "", uiScale);
        std::string arousalStr = std::format("{:.0f}%", (curArousal / maxArousal) * 100.0f);
        UIWidget::drawText(renderer, arousalStr, s2ContentX + labelW + progressW + (4.0f * uiScale), s2Y, Theme::colors.textPrimary, uiScale * 0.62f);
        TooltipManager::setHoverTooltip({ s2ContentX, s2Y, s2ContentW, barH }, mousePos, "Physical Arousal",
                                        "Active physiological excitement during intimate encounters.",
                                        std::format("{:.0f}% Arousal", (curArousal / maxArousal) * 100.0f));
        s2Y += barH + (6.0f * uiScale);

        // Status Effects Chips
        if (effects.empty())
        {
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
            for (size_t i = 0; i < effects.size(); ++i)
            {
                int row = static_cast<int>(i / chipsPerRow);
                int col = static_cast<int>(i % chipsPerRow);

                float chipX = s2ContentX + (col * (chipSize + chipGap));
                float chipY = s2Y + (row * (chipSize + chipGap));
                SDL_FRect chipRect = { chipX, chipY, chipSize, chipSize };

                const auto& eff = effects[i];
                SDL_Color bdCol = eff.isDebuff ? Theme::colors.health : Theme::colors.companion;
                SDL_Color textCol = eff.isDebuff ? Theme::colors.health : Theme::colors.textGold;

                UIWidget::drawPanel(renderer, chipRect, Theme::colors.bgHeader, bdCol);
                std::string code = eff.name.substr(0, std::min<size_t>(2, eff.name.length()));
                float cW = UIWidget::getTextWidth(code, uiScale * 0.58f);
                UIWidget::drawText(renderer, code, chipX + ((chipSize - cW) / 2.0f), chipY + (3.0f * uiScale), textCol, uiScale * 0.58f);

                std::string sub = std::format("{} • {} turns remaining", eff.isDebuff ? "Debuff" : "Buff", eff.durationTurns);
                TooltipManager::setHoverTooltip(chipRect, mousePos, eff.name, eff.description, sub);
            }
        }

        curY += sub2H + (5.0f * uiScale);
        curY += (6.0f * uiScale);

        // Multi-character list (when npcs.size() > 1)
        if (npcs.size() > 1)
        {
            float listHeaderH = 18.0f * uiScale;
            SDL_FRect listHeaderRect = { padX, curY, availableW, listHeaderH };
            std::string listTitle = std::format("CHARACTERS PRESENT ({})", npcs.size());
            UIWidget::drawHeader(renderer, listHeaderRect, listTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.68f);
            curY += listHeaderH + (4.0f * uiScale);

            for (size_t i = 0; i < npcs.size(); ++i)
            {
                entity* ent = npcs[i].get();
                if (!ent) continue;

                bool isSelected = (ent == npc);
                float itemCardH = 46.0f * uiScale;
                SDL_FRect itemRect = { padX, curY, availableW, itemCardH };
                bool itemHov = (mousePos.x >= itemRect.x && mousePos.x <= itemRect.x + itemRect.w &&
                                mousePos.y >= itemRect.y && mousePos.y <= itemRect.y + itemRect.h);

                // Distinct selection outline around the selected character ("there will be an outline around the selected character")
                SDL_Color cardBg = isSelected ? SDL_Color{ 36, 44, 62, 255 } : Theme::colors.bgSlot;
                SDL_Color cardBorder = isSelected ? Theme::colors.borderSelected : (itemHov ? Theme::colors.borderButtonHover : Theme::colors.borderNormal);
                UIWidget::drawPanel(renderer, itemRect, cardBg, cardBorder);

                if (isSelected)
                {
                    SDL_FRect innerOutline = { itemRect.x + 1.0f, itemRect.y + 1.0f, itemRect.w - 2.0f, itemRect.h - 2.0f };
                    SDL_SetRenderDrawColor(renderer, Theme::colors.borderSelected.r, Theme::colors.borderSelected.g, Theme::colors.borderSelected.b, Theme::colors.borderSelected.a);
                    SDL_RenderRect(renderer, &innerOutline);
                }

                float oAvatarSize = 20.0f * uiScale;
                SDL_FRect oAvatarRect = { padX + (5.0f * uiScale), curY + (5.0f * uiScale), oAvatarSize, oAvatarSize };
                UIWidget::drawPanel(renderer, oAvatarRect, isSelected ? Theme::colors.bgHeader : Theme::colors.bgDark, cardBorder);
                std::string oInit = ent->name.empty() ? "?" : ent->name.substr(0, 1);
                float oInitW = UIWidget::getTextWidth(oInit, uiScale * 0.66f);
                UIWidget::drawText(renderer, oInit, oAvatarRect.x + ((oAvatarSize - oInitW) / 2.0f), oAvatarRect.y + (2.0f * uiScale), isSelected ? Theme::colors.textGold : Theme::colors.textSecondary, uiScale * 0.66f);

                float infoX = padX + oAvatarSize + (9.0f * uiScale);
                UIWidget::drawText(renderer, ent->name, infoX, curY + (4.0f * uiScale), isSelected ? Theme::colors.textGold : Theme::colors.textPrimary, uiScale * 0.72f);
                std::string subStr = std::format("Lvl {} • {}", ent->stats.level, ent->anatomy.getRacialTitle().empty() ? "Demon" : ent->anatomy.getRacialTitle());
                UIWidget::drawText(renderer, subStr, infoX, curY + (17.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.60f);

                if (isSelected)
                {
                    std::string selBadge = "[Selected]";
                    float selW = UIWidget::getTextWidth(selBadge, uiScale * 0.60f);
                    UIWidget::drawText(renderer, selBadge, padX + availableW - selW - (6.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.friendly, uiScale * 0.60f);
                }
                else if (itemHov)
                {
                    std::string selBadge = "Click to Select";
                    float selW = UIWidget::getTextWidth(selBadge, uiScale * 0.58f);
                    UIWidget::drawText(renderer, selBadge, padX + availableW - selW - (6.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textMuted, uiScale * 0.58f);
                }

                // Mini Vitals (Health & Lust)
                float oHp = std::clamp(ent->getStat("health"), 0.0f, std::max(1.0f, ent->getStat("max_health")));
                float oMaxHp = std::max(1.0f, ent->getStat("max_health"));
                float miniBarW = availableW - (10.0f * uiScale);
                float miniBarH = 4.0f * uiScale;
                float halfBarW = (miniBarW - (4.0f * uiScale)) / 2.0f;

                UIWidget::drawProgressBar(renderer, { padX + (5.0f * uiScale), curY + (33.0f * uiScale), halfBarW, miniBarH }, oHp, oMaxHp, Theme::colors.health, Theme::colors.bgDark, "", uiScale);
                float oLust = std::clamp(ent->getStat("lust"), 0.0f, 100.0f);
                float oMaxLust = std::max(1.0f, ent->getStat("max_lust"));
                UIWidget::drawProgressBar(renderer, { padX + (5.0f * uiScale) + halfBarW + (4.0f * uiScale), curY + (33.0f * uiScale), halfBarW, miniBarH }, oLust, oMaxLust, Theme::colors.lust, Theme::colors.bgDark, "", uiScale);

                TooltipManager::setHoverTooltip(itemRect, mousePos, ent->name,
                                                std::format("Health: {:.0f}/{:.0f} • Lust: {:.0f}%. Click to select as active target.", oHp, oMaxHp, (oLust / oMaxLust) * 100.0f),
                                                subStr, isSelected ? "Active Target" : "Select Target");

                // Clicking selects this character without altering layout
                if (itemHov && clicked && !isSelected)
                {
                    gameContext->activeTargetNPC = npcs[i];
                    gameContext->activeTargetMode = isHostile ? TargetMode::COMBAT_ENEMY : TargetMode::DIALOGUE;
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }

                curY += itemCardH + (4.0f * uiScale);
            }
        }

        return (curY - startY);
    }

    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        // When interacting with an NPC, replace the normal boxes with the dedicated NPC Character Card
        if (isInteractingWithNPC(gameContext))
        {
            return renderWidgetNPCCard(renderer, gameContext, curX, curY, innerW, uiScale);
        }

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        int pX = gameContext->gridX;
        int pY = gameContext->gridY;
        gameMap* m = gameContext->map;

        // ==========================================
        // CARD 1: Zone & Environment Status Card
        // ==========================================
        float card1H = 44.0f * uiScale;
        SDL_FRect card1Rect = { padX, curY, availableW, card1H };
        UIWidget::drawPanel(renderer, card1Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        std::string locName = "Unknown Area";
        if (m && !m->getName().empty())
        {
            locName = m->getName();
        }

        std::string safeTag = "[ Safe ]";
        SDL_Color safeCol = Theme::colors.companion;

        int danger = 0;
        bool hasAmbush = false;
        if (m)
        {
            TileRuntimeData& rData = m->getRuntimeData(pX, pY);
            danger = rData.getEffectiveDangerLevel();
            hasAmbush = (!rData.ambushState.isDefeated && !rData.ambushState.isPermanentlyRemoved &&
                         (rData.ambushState.npc != nullptr || !rData.ambushState.templateId.empty() || !rData.ambushState.templatePool.empty()));
        }

        if (danger >= 2)
        {
            safeTag = "[ Dangerous ]";
            safeCol = Theme::colors.enemy;
        }
        else if (danger == 1 || hasAmbush)
        {
            safeTag = "[ Risky ]";
            safeCol = Theme::colors.textGold;
        }
        else
        {
            safeTag = "[ Safe ]";
            safeCol = Theme::colors.companion;
        }

        // Subtext loaded 100% data-driven from map JSON (tileTitles, warps, triggers, tagTitles, or defaultTileTitle)
        std::string subText = m ? m->getTileTitle(pX, pY) : "Sanctuary interior";

        UIWidget::drawText(renderer, locName, innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);
        float safeW = UIWidget::getTextWidth(safeTag, uiScale * 0.68f);
        UIWidget::drawText(renderer, safeTag, innerX + cW - safeW, curY + (5.0f * uiScale), safeCol, uiScale * 0.68f);

        UIWidget::drawText(renderer, subText, innerX, curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.66f);

        std::string tooltipDetail = std::format("Coordinates ({}, {}). Safety: {}", pX, pY, safeTag);
        if (!subText.empty())
        {
            tooltipDetail = std::format("{} - {}", subText, tooltipDetail);
        }
        TooltipManager::setHoverTooltip(card1Rect, mousePos, locName, tooltipDetail, safeTag);

        curY += card1H + (8.0f * uiScale);

        // ==========================================
        // CARD 2: Characters Present Card (Basic Info)
        // ==========================================
        auto tileNPCs = gameContext->getTileNPCs();

        if (tileNPCs.empty())
        {
            float card2H = 48.0f * uiScale;
            SDL_FRect card2Rect = { padX, curY, availableW, card2H };
            UIWidget::drawPanel(renderer, card2Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, "CHARACTERS PRESENT", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);
            UIWidget::drawText(renderer, "No characters here.", innerX, curY + (22.0f * uiScale), Theme::colors.textMuted, uiScale * 0.68f);

            curY += card2H + (8.0f * uiScale);
            return (curY - startY);
        }

        float rowH = 40.0f * uiScale;
        float rowSpacing = 4.0f * uiScale;
        float card2H = (24.0f * uiScale) + (static_cast<float>(tileNPCs.size()) * (rowH + rowSpacing)) + (4.0f * uiScale);
        SDL_FRect card2Rect = { padX, curY, availableW, card2H };
        UIWidget::drawPanel(renderer, card2Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        std::string charHeader = std::format("CHARACTERS PRESENT ({})", tileNPCs.size());
        UIWidget::drawText(renderer, charHeader, innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        // Ensure default selection: if unassigned or invalid, default to top of list
        if (!gameContext->activeTargetNPC || std::find(tileNPCs.begin(), tileNPCs.end(), gameContext->activeTargetNPC) == tileNPCs.end())
        {
            gameContext->activeTargetNPC = tileNPCs.front();
        }

        float rowY = curY + (22.0f * uiScale);
        for (const auto& npcShared : tileNPCs)
        {
            entity* npc = npcShared.get();
            if (!npc) continue;

            bool isSelected = (gameContext->getActiveTargetNPC() == npc);

            SDL_FRect rowRect = { innerX, rowY, cW, rowH };
            bool rowHov = (mousePos.x >= rowRect.x && mousePos.x <= rowRect.x + rowRect.w &&
                           mousePos.y >= rowRect.y && mousePos.y <= rowRect.y + rowRect.h);

            SDL_Color rowBg = isSelected ? SDL_Color{ 36, 44, 62, 255 } : (rowHov ? Theme::colors.bgButtonHover : Theme::colors.bgDark);
            SDL_Color rowBorder = isSelected ? Theme::colors.borderSelected : (rowHov ? Theme::colors.borderButtonHover : Theme::colors.borderNormal);
            UIWidget::drawPanel(renderer, rowRect, rowBg, rowBorder);

            if (isSelected)
            {
                SDL_FRect innerOutline = { rowRect.x + 1.0f, rowRect.y + 1.0f, rowRect.w - 2.0f, rowRect.h - 2.0f };
                SDL_SetRenderDrawColor(renderer, Theme::colors.borderSelected.r, Theme::colors.borderSelected.g, Theme::colors.borderSelected.b, Theme::colors.borderSelected.a);
                SDL_RenderRect(renderer, &innerOutline);
            }

            float rowInnerX = innerX + (6.0f * uiScale);

            // Name in gold
            UIWidget::drawText(renderer, npc->name, rowInnerX, rowY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);

            // Basic Info (Lvl, Race, Title)
            std::string raceStr = npc->anatomy.getRacialTitle().empty() ? "Human" : npc->anatomy.getRacialTitle();
            auto nc = NamedCharacterManager::getCharacter(npc->id);
            std::string titleStr = (nc && !nc->title.empty()) ? (" • " + nc->title) : "";
            std::string subStr = std::format("Lvl {} • {}{}", npc->stats.level, raceStr, titleStr);
            UIWidget::drawText(renderer, subStr, rowInnerX, rowY + (17.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.60f);

            // Selected badge or Gender Archetype
            if (isSelected)
            {
                std::string selBadge = "[Selected]";
                float selW = UIWidget::getTextWidth(selBadge, uiScale * 0.56f);
                UIWidget::drawText(renderer, selBadge, innerX + cW - selW - (6.0f * uiScale), rowY + (4.0f * uiScale), Theme::colors.friendly, uiScale * 0.56f);
            }
            else
            {
                std::string genderStr = genderArchetypeToString(npc->genderArchetype);
                float gW = UIWidget::getTextWidth(genderStr, uiScale * 0.56f);
                UIWidget::drawText(renderer, genderStr, innerX + cW - gW - (6.0f * uiScale), rowY + (4.0f * uiScale), Theme::colors.textMuted, uiScale * 0.56f);
            }

            // Mini Vitals (Health & Lust)
            float hp = std::clamp(npc->getStat("health"), 0.0f, std::max(1.0f, npc->getStat("max_health")));
            float maxHp = std::max(1.0f, npc->getStat("max_health"));
            float miniBarW = cW - (12.0f * uiScale);
            float miniBarH = 3.0f * uiScale;
            float halfBarW = (miniBarW - (4.0f * uiScale)) / 2.0f;
            UIWidget::drawProgressBar(renderer, { rowInnerX, rowY + (31.0f * uiScale), halfBarW, miniBarH }, hp, maxHp, Theme::colors.health, Theme::colors.bgDark, "", uiScale);
            float lust = std::clamp(npc->getStat("lust"), 0.0f, 100.0f);
            float maxLust = std::max(1.0f, npc->getStat("max_lust"));
            UIWidget::drawProgressBar(renderer, { rowInnerX + halfBarW + (4.0f * uiScale), rowY + (31.0f * uiScale), halfBarW, miniBarH }, lust, maxLust, Theme::colors.lust, Theme::colors.bgDark, "", uiScale);

            // Tooltip
            TooltipManager::setHoverTooltip(rowRect, mousePos, npc->name,
                std::format("Lvl {} • {}{}\nHealth: {:.0f}/{:.0f} • Lust: {:.0f}%\nClick to select as active target.",
                            npc->stats.level, raceStr, titleStr, hp, maxHp, (lust / maxLust) * 100.0f),
                "Character Present", isSelected ? "Active Selection" : "Click to Select");

            // Clicking row selects this character without altering layout or initiating interaction
            if (rowHov && clicked && !isSelected)
            {
                gameContext->activeTargetNPC = npcShared;
                gameContext->activeTargetMode = TargetMode::NONE;
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }

            rowY += rowH + rowSpacing;
        }

        curY += card2H + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        auto ground = gameContext->getTileInventoryStacked();

        // When interacting with an NPC, suppress empty items card to preserve vertical space
        if (isInteractingWithNPC(gameContext) && ground.empty()) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 3: Items on Ground Card
        // ==========================================
        float card3H = ground.empty() ? (48.0f * uiScale) : ((28.0f * uiScale) + (ground.size() * (26.0f * uiScale)));
        card3H = std::min(card3H, 130.0f * uiScale);

        SDL_FRect card3Rect = { padX, curY, availableW, card3H };
        UIWidget::drawPanel(renderer, card3Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "ITEMS PRESENT", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        if (ground.empty())
        {
            UIWidget::drawText(renderer, "No items dropped.", innerX, curY + (22.0f * uiScale), Theme::colors.textMuted, uiScale * 0.68f);
        }
        else
        {
            float itemY = curY + (22.0f * uiScale);
            for (size_t i = 0; i < ground.size() && i < 3; ++i)
            {
                if (ground[i].itemPtr)
                {
                    std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                    UIWidget::drawText(renderer, line, innerX, itemY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.70f);

                    float pickBtnW = 38.0f * uiScale;
                    float pickBtnH = 18.0f * uiScale;
                    SDL_FRect pickBtn = { innerX + cW - pickBtnW, itemY + (2.0f * uiScale), pickBtnW, pickBtnH };
                    bool pHov = (mousePos.x >= pickBtn.x && mousePos.x <= pickBtn.x + pickBtn.w &&
                                 mousePos.y >= pickBtn.y && mousePos.y <= pickBtn.y + pickBtn.h);
                    UIWidget::drawButton(renderer, pickBtn, "Take", pHov, true, false, uiScale * 0.64f);

                    std::string sub = std::format("Ground Loot • Value: {} ¤", ground[i].itemPtr->baseValue);
                    std::string hk = std::format("x{}", ground[i].totalCount);
                    TooltipManager::setHoverTooltip(pickBtn, mousePos, "Take " + ground[i].itemPtr->name,
                                                    std::format("Transfer 1x {} from the ground into your backpack.", ground[i].itemPtr->name),
                                                    sub, hk);

                    if (pHov && clicked)
                    {
                        gameContext->handlePickupAction(static_cast<int>(i), 1);
                        gameContext->input.consumeMouseClick();
                        break;
                    }

                    itemY += (24.0f * uiScale);
                }
            }
        }

        curY += card3H + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 4: Activity & Event Log Card
        // ==========================================
        float card4H = 145.0f * uiScale;
        SDL_FRect logBox = { padX, curY, availableW, card4H };
        UIWidget::drawPanel(renderer, logBox, Theme::colors.bgSlot, Theme::colors.borderNormal);

        const auto& allLogs = gameContext->getEventLog();
        int totalLogs = static_cast<int>(allLogs.size());
        int maxVisible = 6;
        int maxOffset = std::max(0, totalLogs - maxVisible);

        static int s_eventLogScrollOffset = 0;
        static size_t s_lastLogCount = 0;
        if (allLogs.size() > s_lastLogCount)
        {
            s_eventLogScrollOffset = 0; // Automatically snap to newest entries on top
            s_lastLogCount = allLogs.size();
        }
        s_eventLogScrollOffset = std::clamp(s_eventLogScrollOffset, 0, maxOffset);

        bool logBoxHovered = (mousePos.x >= logBox.x && mousePos.x <= logBox.x + logBox.w &&
                              mousePos.y >= logBox.y && mousePos.y <= logBox.y + logBox.h);

        // Mouse wheel scrolling
        if (logBoxHovered && maxOffset > 0)
        {
            float wheelY = gameContext->input.getMouseWheelY();
            if (wheelY != 0.0f)
            {
                int step = static_cast<int>(std::round(wheelY));
                if (step == 0) step = (wheelY > 0.0f) ? 1 : -1;
                s_eventLogScrollOffset = std::clamp(s_eventLogScrollOffset - step, 0, maxOffset);
                gameContext->input.consumeMouseWheel();
            }
        }

        // Header with title and scroll arrow buttons
        UIWidget::drawText(renderer, "ACTIVITY & EVENT LOG", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        if (totalLogs > maxVisible)
        {
            float btnW = 16.0f * uiScale;
            float btnH = 14.0f * uiScale;
            float btnGap = 2.0f * uiScale;
            float btnY = curY + (4.0f * uiScale);
            float btnDownX = padX + availableW - innerPad - btnW;
            float btnUpX = btnDownX - btnW - btnGap;

            SDL_FRect upRect = { btnUpX, btnY, btnW, btnH };
            bool upHov = (mousePos.x >= upRect.x && mousePos.x <= upRect.x + upRect.w &&
                          mousePos.y >= upRect.y && mousePos.y <= upRect.y + upRect.h);
            bool canUp = (s_eventLogScrollOffset > 0);
            if (upHov && clicked && canUp)
            {
                s_eventLogScrollOffset = std::max(0, s_eventLogScrollOffset - 1);
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, upRect, "^", upHov, canUp, false, uiScale * 0.60f);

            SDL_FRect downRect = { btnDownX, btnY, btnW, btnH };
            bool downHov = (mousePos.x >= downRect.x && mousePos.x <= downRect.x + downRect.w &&
                            mousePos.y >= downRect.y && mousePos.y <= downRect.y + downRect.h);
            bool canDown = (s_eventLogScrollOffset < maxOffset);
            if (downHov && clicked && canDown)
            {
                s_eventLogScrollOffset = std::min(maxOffset, s_eventLogScrollOffset + 1);
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, downRect, "v", downHov, canDown, false, uiScale * 0.60f);
        }

        float logCurY = curY + (22.0f * uiScale);
        float scrollbarW = (totalLogs > maxVisible) ? (12.0f * uiScale) : 0.0f;
        cW = availableW - (innerPad * 2.0f) - scrollbarW;

        if (allLogs.empty())
        {
            UIWidget::drawText(renderer, "No recent events recorded.", innerX, logCurY, Theme::colors.textMuted, uiScale * 0.65f);
        }
        else
        {
            // Render entries with MOST RECENT ON TOP (reverse chronological order)
            // When s_eventLogScrollOffset == 0, row 0 is allLogs[N - 1] (newest)
            for (int r = 0; r < maxVisible; ++r)
            {
                int entryIdx = (totalLogs - 1) - (s_eventLogScrollOffset + r);
                if (entryIdx < 0 || entryIdx >= totalLogs) break;

                const auto& entry = allLogs[entryIdx];
                SDL_Color tagColor = { entry.color.r, entry.color.g, entry.color.b, entry.color.a };
                UIWidget::drawText(renderer, entry.tag, innerX, logCurY, tagColor, uiScale * 0.65f);
                float tagW = UIWidget::getTextWidth(entry.tag, uiScale * 0.65f);

                float maxTextW = cW - tagW - (6.0f * uiScale);
                std::string displayText = entry.text;
                while (!displayText.empty() && UIWidget::getTextWidth(displayText, uiScale * 0.65f) > maxTextW)
                {
                    displayText.pop_back();
                }
                if (displayText.size() < entry.text.size() && displayText.size() > 3)
                {
                    displayText.replace(displayText.size() - 2, 2, "..");
                }

                UIWidget::drawText(renderer, displayText, innerX + tagW + (4.0f * uiScale), logCurY, Theme::colors.textPrimary, uiScale * 0.65f);

                SDL_FRect rowRect = { innerX, logCurY, cW, 16.0f * uiScale };
                TooltipManager::setHoverTooltip(rowRect, mousePos, entry.tag + (!entry.timeStr.empty() ? (" (" + entry.timeStr + ")") : ""), entry.text, "Event Log");

                logCurY += (18.0f * uiScale);
            }

            // Draw scrollbar track and thumb with full mouse click jumping and drag support
            if (totalLogs > maxVisible)
            {
                float barW = 5.0f * uiScale;
                float trackX = padX + availableW - innerPad - barW;
                float trackY = curY + (22.0f * uiScale);
                float trackH = card4H - (26.0f * uiScale);

                float thumbH = std::max(16.0f * uiScale, trackH * (static_cast<float>(maxVisible) / static_cast<float>(totalLogs)));
                float travelH = trackH - thumbH;
                float ratio = (maxOffset > 0 && travelH > 0.0f) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                float thumbY = trackY + ratio * travelH;

                static bool s_isDraggingLogScroll = false;
                static float s_dragStartMouseY = 0.0f;
                static int s_dragStartOffset = 0;

                SDL_FRect trackHitRect = { trackX - (4.0f * uiScale), trackY, barW + (8.0f * uiScale), trackH };
                bool trackHovered = (mousePos.x >= trackHitRect.x && mousePos.x <= trackHitRect.x + trackHitRect.w &&
                                     mousePos.y >= trackHitRect.y && mousePos.y <= trackHitRect.y + trackHitRect.h);

                bool isMouseDown = gameContext->input.isLeftMouseDown();
                if (clicked && trackHovered)
                {
                    s_isDraggingLogScroll = true;
                    s_dragStartMouseY = mousePos.y;
                    s_dragStartOffset = s_eventLogScrollOffset;

                    // If clicked directly on track outside thumb, jump immediately
                    bool onThumb = (mousePos.y >= thumbY && mousePos.y <= thumbY + thumbH);
                    if (!onThumb && travelH > 0.0f)
                    {
                        float clickRatio = std::clamp((mousePos.y - trackY - (thumbH / 2.0f)) / travelH, 0.0f, 1.0f);
                        s_eventLogScrollOffset = std::clamp(static_cast<int>(std::round(clickRatio * maxOffset)), 0, maxOffset);
                        s_dragStartOffset = s_eventLogScrollOffset;
                        ratio = (maxOffset > 0) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                        thumbY = trackY + ratio * travelH;
                    }
                    gameContext->input.consumeMouseClick();
                }

                if (s_isDraggingLogScroll)
                {
                    if (isMouseDown)
                    {
                        float deltaY = mousePos.y - s_dragStartMouseY;
                        if (travelH > 0.0f)
                        {
                            float offsetDelta = (deltaY / travelH) * maxOffset;
                            s_eventLogScrollOffset = std::clamp(s_dragStartOffset + static_cast<int>(std::round(offsetDelta)), 0, maxOffset);
                            ratio = (maxOffset > 0) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                            thumbY = trackY + ratio * travelH;
                        }
                    }
                    else
                    {
                        s_isDraggingLogScroll = false;
                    }
                }

                SDL_FRect trackRect = { trackX, trackY, barW, trackH };
                UIWidget::drawPanel(renderer, trackRect, Theme::colors.bgDark, Theme::colors.borderNormal);

                SDL_FRect thumbRect = { trackX, thumbY, barW, thumbH };
                SDL_Color thumbCol = (s_isDraggingLogScroll || trackHovered) ? Theme::colors.textGold : Theme::colors.borderSelected;
                UIWidget::drawPanel(renderer, thumbRect, thumbCol, thumbCol);
            }
        }

        curY += card4H + (8.0f * uiScale);
        return (curY - startY);
    }
}
