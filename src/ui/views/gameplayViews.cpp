#include "ui/views/gameplayViews.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "ui/tooltipManager.h"
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
#include "quest/questDatabase.h"
#include "items/itemDatabase.h"
#include "items/merchantValuation.h"
#include "ui/views/transformationView.h"
#include <iostream>
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
        UIWidget::drawHeader(renderer, headerRect, std::format("📱 PHONE - {}", appTitle), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (10.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Feedback notification banner
        if (!app->getFeedbackText().empty())
        {
            float fbH = 28.0f * uiScale;
            SDL_FRect fbRect = { padX, curY, innerW, fbH };
            UIWidget::drawPanel(renderer, fbRect, Theme::colors.bgHeader, Theme::colors.borderSelected);
            UIWidget::drawText(renderer, "💬 " + app->getFeedbackText(), padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
            curY += fbH + (10.0f * uiScale);
        }

        const auto& data = app->getAppData();

        if (app->getAppMode() == PhoneAppMode::HOME)
        {
            // 1. Status Bar Card
            float barCardH = 34.0f * uiScale;
            SDL_FRect barRect = { padX, curY, innerW, barCardH };
            UIWidget::drawPanel(renderer, barRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            std::string timeStr = std::format("📅 Day {}, {:02d}:{:02d}", gameContext->gameTime.day, gameContext->gameTime.hour, gameContext->gameTime.minute);
            UIWidget::drawText(renderer, timeStr, padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
            std::string signalStr = "Signal: Arcane 5G [█████]   Battery: 98% ⚡";
            float sigW = UIWidget::getTextWidth(signalStr, uiScale * 0.78f);
            UIWidget::drawText(renderer, signalStr, padX + innerW - sigW - (10.0f * uiScale), curY + (9.0f * uiScale), Theme::colors.companion, uiScale * 0.78f);
            curY += barCardH + (10.0f * uiScale);

            // 2. Active Notification / Ticker Card
            entity* p = gameContext->getPlayer();
            float notifH = 50.0f * uiScale;
            SDL_FRect notifRect = { padX, curY, innerW, notifH };
            UIWidget::drawPanel(renderer, notifRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "NOTIFICATIONS & STATUS", padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.72f);
            std::string notif1 = "• Active Quest: Canis Root Delivery — Chapter 1: The Outskirts";
            UIWidget::drawText(renderer, notif1, padX + (10.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.80f);
            std::string notif2 = p ? std::format("• Vitals: {:.0f}/{:.0f} HP | {:.0f}/{:.0f} MP | Lust: {:.0f}% | Arousal: {:.0f}%",
                p->getStat("health"), std::max(100.0f, p->getStat("max_health")),
                p->getStat("mana"), std::max(80.0f, p->getStat("max_mana")),
                p->getStat("lust"), p->getStat("arousal")) : "";
            UIWidget::drawText(renderer, notif2, padX + (10.0f * uiScale), curY + (34.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
            curY += notifH + (12.0f * uiScale);

            // 3. Installed Applications Directory
            UIWidget::drawText(renderer, "INSTALLED APPLICATIONS & IN-GAME UTILITIES", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
            curY += (18.0f * uiScale);

            struct AppSummary {
                std::string num;
                std::string name;
                std::string desc;
            };
            static const std::vector<AppSummary> apps = {
                { "1", "Quests", "Quest log, active chapters, objective checklist, and rewards." },
                { "2", "Perk Tree", "Browse and allocate talents, character perks, and passive traits." },
                { "3", "Spells", "Review elemental spellbooks, damage types, and out-of-combat casting." },
                { "4", "Fetishes", "Configure 5-tier desire ratings and intimacy preferences." },
                { "5", "Stats", "Detailed diagnostics across Core, Body, Sex, and Pregnancy." },
                { "6", "Selfie", "Detailed character prose description, bodily inspection, and complete stats matrix." },
                { "7", "Contacts", "Directory of known characters, companions, and relationship meters." },
                { "8", "Encyclopedia", "Comprehensive compendium of species, weapons, garments, and items." },
                { "9", "Transform", "Sculpt mutations, bodily morphs, appendages, and racial traits." },
                { "10", "Maps", "Regional overviews, landmarks, coordinates, and local radar." },
                { "11", "Combat Moves", "Prepare and organize active combat action deck slots." },
                { "12", "Masturbate", "Take a secluded moment to build arousal and achieve release." },
                { "13", "Wait / Rest", "Pass in-game time in intervals, sleep, and advance the day cycle." },
                { "14", "Elemental", "Manifest, converse with, and command your elemental companion." }
            };

            for (const auto& a : apps)
            {
                std::string line = std::format("[{}] {} — {}", a.num, a.name, a.desc);
                UIWidget::drawText(renderer, line, padX + (6.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.78f);
                curY += (17.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::WAIT_REST)
        {
            UIWidget::drawText(renderer, "Rest & Recuperation:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            std::string phaseStr = (gameContext->gameTime.getPhase() == TimePhase::NIGHT) ? "Night" :
                                   (gameContext->gameTime.getPhase() == TimePhase::DAWN) ? "Dawn" :
                                   (gameContext->gameTime.getPhase() == TimePhase::DUSK) ? "Dusk" : "Day";
            std::string timeInfo = std::format("Current Time: Day {}, {:02d}:{:02d} ({})",
                gameContext->gameTime.day, gameContext->gameTime.hour, gameContext->gameTime.minute, phaseStr);
            UIWidget::drawText(renderer, timeInfo, padX, curY, Theme::colors.textAccent, uiScale * 0.88f);
            curY += (20.0f * uiScale);

            std::string statInfo = p ? std::format("Current Vitals: {:.0f}/{:.0f} Health | {:.0f}/{:.0f} Mana",
                p->getStat("health"), std::max(100.0f, p->getStat("max_health")),
                p->getStat("mana"), std::max(80.0f, p->getStat("max_mana"))) : "";
            UIWidget::drawText(renderer, statInfo, padX, curY, Theme::colors.friendly, uiScale * 0.85f);
            curY += (22.0f * uiScale);

            // Information Box explaining Resting benefits and directing to Action Grid
            float infoBoxH = 80.0f * uiScale;
            SDL_FRect infoBoxRect = { padX, curY, innerW, infoBoxH };
            UIWidget::drawPanel(renderer, infoBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float iy = curY + (10.0f * uiScale);
            UIWidget::drawText(renderer, "Recuperation Guidelines:", padX + (12.0f * uiScale), iy, Theme::colors.textGold, uiScale * 0.85f);
            iy += (18.0f * uiScale);

            std::string pDesc = "Resting naturally regenerates Health and Mana over time, while advancing world events and merchant restocking cycles. Longer periods of sleep fully restore your reserves.\n\nChoose your desired resting duration using the Action Grid buttons below.";
            UIWidget::drawTextWrapped(renderer, pDesc, padX + (12.0f * uiScale), iy, innerW - (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.80f);
            curY += infoBoxH + (16.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::MASTURBATE)
        {
            UIWidget::drawText(renderer, "Solo Intimacy & Self-Pleasure:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            float curLust = p ? p->getStat("lust") : 0.0f;
            float curArousal = p ? p->getStat("arousal") : 0.0f;

            // Gauges
            float barW = std::min(innerW, 580.0f * uiScale);
            float barX = rect.x + (rect.w - barW) / 2.0f;
            UIWidget::drawText(renderer, std::format("Lust: {:.0f}%", curLust), barX, curY, Theme::colors.lust, uiScale * 0.85f);
            curY += (18.0f * uiScale);
            SDL_FRect lustBarRect = { barX, curY, barW, 12.0f * uiScale };
            UIWidget::drawProgressBar(renderer, lustBarRect, curLust, 100.0f, Theme::colors.lust, Theme::colors.bgSlot, "", uiScale * 0.75f);
            curY += (18.0f * uiScale);

            UIWidget::drawText(renderer, std::format("Arousal: {:.0f}%", curArousal), barX, curY, Theme::colors.textAccent, uiScale * 0.85f);
            curY += (18.0f * uiScale);
            SDL_FRect arousalBarRect = { barX, curY, barW, 12.0f * uiScale };
            UIWidget::drawProgressBar(renderer, arousalBarRect, curArousal, 100.0f, Theme::colors.textAccent, Theme::colors.bgSlot, "", uiScale * 0.75f);
            curY += (24.0f * uiScale);

            std::string intro = "Finding a quiet, secluded alcove away from prying eyes, you allow your thoughts to wander toward sensual indulgence. Warmth steadily tingles across your skin as your fingers explore sensitive curves and private contours.";
            float iH = UIWidget::drawTextWrapped(renderer, intro, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += iH + (14.0f * uiScale);

            // Action Buttons
            float actBoxW = std::min(innerW, 640.0f * uiScale);
            float actBoxX = rect.x + (rect.w - actBoxW) / 2.0f;
            float actBoxH = 44.0f * uiScale;
            SDL_FRect actBoxRect = { actBoxX, curY, actBoxW, actBoxH };
            UIWidget::drawPanel(renderer, actBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            struct MasturbateAct {
                std::string label;
                std::function<void()> act;
                bool enabled;
            };
            std::vector<MasturbateAct> mActs = {
                { "Caress Chest", [=]() { if (p) p->stats.setBaseStat("arousal", std::min(100.0f, p->getStat("arousal") + 15.0f)); app->setFeedbackText("Gently stroked sensitive chest (+15% Arousal)."); }, true },
                { "Fondle Groin", [=]() { if (p) p->stats.setBaseStat("arousal", std::min(100.0f, p->getStat("arousal") + 25.0f)); app->setFeedbackText("Directly stimulated intimate areas (+25% Arousal)."); }, true },
                { "Tease Climax", [=]() { if (p) p->stats.setBaseStat("arousal", std::max(85.0f, p->getStat("arousal") + 20.0f)); app->setFeedbackText("Edged close to orgasm (Arousal 85%+)."); }, true },
                { "Climax & Relief", [=]() {
                    if (p) {
                        p->stats.setBaseStat("arousal", 0.0f);
                        p->stats.setBaseStat("lust", 0.0f);
                        p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 20.0f));
                    }
                    app->setFeedbackText("Overwhelming wave of climax crashes through you! Lust and Arousal reset to 0% (+20 MP).");
                }, curArousal >= 60.0f }
            };

            float mPad = 5.0f * uiScale;
            float mGap = 5.0f * uiScale;
            float mBtnH = actBoxH - (mPad * 2.0f);
            float mBtnW = (actBoxW - (mPad * 2.0f) - (mGap * (mActs.size() - 1))) / static_cast<float>(mActs.size());

            for (size_t i = 0; i < mActs.size(); ++i)
            {
                SDL_FRect bR = { actBoxX + mPad + i * (mBtnW + mGap), actBoxRect.y + mPad, mBtnW, mBtnH };
                bool hov = (mousePos.x >= bR.x && mousePos.x <= bR.x + bR.w &&
                            mousePos.y >= bR.y && mousePos.y <= bR.y + bR.h);
                if (hov && clicked && mActs[i].enabled)
                {
                    mActs[i].act();
                    gameContext->input.consumeMouseClick();
                    gameContext->refreshActionGrid();
                }
                UIWidget::drawButton(renderer, bR, mActs[i].label, hov, mActs[i].enabled, false, uiScale * 0.78f);
            }
            curY += actBoxH + (18.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::TRANSFORM)
        {
            UIWidget::drawText(renderer, "Biological Transformations & Morphology:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            if (p)
            {
                std::string raceStr = p->anatomy.getRacialTitle().empty() ? "Human" : p->anatomy.getRacialTitle();
                UIWidget::drawText(renderer, std::format("Primary Morphology: {}", raceStr), padX, curY, Theme::colors.arcane, uiScale * 0.90f);
                curY += (20.0f * uiScale);

                std::vector<std::string> morphFeatures;
                const bodyPart* h = p->anatomy.getPart(bodySlot::HORNS);
                if (h) morphFeatures.push_back(std::format("Horns: {:.0f}cm length ({})", h->length, h->race));
                else morphFeatures.push_back("Horns: None");

                const bodyPart* w = p->anatomy.getPart(bodySlot::WINGS);
                if (w) morphFeatures.push_back(std::format("Wings: {:.0f}cm span ({})", w->length, w->race));
                else morphFeatures.push_back("Wings: None");

                const bodyPart* t = p->anatomy.getPart(bodySlot::TAIL);
                if (t) morphFeatures.push_back(std::format("Tail: {} ({})", t->race, t->style.empty() ? "Flexible" : t->style));
                else morphFeatures.push_back("Tail: None");

                const bodyPart* e = p->anatomy.getPart(bodySlot::EARS);
                if (e) morphFeatures.push_back(std::format("Ears: {}", e->race));

                const bodyPart* br = p->anatomy.getPart(bodySlot::BREASTS);
                if (br)
                {
                    std::string cup = bodyPart::getCupSizeName(br->cupSize);
                    morphFeatures.push_back(std::format("Breasts: {} Cup (Milk: {:.0f}ml)", cup, br->currentFluidMl));
                }

                if (p->anatomy.hasVagina()) morphFeatures.push_back("Genitalia: Vagina");
                if (p->anatomy.hasPenis())
                {
                    const bodyPart* gr = p->anatomy.getPart(bodySlot::GROIN);
                    float pLen = gr ? gr->length : 15.0f;
                    morphFeatures.push_back(std::format("Genitalia: Penis ({:.0f}cm length)", pLen));
                }

                for (const auto& feat : morphFeatures)
                {
                    UIWidget::drawText(renderer, "• " + feat, padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                    curY += (18.0f * uiScale);
                }

                curY += (10.0f * uiScale);
                std::string morphNote = "Morphological changes stabilize dynamically as alchemical elixirs or arcane transformations take effect.";
                float mH = UIWidget::drawTextWrapped(renderer, morphNote, padX, curY, innerW, Theme::colors.textSecondary, uiScale * 0.82f);
                curY += mH + (14.0f * uiScale);

                // Button to open full studio
                float tfBtnW = 240.0f * uiScale;
                SDL_FRect tfBtnRect = { padX, curY, tfBtnW, 32.0f * uiScale };
                bool tfHov = (mousePos.x >= tfBtnRect.x && mousePos.x <= tfBtnRect.x + tfBtnRect.w &&
                              mousePos.y >= tfBtnRect.y && mousePos.y <= tfBtnRect.y + tfBtnRect.h);
                if (tfHov && clicked)
                {
                    gameContext->changeState(std::make_unique<transformationState>());
                    gameContext->input.consumeMouseClick();
                }
                UIWidget::drawButton(renderer, tfBtnRect, "Open Full Transformation Studio", tfHov, true, false, uiScale * 0.82f);
                curY += tfBtnRect.h + (16.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::STATS)
        {
            // 4 Tabs: Core, Body, Sex, Pregnancy
            float topBoxW = std::min(innerW, 580.0f * uiScale);
            float topBoxX = rect.x + (rect.w - topBoxW) / 2.0f;
            float topBoxH = 40.0f * uiScale;
            SDL_FRect topBoxRect = { topBoxX, curY, topBoxW, topBoxH };
            UIWidget::drawPanel(renderer, topBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            static const std::vector<std::string> statTabs = { "Core Stats", "Body Stats", "Sex Stats", "Pregnancy" };
            float padBox = 5.0f * uiScale;
            float btnGap = 5.0f * uiScale;
            float btnH = topBoxH - (padBox * 2.0f);
            float btnW = (topBoxW - (padBox * 2.0f) - (btnGap * (statTabs.size() - 1))) / static_cast<float>(statTabs.size());

            int activeTab = app->getStatsTab();
            for (size_t i = 0; i < statTabs.size(); ++i)
            {
                SDL_FRect btnR = { topBoxX + padBox + i * (btnW + btnGap), topBoxRect.y + padBox, btnW, btnH };
                bool hov = (mousePos.x >= btnR.x && mousePos.x <= btnR.x + btnR.w &&
                            mousePos.y >= btnR.y && mousePos.y <= btnR.y + btnR.h);
                if (hov && clicked)
                {
                    app->setStatsTab(static_cast<int>(i));
                    gameContext->input.consumeMouseClick();
                    gameContext->refreshActionGrid();
                }
                UIWidget::drawButton(renderer, btnR, statTabs[i], hov, true, activeTab == static_cast<int>(i), uiScale * 0.80f);
            }
            curY += topBoxH + (14.0f * uiScale);

            entity* p = gameContext->getPlayer();
            if (p)
            {
                if (activeTab == 0) // Core Stats
                {
                    UIWidget::drawText(renderer, "Primary Attributes:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);

                    std::vector<std::pair<std::string, float>> statList = {
                        { "Strength", p->getStat("strength") },
                        { "Agility", p->getStat("agility") },
                        { "Toughness", p->getStat("toughness") },
                        { "Intelligence", p->getStat("intelligence") },
                        { "Willpower", p->getStat("willpower") },
                        { "Libido", p->getStat("libido") },
                        { "Allure", p->getStat("allure") },
                        { "Sensitivity", p->getStat("sensitivity") }
                    };

                    for (size_t i = 0; i < statList.size(); i += 2)
                    {
                        std::string col1 = std::format("{:<14}: {:.0f}", statList[i].first, statList[i].second);
                        UIWidget::drawText(renderer, col1, padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                        if (i + 1 < statList.size())
                        {
                            std::string col2 = std::format("{:<14}: {:.0f}", statList[i+1].first, statList[i+1].second);
                            UIWidget::drawText(renderer, col2, padX + (innerW / 2.0f), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                        }
                        curY += (18.0f * uiScale);
                    }

                    curY += (10.0f * uiScale);
                    UIWidget::drawText(renderer, "Vitals & Combat Pools:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);
                    std::string vitals = std::format("HP: {:.0f}/{:.0f}  |  MP: {:.0f}/{:.0f}  |  Lust: {:.0f}%  |  Corruption: {:.0f}%",
                        p->getStat("health"), std::max(100.0f, p->getStat("max_health")),
                        p->getStat("mana"), std::max(80.0f, p->getStat("max_mana")),
                        p->getStat("lust"), p->getStat("corruption"));
                    UIWidget::drawText(renderer, vitals, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                    curY += (20.0f * uiScale);
                }
                else if (activeTab == 1) // Body Stats
                {
                    UIWidget::drawText(renderer, "Anatomical Measurements:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);

                    std::string bDim = std::format("Body Size: {} | Muscle Tone: {} | Height: {:.2f}m",
                        p->anatomy.bodySize, p->anatomy.muscleTone, p->anatomy.heightMeters);
                    UIWidget::drawText(renderer, bDim, padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                    curY += (18.0f * uiScale);

                    const bodyPart* br = p->anatomy.getPart(bodySlot::BREASTS);
                    std::string brInfo = br ? std::format("Breasts: {} Cup  |  Lactation: {:.0f}ml", bodyPart::getCupSizeName(br->cupSize), br->currentFluidMl) : "Breasts: None";
                    UIWidget::drawText(renderer, brInfo, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                    curY += (18.0f * uiScale);

                    const bodyPart* as = p->anatomy.getPart(bodySlot::ASS);
                    std::string asInfo = as ? std::format("Ass Capacity: {:.0f}ml  |  Elasticity: {:.0f}", as->orifice.maxCapacityMl, as->orifice.elasticity) : "Ass: Standard";
                    UIWidget::drawText(renderer, asInfo, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                    curY += (18.0f * uiScale);

                    if (p->anatomy.hasPenis())
                    {
                        const bodyPart* gr = p->anatomy.getPart(bodySlot::GROIN);
                        float pLen = gr ? gr->length : 15.0f;
                        UIWidget::drawText(renderer, std::format("Penis: {:.0f}cm length", pLen), padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                        curY += (18.0f * uiScale);
                    }
                    if (p->anatomy.hasVagina())
                    {
                        UIWidget::drawText(renderer, "Vagina: Present (Capacity: Standard)", padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                        curY += (18.0f * uiScale);
                    }
                    curY += (10.0f * uiScale);
                }
                else if (activeTab == 2) // Sex Stats
                {
                    UIWidget::drawText(renderer, "Lifetime Intimacy Records:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);

                    static const std::vector<std::string> sexStats = {
                        "Total Orgasms: 0",
                        "Partners Seduced: 0",
                        "Vaginal Penetrations: 0",
                        "Anal Penetrations: 0",
                        "Oral Encounters: 0",
                        "Fluids Ingested: 0 ml",
                        "Fluids Ejaculated: 0 ml"
                    };
                    for (const auto& s : sexStats)
                    {
                        UIWidget::drawText(renderer, "• " + s, padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                        curY += (18.0f * uiScale);
                    }
                    curY += (10.0f * uiScale);
                }
                else if (activeTab == 3) // Pregnancy
                {
                    UIWidget::drawText(renderer, "Gestation & Reproductive Dossier:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);

                    bool preg = p->gestation.isPregnant;
                    std::string pregStatus = preg ? "Status: Pregnant" : "Status: Not Pregnant";
                    UIWidget::drawText(renderer, pregStatus, padX + (10.0f * uiScale), curY, preg ? Theme::colors.companion : Theme::colors.textSecondary, uiScale * 0.85f);
                    curY += (18.0f * uiScale);

                    if (preg)
                    {
                        UIWidget::drawText(renderer, std::format("Partner: {} ({})", p->gestation.fatherName, p->gestation.fatherRace), padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                        curY += (18.0f * uiScale);
                        UIWidget::drawText(renderer, std::format("Days Remaining: {}  |  Expected Litter: {}", p->gestation.gestationDaysRemaining, p->gestation.litterSize), padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.82f);
                        curY += (18.0f * uiScale);
                    }
                    else
                    {
                        UIWidget::drawText(renderer, "No active gestations or egg clutches recorded.", padX + (10.0f * uiScale), curY, Theme::colors.textMuted, uiScale * 0.80f);
                        curY += (18.0f * uiScale);
                    }

                    curY += (10.0f * uiScale);
                    UIWidget::drawText(renderer, "Offspring Lineage:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                    curY += (18.0f * uiScale);
                    UIWidget::drawText(renderer, "No recorded offspring in historical archives.", padX + (10.0f * uiScale), curY, Theme::colors.textMuted, uiScale * 0.80f);
                    curY += (20.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::FETISHES)
        {
            UIWidget::drawText(renderer, "Fetishes & Desires (5-Tier Preference Ratings):", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            std::string fDesc = "Configure your character's physiological and psychological reaction to intimacy scenarios. Ratings range across Hate, Dislike, Neutral, Like, and Love.";
            float fH = UIWidget::drawTextWrapped(renderer, fDesc, padX, curY, innerW, Theme::colors.textSecondary, uiScale * 0.82f);
            curY += fH + (12.0f * uiScale);

            static const std::vector<std::pair<std::string, std::string>> fetishList = {
                { "Exhibitionism", "Sensual thrill of public nudity and being watched." },
                { "Anal", "Appreciation for rear intimacy and backdoor pleasures." },
                { "Oral", "Delight in giving and receiving oral stimulation." },
                { "Lactation", "Affinity for swollen lactating breasts and milk production." },
                { "Transformations", "Erotic fascination with bodily mutations and morphs." },
                { "Dominance", "Desire to command, control, and take the lead in intimate scenes." },
                { "Submission", "Surrendering authority and deriving pleasure from obedience." },
                { "Furry", "Attraction to beastkin ears, tails, and animalistic traits." },
                { "BDSM", "Sensory bondage, discipline, and physical restraints." },
                { "Foot Worship", "Adoration and worship of bare feet and soles." }
            };

            static const std::vector<std::pair<std::string, FetishDesireLevel>> desirePills = {
                { "Hate", FetishDesireLevel::HATE },
                { "Dislike", FetishDesireLevel::DISLIKE },
                { "Neutral", FetishDesireLevel::NEUTRAL },
                { "Like", FetishDesireLevel::LIKE },
                { "Love", FetishDesireLevel::LOVE }
            };

            for (const auto& [fName, fDetail] : fetishList)
            {
                float cardW = innerW;
                float cardH = 38.0f * uiScale;
                SDL_FRect cardR = { padX, curY, cardW, cardH };
                UIWidget::drawPanel(renderer, cardR, Theme::colors.bgSlot, Theme::colors.borderNormal);

                // Title & short desc on left
                UIWidget::drawText(renderer, fName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
                UIWidget::drawText(renderer, fDetail, padX + (8.0f * uiScale), curY + (18.0f * uiScale), Theme::colors.textMuted, uiScale * 0.72f);

                // 5 Pills on right
                float pillW = 44.0f * uiScale;
                float pillH = 22.0f * uiScale;
                float pillGap = 3.0f * uiScale;
                float pillsTotalW = (pillW * 5.0f) + (pillGap * 4.0f);
                float pillsStartX = padX + cardW - pillsTotalW - (8.0f * uiScale);
                float pillY = curY + (8.0f * uiScale);

                FetishDesireLevel curLvl = app->getFetishDesire(fName);

                for (size_t pIdx = 0; pIdx < desirePills.size(); ++pIdx)
                {
                    const auto& [pLabel, pLvl] = desirePills[pIdx];
                    SDL_FRect pillRect = { pillsStartX + pIdx * (pillW + pillGap), pillY, pillW, pillH };
                    bool isCur = (curLvl == pLvl);
                    bool hov = (mousePos.x >= pillRect.x && mousePos.x <= pillRect.x + pillRect.w &&
                                mousePos.y >= pillRect.y && mousePos.y <= pillRect.y + pillRect.h);

                    if (hov && clicked)
                    {
                        app->setFetishDesire(fName, pLvl);
                        gameContext->input.consumeMouseClick();
                        gameContext->refreshActionGrid();
                    }

                    SDL_Color borderCol = isCur ? (pLvl == FetishDesireLevel::HATE ? Theme::colors.health :
                                                   pLvl == FetishDesireLevel::DISLIKE ? Theme::colors.textAccent :
                                                   pLvl == FetishDesireLevel::NEUTRAL ? Theme::colors.borderNormal :
                                                   pLvl == FetishDesireLevel::LIKE ? Theme::colors.companion : Theme::colors.textGold)
                                                : Theme::colors.borderNormal;
                    SDL_Color bgCol = isCur ? (pLvl == FetishDesireLevel::HATE ? Theme::colors.enemy :
                                               pLvl == FetishDesireLevel::DISLIKE ? Theme::colors.borderButton :
                                               pLvl == FetishDesireLevel::NEUTRAL ? Theme::colors.bgSlotOccupied :
                                               pLvl == FetishDesireLevel::LIKE ? Theme::colors.bgButtonHover : Theme::colors.bgHeader)
                                            : (hov ? Theme::colors.bgButtonHover : Theme::colors.bgSlot);

                    UIWidget::drawPanel(renderer, pillRect, bgCol, borderCol);
                    float txtW = UIWidget::getTextWidth(pLabel, uiScale * 0.70f);
                    float txtX = pillRect.x + (pillRect.w - txtW) / 2.0f;
                    float txtY = pillRect.y + (3.0f * uiScale);
                    SDL_Color txtCol = isCur ? Theme::colors.textPrimary : Theme::colors.textSecondary;
                    UIWidget::drawText(renderer, pLabel, txtX, txtY, txtCol, uiScale * 0.70f);
                }

                curY += cardH + (6.0f * uiScale);
            }
            curY += (10.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::QUESTS)
        {
            entity* player = gameContext->getPlayer();
            std::vector<QuestDefinition> allQuests = questDatabase::getAllQuests();

            auto mousePos = gameContext->input.getMousePosition();
            bool clicked = gameContext->input.isLeftMouseJustClicked();

            // 1. Top Container Box: 3 category buttons (All, Main, Side) + 1 toggle (Completed)
            float topBoxW = std::min(innerW, 580.0f * uiScale);
            float topBoxX = rect.x + (rect.w - topBoxW) / 2.0f;
            float topBoxH = 44.0f * uiScale;
            SDL_FRect topBoxRect = { topBoxX, curY, topBoxW, topBoxH };
            UIWidget::drawPanel(renderer, topBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float padBox = 6.0f * uiScale;
            float btnGap = 6.0f * uiScale;
            float btnH = topBoxH - (padBox * 2.0f);
            float btnW = (topBoxW - (padBox * 2.0f) - (btnGap * 3.0f)) / 4.0f;

            // Button 0: All
            SDL_FRect btnAllRect = { topBoxX + padBox, topBoxRect.y + padBox, btnW, btnH };
            bool allActive = (app->getQuestCategoryFilter() == QuestCategoryFilter::ALL);
            bool allHov = (mousePos.x >= btnAllRect.x && mousePos.x <= btnAllRect.x + btnAllRect.w &&
                           mousePos.y >= btnAllRect.y && mousePos.y <= btnAllRect.y + btnAllRect.h);
            if (allHov && clicked)
            {
                app->setQuestCategoryFilter(QuestCategoryFilter::ALL);
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, btnAllRect, "All", allHov, true, allActive, uiScale * 0.82f);

            // Button 1: Main
            SDL_FRect btnMainRect = { topBoxX + padBox + (btnW + btnGap), topBoxRect.y + padBox, btnW, btnH };
            bool mainActive = (app->getQuestCategoryFilter() == QuestCategoryFilter::MAIN);
            bool mainHov = (mousePos.x >= btnMainRect.x && mousePos.x <= btnMainRect.x + btnMainRect.w &&
                            mousePos.y >= btnMainRect.y && mousePos.y <= btnMainRect.y + btnMainRect.h);
            if (mainHov && clicked)
            {
                app->setQuestCategoryFilter(QuestCategoryFilter::MAIN);
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, btnMainRect, "Main", mainHov, true, mainActive, uiScale * 0.82f);

            // Button 2: Side
            SDL_FRect btnSideRect = { topBoxX + padBox + 2.0f * (btnW + btnGap), topBoxRect.y + padBox, btnW, btnH };
            bool sideActive = (app->getQuestCategoryFilter() == QuestCategoryFilter::SIDE);
            bool sideHov = (mousePos.x >= btnSideRect.x && mousePos.x <= btnSideRect.x + btnSideRect.w &&
                            mousePos.y >= btnSideRect.y && mousePos.y <= btnSideRect.y + btnSideRect.h);
            if (sideHov && clicked)
            {
                app->setQuestCategoryFilter(QuestCategoryFilter::SIDE);
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, btnSideRect, "Side", sideHov, true, sideActive, uiScale * 0.82f);

            // Toggle 3: Completed
            SDL_FRect btnCompRect = { topBoxX + padBox + 3.0f * (btnW + btnGap), topBoxRect.y + padBox, btnW, btnH };
            bool compActive = app->isShowCompleted();
            bool compHov = (mousePos.x >= btnCompRect.x && mousePos.x <= btnCompRect.x + btnCompRect.w &&
                            mousePos.y >= btnCompRect.y && mousePos.y <= btnCompRect.y + btnCompRect.h);
            if (compHov && clicked)
            {
                app->toggleShowCompleted();
                gameContext->refreshActionGrid();
                gameContext->input.consumeMouseClick();
            }
            std::string compLabel = compActive ? "[✓] Completed" : "[ ] Completed";
            UIWidget::drawButton(renderer, btnCompRect, compLabel, compHov, true, compActive, uiScale * 0.80f);

            curY += topBoxH + (12.0f * uiScale);

            // 2. Filter Quest List
            std::vector<QuestDefinition> filteredQuests;
            for (const auto& q : allQuests)
            {
                if (!player || !player->quests.hasQuest(q.id)) continue;
                bool completed = player->quests.isCompleted(q.id);
                if (app->isShowCompleted() != completed) continue;

                bool isMain = (q.category.find("Main") != std::string::npos || q.id == "root_delivery");
                bool isSide = !isMain;

                if (app->getQuestCategoryFilter() == QuestCategoryFilter::MAIN && !isMain) continue;
                if (app->getQuestCategoryFilter() == QuestCategoryFilter::SIDE && !isSide) continue;

                filteredQuests.push_back(q);
            }

            // Sort alphabetical by name
            std::sort(filteredQuests.begin(), filteredQuests.end(), [](const QuestDefinition& a, const QuestDefinition& b) {
                return a.name < b.name;
            });

            // 3. Bottom Container Box: Quest List (Scrollable)
            float listBoxW = std::min(innerW, 760.0f * uiScale);
            float listBoxX = rect.x + (rect.w - listBoxW) / 2.0f;
            float listBoxY = curY;
            float remainingH = (rect.y + rect.h) - listBoxY - (14.0f * uiScale);
            float listBoxH = std::max(200.0f * uiScale, remainingH);
            SDL_FRect listBoxRect = { listBoxX, listBoxY, listBoxW, listBoxH };
            UIWidget::drawPanel(renderer, listBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            // Mouse wheel scroll handling
            bool mouseInList = (mousePos.x >= listBoxRect.x && mousePos.x <= listBoxRect.x + listBoxRect.w &&
                                mousePos.y >= listBoxRect.y && mousePos.y <= listBoxRect.y + listBoxRect.h);
            float wheelY = gameContext->input.getMouseWheelY();
            if (mouseInList && wheelY != 0.0f)
            {
                app->scrollQuestList(-wheelY * (36.0f * uiScale));
                gameContext->input.consumeMouseWheel();
            }

            if (filteredQuests.empty())
            {
                std::string emptyMsg = app->isShowCompleted()
                    ? "No completed quests recorded in this category."
                    : "No active quests recorded in this category.";
                float msgW = UIWidget::getTextWidth(emptyMsg, uiScale * 0.88f);
                float msgX = listBoxX + (listBoxW - msgW) / 2.0f;
                float msgY = listBoxY + (listBoxH / 2.0f) - (12.0f * uiScale);
                UIWidget::drawText(renderer, emptyMsg, msgX, msgY, Theme::colors.textSecondary, uiScale * 0.88f);

                std::string hintMsg = "Explore the realm, speak with NPCs, and inspect curiosities to uncover new rumors.";
                float hintW = UIWidget::getTextWidth(hintMsg, uiScale * 0.78f);
                float hintX = listBoxX + (listBoxW - hintW) / 2.0f;
                UIWidget::drawText(renderer, hintMsg, hintX, msgY + (24.0f * uiScale), Theme::colors.textMuted, uiScale * 0.78f);
            }
            else
            {
                // Clip rendering to inside list box
                SDL_Rect clipRect = {
                    static_cast<int>(listBoxRect.x + 2.0f * uiScale),
                    static_cast<int>(listBoxRect.y + 2.0f * uiScale),
                    static_cast<int>(listBoxRect.w - 4.0f * uiScale),
                    static_cast<int>(listBoxRect.h - 4.0f * uiScale)
                };
                SDL_SetRenderClipRect(renderer, &clipRect);

                bool hasScrollbar = (app->getQuestMaxScrollY() > 0.0f);
                float cardW = listBoxRect.w - (24.0f * uiScale) - (hasScrollbar ? (10.0f * uiScale) : 0.0f);
                float cardX = listBoxRect.x + (12.0f * uiScale);
                float cardYOffset = (10.0f * uiScale);

                for (size_t qIdx = 0; qIdx < filteredQuests.size(); ++qIdx)
                {
                    const auto& q = filteredQuests[qIdx];
                    bool isCompleted = player ? player->quests.isCompleted(q.id) : false;
                    bool isExpanded = (app->getExpandedQuestId() == q.id);
                    int currentStage = player ? player->quests.getQuestStage(q.id) : 0;
                    bool isMain = (q.category.find("Main") != std::string::npos || q.id == "root_delivery");

                    // Compute dynamic height for expanded vs compact card
                    float compactH = 62.0f * uiScale;
                    float cardH = compactH;
                    float descH = 0.0f;
                    float stagesH = 0.0f;

                    if (isExpanded)
                    {
                        if (!q.description.empty())
                        {
                            descH = UIWidget::getTextWrappedHeight(q.description, cardW - (28.0f * uiScale), uiScale * 0.82f) + (10.0f * uiScale);
                        }
                        if (!q.stages.empty())
                        {
                            for (const auto& [sIdx, sDesc] : q.stages)
                            {
                                if (!isCompleted && sIdx > currentStage) continue;
                                std::string sampleLine = "[✓] " + sDesc;
                                stagesH += UIWidget::getTextWrappedHeight(sampleLine, cardW - (40.0f * uiScale), uiScale * 0.80f) + (4.0f * uiScale);
                            }
                            stagesH += (22.0f * uiScale); // Objectives header
                        }
                        float rewardsH = q.rewardsDescription.empty() ? 0.0f : (24.0f * uiScale);
                        cardH = compactH + (12.0f * uiScale) + descH + stagesH + rewardsH;
                    }

                    float cardY = listBoxY + cardYOffset - app->getQuestScrollY();
                    SDL_FRect cardRect = { cardX, cardY, cardW, cardH };

                    bool cardInView = (cardY + cardH >= listBoxRect.y && cardY <= listBoxRect.y + listBoxRect.h);
                    bool cardHovered = mouseInList && (mousePos.x >= cardRect.x && mousePos.x <= cardRect.x + cardRect.w &&
                                                       mousePos.y >= cardRect.y && mousePos.y <= cardRect.y + cardRect.h);

                    if (cardHovered && clicked)
                    {
                        app->toggleExpandedQuest(q.id);
                        app->setSelectedQuestIndex(static_cast<int>(qIdx));
                        gameContext->refreshActionGrid();
                        gameContext->input.consumeMouseClick();
                    }

                    // Render card if visible in view
                    if (cardInView)
                    {
                        SDL_Color cardBg = isExpanded ? Theme::colors.bgSlotOccupied : (cardHovered ? Theme::colors.bgButtonHover : Theme::colors.bgSlot);
                        SDL_Color cardBorder = isExpanded ? Theme::colors.borderSelected : (cardHovered ? Theme::colors.borderMuted : Theme::colors.borderNormal);
                        UIWidget::drawPanel(renderer, cardRect, cardBg, cardBorder);

                        // Line 1: Title
                        std::string titleStr = q.name;
                        SDL_Color titleCol = isCompleted ? Theme::colors.companion : Theme::colors.textGold;
                        UIWidget::drawText(renderer, titleStr, cardX + (12.0f * uiScale), cardY + (8.0f * uiScale), titleCol, uiScale * 0.92f);

                        // Status Badge & Category tag on right
                        std::string statusTag = isCompleted ? "[ COMPLETED ]" : "[ ACTIVE ]";
                        SDL_Color statusCol = isCompleted ? Theme::colors.companion : Theme::colors.textGold;
                        float stW = UIWidget::getTextWidth(statusTag, uiScale * 0.78f);
                        UIWidget::drawText(renderer, statusTag, cardX + cardW - stW - (12.0f * uiScale), cardY + (8.0f * uiScale), statusCol, uiScale * 0.78f);

                        std::string catTag = isMain ? "[ MAIN ]" : "[ SIDE ]";
                        float catW = UIWidget::getTextWidth(catTag, uiScale * 0.76f);
                        UIWidget::drawText(renderer, catTag, cardX + cardW - stW - catW - (20.0f * uiScale), cardY + (8.0f * uiScale), Theme::colors.arcane, uiScale * 0.76f);

                        // Line 2: Current Objective snippet or Giver/Location
                        std::string objSnippet;
                        auto stIt = q.stages.find(currentStage);
                        if (stIt != q.stages.end() && !isCompleted)
                        {
                            objSnippet = "► Objective: " + stIt->second;
                        }
                        else if (isCompleted)
                        {
                            objSnippet = "✓ Quest completed successfully.";
                        }
                        else
                        {
                            objSnippet = "• Giver: " + q.giver + " | Location: " + q.location;
                        }
                        if (objSnippet.size() > 64) objSnippet = objSnippet.substr(0, 61) + "...";
                        UIWidget::drawText(renderer, objSnippet, cardX + (12.0f * uiScale), cardY + (32.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.80f);

                        // Expand indicator toggle text on bottom-right of header
                        std::string expHint = isExpanded ? "▲ Click to collapse" : "▼ Click for details";
                        float expW = UIWidget::getTextWidth(expHint, uiScale * 0.72f);
                        UIWidget::drawText(renderer, expHint, cardX + cardW - expW - (12.0f * uiScale), cardY + (34.0f * uiScale), Theme::colors.textMuted, uiScale * 0.72f);

                        if (isExpanded)
                        {
                            // Horizontal divider
                            SDL_FRect divRect = { cardX + (10.0f * uiScale), cardY + (54.0f * uiScale), cardW - (20.0f * uiScale), 1.0f };
                            SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, Theme::colors.borderNormal.a);
                            SDL_RenderFillRect(renderer, &divRect);

                            float innerY = cardY + (60.0f * uiScale);

                            // Narrative Description
                            if (!q.description.empty())
                            {
                                float dh = UIWidget::drawTextWrapped(renderer, q.description, cardX + (14.0f * uiScale), innerY, cardW - (28.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
                                innerY += dh + (10.0f * uiScale);
                            }

                            // Objectives Checklist
                            if (!q.stages.empty())
                            {
                                UIWidget::drawText(renderer, "Objectives & Stages:", cardX + (14.0f * uiScale), innerY, Theme::colors.textAccent, uiScale * 0.84f);
                                innerY += (18.0f * uiScale);

                                for (const auto& [sIdx, sDesc] : q.stages)
                                {
                                    if (!isCompleted && sIdx > currentStage) continue;
                                    bool stageFinished = isCompleted || (sIdx < currentStage);
                                    bool stageCurrent = (!isCompleted && sIdx == currentStage);

                                    std::string mark = stageFinished ? "[✓]" : (stageCurrent ? "[►]" : "[ ]");
                                    SDL_Color stCol = stageFinished ? Theme::colors.companion : (stageCurrent ? Theme::colors.textGold : Theme::colors.textMuted);
                                    std::string line = std::format("{} {}", mark, sDesc);
                                    float sH = UIWidget::drawTextWrapped(renderer, line, cardX + (20.0f * uiScale), innerY, cardW - (40.0f * uiScale), stCol, uiScale * 0.80f);
                                    innerY += sH + (4.0f * uiScale);
                                }
                            }

                            // Rewards
                            if (!q.rewardsDescription.empty())
                            {
                                std::string rewStr = "Rewards: " + q.rewardsDescription;
                                UIWidget::drawText(renderer, rewStr, cardX + (14.0f * uiScale), innerY, Theme::colors.currency, uiScale * 0.82f);
                                innerY += (20.0f * uiScale);
                            }
                        }
                    }

                    cardYOffset += cardH + (10.0f * uiScale);
                }

                SDL_SetRenderClipRect(renderer, nullptr);

                float totalCardsH = cardYOffset;
                float maxScroll = std::max(0.0f, totalCardsH - listBoxRect.h);
                app->setQuestMaxScrollY(maxScroll);
                app->setQuestScrollY(std::clamp(app->getQuestScrollY(), 0.0f, maxScroll));

                // Draw custom scrollbar on list container
                if (maxScroll > 0.0f)
                {
                    float barW = 6.0f * uiScale;
                    float trackX = listBoxRect.x + listBoxRect.w - barW - (4.0f * uiScale);
                    float trackY = listBoxRect.y + (4.0f * uiScale);
                    float trackH = listBoxRect.h - (8.0f * uiScale);
                    SDL_FRect trackRect = { trackX, trackY, barW, trackH };
                    UIWidget::drawPanel(renderer, trackRect, Theme::colors.bgSlot, Theme::colors.borderMuted);

                    float thumbH = std::max(24.0f * uiScale, (listBoxRect.h / totalCardsH) * trackH);
                    float thumbY = trackY + (app->getQuestScrollY() / maxScroll) * (trackH - thumbH);
                    SDL_FRect thumbRect = { trackX, thumbY, barW, thumbH };
                    UIWidget::drawPanel(renderer, thumbRect, Theme::colors.borderSelected, Theme::colors.borderSelected);
                }
            }

            curY += listBoxH + (12.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::ENCYCLOPEDIA)
        {
            UIWidget::drawText(renderer, "Arcane Encyclopedia & Lore Compendium:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            // Category Tab Buttons
            std::vector<std::string> catNames;
            if (data.contains("categories") && data["categories"].is_array())
            {
                for (const auto& cat : data["categories"])
                {
                    catNames.push_back(cat.value("name", "Category"));
                }
            }
            if (catNames.empty()) catNames = { "Species & Morphs", "Items & Relics", "Locations" };

            int activeCat = std::clamp(app->getEncyclopediaCategory(), 0, static_cast<int>(catNames.size()) - 1);

            float catBoxW = std::min(innerW, 640.0f * uiScale);
            float catBoxX = rect.x + (rect.w - catBoxW) / 2.0f;
            float catBoxH = 34.0f * uiScale;
            SDL_FRect catBoxRect = { catBoxX, curY, catBoxW, catBoxH };
            UIWidget::drawPanel(renderer, catBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float catPad = 4.0f * uiScale;
            float catGap = 4.0f * uiScale;
            float catBtnH = catBoxH - (catPad * 2.0f);
            float catBtnW = (catBoxW - (catPad * 2.0f) - (catGap * (catNames.size() - 1))) / static_cast<float>(catNames.size());

            for (size_t i = 0; i < catNames.size(); ++i)
            {
                SDL_FRect cR = { catBoxX + catPad + i * (catBtnW + catGap), catBoxRect.y + catPad, catBtnW, catBtnH };
                bool hov = (mousePos.x >= cR.x && mousePos.x <= cR.x + cR.w &&
                            mousePos.y >= cR.y && mousePos.y <= cR.y + cR.h);
                if (hov && clicked)
                {
                    app->setEncyclopediaCategory(static_cast<int>(i));
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                UIWidget::drawButton(renderer, cR, catNames[i], hov, true, (activeCat == static_cast<int>(i)), uiScale * 0.78f);
            }
            curY += catBoxH + (14.0f * uiScale);

            // Entries for selected category
            if (data.contains("categories") && data["categories"].is_array() && activeCat < static_cast<int>(data["categories"].size()))
            {
                const auto& catObj = data["categories"][activeCat];
                if (catObj.contains("entries") && catObj["entries"].is_array())
                {
                    for (const auto& ent : catObj["entries"])
                    {
                        std::string title = ent.value("title", "Unknown");
                        std::string subtitle = ent.value("subtitle", "");
                        std::string desc = ent.value("description", "");
                        bool unlocked = ent.value("unlocked", true);

                        float cardH = 56.0f * uiScale;
                        SDL_FRect cardRect = { padX, curY, innerW, cardH };
                        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                        if (unlocked)
                        {
                            UIWidget::drawText(renderer, title, padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
                            if (!subtitle.empty())
                            {
                                float titleW = UIWidget::getTextWidth(title, uiScale * 0.88f);
                                UIWidget::drawText(renderer, "• " + subtitle, padX + (16.0f * uiScale) + titleW, curY + (7.0f * uiScale), Theme::colors.textAccent, uiScale * 0.76f);
                            }
                            UIWidget::drawTextWrapped(renderer, desc, padX + (10.0f * uiScale), curY + (24.0f * uiScale), innerW - (20.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
                        }
                        else
                        {
                            UIWidget::drawText(renderer, "??? [Undiscovered Entry]", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                            UIWidget::drawText(renderer, "Explore the realm, read tomes, and interact with characters to record lore.", padX + (10.0f * uiScale), curY + (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                        }
                        curY += cardH + (8.0f * uiScale);
                    }
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::SPELLS)
        {
            UIWidget::drawText(renderer, "Arcane Grimoire & Spellbook:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            float curMp = p ? p->getStat("mana") : 0.0f;
            float maxMp = p ? std::max(80.0f, p->getStat("max_mana")) : 80.0f;
            UIWidget::drawText(renderer, std::format("Current Mana Pool: {:.0f} / {:.0f} MP", curMp, maxMp), padX, curY, Theme::colors.arcane, uiScale * 0.85f);
            curY += (18.0f * uiScale);

            // School Filter Tabs: All, Arcane, Fire, Restoration, Translocation
            static const std::vector<std::string> schools = { "All", "Arcane", "Fire", "Restoration", "Translocation" };
            int activeSchool = std::clamp(app->getSpellsSchool(), 0, static_cast<int>(schools.size()) - 1);

            float sBoxW = std::min(innerW, 640.0f * uiScale);
            float sBoxX = rect.x + (rect.w - sBoxW) / 2.0f;
            float sBoxH = 34.0f * uiScale;
            SDL_FRect sBoxRect = { sBoxX, curY, sBoxW, sBoxH };
            UIWidget::drawPanel(renderer, sBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float sPad = 4.0f * uiScale;
            float sGap = 4.0f * uiScale;
            float sBtnH = sBoxH - (sPad * 2.0f);
            float sBtnW = (sBoxW - (sPad * 2.0f) - (sGap * (schools.size() - 1))) / static_cast<float>(schools.size());

            for (size_t i = 0; i < schools.size(); ++i)
            {
                SDL_FRect sR = { sBoxX + sPad + i * (sBtnW + sGap), sBoxRect.y + sPad, sBtnW, sBtnH };
                bool hov = (mousePos.x >= sR.x && mousePos.x <= sR.x + sR.w &&
                            mousePos.y >= sR.y && mousePos.y <= sR.y + sR.h);
                if (hov && clicked)
                {
                    app->setSpellsSchool(static_cast<int>(i));
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                UIWidget::drawButton(renderer, sR, schools[i], hov, true, (activeSchool == static_cast<int>(i)), uiScale * 0.78f);
            }
            curY += sBoxH + (14.0f * uiScale);

            // List spells
            if (data.contains("spells") && data["spells"].is_array())
            {
                for (const auto& sp : data["spells"])
                {
                    std::string sName = sp.value("name", "Spell");
                    std::string school = sp.value("school", "Arcane");
                    int mp = sp.value("mpCost", 10);
                    float dmg = sp.value("damage", 0.0f);
                    float heal = sp.value("heal", 0.0f);
                    float def = sp.value("defense", 0.0f);
                    std::string desc = sp.value("description", "");
                    bool unlocked = sp.value("unlocked", true);

                    if (activeSchool > 0 && school != schools[activeSchool]) continue;

                    float cardH = 50.0f * uiScale;
                    SDL_FRect cardRect = { padX, curY, innerW, cardH };
                    UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    if (unlocked)
                    {
                        std::string titleStr = std::format("{} [{} MP]", sName, mp);
                        UIWidget::drawText(renderer, titleStr, padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.arcane, uiScale * 0.88f);
                        float tW = UIWidget::getTextWidth(titleStr, uiScale * 0.88f);

                        std::string statSummary = "";
                        if (dmg > 0.0f) statSummary += std::format("  •  Dmg: {:.0f}", dmg);
                        if (heal > 0.0f) statSummary += std::format("  •  Heal: {:.0f}", heal);
                        if (def > 0.0f) statSummary += std::format("  •  Barrier: {:.0f}", def);
                        UIWidget::drawText(renderer, statSummary, padX + (12.0f * uiScale) + tW, curY + (7.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

                        UIWidget::drawText(renderer, std::format("School: {}", school), padX + innerW - (210.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                        UIWidget::drawTextWrapped(renderer, desc, padX + (10.0f * uiScale), curY + (24.0f * uiScale), innerW - (130.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

                        // Cast Spell button
                        SDL_FRect castBtnRect = { padX + innerW - (105.0f * uiScale), curY + (10.0f * uiScale), 95.0f * uiScale, 30.0f * uiScale };
                        bool canCast = (p && curMp >= mp);
                        bool castHov = (mousePos.x >= castBtnRect.x && mousePos.x <= castBtnRect.x + castBtnRect.w &&
                                        mousePos.y >= castBtnRect.y && mousePos.y <= castBtnRect.y + castBtnRect.h);
                        if (castHov && clicked && canCast)
                        {
                            p->stats.setBaseStat("mana", curMp - mp);
                            if (heal > 0.0f)
                            {
                                float curHp = p->getStat("health");
                                float maxHp = std::max(100.0f, p->getStat("max_health"));
                                p->stats.setBaseStat("health", std::min(maxHp, curHp + heal));
                                app->setFeedbackText(std::format("Channeled {}! Restored {:.0f} HP (-{} MP).", sName, heal, mp));
                            }
                            else
                            {
                                app->setFeedbackText(std::format("Channeled {}! Arcane energies surge forth (-{} MP).", sName, mp));
                            }
                            gameContext->input.consumeMouseClick();
                            gameContext->refreshActionGrid();
                        }
                        UIWidget::drawButton(renderer, castBtnRect, "Cast Spell", castHov, canCast, false, uiScale * 0.75f);
                    }
                    else
                    {
                        UIWidget::drawText(renderer, "??? [Undiscovered Spell]", padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.85f);
                        UIWidget::drawText(renderer, "Find arcane scrolls or study ancient tomes to decipher this incantation.", padX + (10.0f * uiScale), curY + (26.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                    }
                    curY += cardH + (8.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::PERKS)
        {
            UIWidget::drawText(renderer, "Character Perks & Talents Tree:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            float perkPts = p ? p->getStat("perk_points") : 0.0f;

            // Status bar with reset button
            float statRowH = 34.0f * uiScale;
            SDL_FRect statRowRect = { padX, curY, innerW, statRowH };
            UIWidget::drawPanel(renderer, statRowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, std::format("Available Talent Points: {:.0f}", perkPts), padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.currency, uiScale * 0.85f);

            SDL_FRect resetBtnRect = { padX + innerW - (120.0f * uiScale), curY + (4.0f * uiScale), 110.0f * uiScale, 26.0f * uiScale };
            bool resetHov = (mousePos.x >= resetBtnRect.x && mousePos.x <= resetBtnRect.x + resetBtnRect.w &&
                             mousePos.y >= resetBtnRect.y && mousePos.y <= resetBtnRect.y + resetBtnRect.h);
            if (resetHov && clicked && p)
            {
                p->stats.setBaseStat("perk_points", perkPts + 3.0f);
                app->setFeedbackText("All spent perk points refunded (+3 Points).");
                gameContext->input.consumeMouseClick();
                gameContext->refreshActionGrid();
            }
            UIWidget::drawButton(renderer, resetBtnRect, "Reset Perks", resetHov, true, false, uiScale * 0.75f);
            curY += statRowH + (12.0f * uiScale);

            // Category Filter Tabs: All, Physical, Arcane, Social, Sexual, Demonic
            static const std::vector<std::string> categories = { "All", "Physical", "Arcane", "Social", "Sexual", "Demonic" };
            int activeCat = std::clamp(app->getPerksCategory(), 0, static_cast<int>(categories.size()) - 1);

            float pBoxW = std::min(innerW, 640.0f * uiScale);
            float pBoxX = rect.x + (rect.w - pBoxW) / 2.0f;
            float pBoxH = 34.0f * uiScale;
            SDL_FRect pBoxRect = { pBoxX, curY, pBoxW, pBoxH };
            UIWidget::drawPanel(renderer, pBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float pPad = 4.0f * uiScale;
            float pGap = 4.0f * uiScale;
            float pBtnH = pBoxH - (pPad * 2.0f);
            float pBtnW = (pBoxW - (pPad * 2.0f) - (pGap * (categories.size() - 1))) / static_cast<float>(categories.size());

            for (size_t i = 0; i < categories.size(); ++i)
            {
                SDL_FRect pR = { pBoxX + pPad + i * (pBtnW + pGap), pBoxRect.y + pPad, pBtnW, pBtnH };
                bool hov = (mousePos.x >= pR.x && mousePos.x <= pR.x + pR.w &&
                            mousePos.y >= pR.y && mousePos.y <= pR.y + pR.h);
                if (hov && clicked)
                {
                    app->setPerksCategory(static_cast<int>(i));
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                UIWidget::drawButton(renderer, pR, categories[i], hov, true, (activeCat == static_cast<int>(i)), uiScale * 0.78f);
            }
            curY += pBoxH + (14.0f * uiScale);

            // Perks List
            if (data.contains("perks") && data["perks"].is_array())
            {
                for (const auto& pk : data["perks"])
                {
                    std::string pName = pk.value("name", "Perk");
                    std::string category = pk.value("category", "General");
                    int cost = pk.value("cost", 1);
                    std::string desc = pk.value("description", "");
                    bool unlocked = pk.value("unlocked", false);

                    if (activeCat > 0 && category != categories[activeCat]) continue;

                    float cardH = 48.0f * uiScale;
                    SDL_FRect cardRect = { padX, curY, innerW, cardH };
                    UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    UIWidget::drawText(renderer, pName, padX + (10.0f * uiScale), curY + (6.0f * uiScale), unlocked ? Theme::colors.textGold : Theme::colors.textSecondary, uiScale * 0.88f);
                    float nW = UIWidget::getTextWidth(pName, uiScale * 0.88f);
                    UIWidget::drawText(renderer, std::format("[{}]", category), padX + (16.0f * uiScale) + nW, curY + (7.0f * uiScale), Theme::colors.textAccent, uiScale * 0.75f);

                    UIWidget::drawTextWrapped(renderer, desc, padX + (10.0f * uiScale), curY + (24.0f * uiScale), innerW - (140.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

                    // Learn button
                    SDL_FRect lBtnRect = { padX + innerW - (115.0f * uiScale), curY + (10.0f * uiScale), 105.0f * uiScale, 28.0f * uiScale };
                    bool canLearn = (!unlocked && p && perkPts >= cost);
                    bool lHov = (mousePos.x >= lBtnRect.x && mousePos.x <= lBtnRect.x + lBtnRect.w &&
                                 mousePos.y >= lBtnRect.y && mousePos.y <= lBtnRect.y + lBtnRect.h);
                    if (lHov && clicked && canLearn)
                    {
                        p->stats.setBaseStat("perk_points", perkPts - cost);
                        app->setFeedbackText(std::format("Unlocked talent: {}! (-{} Pts)", pName, cost));
                        gameContext->input.consumeMouseClick();
                        gameContext->refreshActionGrid();
                    }
                    std::string lLabel = unlocked ? "[ Learned ]" : std::format("Learn (-{} Pt)", cost);
                    UIWidget::drawButton(renderer, lBtnRect, lLabel, lHov, canLearn, unlocked, uiScale * 0.75f);

                    curY += cardH + (8.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::CONTACTS)
        {
            int selectedIdx = app->getContactsSelectedIdx();
            if (selectedIdx < 0)
            {
                UIWidget::drawText(renderer, "Address Book & Transceiver Contacts:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
                curY += (22.0f * uiScale);

                if (data.contains("contacts") && data["contacts"].is_array())
                {
                    const auto& contacts = data["contacts"];
                    int expandedIdx = app->getContactsExpandedIdx();

                    for (size_t i = 0; i < contacts.size(); ++i)
                    {
                        const auto& ct = contacts[i];
                        std::string cName = ct.value("name", "Contact");
                        std::string role = ct.value("title", "Acquaintance");
                        std::string race = ct.value("race", "Human");
                        std::string location = ct.value("location", "Unknown");
                        std::string status = ct.value("status", "Available");
                        int level = ct.value("level", 1);
                        int age = ct.value("age", 20);
                        std::string gender = ct.value("gender", "Unknown");
                        int affection = ct.value("affection", ct.value("affinity", 0));
                        bool isSubordinate = ct.value("isSubordinate", false);
                        int obedience = ct.value("obedience", 0);

                        bool isExpanded = (expandedIdx == static_cast<int>(i));
                        float cardH = isExpanded ? (80.0f * uiScale) : (44.0f * uiScale);
                        SDL_FRect cardRect = { padX, curY, innerW, cardH };
                        bool hov = (mousePos.x >= cardRect.x && mousePos.x <= cardRect.x + cardRect.w &&
                                    mousePos.y >= cardRect.y && mousePos.y <= cardRect.y + cardRect.h);

                        SDL_FRect detBtnRect = { padX + innerW - (90.0f * uiScale), curY + cardH - (32.0f * uiScale), 80.0f * uiScale, 26.0f * uiScale };
                        bool detHov = isExpanded && (mousePos.x >= detBtnRect.x && mousePos.x <= detBtnRect.x + detBtnRect.w &&
                                                    mousePos.y >= detBtnRect.y && mousePos.y <= detBtnRect.y + detBtnRect.h);

                        if (detHov && clicked)
                        {
                            app->setContactsSelectedIdx(static_cast<int>(i));
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }
                        else if (hov && clicked)
                        {
                            app->toggleContactsExpanded(static_cast<int>(i));
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }

                        UIWidget::drawPanel(renderer, cardRect, (isExpanded || hov) ? Theme::colors.bgHeader : Theme::colors.bgSlot, isExpanded ? Theme::colors.borderSelected : (hov ? Theme::colors.borderSelected : Theme::colors.borderNormal));

                        // Line 1: Name, Level, Age, Gender + Expand Chevron
                        std::string line1 = std::format("{}  •  Lv. {}  •  Age {}  •  {}", cName, level, age, gender);
                        UIWidget::drawText(renderer, line1, padX + (10.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

                        std::string chevron = isExpanded ? "▲" : "▼";
                        UIWidget::drawText(renderer, chevron, padX + innerW - (22.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);

                        // Line 2: Affection, and Obedience if subordinate
                        std::string line2 = std::format("Affection: {} / 100", affection);
                        if (isSubordinate)
                        {
                            line2 += std::format("  |  Obedience: {} / 100", obedience);
                        }
                        else
                        {
                            line2 += std::format("  |  {}", role);
                        }
                        UIWidget::drawText(renderer, line2, padX + (10.0f * uiScale), curY + (24.0f * uiScale), isSubordinate ? Theme::colors.arcane : Theme::colors.companion, uiScale * 0.76f);

                        // Expanded Section: Location, Status, and Details Button
                        if (isExpanded)
                        {
                            std::string line3 = std::format("Location: {}  •  Status: {}  •  {}", location, status, race);
                            UIWidget::drawText(renderer, line3, padX + (10.0f * uiScale), curY + (48.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.76f);

                            UIWidget::drawButton(renderer, detBtnRect, "Details ►", detHov, true, false, uiScale * 0.74f);
                        }

                        curY += cardH + (6.0f * uiScale);
                    }
                }
            }
            else
            {
                // Detailed Dossier Submenu (Formatted like Selfie inspection screen)
                if (data.contains("contacts") && data["contacts"].is_array() && selectedIdx < static_cast<int>(data["contacts"].size()))
                {
                    const auto& ct = data["contacts"][selectedIdx];
                    std::string cName = ct.value("name", "Contact");
                    std::string role = ct.value("title", "Acquaintance");
                    std::string race = ct.value("race", "Human");
                    int level = ct.value("level", 1);
                    int age = ct.value("age", 20);
                    std::string gender = ct.value("gender", "Unknown");
                    std::string location = ct.value("location", "Unknown");
                    std::string status = ct.value("status", "Available");
                    int affection = ct.value("affection", ct.value("affinity", 0));
                    bool isSubordinate = ct.value("isSubordinate", false);
                    int obedience = ct.value("obedience", 0);
                    int respect = ct.value("respect", 0);
                    std::string artwork = ct.value("artwork", "portrait.png");
                    std::string appDesc = ct.value("appearanceDescription", ct.value("description", ""));
                    std::string bio = ct.value("description", "");

                    // Top Bar with Back Button
                    float topH = 30.0f * uiScale;
                    SDL_FRect bRect = { padX, curY, 140.0f * uiScale, topH };
                    bool bHov = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                 mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);
                    if (bHov && clicked)
                    {
                        app->setContactsSelectedIdx(-1);
                        gameContext->refreshActionGrid();
                        gameContext->input.consumeMouseClick();
                    }
                    UIWidget::drawButton(renderer, bRect, "◄ Back to Contacts", bHov, true, false, uiScale * 0.78f);
                    curY += topH + (12.0f * uiScale);

                    // Identity & Artwork Header Panel
                    float headH = 100.0f * uiScale;
                    SDL_FRect headRect = { padX, curY, innerW, headH };
                    UIWidget::drawPanel(renderer, headRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    // Artwork Frame
                    float artW = 75.0f * uiScale;
                    float artH = headH - (12.0f * uiScale);
                    SDL_FRect artRect = { padX + (8.0f * uiScale), curY + (6.0f * uiScale), artW, artH };
                    UIWidget::drawPanel(renderer, artRect, Theme::colors.bgDark, Theme::colors.borderSelected);
                    UIWidget::drawText(renderer, "🖼️ ART", artRect.x + (14.0f * uiScale), artRect.y + (22.0f * uiScale), Theme::colors.textGold, uiScale * 0.75f);
                    UIWidget::drawText(renderer, "PORTRAIT", artRect.x + (8.0f * uiScale), artRect.y + (42.0f * uiScale), Theme::colors.textMuted, uiScale * 0.65f);

                    // Identity Text
                    float txX = artRect.x + artW + (12.0f * uiScale);
                    float txW = innerW - artW - (28.0f * uiScale);
                    float iy = curY + (8.0f * uiScale);

                    std::string idTitle = std::format("{} — Level {} {}", cName, level, race);
                    UIWidget::drawText(renderer, idTitle, txX, iy, Theme::colors.textGold, uiScale * 0.90f);
                    iy += (18.0f * uiScale);

                    std::string idSub = std::format("Role: {}  •  Age: {}  •  Gender: {}  •  Status: {}", role, age, gender, status);
                    UIWidget::drawText(renderer, idSub, txX, iy, Theme::colors.textSecondary, uiScale * 0.76f);
                    iy += (18.0f * uiScale);

                    std::string idLoc = std::format("Current Location: {}", location);
                    UIWidget::drawText(renderer, idLoc, txX, iy, Theme::colors.textAccent, uiScale * 0.76f);
                    iy += (20.0f * uiScale);

                    // Meter: Affection and Obedience (or Respect)
                    float barW = (txW - (12.0f * uiScale)) / 2.0f;
                    float barH = 16.0f * uiScale;
                    UIWidget::drawProgressBar(renderer, { txX, iy, barW, barH }, static_cast<float>(affection), 100.0f, Theme::colors.companion, Theme::colors.bgDark, std::format("Affection: {}/100", affection), uiScale * 0.72f);
                    if (isSubordinate)
                    {
                        UIWidget::drawProgressBar(renderer, { txX + barW + (10.0f * uiScale), iy, barW, barH }, static_cast<float>(obedience), 100.0f, Theme::colors.arcane, Theme::colors.bgDark, std::format("Obedience: {}/100", obedience), uiScale * 0.72f);
                    }
                    else
                    {
                        UIWidget::drawProgressBar(renderer, { txX + barW + (10.0f * uiScale), iy, barW, barH }, static_cast<float>(respect), 100.0f, Theme::colors.arcane, Theme::colors.bgDark, std::format("Respect: {}/100", respect), uiScale * 0.72f);
                    }

                    curY += headH + (12.0f * uiScale);

                    // Section 1: Complete Stats Matrix (Parity with Selfie)
                    UIWidget::drawText(renderer, "Attribute Matrix & Vitals Overview:", padX, curY, Theme::colors.textAccent, uiScale * 0.86f);
                    curY += (17.0f * uiScale);

                    float statsBoxH = 68.0f * uiScale;
                    SDL_FRect statsBoxRect = { padX, curY, innerW, statsBoxH };
                    UIWidget::drawPanel(renderer, statsBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    float sy = curY + (6.0f * uiScale);
                    float curHp = 100.0f, maxHp = 100.0f, curMp = 50.0f, maxMp = 50.0f;
                    float str = 10.0f, agi = 10.0f, tou = 10.0f, intl = 10.0f, wil = 10.0f, lib = 10.0f, all = 10.0f, sen = 10.0f;

                    if (ct.contains("stats") && ct["stats"].is_object())
                    {
                        const auto& st = ct["stats"];
                        curHp = st.value("health", 100.0f);
                        maxHp = st.value("max_health", 100.0f);
                        curMp = st.value("mana", 50.0f);
                        maxMp = st.value("max_mana", 50.0f);
                        str = st.value("strength", 10.0f);
                        agi = st.value("agility", 10.0f);
                        tou = st.value("toughness", 10.0f);
                        intl = st.value("intelligence", 10.0f);
                        wil = st.value("willpower", 10.0f);
                        lib = st.value("libido", 10.0f);
                        all = st.value("allure", 10.0f);
                        sen = st.value("sensitivity", 10.0f);
                    }

                    std::string vLine = std::format("HP: {:.0f}/{:.0f}   MP: {:.0f}/{:.0f}   Status: Normal", curHp, maxHp, curMp, maxMp);
                    UIWidget::drawText(renderer, vLine, padX + (12.0f * uiScale), sy, Theme::colors.textPrimary, uiScale * 0.78f);
                    sy += (18.0f * uiScale);

                    std::string aLine1 = std::format("STR: {:<4.0f}  AGI: {:<4.0f}  TOU: {:<4.0f}  INT: {:<4.0f}", str, agi, tou, intl);
                    UIWidget::drawText(renderer, aLine1, padX + (12.0f * uiScale), sy, Theme::colors.textGold, uiScale * 0.78f);

                    std::string aLine2 = std::format("WIL: {:<4.0f}  LIB: {:<4.0f}  ALL: {:<4.0f}  SEN: {:<4.0f}", wil, lib, all, sen);
                    UIWidget::drawText(renderer, aLine2, padX + (innerW / 2.0f), sy, Theme::colors.textGold, uiScale * 0.78f);

                    curY += statsBoxH + (12.0f * uiScale);

                    // Section 2: Detailed Textual Appearance Description (Parity with Selfie)
                    UIWidget::drawText(renderer, "Detailed Physical Description:", padX, curY, Theme::colors.textAccent, uiScale * 0.86f);
                    curY += (17.0f * uiScale);

                    float appH = UIWidget::drawTextWrapped(renderer, appDesc, padX + (6.0f * uiScale), curY, innerW - (12.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.80f);
                    curY += appH + (12.0f * uiScale);

                    // Section 3: Biography & Background
                    UIWidget::drawText(renderer, "Character Background & Notes:", padX, curY, Theme::colors.textAccent, uiScale * 0.86f);
                    curY += (17.0f * uiScale);

                    float bioH = UIWidget::drawTextWrapped(renderer, bio, padX + (6.0f * uiScale), curY, innerW - (12.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
                    curY += bioH + (14.0f * uiScale);

                    // Quick Transceiver actions
                    float btnW = 130.0f * uiScale;
                    float btnH = 28.0f * uiScale;
                    SDL_FRect callRect = { padX, curY, btnW, btnH };
                    bool cHov = (mousePos.x >= callRect.x && mousePos.x <= callRect.x + callRect.w &&
                                 mousePos.y >= callRect.y && mousePos.y <= callRect.y + callRect.h);
                    if (cHov && clicked)
                    {
                        app->setFeedbackText(std::format("Placed call to {}. 'Greetings! Stay safe.'", cName));
                        gameContext->input.consumeMouseClick();
                    }
                    UIWidget::drawButton(renderer, callRect, "📞 Call Transceiver", cHov, true, false, uiScale * 0.76f);

                    SDL_FRect msgRect = { padX + (10.0f * uiScale) + btnW, curY, btnW, btnH };
                    bool mHov = (mousePos.x >= msgRect.x && mousePos.x <= msgRect.x + msgRect.w &&
                                 mousePos.y >= msgRect.y && mousePos.y <= msgRect.y + msgRect.h);
                    if (mHov && clicked)
                    {
                        app->setFeedbackText(std::format("Sent message to {}. Delivered.", cName));
                        gameContext->input.consumeMouseClick();
                    }
                    UIWidget::drawButton(renderer, msgRect, "✉️ Send Message", mHov, true, false, uiScale * 0.76f);

                    curY += btnH + (12.0f * uiScale);
                }
            }
        }
        else if (app->getAppMode() == PhoneAppMode::MAPS)
        {
            UIWidget::drawText(renderer, "Territorial Survey & Navigation:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            // Sub-tabs: World Overview, Local Zone, Points of Interest
            static const std::vector<std::string> mapTabs = { "World Overview", "Local Zone", "Points of Interest" };
            int activeTab = std::clamp(app->getMapsView(), 0, static_cast<int>(mapTabs.size()) - 1);

            float mBoxW = std::min(innerW, 640.0f * uiScale);
            float mBoxX = rect.x + (rect.w - mBoxW) / 2.0f;
            float mBoxH = 34.0f * uiScale;
            SDL_FRect mBoxRect = { mBoxX, curY, mBoxW, mBoxH };
            UIWidget::drawPanel(renderer, mBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float mPad = 4.0f * uiScale;
            float mGap = 4.0f * uiScale;
            float mBtnH = mBoxH - (mPad * 2.0f);
            float mBtnW = (mBoxW - (mPad * 2.0f) - (mGap * (mapTabs.size() - 1))) / static_cast<float>(mapTabs.size());

            for (size_t i = 0; i < mapTabs.size(); ++i)
            {
                SDL_FRect mR = { mBoxX + mPad + i * (mBtnW + mGap), mBoxRect.y + mPad, mBtnW, mBtnH };
                bool hov = (mousePos.x >= mR.x && mousePos.x <= mR.x + mR.w &&
                            mousePos.y >= mR.y && mousePos.y <= mR.y + mR.h);
                if (hov && clicked)
                {
                    app->setMapsView(static_cast<int>(i));
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                UIWidget::drawButton(renderer, mR, mapTabs[i], hov, true, (activeTab == static_cast<int>(i)), uiScale * 0.78f);
            }
            curY += mBoxH + (14.0f * uiScale);

            float mapCanvasH = 150.0f * uiScale;
            SDL_FRect mapCanvas = { padX, curY, innerW, mapCanvasH };
            UIWidget::drawPanel(renderer, mapCanvas, Theme::colors.bgSlot, Theme::colors.borderNormal);

            if (activeTab == 0) // World Overview
            {
                UIWidget::drawText(renderer, "🗺️ Regional Geography: The Central Realm & Outlying Territories", padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
                UIWidget::drawText(renderer, "Primary Safe Haven: Sanctuary Manor & Surrounding Grounds", padX + (12.0f * uiScale), curY + (30.0f * uiScale), Theme::colors.companion, uiScale * 0.80f);

                static const std::vector<std::string> sectors = {
                    "• Sanctuary Manor & Estate (Safe Zone, Teleport Node, Residential Wing)",
                    "• Central Market Plaza (Commercial District, Alchemical Reagents, Weapon Smiths)",
                    "• Whispering Wilds (Outskirts, Arcane Flora, Wandering Beastkin)",
                    "• Sunken Catacombs (Ancient Subterranean Leylines, High Danger)"
                };
                float secY = curY + (52.0f * uiScale);
                for (const auto& s : sectors)
                {
                    UIWidget::drawText(renderer, s, padX + (12.0f * uiScale), secY, Theme::colors.textPrimary, uiScale * 0.78f);
                    secY += (18.0f * uiScale);
                }
            }
            else if (activeTab == 1) // Local Zone
            {
                const gameMap* map = gameContext->getActiveMap();
                std::string zoneName = (map && !map->getName().empty()) ? map->getName() : "Sanctuary Manor F1";
                UIWidget::drawText(renderer, std::format("📍 Current Zone: {}", zoneName), padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
                UIWidget::drawText(renderer, "Environmental Condition: Safe Arcane Sanctuary [Combat Disabled]", padX + (12.0f * uiScale), curY + (30.0f * uiScale), Theme::colors.friendly, uiScale * 0.80f);

                static const std::vector<std::string> zoneAmenities = {
                    "• Private Quarters: Comfortable bed, wardrobe mirror, secure storage",
                    "• Research Alcove: Alchemical station, potion synthesizer, arcane library",
                    "• Fast-Travel Waystone: Tuned to Sanctuary resonance frequency",
                    "• Transceiver Relay: Full 5-bar arcane network signal"
                };
                float amY = curY + (52.0f * uiScale);
                for (const auto& a : zoneAmenities)
                {
                    UIWidget::drawText(renderer, a, padX + (12.0f * uiScale), amY, Theme::colors.textSecondary, uiScale * 0.78f);
                    amY += (18.0f * uiScale);
                }
            }
            else if (activeTab == 2) // Points of Interest
            {
                UIWidget::drawText(renderer, "🌟 Notable Points of Interest & Discovered Landmarks:", padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);

                static const std::vector<std::string> pois = {
                    "• Sanctuary Research Lab — Equipped for body analysis, alchemy, and rest.",
                    "• Central Market Fountain — High merchant activity; barter, buy, and sell goods.",
                    "• Ancient Conduit Spire — Massive monolithic pillar radiating raw magical resonance.",
                    "• Forgotten Catacomb Seal — Heavy rune-inscribed iron gates leading underground."
                };
                float pY = curY + (34.0f * uiScale);
                for (const auto& poi : pois)
                {
                    UIWidget::drawText(renderer, poi, padX + (12.0f * uiScale), pY, Theme::colors.textPrimary, uiScale * 0.78f);
                    pY += (22.0f * uiScale);
                }
            }
            curY += mapCanvasH + (12.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::SELFIE)
        {
            UIWidget::drawText(renderer, "Character Overview & Physical Inspection:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            if (p)
            {
                // Top Identity Summary Box
                float idBoxH = 46.0f * uiScale;
                SDL_FRect idBoxRect = { padX, curY, innerW, idBoxH };
                UIWidget::drawPanel(renderer, idBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                std::string pName = p->name.empty() ? "Player Character" : p->name;
                std::string raceStr = p->anatomy.getRacialTitle();
                int pLevel = static_cast<int>(p->getStat("level"));
                if (pLevel <= 0) pLevel = 1;

                std::string idTitle = std::format("{} — Level {} {}", pName, pLevel, raceStr);
                UIWidget::drawText(renderer, idTitle, padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.90f);

                std::string femStr = p->anatomy.isFeminine() ? "Feminine" : "Masculine";
                std::string subTitle = std::format("Gender: {} | Build: {} (Height: {:.2f}m) | Tone: {}",
                    femStr, p->anatomy.bodySize, p->anatomy.heightMeters, p->anatomy.muscleTone);
                UIWidget::drawText(renderer, subTitle, padX + (12.0f * uiScale), curY + (25.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

                curY += idBoxH + (14.0f * uiScale);

                // Section 1: Complete Stats Matrix & Vitals Overview
                UIWidget::drawText(renderer, "Attribute Matrix & Vitals Overview:", padX, curY, Theme::colors.textAccent, uiScale * 0.88f);
                curY += (18.0f * uiScale);

                float statsBoxH = 74.0f * uiScale;
                SDL_FRect statsBoxRect = { padX, curY, innerW, statsBoxH };
                UIWidget::drawPanel(renderer, statsBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                float sy = curY + (7.0f * uiScale);
                std::string vitalsLine = std::format("HP: {:.0f}/{:.0f}   MP: {:.0f}/{:.0f}   Lust: {:.0f}%   Arousal: {:.0f}%   Corruption: {:.0f}%",
                    p->getStat("health"), std::max(100.0f, p->getStat("max_health")),
                    p->getStat("mana"), std::max(80.0f, p->getStat("max_mana")),
                    p->getStat("lust"), p->getStat("arousal"), p->getStat("corruption"));
                UIWidget::drawText(renderer, vitalsLine, padX + (12.0f * uiScale), sy, Theme::colors.textPrimary, uiScale * 0.78f);
                sy += (19.0f * uiScale);

                std::string attrCol1 = std::format("STR: {:<4.0f}  AGI: {:<4.0f}  TOU: {:<4.0f}  INT: {:<4.0f}",
                    p->getStat("strength"), p->getStat("agility"), p->getStat("toughness"), p->getStat("intelligence"));
                UIWidget::drawText(renderer, attrCol1, padX + (12.0f * uiScale), sy, Theme::colors.textGold, uiScale * 0.78f);

                std::string attrCol2 = std::format("WIL: {:<4.0f}  LIB: {:<4.0f}  ALL: {:<4.0f}  SEN: {:<4.0f}",
                    p->getStat("willpower"), p->getStat("libido"), p->getStat("allure"), p->getStat("sensitivity"));
                UIWidget::drawText(renderer, attrCol2, padX + (innerW / 2.0f), sy, Theme::colors.textGold, uiScale * 0.78f);
                sy += (19.0f * uiScale);

                const bodyPart* br = p->anatomy.getPart(bodySlot::BREASTS);
                std::string brCup = br ? bodyPart::getCupSizeName(br->cupSize) : "Flat";
                const bodyPart* gr = p->anatomy.getPart(bodySlot::GROIN);
                float pLen = gr ? gr->length : (p->anatomy.hasPenis() ? 15.0f : 0.0f);
                std::string anatomySummary = std::format("Breasts: {} Cup  |  Penis: {}  |  Vagina: {}",
                    brCup, p->anatomy.hasPenis() ? std::format("{:.0f}cm", pLen) : "None",
                    p->anatomy.hasVagina() ? "Present" : "None");
                UIWidget::drawText(renderer, anatomySummary, padX + (12.0f * uiScale), sy, Theme::colors.textSecondary, uiScale * 0.76f);

                curY += statsBoxH + (14.0f * uiScale);

                // Section 2: Full Detailed Textual Appearance Description
                UIWidget::drawText(renderer, "Detailed Physical Description:", padX, curY, Theme::colors.textAccent, uiScale * 0.88f);
                curY += (18.0f * uiScale);

                std::string fullDescription = characterDescription::generateFullDescription(p);
                float descH = UIWidget::drawTextWrapped(renderer, fullDescription, padX + (6.0f * uiScale), curY, innerW - (12.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
                curY += descH + (16.0f * uiScale);
            }
            else
            {
                UIWidget::drawText(renderer, "No character data detected.", padX, curY, Theme::colors.textSecondary, uiScale * 0.85f);
                curY += (20.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::COMBAT_MOVES)
        {
            UIWidget::drawText(renderer, "Combat Deck & Tactical Moves:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            int selectedSlot = std::clamp(app->getSelectedCombatSlot(), 0, 9);
            std::string curMove = (p && !p->preparedCombatSlots[selectedSlot].empty()) ? p->preparedCombatSlots[selectedSlot] : "[ Empty Slot ]";

            // Active Slot Banner
            float bannerH = 46.0f * uiScale;
            SDL_FRect bannerRect = { padX, curY, innerW, bannerH };
            UIWidget::drawPanel(renderer, bannerRect, Theme::colors.bgSlot, Theme::colors.borderSelected);

            std::string bTitle = std::format("Target Deck Slot #{}: {}", selectedSlot + 1, curMove);
            UIWidget::drawText(renderer, bTitle, padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);

            std::string bSubtitle = "Click an Action Grid button below (Slots 1–10) to choose a target slot, then click any technique below to assign:";
            UIWidget::drawText(renderer, bSubtitle, padX + (12.0f * uiScale), curY + (26.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

            curY += bannerH + (12.0f * uiScale);

            // Category Filter Tabs
            static const std::vector<std::string> combatCategories = { "All", "Physical", "Spells", "Tactical" };
            int activeCombatCat = std::clamp(app->getCombatCategory(), 0, static_cast<int>(combatCategories.size()) - 1);

            float topBoxW = std::min(innerW, 580.0f * uiScale);
            float topBoxX = rect.x + (rect.w - topBoxW) / 2.0f;
            float topBoxH = 34.0f * uiScale;
            SDL_FRect topBoxRect = { topBoxX, curY, topBoxW, topBoxH };
            UIWidget::drawPanel(renderer, topBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float padBox = 4.0f * uiScale;
            float btnGap = 4.0f * uiScale;
            float btnH = topBoxH - (padBox * 2.0f);
            float btnW = (topBoxW - (padBox * 2.0f) - (btnGap * (combatCategories.size() - 1))) / static_cast<float>(combatCategories.size());

            for (size_t i = 0; i < combatCategories.size(); ++i)
            {
                SDL_FRect btnR = { topBoxX + padBox + i * (btnW + btnGap), topBoxRect.y + padBox, btnW, btnH };
                bool hov = (mousePos.x >= btnR.x && mousePos.x <= btnR.x + btnR.w &&
                            mousePos.y >= btnR.y && mousePos.y <= btnR.y + btnR.h);
                if (hov && clicked)
                {
                    app->setCombatCategory(static_cast<int>(i));
                    gameContext->input.consumeMouseClick();
                    gameContext->refreshActionGrid();
                }
                UIWidget::drawButton(renderer, btnR, combatCategories[i], hov, true, activeCombatCat == static_cast<int>(i), uiScale * 0.78f);
            }
            curY += topBoxH + (12.0f * uiScale);

            // Categorized Attack Techniques Catalog
            struct TechEntry {
                std::string name;
                std::string category;
                std::string desc;
                std::string cost;
            };
            static const std::vector<TechEntry> techCatalog = {
                // Physical
                { "Basic Strike", "Physical", "Standard physical attack dealing weapon damage.", "0 MP" },
                { "Heavy Slam", "Physical", "Heavy overhead strike dealing massive stagger and physical blunt force.", "0 MP" },
                { "Flurry", "Physical", "Swift flurry of strikes exploiting openings in enemy guard.", "5 Stamina" },
                { "Shield Bash", "Physical", "Bludgeoning shield strike stunning target for 1 turn.", "10 Stamina" },
                // Spells
                { "Arcane Dart", "Spells", "Swift concentrated mana projectile dealing piercing magic damage.", "10 MP" },
                { "Fireball", "Spells", "Explosive conflagration bursting foes for heavy fire damage.", "25 MP" },
                { "Arcane Shield", "Spells", "Conjured luminous barrier absorbing incoming attacks.", "20 MP" },
                { "Cleanse", "Spells", "Curative restoration purifying status ailments and recovering HP.", "15 MP" },
                // Tactical
                { "Defensive Guard", "Tactical", "Raise defensive posture mitigating 50% incoming damage.", "0 MP" },
                { "Evade", "Tactical", "Nimble evasive footwork greatly increasing dodge chance.", "0 MP" },
                { "Taunt", "Tactical", "Provocative gesture lowering enemy defense while boosting tension.", "0 MP" },
                { "Seduce", "Tactical", "Alluring advance distracting target and inflating enemy Lust.", "10 Lust" }
            };

            for (const auto& tech : techCatalog)
            {
                if (activeCombatCat == 1 && tech.category != "Physical") continue;
                if (activeCombatCat == 2 && tech.category != "Spells") continue;
                if (activeCombatCat == 3 && tech.category != "Tactical") continue;

                float tCardH = 34.0f * uiScale;
                SDL_FRect tCardRect = { padX, curY, innerW, tCardH };
                UIWidget::drawPanel(renderer, tCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                std::string tagStr = std::format("[{}]", tech.category);
                UIWidget::drawText(renderer, tech.name, padX + (10.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
                float tnW = UIWidget::getTextWidth(tech.name, uiScale * 0.82f);

                UIWidget::drawText(renderer, tagStr, padX + (16.0f * uiScale) + tnW, curY + (9.0f * uiScale), Theme::colors.textAccent, uiScale * 0.72f);
                float tgW = UIWidget::getTextWidth(tagStr, uiScale * 0.72f);

                UIWidget::drawText(renderer, "— " + tech.desc, padX + (22.0f * uiScale) + tnW + tgW, curY + (9.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

                SDL_FRect assignBtnRect = { padX + innerW - (115.0f * uiScale), curY + (4.0f * uiScale), 105.0f * uiScale, 26.0f * uiScale };
                bool isEquippedInCurSlot = (p && p->preparedCombatSlots[selectedSlot] == tech.name);
                bool asgHov = (mousePos.x >= assignBtnRect.x && mousePos.x <= assignBtnRect.x + assignBtnRect.w &&
                               mousePos.y >= assignBtnRect.y && mousePos.y <= assignBtnRect.y + assignBtnRect.h);
                if (asgHov && clicked && p && !isEquippedInCurSlot)
                {
                    p->preparedCombatSlots[selectedSlot] = tech.name;
                    app->setFeedbackText(std::format("Assigned '{}' to Deck Slot #{}.", tech.name, selectedSlot + 1));
                    gameContext->input.consumeMouseClick();
                    gameContext->refreshActionGrid();
                }
                std::string asgLabel = isEquippedInCurSlot ? std::format("[ In Slot {} ]", selectedSlot + 1) : std::format("Assign to #{}", selectedSlot + 1);
                UIWidget::drawButton(renderer, assignBtnRect, asgLabel, asgHov, !isEquippedInCurSlot, isEquippedInCurSlot, uiScale * 0.70f);

                curY += tCardH + (5.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::ELEMENTAL)
        {
            UIWidget::drawText(renderer, "Elemental Companion & Arcane Familiar:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            bool isSummoned = app->isElementalSummoned();
            bool isActiveForm = app->isElementalActiveForm();

            // Dossier Box
            float comH = 140.0f * uiScale;
            SDL_FRect comRect = { padX, curY, innerW, comH };
            UIWidget::drawPanel(renderer, comRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float inY = curY + (10.0f * uiScale);
            std::string title = isSummoned ? "Arcane Spirit Companion [Manifested]" : "Arcane Spirit Companion [Dormant in Plane]";
            UIWidget::drawText(renderer, title, padX + (12.0f * uiScale), inY, isSummoned ? Theme::colors.companion : Theme::colors.textMuted, uiScale * 0.92f);
            inY += (22.0f * uiScale);

            std::string formStr = isActiveForm ? "Aspect: Humanoid Battle Form (Tactical Combat)" : "Aspect: Wisp Familiar Form (Compact Pet)";
            UIWidget::drawText(renderer, formStr, padX + (12.0f * uiScale), inY, Theme::colors.arcane, uiScale * 0.82f);
            inY += (20.0f * uiScale);

            UIWidget::drawText(renderer, "Bond Resonance: Tier 3 (Deep Arcane Attunement)  |  Element: Starlight & Aether", padX + (12.0f * uiScale), inY, Theme::colors.textSecondary, uiScale * 0.78f);
            inY += (20.0f * uiScale);

            std::string desc = "A sentient arcane familiar manifested through your magical aura. It loyally aids your adventures, able to condense into a shoulder wisp or assume an agile humanoid battle form.";
            UIWidget::drawTextWrapped(renderer, desc, padX + (12.0f * uiScale), inY, innerW - (24.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

            curY += comH + (14.0f * uiScale);

            // Companion Interaction Controls
            float cBoxW = std::min(innerW, 640.0f * uiScale);
            float cBoxX = rect.x + (rect.w - cBoxW) / 2.0f;
            float cBoxH = 40.0f * uiScale;
            SDL_FRect cBoxRect = { cBoxX, curY, cBoxW, cBoxH };
            UIWidget::drawPanel(renderer, cBoxRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            struct ElemAct {
                std::string label;
                std::function<void()> act;
                bool enabled;
            };
            std::vector<ElemAct> eActs = {
                {
                    isSummoned ? "Dispel Companion" : "Manifest Companion",
                    [=]() {
                        app->toggleElementalSummoned();
                        app->setFeedbackText(app->isElementalSummoned() ? "Manifested elemental companion into reality." : "Dismissed elemental companion back to the spirit plane.");
                    },
                    true
                },
                {
                    isActiveForm ? "Passive Form" : "Active Form",
                    [=]() {
                        app->toggleElementalActiveForm();
                        app->setFeedbackText(app->isElementalActiveForm() ? "Elemental shifted into humanoid battle form." : "Elemental condensed into wisp creature form.");
                    },
                    isSummoned
                },
                {
                    "Commune",
                    [=]() {
                        app->setFeedbackText("You commune telepathically with the elemental. It transmits feelings of warmth and unwavering loyalty.");
                    },
                    isSummoned
                },
                {
                    "Pet Familiar",
                    [=]() {
                        entity* p = gameContext->getPlayer();
                        if (p) p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 10.0f));
                        app->setFeedbackText("You gently stroke the elemental's warm, shimmering aura. It purrs softly (+10 Mana).");
                    },
                    isSummoned
                }
            };

            float epPad = 4.0f * uiScale;
            float epGap = 4.0f * uiScale;
            float epBtnH = cBoxH - (epPad * 2.0f);
            float epBtnW = (cBoxW - (epPad * 2.0f) - (epGap * (eActs.size() - 1))) / static_cast<float>(eActs.size());

            for (size_t i = 0; i < eActs.size(); ++i)
            {
                SDL_FRect ebR = { cBoxX + epPad + i * (epBtnW + epGap), cBoxRect.y + epPad, epBtnW, epBtnH };
                bool hov = (mousePos.x >= ebR.x && mousePos.x <= ebR.x + ebR.w &&
                            mousePos.y >= ebR.y && mousePos.y <= ebR.y + ebR.h);
                if (hov && clicked && eActs[i].enabled)
                {
                    eActs[i].act();
                    gameContext->input.consumeMouseClick();
                    gameContext->refreshActionGrid();
                }
                UIWidget::drawButton(renderer, ebR, eActs[i].label, hov, eActs[i].enabled, false, uiScale * 0.78f);
            }
            curY += cBoxH + (12.0f * uiScale);
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

    // Page state for Player and Target/Ground side (0..4 for pages 1..5, 5 for Key Items)
    static int s_playerInvPage = 0;
    static int s_targetInvPage = 0;

    float renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = rect.x + (6.0f * uiScale);
        float availableW = rect.w - (12.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        float gapBetweenSides = 8.0f * uiScale;
        float halfW = std::floor((availableW - gapBetweenSides) / 2.0f);

        entity* player = gameContext->getPlayer();
        entity* targetNpc = gameContext->getActiveTargetNPC();

        // -------------------------------------------------------------------------
        // HELPER LAMBDA: Render a 5x4 Grid Side (Side 0: Player, Side 1: Target/Floor)
        // -------------------------------------------------------------------------
        auto renderInventorySide = [&](int side, float sideX, float sideW, entity* ent, int& activePage) -> float {
            float sY = curY;
            float headerH = 22.0f * uiScale;
            SDL_FRect hRect = { sideX, sY, sideW, headerH };

            std::string sideTitle = (side == 0)
                ? "PLAYER INVENTORY"
                : (ent ? std::format("NPC: {}'S INVENTORY", ent->name) : "FLOOR / GROUND STORAGE");

            UIWidget::drawHeader(renderer, hRect, sideTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.76f);
            sY += headerH + (4.0f * uiScale);

            // 5x4 Grid: 5 columns x 4 rows = 20 slots
            const int cols = 5;
            const int rows = 4;
            const float slotGap = 4.0f * uiScale;
            const float innerPad = 4.0f * uiScale;
            const float tabW = 28.0f * uiScale;
            const float tabColGap = 4.0f * uiScale;

            float gridInnerW = sideW - tabW - tabColGap - (innerPad * 2.0f);
            float slotSize = std::floor((gridInnerW - (slotGap * static_cast<float>(cols - 1))) / static_cast<float>(cols));
            float totalGridW = (slotSize * cols) + (slotGap * static_cast<float>(cols - 1));
            float totalGridH = (slotSize * rows) + (slotGap * static_cast<float>(rows - 1));
            float containerH = totalGridH + (innerPad * 2.0f);

            // Container card
            SDL_FRect containerCard = { sideX, sY, sideW, containerH };
            UIWidget::drawPanel(renderer, containerCard, Theme::colors.bgSlot, Theme::colors.borderNormal);

            // Positioning:
            // Side 0 (Player): Vertical selector on LEFT outer edge
            // Side 1 (Target/Floor): Vertical selector on RIGHT outer edge
            float tabColX = (side == 0) ? (sideX + innerPad) : (sideX + innerPad + totalGridW + tabColGap);
            float gridStartX = (side == 0) ? (sideX + innerPad + tabW + tabColGap) : (sideX + innerPad);

            // 6 Vertical Page Selector Tabs: [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ Key ]
            static const std::vector<std::string> pageTabs = { "1", "2", "3", "4", "5", "Key" };
            float tabGap = 3.0f * uiScale;
            float tabBtnH = std::floor((totalGridH - (tabGap * 5.0f)) / 6.0f);

            for (size_t p = 0; p < pageTabs.size(); ++p)
            {
                SDL_FRect pRect = { tabColX, sY + innerPad + (p * (tabBtnH + tabGap)), tabW, tabBtnH };
                bool isPageActive = (activePage == static_cast<int>(p));
                bool pHov = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                             mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);

                UIWidget::drawButton(renderer, pRect, pageTabs[p], pHov, true, isPageActive, uiScale * 0.68f);

                std::string pTitle = (p == 5) ? "Key Items Page" : std::format("Inventory Page {}", p + 1);
                std::string pDesc = (p == 5) ? "Dedicated storage for key quest items, special keys, and relics." : std::format("Item slots {} to {}.", (p * 20) + 1, (p + 1) * 20);
                std::string pSub = (side == 0) ? "Player Backpack" : (ent ? std::format("{}'s Inventory", ent->name) : "Floor Storage");
                TooltipManager::setHoverTooltip(pRect, mousePos, pTitle, pDesc, pSub, std::format("[ Page {} ]", pageTabs[p]));

                if (pHov && clicked)
                {
                    activePage = static_cast<int>(p);
                    gameContext->input.consumeMouseClick();
                }
            }

            // Gather items for this side
            std::vector<InventorySlot> allItems;
            if (side == 0)
            {
                allItems = gameContext->getPlayerInventoryStacked();
            }
            else
            {
                allItems = gameContext->getTileInventoryStacked();
            }

            // Partition items: regular items vs key items
            std::vector<std::pair<size_t, InventorySlot>> regularItems;
            std::vector<std::pair<size_t, InventorySlot>> keyItems;

            for (size_t i = 0; i < allItems.size(); ++i)
            {
                if (!allItems[i].itemPtr) continue;
                if (allItems[i].itemPtr->isKeyItem ||
                    (allItems[i].itemPtr->targetSlot == equipSlot::NONE && allItems[i].itemPtr->id.find("key") != std::string::npos))
                {
                    keyItems.push_back({ i, allItems[i] });
                }
                else
                {
                    regularItems.push_back({ i, allItems[i] });
                }
            }

            int startItemIdx = (activePage == 5) ? 0 : (activePage * 20);
            const auto& activeList = (activePage == 5) ? keyItems : regularItems;

            for (int r = 0; r < rows; ++r)
            {
                for (int c = 0; c < cols; ++c)
                {
                    int slotIndex = (r * cols) + c;
                    int itemIdxInList = startItemIdx + slotIndex;

                    float bX = gridStartX + (c * (slotSize + slotGap));
                    float bY = sY + innerPad + (r * (slotSize + slotGap));
                    SDL_FRect bRect = { bX, bY, slotSize, slotSize };

                    bool hasItem = (itemIdxInList >= 0 && itemIdxInList < static_cast<int>(activeList.size()));
                    size_t origBackpackIdx = hasItem ? activeList[itemIdxInList].first : 999999;
                    bool isSelected = (hasItem && gameContext->selectedInventorySide == side &&
                                       gameContext->selectedInventoryIndex == static_cast<int>(origBackpackIdx));

                    bool bHov = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                 mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

                    if (hasItem)
                    {
                        const auto& itSlot = activeList[itemIdxInList].second;
                        SDL_Color fill = isSelected ? Theme::colors.bgHeader : (bHov ? Theme::colors.bgHeader : Theme::colors.bgDark);
                        SDL_Color bd = isSelected ? Theme::colors.borderSelected : (bHov ? Theme::colors.borderButton : Theme::colors.borderButton);
                        UIWidget::drawPanel(renderer, bRect, fill, bd);

                        // Item name (upper portion)
                        std::string itName = itSlot.itemPtr->name;
                        if (itName.length() > 9) itName = itName.substr(0, 8) + "..";
                        UIWidget::drawText(renderer, itName, bX + (3.0f * uiScale), bY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.65f);

                        // Count badge (top right)
                        if (itSlot.totalCount > 1)
                        {
                            std::string cntStr = std::format("x{}", itSlot.totalCount);
                            float cntW = UIWidget::getTextWidth(cntStr, uiScale * 0.62f);
                            UIWidget::drawText(renderer, cntStr, bX + slotSize - cntW - (3.0f * uiScale), bY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.62f);
                        }

                        // Socket/Type abbreviation badge
                        std::string sockStr = "";
                        if (itSlot.itemPtr->targetSlot != equipSlot::NONE)
                        {
                            sockStr = gameContext->formatEquipSlotName(itSlot.itemPtr->targetSlot);
                            if (sockStr.length() > 6) sockStr = sockStr.substr(0, 5);
                        }
                        else if (itSlot.itemPtr->isConsumable)
                        {
                            sockStr = "Item";
                        }
                        if (!sockStr.empty())
                        {
                            UIWidget::drawText(renderer, sockStr, bX + (3.0f * uiScale), bY + slotSize - (24.0f * uiScale), Theme::colors.textAccent, uiScale * 0.56f);
                        }

                        // Base Value (bottom right)
                        std::string valStr = std::format("{}¤", itSlot.itemPtr->baseValue);
                        float valW = UIWidget::getTextWidth(valStr, uiScale * 0.58f);
                        UIWidget::drawText(renderer, valStr, bX + slotSize - valW - (3.0f * uiScale), bY + slotSize - (13.0f * uiScale), Theme::colors.currency, uiScale * 0.58f);

                        std::string targetStr = (itSlot.itemPtr->targetSlot != equipSlot::NONE) ? gameContext->formatEquipSlotName(itSlot.itemPtr->targetSlot) : (itSlot.itemPtr->isConsumable ? "Consumable Item" : "Inventory Item");
                        std::string subInfo = std::format("{} • Base Value: {} ¤", targetStr, itSlot.itemPtr->baseValue);
                        std::string cntHotkey = (itSlot.totalCount > 1) ? std::format("x{}", itSlot.totalCount) : "";
                        std::string itemDesc = !itSlot.itemPtr->tooltip.empty() ? itSlot.itemPtr->tooltip : itSlot.itemPtr->description;
                        TooltipManager::setHoverTooltip(bRect, mousePos, itSlot.itemPtr->name, itemDesc, subInfo, cntHotkey);

                        if (bHov && clicked)
                        {
                            gameContext->selectedInventorySide = side;
                            gameContext->selectedInventoryIndex = static_cast<int>(origBackpackIdx);
                            gameContext->selectedEquipmentSlot = equipSlot::NONE;
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }
                    }
                    else
                    {
                        // Empty slot: clean dark panel with clear border (no noisy tooltip)
                        SDL_Color fill = bHov ? Theme::colors.bgHeader : Theme::colors.bgDark;
                        SDL_Color bd = bHov ? Theme::colors.borderButton : Theme::colors.slotEmptyBorder;
                        UIWidget::drawPanel(renderer, bRect, fill, bd);

                        float dotW = UIWidget::getTextWidth("·", uiScale * 0.62f);
                        UIWidget::drawText(renderer, "·", bX + ((slotSize - dotW) / 2.0f), bY + ((slotSize - (10.0f * uiScale)) / 2.0f), Theme::colors.textDisabled, uiScale * 0.62f);
                    }
                }
            }

            sY += containerH + (4.0f * uiScale);
            return (sY - curY);
        };

        // Render Left Grid: Player (Page tabs on LEFT outer edge)
        float playerGridH = renderInventorySide(0, padX, halfW, player, s_playerInvPage);

        // Render Right Grid: Target NPC or Floor (Page tabs on RIGHT outer edge)
        float targetGridH = renderInventorySide(1, padX + halfW + gapBetweenSides, halfW, targetNpc, s_targetInvPage);

        curY += std::max(playerGridH, targetGridH);
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
        int pX = gameContext->gridX;
        int pY = gameContext->gridY;

        std::string locTitle = "Unknown Area";
        if (m && !m->getName().empty())
        {
            locTitle = m->getName();
        }

        // 1. Zone Header Banner
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, locTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        // 2. Build Dynamic Narrative Prose
        std::string p1 = m ? m->getTileDescription(pX, pY) : "You look around your surroundings.";

        // Time / Lighting Atmosphere
        std::string timeAtmosphere = "";
        TimePhase phase = gameContext->getTime().getPhase();
        if (phase == TimePhase::NIGHT)
        {
            timeAtmosphere = "Night has settled over the realm. Deep shadows drape across the area, illuminated only by ambient wall-sconces and the gentle glow of the moons.";
        }
        else if (phase == TimePhase::DAWN)
        {
            timeAtmosphere = "The pale morning light of dawn filters through the sky, casting long, soft shadows across the ground.";
        }
        else if (phase == TimePhase::DUSK)
        {
            timeAtmosphere = "The sun is sinking low on the horizon, bathing the environment in warm hues of amber and violet.";
        }
        else
        {
            timeAtmosphere = "Clear daylight brightens the surroundings, granting crisp visibility across the paths and structures.";
        }

        // Context / Interactive Objects on Tile
        std::vector<std::string> contextParagraphs;

        if (m)
        {
            auto triggers = m->getTriggersAt(pX, pY);
            for (const auto& trig : triggers)
            {
                if (gameContext->checkConditions(trig.conditions) && !trig.description.empty())
                {
                    contextParagraphs.push_back(trig.description);
                }
            }

            auto& tileData = const_cast<gameMap*>(m)->getRuntimeData(pX, pY);
            if (tileData.persistentNPC)
            {
                contextParagraphs.push_back(std::format("{} is standing here.", tileData.persistentNPC->name));
            }
            if (!tileData.droppedItems.empty())
            {
                if (tileData.droppedItems.size() == 1)
                {
                    contextParagraphs.push_back(std::format("You notice {} lying on the ground here.", tileData.droppedItems[0].itemPtr ? tileData.droppedItems[0].itemPtr->name : "an item"));
                }
                else
                {
                    contextParagraphs.push_back(std::format("You notice {} items scattered on the ground here.", tileData.droppedItems.size()));
                }
            }
            MapWarp warp;
            if (m->checkWarp(pX, pY, warp))
            {
                std::string targetName = warp.targetMap;
                if (targetName == "house_01") targetName = "the Cozy Cottage";
                else if (targetName == "overworld") targetName = "the Town District avenues";
                contextParagraphs.push_back(std::format("A doorway or threshold here leads toward {}.", targetName));
            }
        }

        SDL_FRect cardRect = { padX, curY, innerW, 120.0f * uiScale };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float textY = curY + (12.0f * uiScale);
        float textX = padX + (14.0f * uiScale);
        float textW = innerW - (28.0f * uiScale);

        float h1 = UIWidget::drawTextWrapped(renderer, p1, textX, textY, textW, Theme::colors.textPrimary, uiScale * 0.84f);
        textY += h1 + (10.0f * uiScale);

        float h2 = UIWidget::drawTextWrapped(renderer, timeAtmosphere, textX, textY, textW, Theme::colors.textSecondary, uiScale * 0.82f);
        textY += h2 + (10.0f * uiScale);

        for (const auto& cp : contextParagraphs)
        {
            float hp = UIWidget::drawTextWrapped(renderer, cp, textX, textY, textW, Theme::colors.textGold, uiScale * 0.84f);
            textY += hp + (8.0f * uiScale);
        }

        cardRect.h = (textY - curY) + (4.0f * uiScale);
        curY += cardRect.h + (12.0f * uiScale);

        return (curY - startY);
    }

    float renderShopView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        auto shop = dynamic_cast<shopState*>(gameContext->getActiveState());
        entity* player = gameContext->getPlayer();
        std::shared_ptr<entity> merchant = shop ? shop->getMerchant() : gameContext->getActiveTargetNPCShared();
        if (!merchant) merchant = gameContext->getActiveTargetNPCShared();

        float padX = rect.x + (6.0f * uiScale);
        float availableW = rect.w - (12.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // 1. Top Header Banner
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        std::string shopTitle = merchant ? std::format("MERCHANT STORE - {}", merchant->name) : "MERCHANT TRADING POST";
        UIWidget::drawHeader(renderer, headerRect, shopTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (6.0f * uiScale);

        // 2. Market Information Bar (Player Purse, Merchant Funds, Valuation Modifiers)
        float infoBarH = 24.0f * uiScale;
        SDL_FRect infoBarRect = { padX, curY, availableW, infoBarH };
        UIWidget::drawPanel(renderer, infoBarRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float pPurse = player->getStat("currency");
        float mFunds = merchant ? merchant->getStat("currency") : 0.0f;
        std::string pPurseStr = std::format("Purse: {:.0f}¤", pPurse);
        std::string mFundsStr = std::format("Shop Funds: {:.0f}¤", mFunds);

        float buyMarkupPct = merchant ? (merchant->buyMarkup - 1.0f) * 100.0f : 20.0f;
        float sellMarkdownPct = merchant ? merchant->sellMarkdown * 100.0f : 55.0f;
        float barterBonusPct = player->tradePerkModifier * 100.0f;

        std::string ratesStr = std::format("Rates: Buy +{:.0f}% | Sell {:.0f}% | Barter +{:.0f}%", buyMarkupPct, sellMarkdownPct, barterBonusPct);

        UIWidget::drawText(renderer, pPurseStr, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.currency, uiScale * 0.78f);
        UIWidget::drawText(renderer, ratesStr, padX + (availableW / 2.0f) - (UIWidget::getTextWidth(ratesStr, uiScale * 0.72f) / 2.0f), curY + (5.0f * uiScale), Theme::colors.textAccent, uiScale * 0.72f);
        float mFundsW = UIWidget::getTextWidth(mFundsStr, uiScale * 0.78f);
        UIWidget::drawText(renderer, mFundsStr, padX + availableW - mFundsW - (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

        curY += infoBarH + (6.0f * uiScale);

        // 3. Feedback Banner (if any)
        if (shop && !shop->getFeedbackText().empty())
        {
            float fbH = 22.0f * uiScale;
            SDL_FRect fbRect = { padX, curY, availableW, fbH };
            UIWidget::drawPanel(renderer, fbRect, Theme::colors.bgHeader, Theme::colors.borderSelected);
            float fbW = UIWidget::getTextWidth(shop->getFeedbackText(), uiScale * 0.76f);
            UIWidget::drawText(renderer, shop->getFeedbackText(), padX + ((availableW - fbW) / 2.0f), curY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.76f);
            curY += fbH + (6.0f * uiScale);
        }

        // 4. Two-Sided Inventory View
        float gapBetweenSides = 8.0f * uiScale;
        float halfW = std::floor((availableW - gapBetweenSides) / 2.0f);

        int playerPage = shop ? shop->getPlayerPage() : 0;
        int merchantPage = shop ? shop->getMerchantPage() : 0;

        auto renderTradingSide = [&](int side, float sideX, float sideW, entity* ent, int& activePage) -> float {
            float sY = curY;
            float sideHeaderH = 22.0f * uiScale;
            SDL_FRect hRect = { sideX, sY, sideW, sideHeaderH };

            std::string sideTitle = (side == 0)
                ? std::format("YOUR INVENTORY (SELL)")
                : (ent ? std::format("{}'S STOCK (BUY)", ent->name) : "MERCHANT WARES (BUY)");

            UIWidget::drawHeader(renderer, hRect, sideTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.76f);
            sY += sideHeaderH + (4.0f * uiScale);

            const int cols = 5;
            const int rows = 4;
            const float slotGap = 4.0f * uiScale;
            const float innerPad = 4.0f * uiScale;
            const float tabW = 28.0f * uiScale;
            const float tabColGap = 4.0f * uiScale;

            float gridInnerW = sideW - tabW - tabColGap - (innerPad * 2.0f);
            float slotSize = std::floor((gridInnerW - (slotGap * static_cast<float>(cols - 1))) / static_cast<float>(cols));
            float totalGridW = (slotSize * cols) + (slotGap * static_cast<float>(cols - 1));
            float totalGridH = (slotSize * rows) + (slotGap * static_cast<float>(rows - 1));
            float containerH = totalGridH + (innerPad * 2.0f);

            SDL_FRect containerCard = { sideX, sY, sideW, containerH };
            UIWidget::drawPanel(renderer, containerCard, Theme::colors.bgSlot, Theme::colors.borderNormal);

            // Positioning:
            // Side 0 (Player): Vertical selector on LEFT outer edge
            // Side 1 (Merchant): Vertical selector on RIGHT outer edge
            float tabColX = (side == 0) ? (sideX + innerPad) : (sideX + innerPad + totalGridW + tabColGap);
            float gridStartX = (side == 0) ? (sideX + innerPad + tabW + tabColGap) : (sideX + innerPad);

            // 6 Vertical Page Selector Tabs
            static const std::vector<std::string> pageTabs = { "1", "2", "3", "4", "5", "Key" };
            float tabGap = 3.0f * uiScale;
            float tabBtnH = std::floor((totalGridH - (tabGap * 5.0f)) / 6.0f);

            for (size_t p = 0; p < pageTabs.size(); ++p)
            {
                SDL_FRect pRect = { tabColX, sY + innerPad + (p * (tabBtnH + tabGap)), tabW, tabBtnH };
                bool isPageActive = (activePage == static_cast<int>(p));
                bool pHov = (mousePos.x >= pRect.x && mousePos.x <= pRect.x + pRect.w &&
                             mousePos.y >= pRect.y && mousePos.y <= pRect.y + pRect.h);

                UIWidget::drawButton(renderer, pRect, pageTabs[p], pHov, true, isPageActive, uiScale * 0.68f);

                std::string pTitle = (p == 5) ? "Key Items Page" : std::format("Inventory Page {}", p + 1);
                std::string pDesc = (p == 5) ? "Dedicated storage for key quest items, special keys, and relics." : std::format("Item slots {} to {}.", (p * 20) + 1, (p + 1) * 20);
                std::string pSub = (side == 0) ? "Your Backpack" : (ent ? std::format("{}'s Stock", ent->name) : "Merchant Stock");
                TooltipManager::setHoverTooltip(pRect, mousePos, pTitle, pDesc, pSub, std::format("[ Page {} ]", pageTabs[p]));

                if (pHov && clicked)
                {
                    activePage = static_cast<int>(p);
                    if (shop)
                    {
                        if (side == 0) shop->setPlayerPage(activePage);
                        else shop->setMerchantPage(activePage);
                    }
                    gameContext->input.consumeMouseClick();
                }
            }

            // Gather items for this side
            std::vector<InventorySlot> allItems = ent ? ent->inventory.getStackedView() : std::vector<InventorySlot>{};

            // Partition items: regular items vs key items
            std::vector<std::pair<size_t, InventorySlot>> regularItems;
            std::vector<std::pair<size_t, InventorySlot>> keyItems;

            for (size_t i = 0; i < allItems.size(); ++i)
            {
                if (!allItems[i].itemPtr) continue;
                if (allItems[i].itemPtr->isKeyItem ||
                    (allItems[i].itemPtr->targetSlot == equipSlot::NONE && allItems[i].itemPtr->id.find("key") != std::string::npos))
                {
                    keyItems.push_back({ i, allItems[i] });
                }
                else
                {
                    regularItems.push_back({ i, allItems[i] });
                }
            }

            int startItemIdx = (activePage == 5) ? 0 : (activePage * 20);
            const auto& activeList = (activePage == 5) ? keyItems : regularItems;

            for (int r = 0; r < rows; ++r)
            {
                for (int c = 0; c < cols; ++c)
                {
                    int slotIndex = (r * cols) + c;
                    int itemIdxInList = startItemIdx + slotIndex;

                    float bX = gridStartX + (c * (slotSize + slotGap));
                    float bY = sY + innerPad + (r * (slotSize + slotGap));
                    SDL_FRect bRect = { bX, bY, slotSize, slotSize };

                    bool hasItem = (itemIdxInList >= 0 && itemIdxInList < static_cast<int>(activeList.size()));
                    size_t origBackpackIdx = hasItem ? activeList[itemIdxInList].first : 999999;
                    bool isSelected = (hasItem && gameContext->selectedInventorySide == side &&
                                       gameContext->selectedInventoryIndex == static_cast<int>(origBackpackIdx));

                    bool bHov = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                 mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

                    if (hasItem)
                    {
                        const auto& itSlot = activeList[itemIdxInList].second;
                        SDL_Color fill = isSelected ? Theme::colors.bgHeader : (bHov ? Theme::colors.bgHeader : Theme::colors.bgDark);
                        SDL_Color bd = isSelected ? Theme::colors.borderSelected : (bHov ? Theme::colors.borderButton : Theme::colors.borderButton);
                        UIWidget::drawPanel(renderer, bRect, fill, bd);

                        // Item name (upper portion)
                        std::string itName = itSlot.itemPtr->name;
                        if (itName.length() > 9) itName = itName.substr(0, 8) + "..";
                        UIWidget::drawText(renderer, itName, bX + (3.0f * uiScale), bY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.65f);

                        // Count badge (top right)
                        if (itSlot.totalCount > 1)
                        {
                            std::string cntStr = std::format("x{}", itSlot.totalCount);
                            float cntW = UIWidget::getTextWidth(cntStr, uiScale * 0.62f);
                            UIWidget::drawText(renderer, cntStr, bX + slotSize - cntW - (3.0f * uiScale), bY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.62f);
                        }

                        // Socket/Type abbreviation badge
                        std::string sockStr = "";
                        if (itSlot.itemPtr->targetSlot != equipSlot::NONE)
                        {
                            sockStr = gameContext->formatEquipSlotName(itSlot.itemPtr->targetSlot);
                            if (sockStr.length() > 6) sockStr = sockStr.substr(0, 5);
                        }
                        else if (itSlot.itemPtr->isConsumable)
                        {
                            sockStr = "Item";
                        }
                        if (!sockStr.empty())
                        {
                            UIWidget::drawText(renderer, sockStr, bX + (3.0f * uiScale), bY + slotSize - (24.0f * uiScale), Theme::colors.textAccent, uiScale * 0.56f);
                        }

                        // Buy/Sell Price on slot box (bottom right)
                        int tradePrice = (side == 0)
                            ? merchantValuation::calculateSellPrice(itSlot.itemPtr.get(), player, merchant.get())
                            : merchantValuation::calculateBuyPrice(itSlot.itemPtr.get(), player, merchant.get());

                        std::string priceStr = std::format("{}¤", tradePrice);
                        float valW = UIWidget::getTextWidth(priceStr, uiScale * 0.58f);
                        UIWidget::drawText(renderer, priceStr, bX + slotSize - valW - (3.0f * uiScale), bY + slotSize - (13.0f * uiScale), Theme::colors.currency, uiScale * 0.58f);

                        // Tooltip
                        std::string targetStr = (itSlot.itemPtr->targetSlot != equipSlot::NONE)
                            ? gameContext->formatEquipSlotName(itSlot.itemPtr->targetSlot)
                            : (itSlot.itemPtr->isConsumable ? "Consumable Item" : "Inventory Item");

                        std::string subInfo = (side == 0)
                            ? std::format("{} • Sell: {}¤ • Base: {}¤", targetStr, tradePrice, itSlot.itemPtr->baseValue)
                            : std::format("{} • Buy: {}¤ • Base: {}¤", targetStr, tradePrice, itSlot.itemPtr->baseValue);

                        std::string itemDesc = !itSlot.itemPtr->tooltip.empty() ? itSlot.itemPtr->tooltip : itSlot.itemPtr->description;
                        std::string cntHotkey = (side == 0) ? "[ Click to Sell ]" : "[ Click to Buy ]";
                        TooltipManager::setHoverTooltip(bRect, mousePos, itSlot.itemPtr->name, itemDesc, subInfo, cntHotkey);

                        if (bHov && clicked)
                        {
                            gameContext->selectedInventorySide = side;
                            gameContext->selectedInventoryIndex = static_cast<int>(origBackpackIdx);
                            gameContext->selectedEquipmentSlot = equipSlot::NONE;
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }
                    }
                    else
                    {
                        // Empty slot
                        SDL_Color fill = bHov ? Theme::colors.bgHeader : Theme::colors.bgDark;
                        SDL_Color bd = bHov ? Theme::colors.borderButton : Theme::colors.slotEmptyBorder;
                        UIWidget::drawPanel(renderer, bRect, fill, bd);

                        float dotW = UIWidget::getTextWidth("·", uiScale * 0.62f);
                        UIWidget::drawText(renderer, "·", bX + ((slotSize - dotW) / 2.0f), bY + ((slotSize - (10.0f * uiScale)) / 2.0f), Theme::colors.textDisabled, uiScale * 0.62f);
                    }
                }
            }

            sY += containerH + (4.0f * uiScale);
            return (sY - curY);
        };

        // Render Left Grid: Player Inventory (side 0)
        float playerGridH = renderTradingSide(0, padX, halfW, player, playerPage);

        // Render Right Grid: Merchant Inventory (side 1)
        float merchantGridH = renderTradingSide(1, padX + halfW + gapBetweenSides, halfW, merchant.get(), merchantPage);

        curY += std::max(playerGridH, merchantGridH);
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
