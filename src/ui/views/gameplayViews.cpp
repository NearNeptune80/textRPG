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

        // Feedback notification banner (e.g. rest feedback, masturbation climax)
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
            std::string notif1 = "• Active Quest: Lilaya's Tests — Chapter 1: The New World";
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
                { "2", "Perk Tree", "Browse unlocked talents, character perks, and passive traits." },
                { "3", "Spells & Moves", "Review arcane spells, physical moves, and MP costs." },
                { "4", "Stats & Diagnostics", "Check attributes, combat ratings, and anatomical dimensions." },
                { "5", "Contacts", "Directory of known characters, companions, and relationships." },
                { "6", "Encyclopedia", "Access world lore, demon species biology, and bestiary." },
                { "7", "Regional Maps", "Dominion map overview, district status, and points of interest." },
                { "8", "Transformations", "Inspect bodily morphs, mutations, horns, wings, and tails." },
                { "9", "Wait & Rest", "Pass in-game time, sleep, and recover health/mana." },
                { "10", "Solo Intimacy", "Take a secluded moment to relieve lust and built-up arousal." },
                { "11", "Fetishes", "Configure gameplay content filters and preference ratings." }
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

            std::string pDesc = "You can take a brief break, meditate, or sleep to allow time to pass. Resting naturally regenerates your Health and Mana over time, while advancing world events and merchant restocking cycles.";
            float pH = UIWidget::drawTextWrapped(renderer, pDesc, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += pH + (16.0f * uiScale);

            std::string statInfo = p ? std::format("Current Vitals: {:.0f}/{:.0f} Health | {:.0f}/{:.0f} Mana",
                p->getStat("health"), std::max(100.0f, p->getStat("max_health")),
                p->getStat("mana"), std::max(80.0f, p->getStat("max_mana"))) : "";
            UIWidget::drawText(renderer, statInfo, padX, curY, Theme::colors.friendly, uiScale * 0.85f);
            curY += (20.0f * uiScale);

            UIWidget::drawText(renderer, "Select how long you wish to wait or rest using the action buttons below.", padX, curY, Theme::colors.textSecondary, uiScale * 0.82f);
            curY += (20.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::MASTURBATE)
        {
            UIWidget::drawText(renderer, "Solo Intimacy & Self-Pleasure:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            float curLust = p ? p->getStat("lust") : 0.0f;
            float curArousal = p ? p->getStat("arousal") : 0.0f;

            std::string lustGauge = std::format("Current Lust: {:.0f}%  |  Arousal: {:.0f}%", curLust, curArousal);
            UIWidget::drawText(renderer, lustGauge, padX, curY, Theme::colors.lust, uiScale * 0.88f);
            curY += (20.0f * uiScale);

            std::string intro = "Finding a quiet, secluded alcove away from prying eyes, you allow your thoughts to wander toward sensual indulgence. Heat steadily tingles across your skin as your fingers explore sensitive curves and private contours.";
            float iH = UIWidget::drawTextWrapped(renderer, intro, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += iH + (16.0f * uiScale);

            if (p)
            {
                std::string anatomyDetails;
                const bodyPart* b = p->anatomy.getPart(bodySlot::BREASTS);
                if (b && b->cupSize > 0)
                {
                    anatomyDetails += std::format("Your {} cup breasts swell with warm sensitivity to the touch. ",
                        bodyPart::getCupSizeName(b->cupSize));
                }
                if (p->anatomy.hasVagina())
                {
                    anatomyDetails += "A slick wetness gathers between your thighs, inviting deeper stimulation. ";
                }
                if (p->anatomy.hasPenis())
                {
                    anatomyDetails += "Your shaft pulses with firm, eager warmth in your grasp. ";
                }
                if (!anatomyDetails.empty())
                {
                    float aH = UIWidget::drawTextWrapped(renderer, anatomyDetails, padX, curY, innerW, Theme::colors.textSecondary, uiScale * 0.82f);
                    curY += aH + (16.0f * uiScale);
                }
            }

            UIWidget::drawText(renderer, "Select an intimate action from the buttons below to build arousal toward climax.", padX, curY, Theme::colors.textGold, uiScale * 0.82f);
            curY += (20.0f * uiScale);
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
                std::string morphNote = "Morphological changes stabilize dynamically as demonic elixirs or arcane transformations take effect. Select 'Open Full Editor' below to enter the full transformation studio.";
                float mH = UIWidget::drawTextWrapped(renderer, morphNote, padX, curY, innerW, Theme::colors.textSecondary, uiScale * 0.82f);
                curY += mH + (16.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::STATS)
        {
            UIWidget::drawText(renderer, "Character Diagnostics & Statistics:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            entity* p = gameContext->getPlayer();
            if (p)
            {
                UIWidget::drawText(renderer, "Core Attributes:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
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
                UIWidget::drawText(renderer, "Body Dimensions:", padX, curY, Theme::colors.textAccent, uiScale * 0.90f);
                curY += (18.0f * uiScale);

                std::string bDim = std::format("Body Size: {} | Muscle Tone: {} | Height: {:.2f}m",
                    p->anatomy.bodySize, p->anatomy.muscleTone, p->anatomy.heightMeters);
                UIWidget::drawText(renderer, bDim, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.82f);
                curY += (20.0f * uiScale);
            }
        }
        else if (app->getAppMode() == PhoneAppMode::FETISHES)
        {
            UIWidget::drawText(renderer, "Fetishes & Content Preferences:", padX, curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (22.0f * uiScale);

            std::string fDesc = "TextRPG features a comprehensive content filter and demographic preference matrix. You can adjust your attraction ratings, taboo settings, and anatomical generation weights directly in the options editor.";
            float fH = UIWidget::drawTextWrapped(renderer, fDesc, padX, curY, innerW, Theme::colors.textPrimary, uiScale * 0.85f);
            curY += fH + (16.0f * uiScale);

            static const std::vector<std::string> cats = {
                "• Category 0: Miscellaneous & Narrative Toggles",
                "• Category 1: Gameplay Mechanics & Encounter Balancing",
                "• Category 2: Sex & Physical Intimacy Scenarios",
                "• Category 3: Bodies, Transformations & Extreme Morphs",
                "• Categories 4-7: Gender Demographics (Male, Female, Hermaphrodite)",
                "• Category 8: Granular Fetish Ratings (Dislike, Neutral, Like, Love)"
            };

            for (const auto& cat : cats)
            {
                UIWidget::drawText(renderer, cat, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.80f);
                curY += (18.0f * uiScale);
            }

            curY += (10.0f * uiScale);
            UIWidget::drawText(renderer, "Select 'Configure Options' below to open the complete content configuration panel.", padX, curY, Theme::colors.textGold, uiScale * 0.82f);
            curY += (20.0f * uiScale);
        }
        else if (app->getAppMode() == PhoneAppMode::QUESTS)
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
        else if (app->getAppMode() == PhoneAppMode::CONTACTS)
        {
            UIWidget::drawText(renderer, "Known Contacts & Companions:", padX, curY, Theme::colors.textGold, uiScale * 0.95f);
            curY += (20.0f * uiScale);

            if (data.contains("contacts") && data["contacts"].is_array())
            {
                for (const auto& ct : data["contacts"])
                {
                    std::string cName = ct.value("name", "Contact");
                    std::string role = ct.value("role", "Acquaintance");
                    std::string location = ct.value("location", "Unknown");
                    std::string status = ct.value("status", "Available");

                    SDL_FRect cardRect = { padX, curY, innerW, 36.0f * uiScale };
                    UIWidget::drawPanel(renderer, cardRect);

                    UIWidget::drawText(renderer, cName, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                    UIWidget::drawText(renderer, std::format("Role: {}", role), padX + innerW - (140.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
                    UIWidget::drawText(renderer, std::format("Location: {}  •  Status: {}", location, status), padX + (8.0f * uiScale), curY + (18.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.78f);

                    curY += cardRect.h + (6.0f * uiScale);
                }
            }
            else
            {
                UIWidget::drawText(renderer, "No new messages or contacts recorded in address book.", padX, curY, Theme::colors.textSecondary, uiScale * 0.85f);
                curY += (20.0f * uiScale);
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
                        SDL_Color bd = bHov ? Theme::colors.borderButton : SDL_Color{ 30, 34, 44, 255 };
                        UIWidget::drawPanel(renderer, bRect, fill, bd);

                        float dotW = UIWidget::getTextWidth("·", uiScale * 0.62f);
                        UIWidget::drawText(renderer, "·", bX + ((slotSize - dotW) / 2.0f), bY + ((slotSize - (10.0f * uiScale)) / 2.0f), SDL_Color{ 50, 55, 70, 255 }, uiScale * 0.62f);
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
