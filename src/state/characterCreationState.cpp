#include "state/characterCreationState.h"

#include <algorithm>
#include <random>
#include "core/game.h"
#include "items/itemDatabase.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

static constexpr std::string_view MASC_NAMES[] = {
    "Arthur", "James", "Thomas", "William", "Alexander", "Edward",
    "Henry", "Charles", "Oliver", "George", "Harry", "Jack", "Samuel", "David"
};

static constexpr std::string_view ANDRO_NAMES[] = {
    "Alex", "Sam", "Chris", "Taylor", "Jordan", "Morgan",
    "Riley", "Robin", "Casey", "Jamie", "Quinn", "Avery"
};

static constexpr std::string_view FEM_NAMES[] = {
    "Lily", "Victoria", "Alice", "Charlotte", "Eleanor", "Grace",
    "Sophia", "Emma", "Olivia", "Rose", "Emily", "Isabella"
};

static constexpr std::string_view SURNAMES[] = {
    "Blackwood", "Sterling", "Ashford", "Montgomery", "Pemberton",
    "Sinclair", "Vance", "Harrington", "Kensington", "Thorne"
};

characterCreationState::characterCreationState(int startStep)
    : step(startStep)
{
}

void characterCreationState::initialise(game* gameContext)
{
    if (gameContext)
    {
        gameContext->gameTime.day = 29;
        gameContext->gameTime.hour = 20;
        gameContext->gameTime.minute = 34;
    }
}

void characterCreationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void characterCreationState::onExit(game* gameContext)
{
}

void characterCreationState::update(game* gameContext, float deltaTime)
{
}

void characterCreationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (step == 2) // Step 3: Name text input
    {
        if (event.type == SDL_EVENT_TEXT_INPUT)
        {
            std::string text = event.text.text;
            // Filter out '[' ']' '.'
            text.erase(std::remove_if(text.begin(), text.end(), [](char c) {
                return c == '[' || c == ']' || c == '.';
            }), text.end());

            if (activeNameField == 0)
            {
                if (masculineName == "Unknown") masculineName = "";
                if (masculineName.size() + text.size() <= 32) masculineName += text;
            }
            else if (activeNameField == 1)
            {
                if (androgynousName == "Unknown") androgynousName = "";
                if (androgynousName.size() + text.size() <= 32) androgynousName += text;
            }
            else if (activeNameField == 2)
            {
                if (feminineName == "Unknown") feminineName = "";
                if (feminineName.size() + text.size() <= 32) feminineName += text;
            }
            else if (activeNameField == 3)
            {
                if (surname.size() + text.size() <= 32) surname += text;
            }
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_BACKSPACE)
            {
                if (activeNameField == 0 && !masculineName.empty()) masculineName.pop_back();
                else if (activeNameField == 1 && !androgynousName.empty()) androgynousName.pop_back();
                else if (activeNameField == 2 && !feminineName.empty()) feminineName.pop_back();
                else if (activeNameField == 3 && !surname.empty()) surname.pop_back();
            }
            else if (event.key.key == SDLK_TAB)
            {
                activeNameField = (activeNameField + 1) % 4;
            }
        }
    }
}

void characterCreationState::handleCommand(game* gameContext, const UICommand& cmd)
{
}

void characterCreationState::randomizeFirstNames()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> distM(0, std::size(MASC_NAMES) - 1);
    std::uniform_int_distribution<size_t> distA(0, std::size(ANDRO_NAMES) - 1);
    std::uniform_int_distribution<size_t> distF(0, std::size(FEM_NAMES) - 1);

    masculineName = MASC_NAMES[distM(gen)];
    androgynousName = ANDRO_NAMES[distA(gen)];
    feminineName = FEM_NAMES[distF(gen)];
}

void characterCreationState::randomizeSurname()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> distS(0, std::size(SURNAMES) - 1);
    surname = SURNAMES[distS(gen)];
}

void characterCreationState::finalizeCharacter(game* gameContext)
{
    if (!gameContext) return;

    if (!gameContext->getPlayer())
    {
        gameContext->playerEntity = std::make_shared<entity>("player_main", "Hero");
        gameContext->Player = gameContext->playerEntity.get();
    }

    entity* player = gameContext->getPlayer();
    if (player)
    {
        std::string chosenName = masculineName;
        if (femininity == "Androgynous") chosenName = androgynousName;
        else if (femininity == "Feminine" || femininity == "Very Feminine") chosenName = feminineName;

        if (chosenName == "Unknown" || chosenName.empty())
        {
            chosenName = (gender == "Female") ? "Lily" : "Alex";
        }

        if (!surname.empty())
        {
            player->name = chosenName + " " + surname;
        }
        else
        {
            player->name = chosenName;
        }

        player->anatomy.heightMeters = static_cast<float>(heightCm) / 100.0f;

        // Head & Hair
        bodyPart hair;
        hair.id = "hair_human";
        hair.name = "Hair";
        hair.race = "Human";
        hair.primaryColor = hairColor;
        hair.style = hairStyle;
        hair.length = static_cast<float>(hairLengthCm);
        hair.covering = CoveringType::HAIR_COVERING;
        player->anatomy.setPart(bodySlot::HAIR, hair);

        bodyPart head;
        head.id = "head_human";
        head.name = "Head";
        head.race = "Human";
        head.primaryColor = skinPrimaryColor;
        head.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::HEAD, head);

        bodyPart eyes;
        eyes.id = "eyes_human";
        eyes.name = "Eyes";
        eyes.race = "Human";
        eyes.primaryColor = eyeColor;
        eyes.covering = CoveringType::IRIS;
        player->anatomy.setPart(bodySlot::EYES, eyes);

        bodyPart ears;
        ears.id = "ears_" + earType;
        ears.name = earType + " Ears";
        ears.race = (earType == "Human") ? "Human" : earType + "-morph";
        ears.primaryColor = skinPrimaryColor;
        player->anatomy.setPart(bodySlot::EARS, ears);

        bodyPart mouth;
        mouth.id = "mouth_human";
        mouth.name = "Mouth";
        mouth.race = "Human";
        mouth.covering = CoveringType::FLESH;
        mouth.orifice.exists = true;
        mouth.orifice.elasticity = 70.0f;
        mouth.orifice.maxCapacityMl = 50.0f;
        mouth.orifice.depthCm = 15.0f;
        player->anatomy.setPart(bodySlot::MOUTH, mouth);

        bodyPart torso;
        torso.id = "torso_human";
        torso.name = "Torso";
        torso.race = "Human";
        torso.primaryColor = skinPrimaryColor;
        torso.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::TORSO, torso);

        // Breasts
        bodyPart breasts;
        breasts.id = "breasts_human";
        breasts.name = "Breasts";
        breasts.race = "Human";
        breasts.cupSize = breastCupSize;
        breasts.primaryColor = skinPrimaryColor;
        breasts.isLactating = isLactating;
        breasts.maxFluidMl = milkCapacityMl;
        breasts.currentFluidMl = isLactating ? milkCapacityMl * 0.5f : 0.0f;
        player->anatomy.setPart(bodySlot::BREASTS, breasts);

        bodyPart arms;
        arms.id = "arms_human";
        arms.name = "Arms";
        arms.race = "Human";
        arms.primaryColor = skinPrimaryColor;
        arms.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::ARMS, arms);

        bodyPart hands;
        hands.id = "hands_human";
        hands.name = "Hands";
        hands.race = "Human";
        hands.primaryColor = skinPrimaryColor;
        hands.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::HANDS, hands);

        bodyPart hips;
        hips.id = "hips_human";
        hips.name = "Hips";
        hips.race = "Human";
        hips.primaryColor = skinPrimaryColor;
        player->anatomy.setPart(bodySlot::HIPS, hips);

        bodyPart ass;
        ass.id = "ass_human";
        ass.name = "Ass";
        ass.race = "Human";
        ass.primaryColor = skinPrimaryColor;
        ass.orifice.exists = true;
        ass.orifice.elasticity = anusElasticity;
        ass.orifice.maxCapacityMl = 80.0f;
        ass.orifice.depthCm = 18.0f;
        player->anatomy.setPart(bodySlot::ASS, ass);

        // Genitals
        if (gender == "Female")
        {
            bodyPart vagina;
            vagina.id = "vagina_human";
            vagina.name = "Vagina";
            vagina.race = "Human";
            vagina.tags.push_back("vagina");
            vagina.orifice.exists = true;
            vagina.orifice.elasticity = vaginaElasticity;
            vagina.orifice.wetnessLevel = vaginaWetness;
            vagina.orifice.maxCapacityMl = 120.0f;
            vagina.orifice.depthCm = 14.0f;
            player->anatomy.setPart(bodySlot::GROIN, vagina);
        }
        else
        {
            bodyPart penis;
            penis.id = "penis_human";
            penis.name = "Penis";
            penis.race = "Human";
            penis.length = penisLengthCm;
            penis.diameter = penisDiameterCm;
            penis.currentFluidMl = cumCapacityMl * 0.75f;
            penis.maxFluidMl = cumCapacityMl;
            penis.fluidRegenPerHour = 3.0f;
            penis.tags.push_back("penis");
            player->anatomy.setPart(bodySlot::GROIN, penis);
        }

        bodyPart legs;
        legs.id = "legs_human";
        legs.name = "Legs";
        legs.race = "Human";
        legs.primaryColor = skinPrimaryColor;
        legs.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::LEGS, legs);

        bodyPart feet;
        feet.id = "feet_human";
        feet.name = "Feet";
        feet.race = "Human";
        feet.primaryColor = skinPrimaryColor;
        feet.covering = stringToCoveringType(skinCovering);
        player->anatomy.setPart(bodySlot::FEET, feet);

        // Starting core stats & currency
        player->stats.setBaseStat("currency", 5000.0f);
        player->stats.setBaseStat("health", 40.0f);
        player->stats.setBaseStat("mana", 108.0f);
        player->stats.setBaseStat("lust", 0.0f);
        player->stats.setBaseStat("arcaneEssence", 20.0f);

        // Starting inventory items
        auto phone = itemDatabase::getItem("item_smartphone");
        if (phone) player->inventory.addItem(phone);

        auto stone = itemDatabase::getItem("item_opaque_demonstone");
        if (stone) player->inventory.addItem(stone);
    }

    if (gameContext)
    {
        gameContext->loadMap("overworld", 1, 1);
        gameContext->gameTime.day = 29;
        gameContext->gameTime.hour = 21;
        gameContext->gameTime.minute = 47;
    }

    gameContext->changeState(std::make_unique<explorationState>());
}
