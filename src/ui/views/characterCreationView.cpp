#include "ui/views/characterCreationView.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <vector>

#include "core/game.h"
#include "items/itemDatabase.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/fontManager.h"
#include "ui/tooltipManager.h"
#include "ui/uiRenderer.h"
#include "ui/views/gameplayViews.h"

#include "ui/editorCardWidgets.h"

namespace CharacterCreationView
{
    using namespace EditorCardWidgets;

    static const auto& s_skinTones = EditorCardWidgets::getSkinTones();
    static const auto& s_hairColors = EditorCardWidgets::getHairColors();
    static const auto& s_eyeColors = EditorCardWidgets::getEyeColors();
    static const auto& s_makeupColors = EditorCardWidgets::getMakeupColors();


    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        auto cc = dynamic_cast<characterCreationState*>(gameContext->getActiveState());
        if (!cc) return 0.0f;

        cc->syncPreviewEntity(gameContext);

        float startY = curY;

        // Content spans layout pane width with crisp inner padding
        float padX = rect.x + (10.0f * uiScale);
        float availableW = rect.w - (20.0f * uiScale);

        auto activeTabs = cc->getActiveTabs();
        int tabCount = static_cast<int>(activeTabs.size());
        if (cc->step >= tabCount) cc->step = std::max(0, tabCount - 1);
        EditorTabId currentTab = activeTabs[cc->step];

        // 1. Top Header Banner
        float headerH = 28.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        std::string headerTitle = std::format("{} [{}]", cc->config.title, cc->getTabName(currentTab));
        UIWidget::drawHeader(renderer, headerRect, headerTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        // 2. Character Live Preview Summary Banner
        float prevH = 44.0f * uiScale;
        SDL_FRect prevRect = { padX, curY, availableW, prevH };
        UIWidget::drawPanel(renderer, prevRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string chosenFirst = (cc->gender == "Female") ? cc->feminineName : cc->masculineName;
        std::string fullName = cc->surname.empty() ? chosenFirst : (chosenFirst + " " + cc->surname);
        std::string previewLine1 = std::format("Hero: {}  |  {} ({})  |  Orientation: {}", fullName, cc->gender, cc->femininity, cc->orientation);
        std::string previewLine2 = std::format("Body: {}cm, {}, {}  |  Hair: {} {}  |  Eyes: {}", cc->heightCm, cc->bodySize, cc->muscleDefinition, cc->hairColor, cc->hairStyle, cc->eyeColor);

        UIWidget::drawText(renderer, previewLine1, padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        UIWidget::drawText(renderer, previewLine2, padX + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
        curY += prevH + (12.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        if (currentTab == EditorTabId::IDENTITY)
        {
            // 1. Biological Sex
            if (cc->config.isOptionEnabled("gender"))
            {
                static const std::vector<std::string> genderOpts = { "Male", "Female" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Biological Sex", "Determines initial physical anatomy and starting bodily equipment.", genderOpts, cc->gender, [&](const std::string& g) {
                    cc->gender = g;
                }, uiScale, 2);
            }

            // 2. Femininity
            if (cc->config.isOptionEnabled("femininity"))
            {
                static const std::vector<std::string> allFem = { "Very Masculine", "Masculine", "Androgynous", "Feminine", "Very Feminine" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Femininity", "How feminine or masculine your overall facial and bodily presentation is.", allFem, cc->femininity, [&](const std::string& f) {
                    cc->femininity = f;
                }, uiScale, 5);
            }

            // 3. Orientation
            if (cc->config.isOptionEnabled("orientation"))
            {
                static const std::vector<std::string> allOri = { "Androphilic", "Ambiphilic", "Gynephilic" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Sexual Orientation", "Attraction preference towards masculinity, femininity, or both.", allOri, cc->orientation, [&](const std::string& o) {
                    cc->orientation = o;
                }, uiScale, 3);
            }

            // 4. Starting Month
            if (cc->config.isOptionEnabled("start_month"))
            {
                static const std::vector<std::string> fullMonths = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Starting Calendar Month", "Select the calendar month in which your adventure begins.", fullMonths, cc->startMonth, [&](const std::string& m) {
                    cc->startMonth = m;
                    for (size_t i = 0; i < fullMonths.size(); ++i) if (fullMonths[i] == m) cc->startMonthIdx = static_cast<int>(i);
                }, uiScale, 6);
            }

            // 5. Birthday (Day & Age Steppers)
            curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Birth Day", "Day of birth within the month.", std::format("Day {:02d}", cc->birthDay), [&](int delta) {
                cc->birthDay = std::clamp(cc->birthDay + delta, 1, 31);
            }, uiScale, 1, 5, 0);

            curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Age", "Normal human starting age (18 to 65).", std::format("{} yrs", cc->birthAge), [&](int delta) {
                cc->birthAge = std::clamp(cc->birthAge + delta, 18, 65);
            }, uiScale, 1, 5, 10);

            // 6. Personality Traits
            if (cc->config.isOptionEnabled("personality_traits"))
            {
                static const std::vector<std::string> allP = { "Confident", "Shy", "Kind", "Selfish", "Naive", "Cynical", "Brave", "Cowardly", "Lewd", "Innocent", "Prude" };
                curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Personality Traits", "Click traits to toggle them. Influences roleplaying choices.", allP, cc->personalityTraits, [&](const std::string& t, bool act) {
                    if (act) cc->personalityTraits.insert(t);
                    else cc->personalityTraits.erase(t);
                }, uiScale, 6);
            }
        }
        else if (currentTab == EditorTabId::BODY)
        {
            // 1. Height (Human Range: 140cm to 210cm)
            if (cc->config.isOptionEnabled("height"))
            {
                auto* rule = cc->config.getRule("height");
                int minH = rule ? static_cast<int>(rule->minRange) : 140;
                int maxH = rule ? static_cast<int>(rule->maxRange) : 210;
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Height (cm)", "Normal human standing height (140 - 210 cm).", std::format("{} cm", cc->heightCm), [&](int delta) {
                    cc->heightCm = std::clamp(cc->heightCm + delta, minH, maxH);
                }, uiScale, 1, 5, 10);
            }

            // 2. Skin Tone
            if (cc->config.isOptionEnabled("skin_tone"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Skin Tone", "Natural human skin tone covering your body.", s_skinTones, cc->skinPrimaryColor, [&](const std::string& s) {
                    cc->skinPrimaryColor = s;
                }, uiScale);
            }

            // 3. Body Size
            if (cc->config.isOptionEnabled("body_size"))
            {
                static const std::vector<std::string> allSizes = { "Skinny", "Slender", "Average", "Muscular", "Chubby" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Body Frame Proportion", "Overall fat distribution and build frame.", allSizes, cc->bodySize, [&](const std::string& s) {
                    cc->bodySize = s;
                }, uiScale, 5);
            }

            // 4. Muscle Definition
            if (cc->config.isOptionEnabled("muscle"))
            {
                static const std::vector<std::string> allMusc = { "Soft", "Lightly muscled", "Toned", "Muscular", "Ripped" };
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Muscle Definition", "Muscularity, firmness, and tone.", allMusc, cc->muscleDefinition, [&](const std::string& m) {
                    cc->muscleDefinition = m;
                }, uiScale, 5);
            }

            // 5. Ass Size (Normal Human 0..4)
            static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
            int assIdx = std::clamp(cc->assSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ass Dimensions", "Rear cheek volume and fullness.", allSizes5, allSizes5[assIdx], [&](const std::string& a) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == a) cc->assSize = static_cast<int>(i);
            }, uiScale, 5);

            // 6. Hip Size (Normal Human 0..4)
            int hipIdx = std::clamp(cc->hipSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hip Breadth", "Pelvic width and hourglass curvature.", allSizes5, allSizes5[hipIdx], [&](const std::string& h) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == h) cc->hipSize = static_cast<int>(i);
            }, uiScale, 5);

            // 7. Bleached Anus Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Bleached Anus", "Cosmetic treatment around anal sphincter.", cc->anusBleached, [&](bool b) {
                cc->anusBleached = b;
            }, uiScale, "Bleached", "Natural");

            // 8. Composite Body Shape Card
            std::string shapeRating = EditorConfig::calculateBodyShape(cc->muscleDefinition, cc->bodySize);
            SDL_FRect shapeRect = { padX, curY, availableW, 38.0f * uiScale };
            UIWidget::drawPanel(renderer, shapeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "Composite Human Physique Rating:", padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.82f);
            float ratingW = UIWidget::getTextWidth(shapeRating, uiScale * 0.88f);
            UIWidget::drawText(renderer, shapeRating, padX + availableW - ratingW - (14.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
            curY += (48.0f * uiScale);
        }
        else if (currentTab == EditorTabId::FACE_HAIR)
        {
            // 1. Iris Colour
            if (cc->config.isOptionEnabled("eye_color"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Iris Colour", "Natural eye color.", s_eyeColors, cc->eyeColor, [&](const std::string& e) {
                    cc->eyeColor = e;
                }, uiScale);
            }

            // 2. Lip Size
            static const std::vector<std::string> allLips = { "Thin", "Average", "Full", "Plump", "Huge" };
            int lipIdx = std::clamp(cc->lipSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lip Dimensions", "Size and plumpness of lips.", allLips, allLips[lipIdx], [&](const std::string& l) {
                for (size_t i = 0; i < allLips.size(); ++i) if (allLips[i] == l) cc->lipSize = static_cast<int>(i);
            }, uiScale, 5);

            // 3. Puffy Lips Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Puffy Lips", "Extra softness and swelling.", cc->puffyLips, [&](bool p) {
                cc->puffyLips = p;
            }, uiScale, "Puffy", "Natural");

            // 4. Hair Length (Human Range: 0 to 100 cm)
            if (cc->config.isOptionEnabled("hair_length"))
            {
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Hair Length (cm)", "Normal human head hair length (0 - 100 cm).", std::format("{} cm", cc->hairLengthCm), [&](int delta) {
                    cc->hairLengthCm = std::clamp(cc->hairLengthCm + delta, 0, 100);
                }, uiScale, 1, 5, 15);
            }

            // 5. Hair Style (Filtered by Length Requirement)
            if (cc->config.isOptionEnabled("hair_style"))
            {
                auto validStyles = EditorConfig::getValidHairstyles(cc->hairLengthCm);
                auto styleOpts = cc->config.filterChoices("hair_style", validStyles);
                bool foundStyle = false;
                for (const auto& s : styleOpts) {
                    if (s == cc->hairStyle) { foundStyle = true; break; }
                }
                if (!foundStyle && !styleOpts.empty()) {
                    cc->hairStyle = styleOpts[0];
                }
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hairstyle", "Styling options available for your current hair length.", styleOpts, cc->hairStyle, [&](const std::string& s) {
                    cc->hairStyle = s;
                }, uiScale, 6);
            }

            // 6. Hair Color
            if (cc->config.isOptionEnabled("hair_color"))
            {
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Hair Colour", "Select natural hair pigment.", s_hairColors, cc->hairColor, [&](const std::string& h) {
                    cc->hairColor = h;
                }, uiScale);
            }

            // 7. Ear Type (Only show if multiple choices available, e.g. in salon/transformation)
            if (cc->config.isOptionEnabled("ear_type"))
            {
                static const std::vector<std::string> allEars = { "Human" };
                auto earOpts = cc->config.filterChoices("ear_type", allEars);
                if (earOpts.size() > 1)
                {
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ear Morphology", "Species ear structure.", earOpts, cc->earType, [&](const std::string& e) {
                        cc->earType = e;
                    }, uiScale, 4);
                }
            }
        }
        else if (currentTab == EditorTabId::BREASTS)
        {
            // 1. Human Breast Cup Size (Flat through G-cup)
            static const std::vector<std::string> humanCups = { "Flat", "AA", "A", "B", "C", "D", "DD", "E", "F", "FF", "G" };
            auto cupOpts = cc->config.filterChoices("chest_size", humanCups);
            int cupIdx = std::clamp(cc->breastCupSize, 0, static_cast<int>(cupOpts.size()) - 1);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Breast Cup Size", "Natural human breast cup size.", cupOpts, cupOpts[cupIdx], [&](const std::string& c) {
                for (size_t i = 0; i < cupOpts.size(); ++i) if (cupOpts[i] == c) cc->breastCupSize = static_cast<int>(i);
            }, uiScale, 6);

            // 2. Breast Shape
            static const std::vector<std::string> allBShapes = { "Round", "Pointy", "Perky", "Side-set", "Wide", "Narrow" };
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Breast Shape", "Contour and projection of breasts.", allBShapes, cc->breastShape, [&](const std::string& s) {
                cc->breastShape = s;
            }, uiScale, 6);

            // 3. Nipple & Areolae Sizes (Human 0..4)
            static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
            int nipIdx = std::clamp(cc->nippleSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Nipple Size", "Prominence of nipples.", allSizes5, allSizes5[nipIdx], [&](const std::string& n) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == n) cc->nippleSize = static_cast<int>(i);
            }, uiScale, 5);

            int areIdx = std::clamp(cc->areolaeSize, 0, 4);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Areolae Size", "Diameter of surrounding areolae.", allSizes5, allSizes5[areIdx], [&](const std::string& a) {
                for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == a) cc->areolaeSize = static_cast<int>(i);
            }, uiScale, 5);

            // 4. Puffy Nipples Toggle
            curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Puffy Nipples", "Extra softness and swelling.", cc->puffyNipples, [&](bool p) {
                cc->puffyNipples = p;
            }, uiScale, "Puffy", "Natural");

            // 5. Lactation Status
            static const std::vector<std::string> lactTiers = { "None", "Trickle", "Small Amount" };
            int lactIdx = std::clamp(cc->lactationTier, 0, static_cast<int>(lactTiers.size()) - 1);
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lactation Production", "Mammary milk production level at start.", lactTiers, lactTiers[lactIdx], [&](const std::string& l) {
                for (size_t i = 0; i < lactTiers.size(); ++i) {
                    if (lactTiers[i] == l) {
                        cc->lactationTier = static_cast<int>(i);
                        cc->isLactating = (i > 0);
                    }
                }
            }, uiScale, 3);
        }
        else if (currentTab == EditorTabId::GENITALIA)
        {
            if (cc->gender == "Female")
            {
                static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
                int capIdx = std::clamp(cc->vaginaCapacity, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Vaginal Capacity", "Internal accommodation and depth.", allSizes5, allSizes5[capIdx], [&](const std::string& c) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == c) cc->vaginaCapacity = static_cast<int>(i);
                }, uiScale, 5);

                int labIdx = std::clamp(cc->labiaSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Labia Size", "Outer and inner labia prominence.", allSizes5, allSizes5[labIdx], [&](const std::string& l) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == l) cc->labiaSize = static_cast<int>(i);
                }, uiScale, 5);

                int clitIdx = std::clamp(cc->clitorisSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Clitoris Size", "Clitoral prominence and length.", allSizes5, allSizes5[clitIdx], [&](const std::string& cl) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == cl) cc->clitorisSize = static_cast<int>(i);
                }, uiScale, 5);
            }
            else
            {
                // Normal Human Penis Length: 8.0 cm to 24.0 cm
                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Penis Length (cm)", "Normal human erect length (8.0 - 24.0 cm).", std::format("{:.1f} cm", cc->penisLengthCm), [&](float delta) {
                    cc->penisLengthCm = std::clamp(cc->penisLengthCm + delta, 8.0f, 24.0f);
                }, uiScale, 0.5f, 1.0f, 2.5f);

                static const std::vector<std::string> allSizes5 = { "Tiny", "Small", "Average", "Large", "Huge" };
                int testIdx = std::clamp(cc->testicleSize, 0, 4);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Testicle Size", "Testicular volume and scrotal size.", allSizes5, allSizes5[testIdx], [&](const std::string& t) {
                    for (size_t i = 0; i < allSizes5.size(); ++i) if (allSizes5[i] == t) cc->testicleSize = static_cast<int>(i);
                }, uiScale, 5);

                static const std::vector<std::string> cumTiers = { "Trickle", "Small Amount", "Decent Amount", "Large Amount" };
                int cumIdx = std::clamp(cc->cumProductionTier, 0, static_cast<int>(cumTiers.size()) - 1);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Cum Production", "Volume of ejaculatory output.", cumTiers, cumTiers[cumIdx], [&](const std::string& ct) {
                    for (size_t i = 0; i < cumTiers.size(); ++i) if (cumTiers[i] == ct) cc->cumProductionTier = static_cast<int>(i);
                }, uiScale, 4);
            }
        }
        else if (currentTab == EditorTabId::COSMETICS)
        {
            // Cosmetics Palettes
            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Lipstick", "Cosmetic pigment applied to lips.", s_makeupColors, cc->lipstick, [&](const std::string& c) {
                cc->lipstick = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Blusher / Rouge", "Cheek cosmetic coloring.", s_makeupColors, cc->blusher, [&](const std::string& c) {
                cc->blusher = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Eyeliner", "Cosmetic line around eyes.", s_makeupColors, cc->eyeliner, [&](const std::string& c) {
                cc->eyeliner = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Eye Shadow", "Pigment on eyelids.", s_makeupColors, cc->eyeshadow, [&](const std::string& c) {
                cc->eyeshadow = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Fingernail Polish", "Color on fingernails.", s_makeupColors, cc->nailPolish, [&](const std::string& c) {
                cc->nailPolish = c;
            }, uiScale);

            curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Toenail Polish", "Color on toenails.", s_makeupColors, cc->toenailPolish, [&](const std::string& c) {
                cc->toenailPolish = c;
            }, uiScale);

            // Piercings
            static const std::vector<std::string> pSockets = { "ear", "nose", "lip", "tongue", "navel", "nipple" };
            for (const auto& s : pSockets)
            {
                std::string sTitle = s + " piercing";
                sTitle[0] = static_cast<char>(std::toupper(sTitle[0]));
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, sTitle, "Piercing through " + s + ".", cc->piercings[s], [&cc, s](bool state) {
                    cc->piercings[s] = state;
                }, uiScale, "Pierced", "Unpierced");
            }

            // Body Hair
            static const std::vector<std::string> bhGroom = { "None", "Stubble", "Manicured", "Trimmed", "Natural", "Unkempt", "Bushy", "Wild" };
            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Pubic Hair Grooming", "Hair grooming in the groin area.", bhGroom, cc->pubicHair, [&](const std::string& ph) {
                cc->pubicHair = ph;
            }, uiScale, 4);

            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Underarm Hair Grooming", "Hair grooming in the armpits.", bhGroom, cc->underarmHair, [&](const std::string& uh) {
                cc->underarmHair = uh;
            }, uiScale, 4);

            curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ass Hair Grooming", "Hair grooming around the anus.", bhGroom, cc->assHair, [&](const std::string& ah) {
                cc->assHair = ah;
            }, uiScale, 4);
        }
        else if (currentTab == EditorTabId::WARDROBE)
        {
            if (!cc->wardrobeInitialized) cc->initializeWardrobe();

            // 1. Decency Status Banner
            bool decent = cc->isClothedEnough();
            std::string decencyStatus = cc->getDecencyStatus();
            float bannerH = 34.0f * uiScale;
            SDL_FRect bannerRect = { padX, curY, availableW, bannerH };
            SDL_Color bannerBg = decent ? SDL_Color{ 24, 38, 30, 255 } : SDL_Color{ 45, 24, 24, 255 };
            SDL_Color bannerBorder = decent ? Theme::colors.companion : SDL_Color{ 220, 60, 60, 255 };
            UIWidget::drawPanel(renderer, bannerRect, bannerBg, bannerBorder);
            std::string statusText = std::format("Attire Decency: {}", decencyStatus);
            UIWidget::drawText(renderer, statusText, padX + (12.0f * uiScale), curY + (8.0f * uiScale),
                               decent ? Theme::colors.companion : SDL_Color{ 240, 100, 100, 255 }, uiScale * 0.85f);
            curY += bannerH + (12.0f * uiScale);

            // 2. Dressing Room & Inventory Submenu Card
            float cardH = 136.0f * uiScale;
            SDL_FRect dressCard = { padX, curY, availableW, cardH };
            UIWidget::drawPanel(renderer, dressCard, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float tY = curY + (12.0f * uiScale);
            float tX = padX + (14.0f * uiScale);
            UIWidget::drawText(renderer, "Clothing & Dressing Room:", tX, tY, Theme::colors.textGold, uiScale * 0.95f);
            tY += 20.0f * uiScale;

            std::string pDesc = "All your available garments are laid out on the floor. Open the Clothing Inventory to browse items, inspect lore and descriptions, and equip your starting ensemble for the museum gala.";
            float hDesc = UIWidget::drawTextWrapped(renderer, pDesc, tX, tY, availableW - (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.80f);
            tY += hDesc + (14.0f * uiScale);

            // Large Action Button to Open Inventory
            float btnW = 260.0f * uiScale;
            float btnH = 34.0f * uiScale;
            SDL_FRect openBtn = { tX, tY, btnW, btnH };
            bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h);
            bool bHov = inPanel && (mousePos.x >= openBtn.x && mousePos.x <= openBtn.x + openBtn.w &&
                                   mousePos.y >= openBtn.y && mousePos.y <= openBtn.y + openBtn.h);

            UIWidget::drawButton(renderer, openBtn, "Open Clothing Inventory [ I ]", bHov, true, false, uiScale * 0.84f);
            TooltipManager::setHoverTooltip(openBtn, mousePos, "Open Clothing Submenu", "Launches the full dual inventory screen. Press ESC or click Back to return to Character Creation.", "Clothing");

            if (bHov && clicked)
            {
                gameContext->handleCommand(UICommand{ CommandType::OPEN_INVENTORY });
                gameContext->input.consumeMouseClick();
            }

            curY += cardH + (14.0f * uiScale);

            // 3. Current Attire Breakdown Card
            float summaryH = 150.0f * uiScale;
            SDL_FRect sumCard = { padX, curY, availableW, summaryH };
            UIWidget::drawPanel(renderer, sumCard, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float sY = curY + (10.0f * uiScale);
            UIWidget::drawText(renderer, "Current Attire Summary:", tX, sY, Theme::colors.textPrimary, uiScale * 0.90f);
            sY += 22.0f * uiScale;

            auto p = gameContext->getPlayer();
            auto getSlotName = [&](equipSlot slot) -> std::string {
                if (!p) return "None";
                auto eq = p->inventory.getEquippedItem(slot);
                return eq ? eq->name : "None (Unclad)";
            };

            std::string torsoStr = std::format("Upper Body: {} / {}", getSlotName(equipSlot::TORSO_OVER), getSlotName(equipSlot::TORSO_UNDER));
            std::string legsStr = std::format("Lower Body: {}", getSlotName(equipSlot::LEGS_OUTER));
            std::string underStr = std::format("Undergarments: {} / {}", getSlotName(equipSlot::CHEST_WEAR), getSlotName(equipSlot::GROIN_OVER));
            std::string feetStr = std::format("Footwear: {}", getSlotName(equipSlot::FEET));

            UIWidget::drawText(renderer, torsoStr, tX + (8.0f * uiScale), sY, Theme::colors.textSecondary, uiScale * 0.78f); sY += 18.0f * uiScale;
            UIWidget::drawText(renderer, legsStr, tX + (8.0f * uiScale), sY, Theme::colors.textSecondary, uiScale * 0.78f); sY += 18.0f * uiScale;
            UIWidget::drawText(renderer, underStr, tX + (8.0f * uiScale), sY, Theme::colors.textSecondary, uiScale * 0.78f); sY += 18.0f * uiScale;
            UIWidget::drawText(renderer, feetStr, tX + (8.0f * uiScale), sY, Theme::colors.textSecondary, uiScale * 0.78f); sY += 22.0f * uiScale;

            std::string tipStr = "Tip: You can also click the [ Inv ] button under the paperdoll equipment grid at any time.";
            UIWidget::drawText(renderer, tipStr, tX, sY, Theme::colors.textMuted, uiScale * 0.72f);

            curY += summaryH + (12.0f * uiScale);
        }
        else if (currentTab == EditorTabId::NAME_FINISH)
        {
            // 1. Name Input Box Card
            if (cc->config.isOptionEnabled("first_name") || cc->config.isOptionEnabled("surname"))
            {
                float nameCardH = 92.0f * uiScale;
                SDL_FRect ncRect = { padX, curY, availableW, nameCardH };
                UIWidget::drawPanel(renderer, ncRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, "Character Identity & Names:", padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.90f);
                UIWidget::drawText(renderer, "Type to enter your name, or click Random to roll from the pool.", padX + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);

                float fieldY = curY + (40.0f * uiScale);
                float boxH = 24.0f * uiScale;
                float rBtnW = 60.0f * uiScale;
                float labelW = 36.0f * uiScale;
                float inputW = 150.0f * uiScale;
                float colW = labelW + inputW + (6.0f * uiScale) + rBtnW;
                bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h);

                // First Name Field
                UIWidget::drawText(renderer, "First:", padX + (12.0f * uiScale), fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
                SDL_FRect firstBox = { padX + (12.0f * uiScale) + labelW, fieldY, inputW, boxH };
                std::string chosenFirst = (cc->gender == "Female") ? cc->feminineName : cc->masculineName;
                bool fnHover = inPanel && (mousePos.x >= firstBox.x && mousePos.x <= firstBox.x + firstBox.w && mousePos.y >= firstBox.y && mousePos.y <= firstBox.y + firstBox.h);
                SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
                SDL_RenderFillRect(renderer, &firstBox);
                SDL_Color fnBorder = fnHover ? Theme::colors.textGold : (cc->activeNameField == 0 ? Theme::colors.borderSelected : Theme::colors.borderNormal);
                SDL_SetRenderDrawColor(renderer, fnBorder.r, fnBorder.g, fnBorder.b, fnBorder.a);
                SDL_RenderRect(renderer, &firstBox);
                UIWidget::drawText(renderer, chosenFirst, firstBox.x + (6.0f * uiScale), fieldY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                if (fnHover && clicked) { cc->activeNameField = 0; }

                SDL_FRect r1Btn = { firstBox.x + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
                bool r1Hover = inPanel && (mousePos.x >= r1Btn.x && mousePos.x <= r1Btn.x + r1Btn.w && mousePos.y >= r1Btn.y && mousePos.y <= r1Btn.y + r1Btn.h);
                UIWidget::drawColoredButton(renderer, r1Btn, "Random", Theme::colors.bgButton, r1Hover ? Theme::colors.textGold : Theme::colors.textSecondary, false, uiScale * 0.75f);
                if (r1Hover && clicked)
                {
                    cc->randomizeFirstNames();
                    gameContext->input.consumeMouseClick();
                }

                // Surname Field
                float lastX = padX + (12.0f * uiScale) + colW + (40.0f * uiScale);
                if (lastX + colW <= padX + availableW)
                {
                    UIWidget::drawText(renderer, "Last:", lastX, fieldY + (4.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
                    SDL_FRect lastBox = { lastX + labelW, fieldY, inputW, boxH };
                    bool lnHover = inPanel && (mousePos.x >= lastBox.x && mousePos.x <= lastBox.x + lastBox.w && mousePos.y >= lastBox.y && mousePos.y <= lastBox.y + lastBox.h);
                    SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
                    SDL_RenderFillRect(renderer, &lastBox);
                    SDL_Color lnBorder = lnHover ? Theme::colors.textGold : (cc->activeNameField == 3 ? Theme::colors.borderSelected : Theme::colors.borderNormal);
                    SDL_SetRenderDrawColor(renderer, lnBorder.r, lnBorder.g, lnBorder.b, lnBorder.a);
                    SDL_RenderRect(renderer, &lastBox);
                    UIWidget::drawText(renderer, cc->surname.empty() ? "(None)" : cc->surname, lastBox.x + (6.0f * uiScale), fieldY + (4.0f * uiScale), cc->surname.empty() ? Theme::colors.textMuted : Theme::colors.textPrimary, uiScale * 0.85f);
                    if (lnHover && clicked) { cc->activeNameField = 3; }

                    SDL_FRect r2Btn = { lastBox.x + inputW + (6.0f * uiScale), fieldY, rBtnW, boxH };
                    bool r2Hover = inPanel && (mousePos.x >= r2Btn.x && mousePos.x <= r2Btn.x + r2Btn.w && mousePos.y >= r2Btn.y && mousePos.y <= r2Btn.y + r2Btn.h);
                    UIWidget::drawColoredButton(renderer, r2Btn, "Random", Theme::colors.bgButton, r2Hover ? Theme::colors.textGold : Theme::colors.textSecondary, false, uiScale * 0.75f);
                    if (r2Hover && clicked)
                    {
                        cc->randomizeSurname();
                        gameContext->input.consumeMouseClick();
                    }
                }

                UIWidget::drawText(renderer, "Surname may be left blank. First name adapts to chosen gender & femininity.", padX + (12.0f * uiScale), fieldY + boxH + (6.0f * uiScale), Theme::colors.textMuted, uiScale * 0.72f);
                curY += nameCardH + (12.0f * uiScale);
            }

            // 2. Character Body Inspection & Profile Sheet
            std::string fullDesc = cc->generateAppearanceDescription();
            std::vector<std::string> paragraphs;
            size_t startP = 0;
            while (startP < fullDesc.length())
            {
                size_t nextP = fullDesc.find("\n\n", startP);
                if (nextP == std::string::npos) nextP = fullDesc.length();
                std::string pText = fullDesc.substr(startP, nextP - startP);
                size_t f = pText.find_first_not_of(" \t\r\n");
                size_t l = pText.find_last_not_of(" \t\r\n");
                if (f != std::string::npos && l != std::string::npos)
                {
                    paragraphs.push_back(pText.substr(f, l - f + 1));
                }
                startP = nextP + 2;
            }

            float textPadX = padX + (14.0f * uiScale);
            float textAvailableW = availableW - (28.0f * uiScale);
            float cardHeaderH = 26.0f * uiScale;
            float paragraphGap = 10.0f * uiScale;

            // Pre-calculate height of all paragraphs
            float contentH = cardHeaderH + (12.0f * uiScale);
            for (const auto& p : paragraphs)
            {
                contentH += fontManager::getInstance().getTextWrappedHeight(p, textAvailableW, uiScale * 0.82f) + paragraphGap;
            }
            contentH += (38.0f * uiScale); // traits & summary lines

            SDL_FRect sumCardRect = { padX, curY, availableW, contentH };
            UIWidget::drawPanel(renderer, sumCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawHeader(renderer, { padX, curY, availableW, cardHeaderH }, "CHARACTER BODY INSPECTION & PROFILE", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.78f);

            float pY = curY + cardHeaderH + (10.0f * uiScale);
            for (const auto& p : paragraphs)
            {
                float pH = UIWidget::drawTextWrapped(renderer, p, textPadX, pY, textAvailableW, Theme::colors.textPrimary, uiScale * 0.82f);
                pY += pH + paragraphGap;
            }

            // Summary Footer
            std::string traitList = "";
            for (const auto& tr : cc->personalityTraits) {
                if (!traitList.empty()) traitList += ", ";
                traitList += tr;
            }
            if (traitList.empty()) traitList = "None";

            UIWidget::drawText(renderer, "• Personality Traits: " + traitList, textPadX, pY, Theme::colors.companion, uiScale * 0.80f);
            pY += (18.0f * uiScale);
            UIWidget::drawText(renderer, "• Starting Attire: " + cc->getDecencyStatus(), textPadX, pY, Theme::colors.textSecondary, uiScale * 0.80f);

            curY += contentH + (14.0f * uiScale);
        }

        return (curY - startY);
    }
}
