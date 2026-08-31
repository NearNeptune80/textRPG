#include "ui/views/transformationView.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>
#include <cmath>

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
        std::string id;
        std::string name;
        SDL_Color color;
    };

    static const std::vector<ColorOption> tfColors = {
        { "PALE", "Pale", { 243, 215, 196, 255 } },
        { "LIGHT", "Light", { 232, 196, 168, 255 } },
        { "PORCELAIN", "Porcelain", { 247, 230, 216, 255 } },
        { "ROSY", "Rosy", { 232, 179, 154, 255 } },
        { "OLIVE", "Olive", { 196, 146, 98, 255 } },
        { "TANNED", "Tanned", { 198, 134, 66, 255 } },
        { "DARK", "Dark", { 141, 85, 36, 255 } },
        { "EBONY", "Ebony", { 59, 34, 19, 255 } },
        { "BLACK", "Raven Black", { 30, 30, 30, 255 } },
        { "WHITE", "Pure White", { 245, 245, 245, 255 } },
        { "GREY", "Silver Grey", { 160, 160, 160, 255 } },
        { "RED", "Ruby Red", { 220, 20, 60, 255 } },
        { "CRIMSON", "Crimson", { 139, 0, 0, 255 } },
        { "PURPLE", "Royal Purple", { 123, 63, 161, 255 } },
        { "VIOLET", "Violet", { 155, 89, 182, 255 } },
        { "LILAC", "Lilac", { 200, 162, 200, 255 } },
        { "PINK", "Pastel Pink", { 255, 107, 218, 255 } },
        { "HOT_PINK", "Hot Pink", { 255, 20, 147, 255 } },
        { "BLUE", "Azure Blue", { 59, 110, 165, 255 } },
        { "SKY_BLUE", "Sky Blue", { 135, 206, 235, 255 } },
        { "GREEN", "Emerald Green", { 61, 140, 64, 255 } },
        { "GOLD", "Radiant Gold", { 212, 175, 55, 255 } },
        { "AMBER", "Golden Amber", { 196, 123, 23, 255 } },
        { "BROWN", "Chestnut Brown", { 107, 63, 29, 255 } }
    };

    static const std::vector<std::string> racialTypes = {
        "Human", "Demon", "Cat-morph", "Dog-morph", "Wolf-morph", "Horse-morph", "Fox-morph", "Harpy",
        "Bovine-morph", "Dragon-morph", "Rabbit-morph", "Bat-morph", "Alligator-morph", "Rat-morph",
        "Squirrel-morph", "Reindeer-morph", "Spider-morph", "Shark-morph", "Dolphin-morph", "Elf", "Slime", "Angel", "Imp"
    };

    static const std::vector<std::string> minorRacesWithNone = {
        "None", "Human", "Demon", "Cat-morph", "Dog-morph", "Wolf-morph", "Horse-morph", "Fox-morph", "Harpy",
        "Bovine-morph", "Dragon-morph", "Rabbit-morph", "Bat-morph", "Alligator-morph", "Rat-morph",
        "Squirrel-morph", "Reindeer-morph", "Spider-morph", "Shark-morph", "Dolphin-morph", "Elf", "Slime", "Angel", "Imp"
    };

    static const std::vector<std::string> bodySizes = { "Skinny", "Slender", "Average", "Large", "Huge" };
    static const std::vector<std::string> muscleTiers = { "Soft", "Lightly Muscled", "Toned", "Muscular", "Ripped" };
    static const std::vector<std::string> bodyMaterials = { "Flesh", "Slime", "Fire", "Ice", "Air", "Earth", "Water", "Arcane", "Rubber" };
    static const std::vector<std::string> footTypes = { "Plantigrade", "Digitigrade", "Unguligrade", "Arachnoid" };
    static const std::vector<std::string> legConfigs = { "Bipedal", "Taur", "Serpent", "Arachnid", "Cephalopod", "Avian" };
    static const std::vector<std::string> genitalPlacements = { "Normal", "Cloaca", "Rear Cloaca" };
    static const std::vector<std::string> bodyHairOptions = { "None", "Stubble", "Manicured", "Trimmed", "Natural", "Unkempt", "Bushy", "Wild" };

    static const std::vector<std::string> eyeShapes = { "Round", "Horizontal", "Vertical", "Heart", "Star" };
    static const std::vector<std::string> hairLengths = { "Bald", "Very Short", "Short", "Shoulder-length", "Long", "Very Long", "Incredibly Long" };
    static const std::vector<std::string> allHairStyles = {
        "Bald", "Messy", "Loose", "Slicked-back", "Mohawk", "Afro", "Sidecut", "Pixie", "Bob cut",
        "Straight", "Wavy", "Curly", "Ponytail", "Low ponytail", "Bun", "Chignon", "Braided",
        "Twin tails", "Twin braids", "Side braids", "Crown braid", "Hime cut", "Topknot", "Dreadlocks"
    };

    static const std::vector<std::string> hornTypes = { "None", "Curved", "Swept-back", "Spiral", "Ram", "Demon", "Antlers", "Bull", "Unicorn", "Dragon" };
    static const std::vector<std::string> antennaTypes = { "None", "Moth", "Bee", "Butterfly", "Ant" };
    static const std::vector<std::string> lipSizes = { "Thin", "Average", "Full", "Plump", "Huge" };

    static const std::vector<std::string> wetnessLevels = { "Dry", "Slightly Moist", "Moist", "Wet", "Slimy", "Sloppy", "Sopping Wet", "Drooling" };
    static const std::vector<std::string> capacityLevels = { "Extremely Tight", "Tight", "Average", "Roomy", "Gaping" };
    static const std::vector<std::string> depthLevels = { "Extremely Shallow", "Shallow", "Average", "Deep", "Very Deep", "Cavernous", "Unfathomable" };
    static const std::vector<std::string> elasticityLevels = { "Unyielding", "Rigid", "Firm", "Flexible", "Limber", "Stretchy", "Supple", "Elastic" };
    static const std::vector<std::string> plasticityLevels = { "Rubbery", "Springy", "Tensile", "Resilient", "Accommodating", "Yielding", "Malleable", "Mouldable" };
    static const std::vector<std::string> orificeModifiers = { "Puffy", "Internally-ribbed", "Tentacled", "Internally-muscled" };
    static const std::vector<std::string> penetrationModifiers = { "Sheathed", "Ribbed", "Tentacled", "Knotted", "Blunt", "Tapered", "Flared", "Barbed", "Veiny", "Prehensile", "Ovipositor" };
    static const std::vector<std::string> tongueModifiers = { "Ribbed", "Tentacled", "Bifurcated", "Wide", "Flat", "Strong", "Tapered" };

    static const std::vector<std::string> cupSizes = {
        "Flat", "AA", "A", "B", "C", "D", "DD", "E", "F", "FF", "G", "GG", "H", "HH", "J", "JJ", "K", "KK", "L", "M", "N", "O", "P", "Enormous"
    };
    static const std::vector<std::string> breastShapes = { "Round", "Pointy", "Perky", "Side-set", "Wide", "Narrow" };
    static const std::vector<std::string> nippleShapes = { "Normal", "Inverted", "Lips", "Vagina" };
    static const std::vector<std::string> areolaeShapes = { "Round", "Heart", "Star" };
    static const std::vector<std::string> girthLevels = { "Thin", "Slender", "Narrow", "Average", "Girthy", "Thick", "Chubby", "Fat" };
    static const std::vector<std::string> wingSizes = { "None", "Tiny", "Small", "Average", "Large", "Huge", "Massive", "Unreasonable" };
    static const std::vector<std::string> size5 = { "Tiny", "Small", "Average", "Large", "Huge" };

    static const std::vector<std::string> fluidFlavours = {
        "Milk", "Cum", "Girlcum", "Bubblegum", "Beer", "Vanilla", "Strawberry", "Chocolate", "Pineapple",
        "Honey", "Mint", "Cherry", "Coffee", "Tea", "Maple", "Cinnamon", "Lemon", "Orange", "Grape",
        "Melon", "Coconut", "Blueberry", "Banana"
    };

    static const std::vector<std::string> fluidModifiers = {
        "Viscous", "Sticky", "Slimy", "Bubbling", "Musky", "Mineral Oil", "Alcoholic", "Strongly Alcoholic", "Addictive", "Psychoactive"
    };

    static float drawSectionTitle(SDL_Renderer* renderer, float padX, float curY, float availableW, std::string_view title, std::string_view subtitle, float uiScale)
    {
        float startY = curY;
        curY += (12.0f * uiScale);

        SDL_FRect barRect = { padX, curY, availableW, 24.0f * uiScale };
        UIWidget::drawPanel(renderer, barRect, Theme::colors.bgHeader, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, title, padX + (10.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        curY += barRect.h + (4.0f * uiScale);

        if (!subtitle.empty())
        {
            UIWidget::drawText(renderer, subtitle, padX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.76f);
            curY += (16.0f * uiScale);
        }
        else
        {
            curY += (4.0f * uiScale);
        }
        return curY - startY;
    }

    static float drawPillGrid(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, const std::vector<std::string>& options, const std::string& currentSelected, auto onSelect, float uiScale, int cols = 5)
    {
        float startY = curY;
        float gapX = 6.0f * uiScale;
        float gapY = 6.0f * uiScale;
        float btnW = (availableW - (gapX * (cols - 1))) / cols;
        float btnH = 26.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + c * (btnW + gapX), curY + r * (btnH + gapY), btnW, btnH };

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
        curY += (totalRows * (btnH + gapY)) + (8.0f * uiScale);
        return curY - startY;
    }

    static float drawTogglePills(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, const std::vector<std::string>& options, const std::vector<std::string>& activeItems, auto onToggle, float uiScale, int cols = 4)
    {
        float startY = curY;
        float gapX = 6.0f * uiScale;
        float gapY = 6.0f * uiScale;
        float btnW = (availableW - (gapX * (cols - 1))) / cols;
        float btnH = 26.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect bRect = { padX + c * (btnW + gapX), curY + r * (btnH + gapY), btnW, btnH };

            bool isActive = (std::find(activeItems.begin(), activeItems.end(), options[i]) != activeItems.end());
            bool hovered = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                            mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, options[i], hovered, true, isActive, uiScale * 0.78f);

            if (hovered && clicked)
            {
                onToggle(options[i], !isActive);
                gameContext->input.consumeMouseClick();
            }
        }

        int totalRows = static_cast<int>((options.size() + cols - 1) / cols);
        curY += (totalRows * (btnH + gapY)) + (8.0f * uiScale);
        return curY - startY;
    }

    static float drawColorSwatchPalette(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, const std::vector<ColorOption>& options, const std::string& currentSelected, auto onSelect, float uiScale)
    {
        float startY = curY;
        
        auto equalsIgnoreCase = [](std::string_view a, std::string_view b) {
            return std::ranges::equal(a, b, [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb));
            });
        };

        std::string selectedName = options.empty() ? "Default" : options[0].name;
        SDL_Color selectedColor = options.empty() ? Theme::colors.textGold : options[0].color;
        for (const auto& opt : options)
        {
            if (equalsIgnoreCase(opt.name, currentSelected) || equalsIgnoreCase(opt.id, currentSelected))
            {
                selectedName = opt.name;
                selectedColor = opt.color;
                break;
            }
        }

        // Header Line: Selected Color + Preview Box
        SDL_FRect previewBox = { padX, curY + (2.0f * uiScale), 18.0f * uiScale, 18.0f * uiScale };
        UIWidget::drawPanel(renderer, previewBox, selectedColor, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, std::format("Selected Color: {}", selectedName), padX + (26.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        curY += (28.0f * uiScale);

        // Small Square Swatch Tiles Grid (12 cols x 2 rows, or 8 cols x 3 rows)
        float tileSize = 28.0f * uiScale;
        float gap = 8.0f * uiScale;
        int cols = 12; // 12x2 grid for 24 colors
        if (options.size() <= 8) cols = static_cast<int>(options.size());

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < options.size(); ++i)
        {
            int r = static_cast<int>(i / cols);
            int c = static_cast<int>(i % cols);
            SDL_FRect sRect = { padX + c * (tileSize + gap), curY + r * (tileSize + gap), tileSize, tileSize };

            bool isSelected = equalsIgnoreCase(options[i].name, selectedName);
            bool hovered = (mousePos.x >= sRect.x && mousePos.x <= sRect.x + sRect.w &&
                            mousePos.y >= sRect.y && mousePos.y <= sRect.y + sRect.h);

            UIWidget::drawColorSwatch(renderer, sRect, options[i].color, isSelected, hovered, uiScale);

            if (hovered && clicked)
            {
                onSelect(options[i].name);
                gameContext->input.consumeMouseClick();
            }
        }

        int totalRows = static_cast<int>((options.size() + cols - 1) / cols);
        curY += (totalRows * (tileSize + gap)) + (10.0f * uiScale);
        return curY - startY;
    }

    static float drawStepper(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, std::string_view label, std::string_view displayVal, auto onStep, float uiScale, int stepDelta = 1)
    {
        float startY = curY;
        curY += (3.0f * uiScale);

        float labelW = availableW * 0.42f;
        float btnW = 32.0f * uiScale;
        float valW = 150.0f * uiScale;
        float h = 26.0f * uiScale;

        UIWidget::drawText(renderer, label, padX, curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);

        float curX = padX + labelW;
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Left Arrow [ < ]
        SDL_FRect decRect = { curX, curY, btnW, h };
        bool decHov = (mousePos.x >= decRect.x && mousePos.x <= decRect.x + decRect.w &&
                       mousePos.y >= decRect.y && mousePos.y <= decRect.y + decRect.h);
        UIWidget::drawButton(renderer, decRect, "<", decHov, true, false, uiScale * 0.85f);
        if (decHov && clicked)
        {
            onStep(-stepDelta);
            gameContext->input.consumeMouseClick();
        }
        curX += btnW + (4.0f * uiScale);

        // Center Value Display
        SDL_FRect valRect = { curX, curY, valW, h };
        UIWidget::drawPanel(renderer, valRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        float valTextW = UIWidget::getTextWidth(displayVal, uiScale * 0.82f);
        float valTextX = curX + std::max(4.0f * uiScale, (valW - valTextW) / 2.0f);
        UIWidget::drawText(renderer, displayVal, valTextX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
        curX += valW + (4.0f * uiScale);

        // Right Arrow [ > ]
        SDL_FRect incRect = { curX, curY, btnW, h };
        bool incHov = (mousePos.x >= incRect.x && mousePos.x <= incRect.x + incRect.w &&
                       mousePos.y >= incRect.y && mousePos.y <= incRect.y + incRect.h);
        UIWidget::drawButton(renderer, incRect, ">", incHov, true, false, uiScale * 0.85f);
        if (incHov && clicked)
        {
            onStep(stepDelta);
            gameContext->input.consumeMouseClick();
        }

        curY += h + (8.0f * uiScale);
        return curY - startY;
    }

    static float drawToggle(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, std::string_view label, bool currentState, auto onToggle, float uiScale, std::string_view trueLabel = "Yes", std::string_view falseLabel = "No")
    {
        float startY = curY;
        curY += (3.0f * uiScale);

        float labelW = availableW * 0.50f;
        float btnW = 75.0f * uiScale;
        float h = 26.0f * uiScale;

        UIWidget::drawText(renderer, label, padX, curY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.85f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        float btn1X = padX + labelW;
        SDL_FRect r1 = { btn1X, curY, btnW, h };
        bool hov1 = (mousePos.x >= r1.x && mousePos.x <= r1.x + r1.w && mousePos.y >= r1.y && mousePos.y <= r1.y + r1.h);
        UIWidget::drawButton(renderer, r1, trueLabel, hov1, true, currentState, uiScale * 0.82f);
        if (hov1 && clicked)
        {
            onToggle(true);
            gameContext->input.consumeMouseClick();
        }

        float btn2X = btn1X + btnW + (6.0f * uiScale);
        SDL_FRect r2 = { btn2X, curY, btnW, h };
        bool hov2 = (mousePos.x >= r2.x && mousePos.x <= r2.x + r2.w && mousePos.y >= r2.y && mousePos.y <= r2.y + r2.h);
        UIWidget::drawButton(renderer, r2, falseLabel, hov2, true, !currentState, uiScale * 0.82f);
        if (hov2 && clicked)
        {
            onToggle(false);
            gameContext->input.consumeMouseClick();
        }

        curY += h + (8.0f * uiScale);
        return curY - startY;
    }

    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        transformationState* state = dynamic_cast<transformationState*>(gameContext->getActiveState());
        entity* player = gameContext->getPlayer();
        if (!state || !player) return 0.0f;

        float startY = curY;
        float padX = rect.x + (16.0f * uiScale);
        float availableW = rect.w - (32.0f * uiScale);

        // 1. Full-Width Header Banner
        float headerH = 28.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, std::format("FULL TRANSFORMATION & BODY MODIFICATION [{}]", transformationTabToString(state->currentTab)), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (8.0f * uiScale);

        // 2. Overview Status Card
        SDL_FRect statusCardRect = { padX, curY, availableW, 38.0f * uiScale };
        UIWidget::drawPanel(renderer, statusCardRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string genderStr = genderArchetypeToString(player->anatomy.getGenderArchetype());
        std::string racialTitle = player->anatomy.getRacialTitle();
        UIWidget::drawText(renderer, std::format("Subject: {} | Form: {} ({}) | Height: {:.0f}cm", player->name, racialTitle, genderStr, player->anatomy.heightMeters * 100.0f), padX + (10.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        std::string line2 = "Features: ";
        if (const bodyPart* b = player->anatomy.getPart(bodySlot::BREASTS)) line2 += std::format("Breasts: {} ", bodyPart::getCupSizeName(b->cupSize));
        if (player->anatomy.hasPenis()) line2 += "• Penis ";
        if (player->anatomy.hasVagina()) line2 += "• Vagina ";
        if (player->anatomy.hasPart(bodySlot::HORNS)) line2 += "• Horns ";
        if (player->anatomy.hasPart(bodySlot::WINGS)) line2 += "• Wings ";
        if (player->anatomy.hasPart(bodySlot::TAIL)) line2 += "• Tail ";
        UIWidget::drawText(renderer, line2, padX + (10.0f * uiScale), curY + (20.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
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
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Core Physical Proportions", "Adjust appeared age, femininity, height, and body shape.", uiScale);

                int age = player->stats.getBaseStat("appeared_age") > 0 ? static_cast<int>(player->stats.getBaseStat("appeared_age")) : 20;
                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Age Appearance", std::format("{} years", age), [&](int delta) {
                    int newAge = std::clamp(age + delta, 18, 50);
                    player->stats.setBaseStat("appeared_age", static_cast<float>(newAge));
                }, uiScale, 1);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Height (cm)", std::format("{:.0f} cm", player->anatomy.heightMeters * 100.0f), [&](int delta) {
                    player->anatomy.heightMeters = std::clamp(player->anatomy.heightMeters + (delta * 0.01f), 1.22f, 3.66f);
                }, uiScale, 2);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Body Size & Muscle Proportions", "Adjust body fat and muscularity to shape your figure.", uiScale);
                std::string curSize = player->anatomy.bodySize.empty() ? "Average" : player->anatomy.bodySize;
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, bodySizes, curSize, [&](const std::string& s) {
                    player->anatomy.bodySize = s;
                }, uiScale, 5);

                std::string curMuscle = player->anatomy.muscleTone.empty() ? "Toned" : player->anatomy.muscleTone;
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, muscleTiers, curMuscle, [&](const std::string& m) {
                    player->anatomy.muscleTone = m;
                }, uiScale, 5);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Racial Morphs & Body Structure", "Configure racial base types across individual sockets.", uiScale);
                
                bodyPart* face = player->anatomy.getPart(bodySlot::HEAD);
                std::string curFace = face ? face->race : "Human";
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, racialTypes, curFace, [&](const std::string& r) {
                    if (!player->anatomy.hasPart(bodySlot::HEAD)) {
                        bodyPart p; p.id = "head"; p.name = "Head"; p.race = r; player->anatomy.setPart(bodySlot::HEAD, p);
                    } else player->anatomy.getPart(bodySlot::HEAD)->race = r;
                }, uiScale, 6);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Skin Material & Colour Palette", "Change body covering type and skin color.", uiScale);
                bodyPart* torso = player->anatomy.getPart(bodySlot::TORSO);
                std::string torsoColor = torso ? torso->primaryColor : "Pale";
                curY += drawColorSwatchPalette(renderer, gameContext, padX, curY, availableW, tfColors, torsoColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::TORSO)) {
                        bodyPart tp; tp.id = "torso"; tp.name = "Torso"; tp.race = "Human"; tp.primaryColor = col;
                        player->anatomy.setPart(bodySlot::TORSO, tp);
                    } else {
                        player->anatomy.getPart(bodySlot::TORSO)->primaryColor = col;
                    }
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Limbs, Feet & Lower Body Configuration", "Change arm pairs, leg setup, and foot structure.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, footTypes, "Plantigrade", [](const std::string&) {}, uiScale, 4);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, legConfigs, "Bipedal", [](const std::string&) {}, uiScale, 6);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, genitalPlacements, "Normal", [](const std::string&) {}, uiScale, 3);
                break;
            }

            case TransformationTab::EYES:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ocular Morphology", "Customize eye race, shape, count, and color.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, racialTypes, player->anatomy.hasPart(bodySlot::EYES) ? player->anatomy.getPart(bodySlot::EYES)->race : "Human", [&](const std::string& r) {
                    bodyPart ep; ep.id = "eyes_" + r; ep.name = "Eyes"; ep.race = r; ep.primaryColor = "Azure Blue";
                    player->anatomy.setPart(bodySlot::EYES, ep);
                }, uiScale, 6);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Iris & Pupil Shapes", "Change iris and pupil structure.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, eyeShapes, "Round", [](const std::string&) {}, uiScale, 5);

                bodyPart* eyes = player->anatomy.getPart(bodySlot::EYES);
                std::string eyeColor = eyes ? eyes->primaryColor : "Azure Blue";
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Iris Color Swatches", "Color of the eye irises.", uiScale);
                curY += drawColorSwatchPalette(renderer, gameContext, padX, curY, availableW, tfColors, eyeColor, [&](const std::string& col) {
                    if (auto* ep = player->anatomy.getPart(bodySlot::EYES)) ep->primaryColor = col;
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Sclera Color Swatches", "Color of the sclerae.", uiScale);
                curY += drawColorSwatchPalette(renderer, gameContext, padX, curY, availableW, tfColors, "Pure White", [](const std::string&) {}, uiScale);
                break;
            }

            case TransformationTab::HAIR:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Hair Length & Style", "Choose length, style, and grooming.", uiScale);
                bodyPart* hair = player->anatomy.getPart(bodySlot::HAIR);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Hair Length (cm)", std::format("{:.0f} cm", hair ? hair->length : 15.0f), [&](int delta) {
                    if (!hair) { bodyPart hp; hp.id = "hair"; hp.name = "Hair"; hp.length = 15.0f; player->anatomy.setPart(bodySlot::HAIR, hp); }
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->length = std::clamp(hp->length + (delta * 3.0f), 0.0f, 150.0f);
                }, uiScale, 3);

                std::string curStyle = hair ? hair->style : "Short";
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, allHairStyles, curStyle, [&](const std::string& s) {
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->style = s;
                }, uiScale, 6);

                std::string hairColor = hair ? hair->primaryColor : "Chestnut Brown";
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Hair Color", "Select head hair color.", uiScale);
                curY += drawColorSwatchPalette(renderer, gameContext, padX, curY, availableW, tfColors, hairColor, [&](const std::string& col) {
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->primaryColor = col;
                }, uiScale);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Facial Hair & Grooming", "Beard and facial hair.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, bodyHairOptions, "None", [](const std::string&) {}, uiScale, 4);
                break;
            }

            case TransformationTab::HEAD_FACE:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ears & Facial Structure", "Change ear morphology and race.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, racialTypes, player->anatomy.hasPart(bodySlot::EARS) ? player->anatomy.getPart(bodySlot::EARS)->race : "Human", [&](const std::string& r) {
                    bodyPart ear; ear.id = "ears_" + r; ear.name = "Ears"; ear.race = r;
                    player->anatomy.setPart(bodySlot::EARS, ear);
                }, uiScale, 6);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Horns & Crests", "Grow, lengthen, or remove horns.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, hornTypes, player->anatomy.hasPart(bodySlot::HORNS) ? player->anatomy.getPart(bodySlot::HORNS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::HORNS);
                    else {
                        bodyPart h; h.id = "horns_" + r; h.name = "Horns"; h.race = r; h.count = 2; h.length = 20.0f;
                        player->anatomy.setPart(bodySlot::HORNS, h);
                    }
                }, uiScale, 5);

                if (bodyPart* horns = player->anatomy.getPart(bodySlot::HORNS))
                {
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Horn Length (cm)", std::format("{:.0f} cm", horns->length), [&](int delta) {
                        horns->length = std::clamp(horns->length + (delta * 2.0f), 2.0f, 80.0f);
                    }, uiScale, 2);

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Horn Rows", std::format("{}", horns->count / 2), [&](int delta) {
                        horns->count = std::clamp(horns->count + (delta * 2), 2, 8);
                    }, uiScale, 1);
                }

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Antennae", "Grow, lengthen, or remove antennae.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, antennaTypes, "None", [](const std::string&) {}, uiScale, 5);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Lips, Mouth & Tongue", "Modify lip dimensions, throat orifice, and tongue.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, lipSizes, "Average", [](const std::string&) {}, uiScale, 5);
                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Puffy Lips", false, [](bool) {}, uiScale, "Puffy", "Natural");

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Throat Orifice Modifiers", "Special internal qualities of your throat.", uiScale);
                curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, orificeModifiers, {}, [](const std::string&, bool) {}, uiScale, 4);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, wetnessLevels, "Moist", [](const std::string&) {}, uiScale, 4);
                break;
            }

            case TransformationTab::ASS_HIPS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Ass & Hip Dimensions", "Customize rear proportions.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, size5, "Average", [](const std::string&) {}, uiScale, 5);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, size5, "Average", [](const std::string&) {}, uiScale, 5);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Anus Orifice & Rectum", "Elasticity, depth, and internal qualities.", uiScale);
                bodyPart* ass = player->anatomy.getPart(bodySlot::ASS);
                if (!ass) { bodyPart ap; ap.id = "ass"; ap.name = "Ass"; ap.orifice.exists = true; player->anatomy.setPart(bodySlot::ASS, ap); ass = player->anatomy.getPart(bodySlot::ASS); }

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Anus Depth (cm)", std::format("{:.0f} cm", ass->orifice.depthCm), [&](int delta) {
                    ass->orifice.depthCm = std::clamp(ass->orifice.depthCm + delta, 5.0f, 40.0f);
                }, uiScale, 1);

                curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Elasticity", std::format("{:.0f}%", ass->orifice.elasticity), [&](int delta) {
                    ass->orifice.elasticity = std::clamp(ass->orifice.elasticity + (delta * 5.0f), 0.0f, 100.0f);
                }, uiScale, 5);

                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Bleached Anus", ass->tags.empty() ? false : std::ranges::find(ass->tags, std::string("bleached")) != ass->tags.end(), [&](bool state) {
                    if (state) ass->tags.push_back("bleached");
                    else std::erase(ass->tags, "bleached");
                }, uiScale);

                curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, orificeModifiers, {}, [](const std::string&, bool) {}, uiScale, 4);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, wetnessLevels, "Moist", [](const std::string&) {}, uiScale, 4);
                break;
            }

            case TransformationTab::BREASTS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Breasts & Lactation", "Modify chest volume, shape, rows, and milk output.", uiScale);
                bodyPart* breasts = player->anatomy.getPart(bodySlot::BREASTS);
                if (!breasts) { bodyPart bp; bp.id = "breasts"; bp.name = "Breasts"; player->anatomy.setPart(bodySlot::BREASTS, bp); breasts = player->anatomy.getPart(bodySlot::BREASTS); }

                std::string curCup = bodyPart::getCupSizeName(breasts->cupSize);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, cupSizes, curCup, [&](const std::string& c) {
                    for (size_t i = 0; i < cupSizes.size(); ++i) if (cupSizes[i] == c) breasts->cupSize = static_cast<int>(i);
                }, uiScale, 8);

                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, breastShapes, "Round", [](const std::string&) {}, uiScale, 6);

                curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Lactation Active", breasts->isLactating, [&](bool l) {
                    breasts->isLactating = l;
                    if (l && breasts->maxFluidMl <= 0.0f) { breasts->maxFluidMl = 1000.0f; breasts->currentFluidMl = 250.0f; }
                }, uiScale);

                if (breasts->isLactating)
                {
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Milk Storage (ml)", std::format("{:.0f} ml", breasts->maxFluidMl), [&](int delta) {
                        breasts->maxFluidMl = std::clamp(breasts->maxFluidMl + (delta * 100.0f), 100.0f, 10000.0f);
                        breasts->currentFluidMl = std::min(breasts->currentFluidMl, breasts->maxFluidMl);
                    }, uiScale, 100);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Milk Flavours & Modifiers", "Flavor and psychoactive qualities of milk.", uiScale);
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, fluidFlavours, "Milk", [](const std::string&) {}, uiScale, 6);
                    curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, fluidModifiers, {}, [](const std::string&, bool) {}, uiScale, 5);
                }

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Nipples & Areolae", "Nipple count, shape, and elasticity.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, nippleShapes, "Normal", [](const std::string&) {}, uiScale, 4);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, areolaeShapes, "Round", [](const std::string&) {}, uiScale, 3);
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
                    }, uiScale, 1);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Clitoris Dimensions & Modifiers", "Clitoral size and penetration traits.", uiScale);
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, size5, "Average", [](const std::string&) {}, uiScale, 5);
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, girthLevels, "Average", [](const std::string&) {}, uiScale, 4);
                    curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, penetrationModifiers, {}, [](const std::string&, bool) {}, uiScale, 4);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Female Reproduction & Orgasms", "Squirting, egg-laying, and hymen.", uiScale);
                    curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Squirter", false, [](bool) {}, uiScale, "Squirter", "Normal");
                    curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Hymen Intact", true, [](bool) {}, uiScale, "Intact", "Broken");
                    curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Egg Layer", false, [](bool) {}, uiScale, "Egg-layer", "Live young");

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Girlcum Flavours & Modifiers", "Taste and qualities of female arousal fluids.", uiScale);
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, fluidFlavours, "Girlcum", [](const std::string&) {}, uiScale, 6);
                    curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, fluidModifiers, {}, [](const std::string&, bool) {}, uiScale, 5);
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
                        g->length = std::clamp(g->length + delta, 3.0f, 100.0f);
                    }, uiScale, 1);

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Girth (cm)", std::format("{:.1f} cm", g->diameter), [&](int delta) {
                        g->diameter = std::clamp(g->diameter + (delta * 0.2f), 1.5f, 15.0f);
                    }, uiScale, 1);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Penis Modifiers", "Knots, barbs, flares, sheaths, and special traits.", uiScale);
                    curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, penetrationModifiers, {}, [](const std::string&, bool) {}, uiScale, 4);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Testicles & Cum Production", "Testicle count, storage volume, and cum traits.", uiScale);
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Testicle Count", "2", [](int) {}, uiScale, 1);
                    curY += drawToggle(renderer, gameContext, padX, curY, availableW, "Internal Testicles", false, [](bool) {}, uiScale, "Internal", "External");

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Cum Storage", std::format("{:.0f} ml", g->maxFluidMl), [&](int delta) {
                        g->maxFluidMl = std::clamp(g->maxFluidMl + (delta * 25.0f), 5.0f, 10000.0f);
                        g->currentFluidMl = std::min(g->currentFluidMl, g->maxFluidMl);
                    }, uiScale, 25);

                    curY += drawSectionTitle(renderer, padX, curY, availableW, "Cum Flavours & Modifiers", "Taste and qualities of male seed.", uiScale);
                    curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, fluidFlavours, "Cum", [](const std::string&) {}, uiScale, 6);
                    curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, fluidModifiers, {}, [](const std::string&, bool) {}, uiScale, 5);
                }
                break;
            }

            case TransformationTab::CROTCH_BOOBS:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Crotch-Boobs & Udders", "Secondary udder / crotch mammary glands.", uiScale);
                static const std::vector<std::string> crotchTypes = { "None", "Bovine Udders", "Caprine Udders", "Feline Crotch-Boobs", "Canine Teats" };
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, crotchTypes, "None", [](const std::string&) {}, uiScale, 5);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, cupSizes, "Flat", [](const std::string&) {}, uiScale, 8);
                break;
            }

            case TransformationTab::APPENDAGES:
            {
                curY += drawSectionTitle(renderer, padX, curY, availableW, "Wings & Flight Organs", "Grow, size, or modify wings.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, minorRacesWithNone, player->anatomy.hasPart(bodySlot::WINGS) ? player->anatomy.getPart(bodySlot::WINGS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::WINGS);
                    else {
                        bodyPart w; w.id = "wings_" + r; w.name = "Wings"; w.race = r; w.count = 2;
                        player->anatomy.setPart(bodySlot::WINGS, w);
                    }
                }, uiScale, 6);

                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, wingSizes, "Average", [](const std::string&) {}, uiScale, 4);

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Tails & Prehensile Appendages", "Grow or modify tail length and count.", uiScale);
                curY += drawPillGrid(renderer, gameContext, padX, curY, availableW, minorRacesWithNone, player->anatomy.hasPart(bodySlot::TAIL) ? player->anatomy.getPart(bodySlot::TAIL)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::TAIL);
                    else {
                        bodyPart t; t.id = "tail_" + r; t.name = "Tail"; t.race = r; t.count = 1; t.length = 75.0f;
                        player->anatomy.setPart(bodySlot::TAIL, t);
                    }
                }, uiScale, 6);

                if (bodyPart* tail = player->anatomy.getPart(bodySlot::TAIL))
                {
                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Tail Count", std::format("{}", tail->count), [&](int delta) {
                        tail->count = std::clamp(tail->count + delta, 1, 9);
                    }, uiScale, 1);

                    curY += drawStepper(renderer, gameContext, padX, curY, availableW, "Tail Length (cm)", std::format("{:.0f} cm", tail->length), [&](int delta) {
                        tail->length = std::clamp(tail->length + (delta * 5.0f), 10.0f, 250.0f);
                    }, uiScale, 5);
                }

                curY += drawSectionTitle(renderer, padX, curY, availableW, "Spinneret (Arachnid Silk Gland)", "Silk production orifice.", uiScale);
                curY += drawTogglePills(renderer, gameContext, padX, curY, availableW, orificeModifiers, {}, [](const std::string&, bool) {}, uiScale, 4);
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
                SDL_FRect saveBtnRect = { padX, curY, 140.0f * uiScale, 24.0f * uiScale };
                auto mousePos = gameContext->input.getMousePosition();
                bool clicked = gameContext->input.isLeftMouseJustClicked();
                bool sHov = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                             mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

                UIWidget::drawButton(renderer, saveBtnRect, "Save New Preset", sHov, true, false, uiScale * 0.8f);
                if (sHov && clicked)
                {
                    state->savePreset(gameContext, "Preset_" + player->anatomy.getRacialTitle());
                    gameContext->input.consumeMouseClick();
                }

                float rBtnX = padX + (150.0f * uiScale);
                SDL_FRect rBtnRect = { rBtnX, curY, 140.0f * uiScale, 24.0f * uiScale };
                bool rHov = (mousePos.x >= rBtnRect.x && mousePos.x <= rBtnRect.x + rBtnRect.w &&
                             mousePos.y >= rBtnRect.y && mousePos.y <= rBtnRect.y + rBtnRect.h);
                UIWidget::drawButton(renderer, rBtnRect, "Reset Human", rHov, true, false, uiScale * 0.8f);
                if (rHov && clicked)
                {
                    state->resetToHuman(gameContext);
                    gameContext->input.consumeMouseClick();
                }
                curY += (32.0f * uiScale);

                // List saved presets
                auto presets = state->getPresetNames();
                if (!presets.empty())
                {
                    for (const auto& pName : presets)
                    {
                        SDL_FRect pRect = { padX, curY, availableW - (100.0f * uiScale), 22.0f * uiScale };
                        UIWidget::drawPanel(renderer, pRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                        UIWidget::drawText(renderer, pName, pRect.x + (8.0f * uiScale), curY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

                        // Load button
                        SDL_FRect loadRect = { padX + availableW - (95.0f * uiScale), curY, 45.0f * uiScale, 22.0f * uiScale };
                        bool lHov = (mousePos.x >= loadRect.x && mousePos.x <= loadRect.x + loadRect.w &&
                                     mousePos.y >= loadRect.y && mousePos.y <= loadRect.y + loadRect.h);
                        UIWidget::drawButton(renderer, loadRect, "Load", lHov, true, false, uiScale * 0.72f);
                        if (lHov && clicked)
                        {
                            state->loadPreset(gameContext, pName);
                            gameContext->input.consumeMouseClick();
                        }

                        // Del button
                        SDL_FRect delRect = { padX + availableW - (45.0f * uiScale), curY, 40.0f * uiScale, 22.0f * uiScale };
                        bool dHov = (mousePos.x >= delRect.x && mousePos.x <= delRect.x + delRect.w &&
                                     mousePos.y >= delRect.y && mousePos.y <= delRect.y + delRect.h);
                        UIWidget::drawButton(renderer, delRect, "Del", dHov, true, false, uiScale * 0.72f);
                        if (dHov && clicked)
                        {
                            state->deletePreset(pName);
                            gameContext->input.consumeMouseClick();
                        }
                        curY += (26.0f * uiScale);
                    }
                }
                break;
            }
        }

        return curY - startY;
    }
}
