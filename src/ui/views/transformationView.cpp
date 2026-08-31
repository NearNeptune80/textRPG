#include "ui/views/transformationView.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include "core/game.h"
#include "core/characterDescription.h"
#include "entities/entity.h"
#include "state/transformationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

namespace TransformationView
{
    struct ColorOption
    {
        std::string name;
        SDL_Color color;
    };

    static const std::vector<ColorOption> tfColors = {
        { "Pale", { 243, 215, 196, 255 } },
        { "Light", { 232, 196, 168, 255 } },
        { "Porcelain", { 247, 230, 216, 255 } },
        { "Rosy", { 232, 179, 154, 255 } },
        { "Olive", { 196, 146, 98, 255 } },
        { "Tanned", { 198, 134, 66, 255 } },
        { "Dark", { 141, 85, 36, 255 } },
        { "Ebony", { 59, 34, 19, 255 } },
        { "Raven Black", { 30, 30, 30, 255 } },
        { "Pure White", { 245, 245, 245, 255 } },
        { "Silver", { 192, 192, 192, 255 } },
        { "Crimson Red", { 196, 30, 58, 255 } },
        { "Royal Purple", { 123, 63, 161, 255 } },
        { "Lilac", { 200, 162, 200, 255 } },
        { "Pastel Pink", { 255, 107, 218, 255 } },
        { "Azure Blue", { 59, 110, 165, 255 } },
        { "Emerald Green", { 61, 140, 64, 255 } },
        { "Radiant Gold", { 212, 175, 55, 255 } },
        { "Golden Amber", { 196, 123, 23, 255 } },
        { "Chestnut Brown", { 107, 63, 29, 255 } }
    };

    static const std::vector<std::string> racialTypes = {
        "Human", "Demon", "Cat-morph", "Dog-morph", "Wolf-morph", "Fox-morph", "Harpy", "Bovine-morph", "Dragon-morph", "Elf"
    };

    static const std::vector<std::string> minorRacesWithNone = {
        "None", "Human", "Demon", "Cat-morph", "Dog-morph", "Wolf-morph", "Fox-morph", "Harpy", "Bovine-morph", "Dragon-morph"
    };

    static float drawSectionTitle(SDL_Renderer* renderer, float padX, float curY, float availableW, std::string_view title, std::string_view subtitle, float uiScale)
    {
        float startY = curY;
        SDL_FRect barRect = { padX, curY, availableW, 22.0f * uiScale };
        UIWidget::drawPanel(renderer, barRect, Theme::colors.bgHeader, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, title, padX + (8.0f * uiScale), curY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        curY += barRect.h + (3.0f * uiScale);

        if (!subtitle.empty())
        {
            UIWidget::drawText(renderer, subtitle, padX + (8.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.75f);
            curY += (14.0f * uiScale);
        }
        return curY - startY;
    }

    static float drawPillGrid(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, const std::vector<std::string>& options, const std::string& currentSelected, auto onSelect, float uiScale, int cols = 5)
    {
        float startY = curY;
        float gap = 4.0f * uiScale;
        float btnW = (availableW - (gap * (cols - 1))) / cols;
        float btnH = 22.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + c * (btnW + gap), curY + r * (btnH + gap), btnW, btnH };

            bool isSelected = (options[i] == currentSelected);
            bool hovered = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                            mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isSelected, uiScale * 0.78f);

            if (hovered && clicked)
            {
                onSelect(options[i]);
                gameContext->input.consumeMouseClick();
            }
        }

        int totalRows = static_cast<int>((options.size() + cols - 1) / cols);
        curY += (totalRows * (btnH + gap)) + (4.0f * uiScale);
        return curY - startY;
    }

    static float drawColorGrid(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, const std::vector<ColorOption>& options, const std::string& currentSelected, auto onSelect, float uiScale, int cols = 5)
    {
        float startY = curY;
        float gap = 4.0f * uiScale;
        float btnW = (availableW - (gap * (cols - 1))) / cols;
        float btnH = 22.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + c * (btnW + gap), curY + r * (btnH + gap), btnW, btnH };

            bool isSelected = (options[i].name == currentSelected);
            bool hovered = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                            mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            SDL_Color bg = isSelected ? Theme::colors.bgHeader : Theme::colors.bgSlot;
            UIWidget::drawColoredButton(renderer, bRect, options[i].name, bg, options[i].color, isSelected, uiScale * 0.75f);

            if (hovered && clicked)
            {
                onSelect(options[i].name);
                gameContext->input.consumeMouseClick();
            }
        }

        int totalRows = static_cast<int>((options.size() + cols - 1) / cols);
        curY += (totalRows * (btnH + gap)) + (4.0f * uiScale);
        return curY - startY;
    }

    static float drawStepper(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, std::string_view label, std::string_view displayVal, auto onStep, float uiScale)
    {
        float startY = curY;
        float labelW = availableW * 0.40f;
        float btnW = 32.0f * uiScale;
        float valW = availableW - labelW - (btnW * 4) - (16.0f * uiScale);
        float h = 22.0f * uiScale;

        UIWidget::drawText(renderer, label, padX, curY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

        float curX = padX + labelW;
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        auto drawStepBtn = [&](std::string_view txt, int delta) {
            SDL_FRect r = { curX, curY, btnW, h };
            bool hov = (mousePos.x >= r.x && mousePos.x <= r.x + r.w && mousePos.y >= r.y && mousePos.y <= r.y + r.h);
            UIWidget::drawButton(renderer, r, txt, hov, true, false, uiScale * 0.75f);
            if (hov && clicked)
            {
                onStep(delta);
                gameContext->input.consumeMouseClick();
            }
            curX += btnW + (4.0f * uiScale);
        };

        drawStepBtn("--", -5);
        drawStepBtn("-", -1);

        SDL_FRect valRect = { curX, curY, valW, h };
        UIWidget::drawPanel(renderer, valRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, displayVal, curX + (6.0f * uiScale), curY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.8f);
        curX += valW + (4.0f * uiScale);

        drawStepBtn("+", 1);
        drawStepBtn("++", 5);

        curY += h + (6.0f * uiScale);
        return curY - startY;
    }

    static float drawToggle(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, std::string_view label, bool currentState, auto onToggle, float uiScale, std::string_view trueLabel = "Yes", std::string_view falseLabel = "No")
    {
        float startY = curY;
        float labelW = availableW * 0.55f;
        float btnW = (availableW - labelW - (8.0f * uiScale)) / 2.0f;
        float h = 22.0f * uiScale;

        UIWidget::drawText(renderer, label, padX, curY + (3.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        float btn1X = padX + labelW;
        SDL_FRect r1 = { btn1X, curY, btnW, h };
        bool hov1 = (mousePos.x >= r1.x && mousePos.x <= r1.x + r1.w && mousePos.y >= r1.y && mousePos.y <= r1.y + r1.h);
        UIWidget::drawButton(renderer, r1, trueLabel, hov1, true, currentState, uiScale * 0.78f);
        if (hov1 && clicked)
        {
            onToggle(true);
            gameContext->input.consumeMouseClick();
        }

        float btn2X = btn1X + btnW + (4.0f * uiScale);
        SDL_FRect r2 = { btn2X, curY, btnW, h };
        bool hov2 = (mousePos.x >= r2.x && mousePos.x <= r2.x + r2.w && mousePos.y >= r2.y && mousePos.y <= r2.y + r2.h);
        UIWidget::drawButton(renderer, r2, falseLabel, hov2, true, !currentState, uiScale * 0.78f);
        if (hov2 && clicked)
        {
            onToggle(false);
            gameContext->input.consumeMouseClick();
        }

        curY += h + (6.0f * uiScale);
        return curY - startY;
    }

    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        transformationState* state = dynamic_cast<transformationState*>(gameContext->getActiveState());
        entity* player = gameContext->getPlayer();
        if (!state || !player) return 0.0f;

        float startY = curY;
        float padX = rect.x + (14.0f * uiScale);
        float availableW = rect.w - (28.0f * uiScale);

        // 1. Header Banner
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, std::format("FULL TRANSFORMATION & BODY MODIFICATION [{}]", transformationTabToString(state->currentTab)), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (6.0f * uiScale);

        // 2. Persistent Live Status Card
        SDL_FRect statusCardRect = { padX, curY, availableW, 36.0f * uiScale };
        UIWidget::drawPanel(renderer, statusCardRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string genderStr = genderArchetypeToString(player->anatomy.getGenderArchetype());
        std::string racialTitle = player->anatomy.getRacialTitle();
        UIWidget::drawText(renderer, std::format("Subject: {} | Form: {} ({}) | Height: {:.0f}cm", player->name, racialTitle, genderStr, player->anatomy.heightMeters * 100.0f), padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);

        std::string line2 = "Features: ";
        if (const bodyPart* b = player->anatomy.getPart(bodySlot::BREASTS)) line2 += std::format("Breasts: {} ", bodyPart::getCupSizeName(b->cupSize));
        if (player->anatomy.hasPenis()) line2 += "• Penis ";
        if (player->anatomy.hasVagina()) line2 += "• Vagina ";
        if (player->anatomy.hasPart(bodySlot::HORNS)) line2 += "• Horns ";
        if (player->anatomy.hasPart(bodySlot::WINGS)) line2 += "• Wings ";
        if (player->anatomy.hasPart(bodySlot::TAIL)) line2 += "• Tail ";
        UIWidget::drawText(renderer, line2, padX + (8.0f * uiScale), curY + (18.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.75f);
        curY += statusCardRect.h + (8.0f * uiScale);

        if (!state->statusMessage.empty())
        {
            UIWidget::drawText(renderer, state->statusMessage, padX, curY, Theme::colors.lust, uiScale * 0.85f);
            curY += (18.0f * uiScale);
        }

        // 3. Tab Specific Renderers
        switch (state->currentTab)
        {
            case TransformationTab::CORE:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Core Physical Proportions", "Adjust age, height, and racial base morphology.", uiScale);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Height (cm)", std::format("{:.0f} cm", player->anatomy.heightMeters * 100.0f), [&](int delta) {
                    player->anatomy.heightMeters = std::clamp(player->anatomy.heightMeters + (delta * 0.01f), 1.20f, 3.50f);
                }, uiScale);

                // Body Morphology
                static const std::vector<std::string> morphSizes = { "Skinny", "Slender", "Average", "Large", "Huge" };
                static const std::vector<std::string> muscleTiers = { "Soft", "Lightly Muscled", "Toned", "Muscular", "Ripped" };

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Torso & Skin Covering", "Change body covering type and skin palette.", uiScale);
                bodyPart* torso = player->anatomy.getPart(bodySlot::TORSO);
                std::string torsoColor = torso ? torso->primaryColor : "Fair";
                curY += drawColorGrid(renderer, gameContext, padX, curY, availableW, tfColors, torsoColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::TORSO)) {
                        bodyPart tp; tp.id = "torso"; tp.name = "Torso"; tp.race = "Human"; tp.primaryColor = col;
                        player->anatomy.setPart(bodySlot::TORSO, tp);
                    } else {
                        player->anatomy.getPart(bodySlot::TORSO)->primaryColor = col;
                    }
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Limbs & Foot Structure", "Change arm/leg morphology.", uiScale);
                static const std::vector<std::string> footTypes = { "Plantigrade", "Digitigrade", "Unguligrade", "Arachnoid" };
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, footTypes, "Plantigrade", [](const std::string&) {}, uiScale);
                break;
            }

            case TransformationTab::EYES:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ocular Morphology", "Customize eye shape, count, and color.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, racialTypes, player->anatomy.hasPart(bodySlot::EYES) ? player->anatomy.getPart(bodySlot::EYES)->race : "Human", [&](const std::string& r) {
                    bodyPart ep; ep.id = "eyes_" + r; ep.name = "Eyes"; ep.race = r; ep.primaryColor = "Blue";
                    player->anatomy.setPart(bodySlot::EYES, ep);
                }, uiScale);

                bodyPart* eyes = player->anatomy.getPart(bodySlot::EYES);
                std::string eyeColor = eyes ? eyes->primaryColor : "Blue";
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Iris Color Swatches", "Color of the eye irises.", uiScale);
                curY += drawColorGrid(renderer, gameContext, padX, curY, availableW, tfColors, eyeColor, [&](const std::string& col) {
                    if (auto* ep = player->anatomy.getPart(bodySlot::EYES)) ep->primaryColor = col;
                }, uiScale);
                break;
            }

            case TransformationTab::HAIR:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Hair Length & Style", "Choose length, style, and grooming.", uiScale);
                bodyPart* hair = player->anatomy.getPart(bodySlot::HAIR);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Hair Length (cm)", std::format("{:.0f} cm", hair ? hair->length : 15.0f), [&](int delta) {
                    if (!hair) { bodyPart hp; hp.id = "hair"; hp.name = "Hair"; hp.length = 15.0f; player->anatomy.setPart(bodySlot::HAIR, hp); }
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->length = std::clamp(hp->length + (delta * 3.0f), 0.0f, 150.0f);
                }, uiScale);

                static const std::vector<std::string> styles = { "Short", "Messy", "Slicked-back", "Mohawk", "Afro", "Pixie-cut", "Bob Cut", "Straight", "Wavy", "Curly", "Ponytail", "Braided", "Twin Tails" };
                std::string curStyle = hair ? hair->style : "Short";
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, styles, curStyle, [&](const std::string& s) {
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->style = s;
                }, uiScale);

                std::string hairColor = hair ? hair->primaryColor : "Brown";
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Hair Color", "Select mane or head hair color.", uiScale);
                curY += drawColorGrid(renderer, gameContext, padX, curY, availableW, tfColors, hairColor, [&](const std::string& col) {
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->primaryColor = col;
                }, uiScale);
                break;
            }

            case TransformationTab::HEAD_FACE:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ears & Facial Structure", "Change ear morphology and features.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, racialTypes, player->anatomy.hasPart(bodySlot::EARS) ? player->anatomy.getPart(bodySlot::EARS)->race : "Human", [&](const std::string& r) {
                    bodyPart ear; ear.id = "ears_" + r; ear.name = "Ears"; ear.race = r;
                    player->anatomy.setPart(bodySlot::EARS, ear);
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Horns & Crests", "Grow, lengthen, or remove horns.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, minorRacesWithNone, player->anatomy.hasPart(bodySlot::HORNS) ? player->anatomy.getPart(bodySlot::HORNS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::HORNS);
                    else {
                        bodyPart h; h.id = "horns_" + r; h.name = "Horns"; h.race = r; h.count = 2; h.length = 20.0f;
                        player->anatomy.setPart(bodySlot::HORNS, h);
                    }
                }, uiScale);

                if (bodyPart* horns = player->anatomy.getPart(bodySlot::HORNS))
                {
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Horn Length (cm)", std::format("{:.0f} cm", horns->length), [&](int delta) {
                        horns->length = std::clamp(horns->length + (delta * 2.0f), 2.0f, 80.0f);
                    }, uiScale);
                }
                break;
            }

            case TransformationTab::ASS_HIPS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ass, Hips & Anus", "Customize rear proportions and elasticity.", uiScale);
                bodyPart* ass = player->anatomy.getPart(bodySlot::ASS);
                if (!ass) { bodyPart ap; ap.id = "ass"; ap.name = "Ass"; ap.orifice.exists = true; player->anatomy.setPart(bodySlot::ASS, ap); ass = player->anatomy.getPart(bodySlot::ASS); }

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Anus Depth (cm)", std::format("{:.0f} cm", ass->orifice.depthCm), [&](int delta) {
                    ass->orifice.depthCm = std::clamp(ass->orifice.depthCm + delta, 5.0f, 40.0f);
                }, uiScale);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Elasticity", std::format("{:.0f}%", ass->orifice.elasticity), [&](int delta) {
                    ass->orifice.elasticity = std::clamp(ass->orifice.elasticity + (delta * 5.0f), 0.0f, 100.0f);
                }, uiScale);

                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Bleached Anus", ass->tags.empty() ? false : std::ranges::find(ass->tags, std::string("bleached")) != ass->tags.end(), [&](bool state) {
                    if (state) ass->tags.push_back("bleached");
                    else std::erase(ass->tags, "bleached");
                }, uiScale);
                break;
            }

            case TransformationTab::BREASTS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Breasts & Lactation", "Modify chest volume, rows, and milk output.", uiScale);
                bodyPart* breasts = player->anatomy.getPart(bodySlot::BREASTS);
                if (!breasts) { bodyPart bp; bp.id = "breasts"; bp.name = "Breasts"; player->anatomy.setPart(bodySlot::BREASTS, bp); breasts = player->anatomy.getPart(bodySlot::BREASTS); }

                static const std::vector<std::string> cups = { "Flat", "A", "B", "C", "D", "DD", "E", "F", "FF", "G", "GG", "H", "HH", "J" };
                std::string curCup = bodyPart::getCupSizeName(breasts->cupSize);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, cups, curCup, [&](const std::string& c) {
                    for (size_t i = 0; i < cups.size(); ++i) if (cups[i] == c) breasts->cupSize = static_cast<int>(i);
                }, uiScale, 7);

                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Lactation Active", breasts->isLactating, [&](bool l) {
                    breasts->isLactating = l;
                    if (l && breasts->maxFluidMl <= 0.0f) { breasts->maxFluidMl = 1000.0f; breasts->currentFluidMl = 250.0f; }
                }, uiScale);

                if (breasts->isLactating)
                {
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Milk Storage (ml)", std::format("{:.0f} ml", breasts->maxFluidMl), [&](int delta) {
                        breasts->maxFluidMl = std::clamp(breasts->maxFluidMl + (delta * 100.0f), 100.0f, 10000.0f);
                        breasts->currentFluidMl = std::min(breasts->currentFluidMl, breasts->maxFluidMl);
                    }, uiScale);
                }
                break;
            }

            case TransformationTab::VAGINA:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Female Genitalia", "Grow, modify, or remove vagina and clitoris.", uiScale);
                bool hasVag = player->anatomy.hasVagina();
                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Has Vagina", hasVag, [&](bool v) {
                    if (v && !player->anatomy.hasVagina()) {
                        bodyPart gp; gp.id = "vagina"; gp.name = "Vagina"; gp.orifice.exists = true; gp.orifice.depthCm = 16.0f;
                        player->anatomy.setPart(bodySlot::GROIN, gp);
                    } else if (!v && player->anatomy.hasVagina()) {
                        player->anatomy.removePart(bodySlot::GROIN);
                    }
                }, uiScale);

                if (player->anatomy.hasVagina())
                {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Vaginal Depth", std::format("{:.0f} cm", g->orifice.depthCm), [&](int delta) {
                        g->orifice.depthCm = std::clamp(g->orifice.depthCm + delta, 5.0f, 35.0f);
                    }, uiScale);

                    static const std::vector<std::string> flavors = { "Girlcum", "Vanilla", "Honey", "Strawberry", "Cherry", "Mint" };
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, flavors, "Girlcum", [](const std::string&) {}, uiScale, 6);
                }
                break;
            }

            case TransformationTab::PENIS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Male Genitalia", "Grow, modify, or remove penis, testicles, and cum production.", uiScale);
                bool hasPen = player->anatomy.hasPenis();
                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Has Penis", hasPen, [&](bool p) {
                    if (p && !player->anatomy.hasPenis()) {
                        bodyPart gp; gp.id = "penis"; gp.name = "Penis"; gp.length = 16.0f; gp.diameter = 3.8f; gp.maxFluidMl = 30.0f; gp.currentFluidMl = 15.0f;
                        player->anatomy.setPart(bodySlot::GROIN, gp);
                    } else if (!p && player->anatomy.hasPenis()) {
                        player->anatomy.removePart(bodySlot::GROIN);
                    }
                }, uiScale);

                if (player->anatomy.hasPenis())
                {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Length (cm)", std::format("{:.1f} cm", g->length), [&](int delta) {
                        g->length = std::clamp(g->length + delta, 3.0f, 80.0f);
                    }, uiScale);

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Girth (cm)", std::format("{:.1f} cm", g->diameter), [&](int delta) {
                        g->diameter = std::clamp(g->diameter + (delta * 0.2f), 1.5f, 15.0f);
                    }, uiScale);

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Cum Capacity", std::format("{:.0f} ml", g->maxFluidMl), [&](int delta) {
                        g->maxFluidMl = std::clamp(g->maxFluidMl + (delta * 10.0f), 5.0f, 2000.0f);
                        g->currentFluidMl = std::min(g->currentFluidMl, g->maxFluidMl);
                    }, uiScale);
                }
                break;
            }

            case TransformationTab::CROTCH_BOOBS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Crotch-Boobs & Udders", "Secondary udder / crotch mammary glands.", uiScale);
                static const std::vector<std::string> crotchTypes = { "None", "Bovine Udders", "Caprine Udders", "Feline Crotch-Boobs", "Canine Teats" };
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, crotchTypes, "None", [](const std::string&) {}, uiScale);
                break;
            }

            case TransformationTab::APPENDAGES:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Wings & Flight Organs", "Grow or modify wings.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, minorRacesWithNone, player->anatomy.hasPart(bodySlot::WINGS) ? player->anatomy.getPart(bodySlot::WINGS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::WINGS);
                    else {
                        bodyPart w; w.id = "wings_" + r; w.name = "Wings"; w.race = r; w.count = 2;
                        player->anatomy.setPart(bodySlot::WINGS, w);
                    }
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Tails & Prehensile Appendages", "Grow or modify tail.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, minorRacesWithNone, player->anatomy.hasPart(bodySlot::TAIL) ? player->anatomy.getPart(bodySlot::TAIL)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::TAIL);
                    else {
                        bodyPart t; t.id = "tail_" + r; t.name = "Tail"; t.race = r; t.count = 1; t.length = 75.0f;
                        player->anatomy.setPart(bodySlot::TAIL, t);
                    }
                }, uiScale);
                break;
            }

            case TransformationTab::INSPECT_PRESETS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Live Anatomical Prose Description", "Prose readout reflecting all active mutations.", uiScale);
                std::string fullDesc = characterDescription::generateFullDescription(player);
                float descH = UIWidget::drawTextWrapped(renderer, fullDesc, padX, curY, availableW, Theme::colors.textPrimary, uiScale * 0.82f);
                curY += descH + (12.0f * uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Transformation Presets", "Save or reload whole-body configurations.", uiScale);

                // Save current preset button
                SDL_FRect saveBtnRect = { padX, curY, 130.0f * uiScale, 22.0f * uiScale };
                auto mousePos = gameContext->input.getMousePosition();
                bool clicked = gameContext->input.isLeftMouseJustClicked();
                bool sHov = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                             mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

                UIWidget::drawButton(renderer, saveBtnRect, "Save New Preset", sHov, true, false, uiScale * 0.78f);
                if (sHov && clicked)
                {
                    state->savePreset(gameContext, "Preset_" + player->anatomy.getRacialTitle());
                    gameContext->input.consumeMouseClick();
                }

                float rBtnX = padX + (140.0f * uiScale);
                SDL_FRect rBtnRect = { rBtnX, curY, 130.0f * uiScale, 22.0f * uiScale };
                bool rHov = (mousePos.x >= rBtnRect.x && mousePos.x <= rBtnRect.x + rBtnRect.w &&
                             mousePos.y >= rBtnRect.y && mousePos.y <= rBtnRect.y + rBtnRect.h);
                UIWidget::drawButton(renderer, rBtnRect, "Reset Human", rHov, true, false, uiScale * 0.78f);
                if (rHov && clicked)
                {
                    state->resetToHuman(gameContext);
                    gameContext->input.consumeMouseClick();
                }
                curY += (28.0f * uiScale);

                // List saved presets
                auto presets = state->getPresetNames();
                if (!presets.empty())
                {
                    for (const auto& pName : presets)
                    {
                        SDL_FRect pRect = { padX, curY, availableW - (90.0f * uiScale), 20.0f * uiScale };
                        UIWidget::drawPanel(renderer, pRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                        UIWidget::drawText(renderer, pName, pRect.x + (6.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.75f);

                        // Load button
                        SDL_FRect loadRect = { padX + availableW - (85.0f * uiScale), curY, 40.0f * uiScale, 20.0f * uiScale };
                        bool lHov = (mousePos.x >= loadRect.x && mousePos.x <= loadRect.x + loadRect.w &&
                                     mousePos.y >= loadRect.y && mousePos.y <= loadRect.y + loadRect.h);
                        UIWidget::drawButton(renderer, loadRect, "Load", lHov, true, false, uiScale * 0.7f);
                        if (lHov && clicked)
                        {
                            state->loadPreset(gameContext, pName);
                            gameContext->input.consumeMouseClick();
                        }

                        // Del button
                        SDL_FRect delRect = { padX + availableW - (40.0f * uiScale), curY, 36.0f * uiScale, 20.0f * uiScale };
                        bool dHov = (mousePos.x >= delRect.x && mousePos.x <= delRect.x + delRect.w &&
                                     mousePos.y >= delRect.y && mousePos.y <= delRect.y + delRect.h);
                        UIWidget::drawButton(renderer, delRect, "Del", dHov, true, false, uiScale * 0.7f);
                        if (dHov && clicked)
                        {
                            state->deletePreset(pName);
                            gameContext->input.consumeMouseClick();
                        }
                        curY += (24.0f * uiScale);
                    }
                }
                break;
            }
        }

        return curY - startY;
    }
}
