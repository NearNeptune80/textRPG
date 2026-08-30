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
    : step(startStep), config(EditorConfig::newGamePreset())
{
}

characterCreationState::characterCreationState(EditorConfig cfg, int startStep)
    : step(startStep), config(std::move(cfg))
{
}

std::vector<EditorTabId> characterCreationState::getActiveTabs() const
{
    std::vector<EditorTabId> tabs;

    if (config.hasAnyOptionInList({ "gender", "femininity", "orientation", "start_month" }))
        tabs.push_back(EditorTabId::IDENTITY);

    if (config.hasAnyOptionInList({ "height", "body_size", "muscle", "skin_tone", "skin_covering" }))
        tabs.push_back(EditorTabId::BODY);

    if (config.hasAnyOptionInList({ "eye_color", "hair_style", "hair_color", "hair_length", "ear_type" }))
        tabs.push_back(EditorTabId::FACE_HAIR);

    if (config.hasAnyOptionInList({ "chest_size", "nipple_size", "lactation" }) && !config.isNewGameCreation)
        tabs.push_back(EditorTabId::BREASTS);

    if (config.hasAnyOptionInList({ "genitals", "wetness", "testicles" }) && !config.isNewGameCreation)
        tabs.push_back(EditorTabId::GENITALIA);

    if (config.hasAnyOptionInList({ "horns", "wings", "tails" }))
        tabs.push_back(EditorTabId::APPENDAGES);

    if (config.hasAnyOptionInList({ "piercings", "tattoos", "makeup", "pubic_hair" }))
        tabs.push_back(EditorTabId::COSMETICS);

    if (config.hasAnyOptionInList({ "wardrobe" }))
        tabs.push_back(EditorTabId::WARDROBE);

    if (config.hasAnyOptionInList({ "first_name", "surname" }) || config.isNewGameCreation)
        tabs.push_back(EditorTabId::NAME_FINISH);

    if (tabs.empty())
        tabs.push_back(EditorTabId::IDENTITY);

    return tabs;
}

int characterCreationState::getActiveTabCount() const
{
    return static_cast<int>(getActiveTabs().size());
}

EditorTabId characterCreationState::getCurrentTabId() const
{
    auto tabs = getActiveTabs();
    if (step >= 0 && step < static_cast<int>(tabs.size()))
    {
        return tabs[step];
    }
    return tabs.empty() ? EditorTabId::IDENTITY : tabs[0];
}

std::string characterCreationState::getTabName(EditorTabId tab) const
{
    switch (tab)
    {
    case EditorTabId::IDENTITY: return "Identity";
    case EditorTabId::BODY: return "Body";
    case EditorTabId::FACE_HAIR: return "Face & Hair";
    case EditorTabId::BREASTS: return "Breasts";
    case EditorTabId::GENITALIA: return "Genitalia";
    case EditorTabId::APPENDAGES: return "Appendages";
    case EditorTabId::COSMETICS: return "Cosmetics";
    case EditorTabId::WARDROBE: return "Wardrobe";
    case EditorTabId::PERSONALITY: return "Personality";
    case EditorTabId::NAME_FINISH: return config.isNewGameCreation ? "Name & Finish" : "Finalize";
    default: return "Customization";
    }
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

void characterCreationState::randomizeAll()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    randomizeFirstNames();
    randomizeSurname();

    static const char* genders[] = { "Male", "Female" };
    static const char* femLevels[] = { "Very Masculine", "Masculine", "Androgynous", "Feminine", "Very Feminine" };
    static const char* orients[] = { "Androphilic", "Ambiphilic", "Gynephilic" };
    static const char* skins[] = { "Fair", "Pale", "Tan", "Olive", "Dark", "Ebony" };
    static const char* bodySizes[] = { "Skinny", "Slender", "Average", "Muscular", "Chubby" };
    static const char* muscles[] = { "Soft", "Lightly muscled", "Toned", "Muscular", "Ripped" };
    static const char* eyes[] = { "Blue", "Green", "Brown", "Amber", "Hazel", "Violet" };
    static const char* hairStyles[] = { "Short", "Bob", "Shoulder-length", "Long", "Braided", "Ponytail", "Messy" };
    static const char* hairColors[] = { "Black", "Dark Brown", "Auburn", "Blonde", "Silver", "Red" };
    static const char* traitsList[] = { "Confident", "Lewd", "Shy", "Bold", "Inquisitive", "Dominant", "Submissive" };

    std::uniform_int_distribution<int> dG(0, 1);
    gender = genders[dG(gen)];

    std::uniform_int_distribution<int> dF(0, 4);
    femininity = femLevels[dF(gen)];

    std::uniform_int_distribution<int> dO(0, 2);
    orientation = orients[dO(gen)];

    std::uniform_int_distribution<int> dH(155, 195);
    heightCm = dH(gen);

    std::uniform_int_distribution<int> dSkin(0, 5);
    skinPrimaryColor = skins[dSkin(gen)];

    std::uniform_int_distribution<int> dBody(0, 4);
    bodySize = bodySizes[dBody(gen)];

    std::uniform_int_distribution<int> dM(0, 4);
    muscleDefinition = muscles[dM(gen)];

    std::uniform_int_distribution<int> dE(0, 5);
    eyeColor = eyes[dE(gen)];

    std::uniform_int_distribution<int> dHS(0, 6);
    hairStyle = hairStyles[dHS(gen)];

    std::uniform_int_distribution<int> dHC(0, 5);
    hairColor = hairColors[dHC(gen)];

    std::uniform_int_distribution<int> dHL(5, 50);
    hairLengthCm = dHL(gen);

    personalityTraits.clear();
    std::uniform_int_distribution<int> dTraitCount(1, 3);
    int tCount = dTraitCount(gen);
    for (int t = 0; t < tCount; ++t)
    {
        std::uniform_int_distribution<int> dTr(0, 6);
        personalityTraits.insert(traitsList[dTr(gen)]);
    }
}

void characterCreationState::initializeWardrobe()
{
    if (wardrobeInitialized) return;
    wardrobeInitialized = true;
    availableWardrobe.clear();
    for (auto& s : wardrobeEquipped) s = nullptr;

    bool isFem = (gender == "Female" || femininity == "Feminine" || femininity == "Very Feminine");

    auto shirt = itemDatabase::getItem("item_linen_shirt");
    auto trousers = itemDatabase::getItem("item_leather_trousers");
    auto boots = itemDatabase::getItem("item_leather_boots");
    auto suit = itemDatabase::getItem("item_formal_suit");
    auto dress = itemDatabase::getItem("item_evening_dress");
    auto skirt = itemDatabase::getItem("item_leather_skirt");
    auto boxers = itemDatabase::getItem("item_cotton_boxers");
    auto panties = itemDatabase::getItem("item_silk_panties");
    auto bra = itemDatabase::getItem("item_silk_bra");
    auto heels = itemDatabase::getItem("item_high_heels");
    auto dressShoes = itemDatabase::getItem("item_dress_shoes");
    auto choker = itemDatabase::getItem("item_leather_choker");
    auto gloves = itemDatabase::getItem("item_cloth_gloves");

    if (isFem)
    {
        if (dress) wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] = dress;
        else if (shirt) wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = shirt;

        if (skirt) wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = skirt;
        if (panties) wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = panties;
        if (bra) wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] = bra;
        if (heels) wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = heels;
        else if (boots) wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = boots;

        if (shirt && wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] != shirt) availableWardrobe.push_back(shirt);
        if (trousers) availableWardrobe.push_back(trousers);
        if (boots && wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] != boots) availableWardrobe.push_back(boots);
        if (suit) availableWardrobe.push_back(suit);
        if (boxers) availableWardrobe.push_back(boxers);
        if (choker) availableWardrobe.push_back(choker);
        if (gloves) availableWardrobe.push_back(gloves);
    }
    else
    {
        if (shirt) wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = shirt;
        if (trousers) wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = trousers;
        if (boxers) wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = boxers;
        if (dressShoes) wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = dressShoes;
        else if (boots) wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = boots;

        if (suit) availableWardrobe.push_back(suit);
        if (dress) availableWardrobe.push_back(dress);
        if (skirt) availableWardrobe.push_back(skirt);
        if (panties) availableWardrobe.push_back(panties);
        if (bra) availableWardrobe.push_back(bra);
        if (heels) availableWardrobe.push_back(heels);
        if (boots && wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] != boots) availableWardrobe.push_back(boots);
        if (choker) availableWardrobe.push_back(choker);
        if (gloves) availableWardrobe.push_back(gloves);
    }
}

bool characterCreationState::equipWardrobeItem(size_t wardrobeIndex)
{
    if (wardrobeIndex >= availableWardrobe.size()) return false;
    auto itemToEquip = availableWardrobe[wardrobeIndex];
    if (!itemToEquip || !itemToEquip->isEquippable) return false;

    equipSlot target = itemToEquip->targetSlot;
    if (target == equipSlot::NONE) return false;

    size_t slotIdx = static_cast<size_t>(target);
    auto existingItem = wardrobeEquipped[slotIdx];

    // Remove from wardrobe pile
    availableWardrobe.erase(availableWardrobe.begin() + wardrobeIndex);

    // If an item was already equipped, return it to wardrobe pile
    if (existingItem)
    {
        availableWardrobe.push_back(existingItem);
    }

    wardrobeEquipped[slotIdx] = itemToEquip;
    return true;
}

bool characterCreationState::unequipWardrobeItem(equipSlot slot)
{
    if (slot == equipSlot::NONE) return false;
    size_t slotIdx = static_cast<size_t>(slot);
    auto equipped = wardrobeEquipped[slotIdx];
    if (!equipped) return false;

    wardrobeEquipped[slotIdx] = nullptr;
    availableWardrobe.push_back(equipped);
    return true;
}

bool characterCreationState::isClothedEnough() const
{
    // 1. Must wear footwear
    if (!wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)]) return false;

    // 2. Must cover groin
    bool groinCovered = (wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] != nullptr ||
                         wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] != nullptr ||
                         (wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] &&
                          wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)]->id == "item_evening_dress"));
    if (!groinCovered) return false;

    // 3. Must cover chest if breasts exist or character is feminine
    bool isFem = (gender == "Female" || femininity == "Feminine" || femininity == "Very Feminine" || breastCupSize > 0);
    if (isFem)
    {
        bool chestCovered = (wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] != nullptr ||
                             wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] != nullptr ||
                             wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] != nullptr);
        if (!chestCovered) return false;
    }

    return true;
}

std::string characterCreationState::getDecencyStatus() const
{
    if (isClothedEnough())
    {
        return "Decent: Ready to attend the museum opening gala.";
    }

    std::string missing = "Indecent: ";
    bool hasFeet = (wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] != nullptr);
    bool hasGroin = (wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] != nullptr ||
                     wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] != nullptr ||
                     (wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] &&
                      wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)]->id == "item_evening_dress"));
    bool isFem = (gender == "Female" || femininity == "Feminine" || femininity == "Very Feminine" || breastCupSize > 0);
    bool hasChest = (!isFem ||
                     wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] != nullptr ||
                     wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] != nullptr ||
                     wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] != nullptr);

    std::vector<std::string> parts;
    if (!hasFeet) parts.push_back("Must put on footwear");
    if (!hasGroin) parts.push_back("Must conceal groin");
    if (!hasChest) parts.push_back("Must conceal chest");

    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0) missing += ", ";
        missing += parts[i];
    }
    missing += ".";
    return missing;
}

std::string characterCreationState::generateAppearanceDescription() const
{
    std::string desc = "";
    int totalInches = static_cast<int>(heightCm / 2.54f);
    int feet = totalInches / 12;
    int inches = totalInches % 12;

    std::string ageStr = (birthAge < 30) ? "twenties" : ((birthAge < 40) ? "thirties" : "forties");
    std::string compositeShape = EditorConfig::calculateBodyShape(muscleDefinition, bodySize);

    desc += std::format("Standing at full height, you measure {}cm ({}'{}\"). You are a {} human {} in your {}.\n\n",
                         heightCm, feet, inches, femininity, gender, ageStr);

    desc += std::format("You have a {} human face, with {} skin. You have {} hair, which has been styled into {} ({}cm). You have {} eyes, with round irises. You have a pair of {} ears.\n\n",
                         femininity, skinPrimaryColor, hairColor, hairStyle, hairLengthCm, eyeColor, earType);

    static const char* s_lipSizes[5] = { "thin", "average-sized", "full", "plump", "huge" };
    std::string lipStr = (lipSize >= 0 && lipSize < 5) ? s_lipSizes[lipSize] : "average-sized";
    desc += std::format("You have {} lips{}.\n\n", lipStr, (puffyLips ? " that are extra puffy" : ""));

    desc += std::format("Your body is human in form, and is covered in {} skin. You have a {}, {} body, giving you an appearance of being: {}.\n\n",
                         skinPrimaryColor, bodySize, muscleDefinition, compositeShape);

    if (breastCupSize == 0)
    {
        desc += "You have a flat human chest.\n\n";
    }
    else
    {
        static const char* s_cups[13] = { "flat", "AA-cup", "A-cup", "B-cup", "C-cup", "D-cup", "DD-cup", "E-cup", "F-cup", "FF-cup", "G-cup", "GG-cup", "H-cup" };
        static const char* s_sizes5[5] = { "tiny", "small", "average-sized", "large", "huge" };
        std::string cupStr = (breastCupSize >= 0 && breastCupSize < 13) ? s_cups[breastCupSize] : "C-cup";
        std::string nipStr = (nippleSize >= 0 && nippleSize < 5) ? s_sizes5[nippleSize] : "average-sized";
        std::string areStr = (areolaeSize >= 0 && areolaeSize < 5) ? s_sizes5[areolaeSize] : "average-sized";

        desc += std::format("You have a pair of {} breasts, which fit comfortably into a {} bra. On each of your {} breasts, you have {} nipples{}, surrounded by {} areolae.\n\n",
                             breastShape, cupStr, breastShape, nipStr, (puffyNipples ? " (puffy)" : ""), areStr);
    }

    desc += "You have a pair of human arms and legs, ending in plantigrade feet. ";
    if (underarmHair == "none" || underarmHair == "Hairless") desc += "There is no trace of any hair in your armpits.\n\n";
    else desc += std::format("You have {} underarm hair.\n\n", underarmHair);

    static const char* s_sizes5[5] = { "tiny", "small", "average-sized", "large", "huge" };
    std::string hipStr = (hipSize >= 0 && hipSize < 5) ? s_sizes5[hipSize] : "average-sized";
    std::string assStr = (assSize >= 0 && assSize < 5) ? s_sizes5[assSize] : "average-sized";
    desc += std::format("Your {} hips and {} ass are covered in the same {} skin as the rest of your body.{}\n\n",
                         hipStr, assStr, skinPrimaryColor, (anusBleached ? " You have a bleached anus." : ""));

    if (gender == "Female")
    {
        std::string capStr = (vaginaCapacity >= 0 && vaginaCapacity < 5) ? s_sizes5[vaginaCapacity] : "average-sized";
        std::string labStr = (labiaSize >= 0 && labiaSize < 5) ? s_sizes5[labiaSize] : "average-sized";
        std::string clitStr = (clitorisSize >= 0 && clitorisSize < 5) ? s_sizes5[clitorisSize] : "tiny";
        desc += std::format("You have a human vagina, with {} labia, a {} clit, and {} capacity.\n\n",
                             labStr, clitStr, capStr);
    }
    else
    {
        std::string testStr = (testicleSize >= 0 && testicleSize < 5) ? s_sizes5[testicleSize] : "average-sized";
        desc += std::format("You have a {:.1f}cm human penis and {} testicles.\n\n", penisLengthCm, testStr);
    }

    return desc;
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
        if (gender == "Female" || femininity == "Feminine" || femininity == "Very Feminine") chosenName = feminineName;
        else if (femininity == "Androgynous") chosenName = androgynousName;

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
        breasts.isLactating = (lactationTier > 0 || isLactating);
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

        // Transfer Cosmetics
        player->cosmetics["blusher"] = blusher;
        player->cosmetics["lipstick"] = lipstick;
        player->cosmetics["eyeliner"] = eyeliner;
        player->cosmetics["eyeshadow"] = eyeshadow;
        player->cosmetics["nailPolish"] = nailPolish;
        player->cosmetics["toenailPolish"] = toenailPolish;

        // Transfer Piercings
        player->piercings = piercings;

        // Transfer Tattoos
        player->tattoos = tattoos;

        // Transfer Body Hair
        player->bodyHair["colour"] = bodyHairColour;
        player->bodyHair["facial"] = facialHair;
        player->bodyHair["pubic"] = pubicHair;
        player->bodyHair["underarm"] = underarmHair;
        player->bodyHair["ass"] = assHair;

        // Transfer Personality & Occupation
        for (const auto& tr : personalityTraits) player->personalityTraits.push_back(tr);
        player->startingOccupation = startingOccupation;

        // Starting core stats & currency
        player->stats.setBaseStat("currency", 5000.0f);
        player->stats.setBaseStat("health", 40.0f);
        player->stats.setBaseStat("mana", 108.0f);
        player->stats.setBaseStat("lust", 0.0f);
        player->stats.setBaseStat("arcaneEssence", 20.0f);

        // Transfer custom wardrobe choices
        if (!wardrobeInitialized) initializeWardrobe();
        for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
        {
            if (wardrobeEquipped[s])
            {
                player->inventory.addItem(wardrobeEquipped[s]);
                player->inventory.equipItem(player->inventory.backpack.size() - 1, static_cast<equipSlot>(s));
            }
        }
        for (const auto& it : availableWardrobe)
        {
            if (it) player->inventory.addItem(it);
        }

        auto pendant = itemDatabase::getItem("item_golden_pendant");
        if (pendant) player->inventory.addItem(pendant);
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
