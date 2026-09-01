#include "ui/views/gameplayViews.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "core/characterDescription.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/sexState.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/phoneAppsState.h"
#include "state/shopState.h"
#include "state/transformationState.h"
#include "ui/views/transformationView.h"
#include <format>
#include <vector>
#include <string>
#include <algorithm>
#include <string_view>

namespace GameplayViews
{
    float renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        const questScene& scene = gameContext->getCurrentScene();
        float startY = curY;
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, scene.speakerName.empty() ? "NARRATIVE SCENE" : scene.speakerName, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (10.0f * uiScale);

        float padX = rect.x + (12.0f * uiScale);
        float innerW = rect.w - (24.0f * uiScale);
        float textH = UIWidget::drawTextWrapped(renderer, scene.bodyText, padX, curY, innerW, Theme::colors.textPrimary, uiScale);
        curY += textH + (16.0f * uiScale);

        return (curY - startY);
    }

    float renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        sexState* sex = dynamic_cast<sexState*>(gameContext->getActiveState());
        if (!sex) return 0.0f;

        float startY = curY;
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };

        if (sex->isSolo())
        {
            UIWidget::drawHeader(renderer, headerRect, "SOLO INTIMATE ENCOUNTER", Theme::colors.bgHeader, Theme::colors.lust, uiScale);
            curY += headerH + (10.0f * uiScale);

            float padX = rect.x + (12.0f * uiScale);
            float innerW = rect.w - (24.0f * uiScale);
            float barH = 18.0f * uiScale;

            UIWidget::drawText(renderer, std::format("Position: {} | Private Solitude", sexStanceToString(sex->getStance())), padX, curY, Theme::colors.textGold, uiScale);
            curY += (22.0f * uiScale);

            UIWidget::drawProgressBar(renderer, { padX, curY, innerW, barH }, sex->getPlayerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Your Arousal: {:.0f}/100", sex->getPlayerArousal()), uiScale);
            curY += (barH + 12.0f * uiScale);

            UIWidget::drawText(renderer, "INTIMATE LOG:", padX, curY, Theme::colors.textGold, uiScale);
            curY += (18.0f * uiScale);
            float textH = UIWidget::drawTextWrapped(renderer, sex->getNarrativeLog(), padX, curY, innerW, Theme::colors.textSecondary, uiScale);
            curY += textH + (6.0f * uiScale);

            return (curY - startY);
        }

        UIWidget::drawHeader(renderer, headerRect, "INTERACTIVE CYOA EROTIC ENCOUNTER", Theme::colors.bgHeader, Theme::colors.lust, uiScale);
        curY += headerH + (10.0f * uiScale);

        entity* partner = sex->getPartner();
        std::string partnerName = partner ? partner->name : "Partner";

        float padX = rect.x + (12.0f * uiScale);
        float innerW = rect.w - (24.0f * uiScale);
        float halfW = (innerW - (10.0f * uiScale)) / 2.0f;
        float barH = 18.0f * uiScale;

        UIWidget::drawText(renderer, std::format("Partner: {} | Stance: {}", partnerName, sexStanceToString(sex->getStance())), padX, curY, Theme::colors.textGold, uiScale);
        curY += (22.0f * uiScale);

        UIWidget::drawProgressBar(renderer, { padX, curY, halfW, barH }, sex->getPlayerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Your Arousal: {:.0f}/100", sex->getPlayerArousal()), uiScale);
        UIWidget::drawProgressBar(renderer, { padX + halfW + (10.0f * uiScale), curY, halfW, barH }, sex->getPartnerArousal(), 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("{} Arousal: {:.0f}/100", partnerName, sex->getPartnerArousal()), uiScale);
        curY += (barH + 8.0f * uiScale);

        float dom = sex->getPlayerDominance();
        UIWidget::drawProgressBar(renderer, { padX, curY, innerW, barH }, dom + 100.0f, 200.0f, Theme::colors.textAccent, Theme::colors.bgDark, std::format("Dominance Continuum: {:.0f} ({})", dom, sex->isPlayerDominant() ? "Dominant" : "Submissive"), uiScale);
        curY += (barH + 10.0f * uiScale);

        UIWidget::drawText(renderer, "NARRATIVE LOG:", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);
        float textH = UIWidget::drawTextWrapped(renderer, sex->getNarrativeLog(), padX, curY, innerW, Theme::colors.textSecondary, uiScale);
        curY += textH + (6.0f * uiScale);

        return (curY - startY);
    }

    float renderPhoneAppView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        phoneAppsState* app = dynamic_cast<phoneAppsState*>(gameContext->getActiveState());
        if (!app) return 0.0f;

        float startY = curY;
        float padX = rect.x + (16.0f * uiScale);
        float innerW = rect.w - (32.0f * uiScale);

        std::string appTitle = phoneAppModeToString(app->getAppMode());
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, std::format("📱 PHONE APP - {}", appTitle), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        const auto& data = app->getAppData();

        if (app->getAppMode() == PhoneAppMode::QUESTS)
        {
            // Quest Journal
            UIWidget::drawText(renderer, "Lilaya's Tests (Active Main Quest)", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (20.0f * uiScale);
            UIWidget::drawText(renderer, "Chapter 1: The New World", padX, curY, Theme::colors.textAccent, uiScale * 0.85f);
            curY += (18.0f * uiScale);

            std::string qDesc = "Having arrived at Lilaya's residence, you must explore your surroundings and undergo initial arcane resonance testing in her laboratory.";
            float qH = UIWidget::drawTextWrapped(renderer, qDesc, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += qH + (14.0f * uiScale);

            UIWidget::drawText(renderer, "Objectives:", padX, curY, Theme::colors.textGold, uiScale * 0.9f);
            curY += (18.0f * uiScale);

            static const std::vector<std::pair<std::string, bool>> objectives = {
                { "[✓] Awaken in Lilaya's Home F1", true },
                { "[ ] Explore the first floor corridors", false },
                { "[ ] Speak with Lilaya in her laboratory", false }
            };
            for (const auto& obj : objectives)
            {
                UIWidget::drawText(renderer, obj.first, padX + (8.0f * uiScale), curY, obj.second ? Theme::colors.friendly : Theme::colors.textSecondary, uiScale * 0.85f);
                curY += (16.0f * uiScale);
            }
            curY += (10.0f * uiScale);

            UIWidget::drawText(renderer, "Rewards: ¤ 5,000 | 150 XP | Arcane Essence x5", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
            curY += (20.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::ENCYCLOPEDIA)
        {
            // Encyclopedia
            if (data.contains("categories") && data["categories"].is_array())
            {
                for (const auto& cat : data["categories"])
                {
                    std::string catName = cat.value("name", "Category");
                    UIWidget::drawText(renderer, catName, padX, curY, Theme::colors.textGold, uiScale * 0.95f);
                    curY += (18.0f * uiScale);

                    if (cat.contains("entries") && cat["entries"].is_array())
                    {
                        for (const auto& ent : cat["entries"])
                        {
                            std::string title = ent.value("title", "Unknown");
                            std::string subtitle = ent.value("subtitle", "");
                            std::string desc = ent.value("description", "");
                            bool unlocked = ent.value("unlocked", true);

                            SDL_FRect cardRect = { padX, curY, innerW, 42.0f * uiScale };
                            UIWidget::drawPanel(renderer, cardRect);

                            if (unlocked)
                            {
                                UIWidget::drawText(renderer, title, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                                if (!subtitle.empty())
                                {
                                    float titleW = UIWidget::getTextWidth(title, uiScale * 0.85f);
                                    UIWidget::drawText(renderer, subtitle, padX + (12.0f * uiScale) + titleW, curY + (5.0f * uiScale), Theme::colors.textAccent, uiScale * 0.72f);
                                }
                                UIWidget::drawTextWrapped(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), innerW - (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                            }
                            else
                            {
                                UIWidget::drawText(renderer, "??? [Locked Entry]", padX + (8.0f * uiScale), curY + (12.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                            }
                            curY += cardRect.h + (6.0f * uiScale);
                        }
                    }
                    curY += (8.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::SPELLS)
        {
            // Spellbook
            UIWidget::drawText(renderer, "Unlocked Arcane Spells & Abilities:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            if (data.contains("spells") && data["spells"].is_array())
            {
                for (const auto& sp : data["spells"])
                {
                    std::string sName = sp.value("name", "Spell");
                    std::string school = sp.value("school", "Arcane");
                    int mp = sp.value("mpCost", 10);
                    std::string desc = sp.value("description", "");
                    bool unlocked = sp.value("unlocked", true);

                    SDL_FRect cardRect = { padX, curY, innerW, 36.0f * uiScale };
                    UIWidget::drawPanel(renderer, cardRect);

                    if (unlocked)
                    {
                        UIWidget::drawText(renderer, std::format("{} ({} MP)", sName, mp), padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.arcane, uiScale * 0.85f);
                        UIWidget::drawText(renderer, std::format("School: {}", school), padX + innerW - (120.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                        UIWidget::drawText(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);
                    }
                    else
                    {
                        UIWidget::drawText(renderer, "??? [Undiscovered Spell]", padX + (8.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                    }
                    curY += cardRect.h + (6.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::PERKS)
        {
            // Perks & Talents Tree
            UIWidget::drawText(renderer, "Character Perks & Talents:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            if (data.contains("perks") && data["perks"].is_array())
            {
                for (const auto& pk : data["perks"])
                {
                    std::string pName = pk.value("name", "Perk");
                    std::string category = pk.value("category", "General");
                    std::string desc = pk.value("description", "");
                    bool active = pk.value("active", true);

                    SDL_FRect cardRect = { padX, curY, innerW, 36.0f * uiScale };
                    UIWidget::drawPanel(renderer, cardRect);

                    UIWidget::drawText(renderer, pName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), active ? Theme::colors.textGold : Theme::colors.textMuted, uiScale * 0.85f);
                    UIWidget::drawText(renderer, std::format("[{}]", category), padX + innerW - (100.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                    UIWidget::drawText(renderer, desc, padX + (8.0f * uiScale), curY + (18.0f * uiScale), active ? Theme::colors.textPrimary : Theme::colors.textMuted, uiScale * 0.78f);

                    curY += cardRect.h + (6.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::MAPS)
        {
            // World Overview Maps
            UIWidget::drawText(renderer, "Dominion Regional Map:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            SDL_FRect mapCanvas = { padX, curY, innerW, 140.0f * uiScale };
            UIWidget::drawPanel(renderer, mapCanvas, Theme::colors.bgHeader, Theme::colors.borderButton);

            UIWidget::drawText(renderer, "🗺️ [Lilaya's Laboratory Complex & Dominion Outskirts]", padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.9f);
            UIWidget::drawText(renderer, "Current Location: Sector Alpha-1 (Safe Sanctuary)", padX + (12.0f * uiScale), curY + (32.0f * uiScale), Theme::colors.companion, uiScale * 0.82f);
            UIWidget::drawText(renderer, "Connected Zones: Dominion Plaza, Arcade Alley, Subways", padX + (12.0f * uiScale), curY + (52.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);

            curY += mapCanvas.h + (10.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        CombatState* combat = dynamic_cast<CombatState*>(gameContext->getActiveState());
        if (!combat) return 0.0f;

        float startY = curY;
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, std::format("TACTICAL COMBAT (Round {})", combat->getEngine().getCurrentRound()), Theme::colors.bgHeader, Theme::colors.enemy, uiScale);
        curY += headerH + (10.0f * uiScale);

        float padX = rect.x + (12.0f * uiScale);
        float innerW = rect.w - (24.0f * uiScale);
        float halfW = (innerW - (10.0f * uiScale)) / 2.0f;
        float barH = 18.0f * uiScale;

        UIWidget::drawText(renderer, "PARTY STATUS", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);

        for (const auto& p : combat->getEngine().getPlayerParty())
        {
            if (p.character)
            {
                float hp = p.character->getStat("health");
                UIWidget::drawProgressBar(renderer, { padX, curY, halfW, barH }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("{} HP: {:.0f} (AP: {})", p.character->name, hp, p.currentAp), uiScale);
            }
        }

        for (const auto& enemyP : combat->getEngine().getEnemyParty())
        {
            if (enemyP.character)
            {
                float hp = enemyP.character->getStat("health");
                UIWidget::drawProgressBar(renderer, { padX + halfW + (10.0f * uiScale), curY, halfW, barH }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("{} HP: {:.0f}", enemyP.character->name, hp), uiScale);
            }
        }
        curY += (barH + 12.0f * uiScale);

        UIWidget::drawText(renderer, "COMBAT LOG:", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);
        for (const auto& logEntry : combat->getEngine().getCombatLog())
        {
            UIWidget::drawText(renderer, logEntry, padX, curY, Theme::colors.textSecondary, uiScale);
            curY += (16.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        encounterResolutionState* res = dynamic_cast<encounterResolutionState*>(gameContext->getActiveState());
        if (!res) return 0.0f;

        float startY = curY;
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, "POST-COMBAT RESOLUTION HUB", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (10.0f * uiScale);

        float padX = rect.x + (12.0f * uiScale);
        UIWidget::drawText(renderer, res->getResolutionLog(), padX, curY, Theme::colors.textAccent, uiScale);
        curY += (26.0f * uiScale);

        UIWidget::drawText(renderer, "DEFEATED ENEMIES AT YOUR MERCY:", padX, curY, Theme::colors.textGold, uiScale);
        curY += (20.0f * uiScale);

        const auto& records = res->getDefeatedRecords();
        size_t selected = res->getSelectedIndex();

        for (size_t i = 0; i < records.size(); ++i)
        {
            const auto& rec = records[i];
            std::string line = std::format("{} [{}] {} | Looted: {} | Stripped: {} | Sex: {} | Subjugated: {}",
                                           (i == selected ? "->" : "  "), i,
                                           (rec.npc ? rec.npc->name : "Enemy"),
                                           rec.isLooted ? "Yes" : "No",
                                           rec.isStripped ? "Yes" : "No",
                                           rec.hadSex ? "Yes" : "No",
                                           rec.isSubjugated ? "Yes" : "No");

            SDL_Color c = (i == selected) ? Theme::colors.textGold : Theme::colors.textPrimary;
            UIWidget::drawText(renderer, line, padX, curY, c, uiScale);
            curY += (18.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = rect.x + (8.0f * uiScale);
        float availableW = rect.w - (16.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // 1. Header
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "BACKPACK & STORAGE", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.85f);
        curY += headerH + (8.0f * uiScale);

        // 2. Category Tabs: [ All ] [ Gear ] [ Clothes ] [ Consumable ] [ Piercings ] [ Misc ]
        static int activeCategory = 0;
        static const std::vector<std::string> categories = {
            "All", "Gear", "Clothes", "Items", "Piercings", "Misc"
        };

        float tabH = 22.0f * uiScale;
        float tabW = (availableW - ((categories.size() - 1) * 3.0f * uiScale)) / static_cast<float>(categories.size());

        for (size_t t = 0; t < categories.size(); ++t)
        {
            SDL_FRect tRect = { padX + (t * (tabW + (3.0f * uiScale))), curY, tabW, tabH };
            bool isTabActive = (activeCategory == static_cast<int>(t));
            bool tHov = (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w &&
                         mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);

            UIWidget::drawButton(renderer, tRect, categories[t], tHov, true, isTabActive, uiScale * 0.70f);

            if (tHov && clicked)
            {
                activeCategory = static_cast<int>(t);
                gameContext->input.consumeMouseClick();
            }
        }
        curY += tabH + (8.0f * uiScale);

        // 3. Backpack Item Cards List
        float cardRowH = 28.0f * uiScale;
        auto backpack = gameContext->getPlayerInventoryStacked();

        // Filter items based on active category
        struct FilteredItem {
            size_t originalIndex;
            InventorySlot slot;
        };
        std::vector<FilteredItem> filteredItems;

        for (size_t i = 0; i < backpack.size(); ++i)
        {
            if (!backpack[i].itemPtr) continue;
            const auto& it = backpack[i].itemPtr;

            bool matches = false;
            switch (activeCategory)
            {
                case 0: // All
                    matches = true;
                    break;
                case 1: // Gear
                    matches = it->isEquippable && (it->targetSlot == equipSlot::WEAPON_MAIN || it->targetSlot == equipSlot::WEAPON_OFF || it->targetSlot == equipSlot::TORSO_OVER || it->targetSlot == equipSlot::HEADWEAR);
                    break;
                case 2: // Clothes
                    matches = it->isEquippable && (!it->supportedDisplacements.empty() || it->targetSlot == equipSlot::TORSO_UNDER || it->targetSlot == equipSlot::CHEST_WEAR || it->targetSlot == equipSlot::LEGS_OUTER || it->targetSlot == equipSlot::GROIN_OVER || it->targetSlot == equipSlot::CALVES || it->targetSlot == equipSlot::FEET);
                    break;
                case 3: // Items / Consumables
                    matches = it->isConsumable;
                    break;
                case 4: // Piercings / Jewelry
                    matches = it->isEquippable && (it->targetSlot == equipSlot::PIERCING_EAR || it->targetSlot == equipSlot::PIERCING_NOSE || it->targetSlot == equipSlot::PIERCING_LIP || it->targetSlot == equipSlot::PIERCING_TONGUE || it->targetSlot == equipSlot::PIERCING_NIPPLE || it->targetSlot == equipSlot::PIERCING_NAVEL || it->targetSlot == equipSlot::PIERCING_COCK || it->targetSlot == equipSlot::PIERCING_VAGINA || it->targetSlot == equipSlot::FINGER_PRIMARY || it->targetSlot == equipSlot::NECKWEAR || it->targetSlot == equipSlot::WRISTS);
                    break;
                case 5: // Misc
                    matches = !it->isEquippable && !it->isConsumable;
                    break;
            }

            if (matches) filteredItems.push_back({ i, backpack[i] });
        }

        if (filteredItems.empty())
        {
            SDL_FRect emptyBox = { padX, curY, availableW, 36.0f * uiScale };
            UIWidget::drawPanel(renderer, emptyBox, Theme::colors.bgDark, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "No items in this category.", padX + (8.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
            curY += 44.0f * uiScale;
        }
        else
        {
            for (size_t f = 0; f < filteredItems.size() && f < 8; ++f)
            {
                size_t origIdx = filteredItems[f].originalIndex;
                const auto& itemSlot = filteredItems[f].slot;

                SDL_FRect itemRect = { padX, curY, availableW, cardRowH };
                bool isSelected = (gameContext->selectedInventorySide == 0 && gameContext->selectedInventoryIndex == static_cast<int>(origIdx));
                bool iHov = (mousePos.x >= itemRect.x && mousePos.x <= itemRect.x + itemRect.w &&
                             mousePos.y >= itemRect.y && mousePos.y <= itemRect.y + itemRect.h);

                SDL_Color bg = isSelected ? Theme::colors.bgHeader : (iHov ? Theme::colors.bgHeader : Theme::colors.bgSlot);
                SDL_Color bd = isSelected ? Theme::colors.borderSelected : (iHov ? Theme::colors.borderButton : Theme::colors.borderNormal);
                UIWidget::drawPanel(renderer, itemRect, bg, bd);

                // Count Badge
                std::string countStr = std::format("{}x", itemSlot.totalCount);
                UIWidget::drawText(renderer, countStr, padX + (6.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);

                // Item Name
                float cW = UIWidget::getTextWidth(countStr, uiScale * 0.72f);
                UIWidget::drawText(renderer, itemSlot.itemPtr->name, padX + cW + (12.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

                // Value
                std::string valStr = std::format("{} ¤", itemSlot.itemPtr->baseValue);
                float valW = UIWidget::getTextWidth(valStr, uiScale * 0.70f);
                UIWidget::drawText(renderer, valStr, padX + availableW - valW - (6.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.currency, uiScale * 0.70f);

                if (iHov && clicked)
                {
                    gameContext->selectedInventorySide = 0;
                    gameContext->selectedInventoryIndex = static_cast<int>(origIdx);
                    gameContext->selectedEquipmentSlot = equipSlot::NONE;
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }

                curY += cardRowH + (3.0f * uiScale);
            }
        }

        // 4. Ground / Tile Items Section
        curY += (6.0f * uiScale);
        float gHeaderH = 22.0f * uiScale;
        SDL_FRect gHeaderRect = { padX, curY, availableW, gHeaderH };
        UIWidget::drawHeader(renderer, gHeaderRect, "ITEMS ON GROUND", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.75f);
        curY += gHeaderH + (6.0f * uiScale);

        auto ground = gameContext->getTileInventoryStacked();
        if (ground.empty())
        {
            SDL_FRect emptyGround = { padX, curY, availableW, 30.0f * uiScale };
            UIWidget::drawPanel(renderer, emptyGround, Theme::colors.bgDark, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "No items dropped here.", padX + (8.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
            curY += 36.0f * uiScale;
        }
        else
        {
            for (size_t g = 0; g < ground.size() && g < 4; ++g)
            {
                if (!ground[g].itemPtr) continue;

                SDL_FRect gRect = { padX, curY, availableW, cardRowH };
                bool isSelected = (gameContext->selectedInventorySide == 1 && gameContext->selectedInventoryIndex == static_cast<int>(g));
                bool gHov = (mousePos.x >= gRect.x && mousePos.x <= gRect.x + gRect.w &&
                             mousePos.y >= gRect.y && mousePos.y <= gRect.y + gRect.h);

                SDL_Color bg = isSelected ? Theme::colors.bgHeader : (gHov ? Theme::colors.bgHeader : Theme::colors.bgSlot);
                SDL_Color bd = isSelected ? Theme::colors.borderSelected : (gHov ? Theme::colors.borderButton : Theme::colors.borderNormal);
                UIWidget::drawPanel(renderer, gRect, bg, bd);

                std::string countStr = std::format("{}x", ground[g].totalCount);
                UIWidget::drawText(renderer, countStr, padX + (6.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.companion, uiScale * 0.72f);
                float cW = UIWidget::getTextWidth(countStr, uiScale * 0.72f);
                UIWidget::drawText(renderer, ground[g].itemPtr->name, padX + cW + (12.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

                if (gHov && clicked)
                {
                    gameContext->selectedInventorySide = 1;
                    gameContext->selectedInventoryIndex = static_cast<int>(g);
                    gameContext->selectedEquipmentSlot = equipSlot::NONE;
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }

                curY += cardRowH + (3.0f * uiScale);
            }
        }

        return (curY - startY);
    }

    float renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        float startY = curY;
        float padX = rect.x + (14.0f * uiScale);
        float innerW = rect.w - (28.0f * uiScale);

        if (gameContext->isPhoneMenuOpen)
        {
            float headerH = 26.0f * uiScale;
            SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
            UIWidget::drawHeader(renderer, headerRect, "ARCANE COMMUNICATOR & ENCYCLOPEDIA", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
            curY += headerH + (12.0f * uiScale);

            SDL_FRect cardRect = { padX, curY, innerW, 140.0f * uiScale };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float pY = curY + (10.0f * uiScale);
            float pX = padX + (12.0f * uiScale);
            float pW = innerW - (24.0f * uiScale);

            std::string p1 = "You pull out your phone and tap in the unlock code.";
            float h1 = UIWidget::drawTextWrapped(renderer, p1, pX, pY, pW, Theme::colors.textGold, uiScale * 0.88f);
            pY += h1 + (12.0f * uiScale);

            std::string p2 = "Using your powerful aura, you've managed to figure out a way to channel the arcane into charging the battery of your phone. You're using it to catalog people, items, quests, and regional maps in this strange new world.";
            float h2 = UIWidget::drawTextWrapped(renderer, p2, pX, pY, pW, Theme::colors.textPrimary, uiScale * 0.82f);
            pY += h2 + (12.0f * uiScale);

            curY += cardRect.h + (12.0f * uiScale);
            return (curY - startY);
        }

        const gameMap* m = gameContext->getActiveMap();
        std::string locTitle = "Lilaya's Home F1 — Main Corridor";
        if (m && !m->getName().empty() && m->getName() != "Enchanted Forest" && m->getName() != "District Map")
        {
            locTitle = m->getName();
        }

        // 1. Zone Header Banner
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, locTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        // 2. Main Narrative Story Card
        std::string p1 = "Immaculately-clean red carpet runs down the centre of the wide corridor which you're currently walking down, while the walls are decorated in a pale, light-blue wallpaper that sports a series of delicate white floral patterns. Fine landscape paintings and portraits of beautiful demons are hung up on the walls at regular intervals, while all manner of antique curiosities are perched upon the many wooden cabinets which line the sides of these hallways.";
        std::string p2 = "As it's currently night time, heavy fabric curtains have been drawn over the corridor's large glass windows, leaving the area to be illuminated by the many arcane-powered wall-lights.";
        std::string p3 = "This corridor is quiet at the moment. You can explore adjacent wings of the manor or check your inventory and status using the action commands below.";

        SDL_FRect cardRect = { padX, curY, innerW, 200.0f * uiScale };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float pY = curY + (12.0f * uiScale);
        float pX = padX + (14.0f * uiScale);
        float pW = innerW - (28.0f * uiScale);

        float h1 = UIWidget::drawTextWrapped(renderer, p1, pX, pY, pW, Theme::colors.textPrimary, uiScale * 0.84f);
        pY += h1 + (12.0f * uiScale);

        float h2 = UIWidget::drawTextWrapped(renderer, p2, pX, pY, pW, Theme::colors.textPrimary, uiScale * 0.84f);
        pY += h2 + (12.0f * uiScale);

        float h3 = UIWidget::drawTextWrapped(renderer, p3, pX, pY, pW, Theme::colors.textSecondary, uiScale * 0.82f);
        pY += h3 + (12.0f * uiScale);

        cardRect.h = (pY - curY) + (4.0f * uiScale);
        curY += cardRect.h + (12.0f * uiScale);

        return (curY - startY);
    }

    float renderShopView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        float startY = curY;
        float padX = rect.x + (16.0f * uiScale);
        float availableW = rect.w - (32.0f * uiScale);

        float headerH = 28.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, "ENCHANTED APOTHECARY & SHOP", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        float greetH = UIWidget::drawTextWrapped(renderer, "\"Welcome, traveler. Browse my finest potions, mystical essences, and enchanted wares. Select an item from the command grid to purchase.\"", padX, curY, availableW, Theme::colors.textAccent, uiScale);
        curY += greetH + (14.0f * uiScale);

        if (auto shop = dynamic_cast<shopState*>(gameContext->getActiveState()))
        {
            const auto& catalog = shop->getCatalog();
            for (size_t i = 0; i < catalog.size(); ++i)
            {
                SDL_FRect itemRect = { padX, curY, availableW, 30.0f * uiScale };
                UIWidget::drawPanel(renderer, itemRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, std::format("[{}] {} ({})", i, catalog[i].name, catalog[i].type), padX + 8.0f * uiScale, curY + 6.0f * uiScale, Theme::colors.textPrimary, uiScale);
                UIWidget::drawText(renderer, std::format("Price: {}¤ | Stock: {}", catalog[i].price, catalog[i].stock), padX + availableW - 130.0f * uiScale, curY + 6.0f * uiScale, Theme::colors.textGold, uiScale);
                curY += (34.0f * uiScale);
            }
        }

        return (curY - startY);
    }

    float renderTransformationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        return TransformationView::render(renderer, gameContext, rect, curY, uiScale);
    }

    float renderEnchantingView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        float startY = curY;
        float padX = rect.x + (14.0f * uiScale);
        float availableW = rect.w - (28.0f * uiScale);

        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, "ENCHANTING & INFUSION ALTAR", Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (10.0f * uiScale);

        float halfW = (availableW - (12.0f * uiScale)) / 2.0f;

        // 1. Primary & Secondary Modifier Headers & Grids
        UIWidget::drawText(renderer, "Primary Modifier", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
        UIWidget::drawText(renderer, "Secondary Modifier", padX + halfW + (12.0f * uiScale), curY, Theme::colors.textAccent, uiScale * 0.95f);
        curY += (18.0f * uiScale);

        // Primary Modifiers Matrix (Left)
        float btnSize = 22.0f * uiScale;
        float gap = 4.0f * uiScale;
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 8; ++c)
            {
                SDL_FRect mRect = { padX + (c * (btnSize + gap)), curY + (r * (btnSize + gap)), btnSize, btnSize };
                bool isSelected = (r == 0 && c == 0);
                UIWidget::drawPanel(renderer, mRect, isSelected ? Theme::colors.lust : Theme::colors.bgSlot, isSelected ? Theme::colors.borderButton : Theme::colors.borderNormal);
            }
        }

        // Secondary Modifiers Matrix (Right)
        float secX = padX + halfW + (12.0f * uiScale);
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 8; ++c)
            {
                SDL_FRect mRect = { secX + (c * (btnSize + gap)), curY + (r * (btnSize + gap)), btnSize, btnSize };
                bool isSelected = (r == 1 && c == 1);
                UIWidget::drawPanel(renderer, mRect, isSelected ? Theme::colors.arcane : Theme::colors.bgSlot, isSelected ? Theme::colors.textGold : Theme::colors.borderNormal);
            }
        }

        curY += (3 * (btnSize + gap)) + (12.0f * uiScale);

        // 2. Strength Selector Buttons Row
        static constexpr std::string_view tiers[] = { "Major Drain", "Drain", "Minor Drain", "Minor Boost", "Boost", "Major Boost" };
        float tierW = (availableW - (gap * 5)) / 6.0f;
        float tierH = 22.0f * uiScale;

        for (size_t i = 0; i < std::size(tiers); ++i)
        {
            SDL_FRect tRect = { padX + (i * (tierW + gap)), curY, tierW, tierH };
            bool isSelected = (i == 5); // Major Boost
            UIWidget::drawButton(renderer, tRect, std::string(tiers[i]), isSelected, true, isSelected, uiScale * 0.8f);
        }
        curY += tierH + (10.0f * uiScale);

        // 3. Effect To Be Added Banner & Add Button
        SDL_FRect addBarRect = { padX, curY, availableW, 26.0f * uiScale };
        UIWidget::drawPanel(renderer, addBarRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Effect to be added: Huge increase in milk regeneration.", padX + (8.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.lust, uiScale * 0.9f);

        float addBtnW = 75.0f * uiScale;
        SDL_FRect addBtnRect = { padX + availableW - addBtnW - (4.0f * uiScale), curY + (3.0f * uiScale), addBtnW, 20.0f * uiScale };
        UIWidget::drawButton(renderer, addBtnRect, "Add | 13*", false, true, false, uiScale * 0.8f);
        curY += addBarRect.h + (12.0f * uiScale);

        // 4. Recipe Craft Container (Input, Name + Effects List, Output)
        SDL_FRect recipeRect = { padX, curY, availableW, 110.0f * uiScale };
        UIWidget::drawPanel(renderer, recipeRect, Theme::colors.bgDark, Theme::colors.borderButton);

        // Input Slot
        UIWidget::drawText(renderer, "Input (x1)", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.85f);
        SDL_FRect inSlotRect = { padX + (10.0f * uiScale), curY + (24.0f * uiScale), 48.0f * uiScale, 48.0f * uiScale };
        UIWidget::drawPanel(renderer, inSlotRect, Theme::colors.bgHeader, Theme::colors.borderButton);
        UIWidget::drawText(renderer, "ELIXIR", inSlotRect.x + (6.0f * uiScale), inSlotRect.y + (16.0f * uiScale), Theme::colors.textGold, uiScale * 0.8f);

        // Effects Middle List
        float midX = padX + (70.0f * uiScale);
        float midW = availableW - (150.0f * uiScale);
        UIWidget::drawText(renderer, "Effects (5/100) | Cost: 46*", midX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        SDL_FRect nameInputRect = { midX, curY + (22.0f * uiScale), midW, 20.0f * uiScale };
        UIWidget::drawPanel(renderer, nameInputRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Bovine elixir", nameInputRect.x + (6.0f * uiScale), nameInputRect.y + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        static constexpr std::string_view appliedEffects[] = {
            "Bovine tail transformation.",
            "Bovine ears transformation.",
            "Grows curved horns.",
            "Huge increase in breast size. (+3 breast size.)"
        };

        float effY = curY + (46.0f * uiScale);
        for (const auto& eff : appliedEffects)
        {
            UIWidget::drawText(renderer, std::format("• {}", eff), midX, effY, Theme::colors.textPrimary, uiScale * 0.8f);
            effY += (14.0f * uiScale);
        }

        // Output Slot
        float outX = padX + availableW - (65.0f * uiScale);
        UIWidget::drawText(renderer, "Output", outX + (4.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textAccent, uiScale * 0.85f);
        SDL_FRect outSlotRect = { outX, curY + (24.0f * uiScale), 48.0f * uiScale, 48.0f * uiScale };
        UIWidget::drawPanel(renderer, outSlotRect, Theme::colors.bgHeader, Theme::colors.lust);
        UIWidget::drawText(renderer, "RUNIC", outSlotRect.x + (6.0f * uiScale), outSlotRect.y + (16.0f * uiScale), Theme::colors.lust, uiScale * 0.8f);

        curY += recipeRect.h + (8.0f * uiScale);
        return (curY - startY);
    }
}
