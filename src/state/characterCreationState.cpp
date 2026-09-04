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
    case EditorTabId::WARDROBE: return "Clothing";
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

void characterCreationState::syncPreviewEntity(game* gameContext)
{
    if (!gameContext) return;
    if (!gameContext->playerEntity)
    {
        gameContext->playerEntity = std::make_shared<entity>("player_main", "Hero");
        gameContext->Player = gameContext->playerEntity.get();
        gameContext->Player->stats.setBaseStat("currency", 5000.0f);
        gameContext->Player->stats.setBaseStat("health", 100.0f);
        gameContext->Player->stats.setBaseStat("max_health", 100.0f);
        gameContext->Player->stats.setBaseStat("mana", 100.0f);
        gameContext->Player->stats.setBaseStat("max_mana", 100.0f);
        gameContext->Player->stats.setBaseStat("lust", 0.0f);
        gameContext->Player->stats.setBaseStat("max_lust", 100.0f);
        gameContext->Player->stats.setBaseStat("arousal", 0.0f);
        gameContext->Player->stats.setBaseStat("max_arousal", 100.0f);
        gameContext->Player->stats.setBaseStat("arcaneEssence", 20.0f);
        gameContext->Player->stats.setBaseStat("physique", 10.0f);
        gameContext->Player->stats.setBaseStat("agility", 10.0f);
        gameContext->Player->stats.setBaseStat("arcane", 10.0f);
    }

    entity* p = gameContext->getPlayer();
    if (!p) return;

    applyToEntity(p);

    if (!wardrobeInitialized) initializeWardrobe();
    for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
    {
        wardrobeEquipped[s] = p->inventory.equipped[s];
    }
}

void characterCreationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        syncPreviewEntity(gameContext);
        gameContext->refreshActionGrid();
    }
}

void characterCreationState::onExit(game* gameContext)
{
}

void characterCreationState::update(game* gameContext, float deltaTime)
{
}

void characterCreationState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::TEXT_INPUT)
    {
        std::string text = cmd.stringPayload;
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
    else if (cmd.type == CommandType::TEXT_BACKSPACE)
    {
        if (activeNameField == 0 && !masculineName.empty()) masculineName.pop_back();
        else if (activeNameField == 1 && !androgynousName.empty()) androgynousName.pop_back();
        else if (activeNameField == 2 && !feminineName.empty()) feminineName.pop_back();
        else if (activeNameField == 3 && !surname.empty()) surname.pop_back();
    }
    else if (cmd.type == CommandType::SELECT_TAB)
    {
        if (cmd.intPayload1 >= 0)
        {
            step = cmd.intPayload1;
        }
        else
        {
            activeNameField = (activeNameField + 1) % 4;
        }
    }
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

    static const char* allItemIds[] = {
        "item_linen_shirt",
        "item_formal_suit",
        "item_evening_dress",
        "item_leather_trousers",
        "item_leather_skirt",
        "item_cotton_boxers",
        "item_silk_panties",
        "item_silk_bra",
        "item_dress_shoes",
        "item_leather_boots",
        "item_high_heels",
        "item_leather_choker",
        "item_cloth_gloves"
    };

    for (const char* id : allItemIds)
    {
        auto it = itemDatabase::getItem(id);
        if (it) availableWardrobe.push_back(it);
    }
}

void characterCreationState::rebuildAvailableWardrobe()
{
    availableWardrobe.clear();
    static const char* allItemIds[] = {
        "item_linen_shirt", "item_leather_trousers", "item_leather_boots",
        "item_formal_suit", "item_evening_dress", "item_leather_skirt",
        "item_cotton_boxers", "item_silk_panties", "item_silk_bra",
        "item_high_heels", "item_dress_shoes", "item_leather_choker", "item_cloth_gloves"
    };
    for (const char* id : allItemIds)
    {
        auto it = itemDatabase::getItem(id);
        if (!it) continue;
        bool isEquipped = false;
        for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
        {
            if (wardrobeEquipped[s] && wardrobeEquipped[s]->id == it->id)
            {
                isEquipped = true;
                break;
            }
        }
        if (!isEquipped)
        {
            availableWardrobe.push_back(it);
        }
    }
}

std::string characterCreationState::getEquippedItemName(equipSlot slot) const
{
    if (slot == equipSlot::NONE) return "None";
    size_t s = static_cast<size_t>(slot);
    if (s < EQUIP_SLOT_COUNT && wardrobeEquipped[s])
    {
        return wardrobeEquipped[s]->name;
    }
    return "None";
}

void characterCreationState::applyWardrobePreset(const std::string& presetName)
{
    if (!wardrobeInitialized) initializeWardrobe();
    for (auto& s : wardrobeEquipped) s = nullptr;

    if (presetName == "Formal Suit")
    {
        wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] = itemDatabase::getItem("item_formal_suit");
        wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = itemDatabase::getItem("item_linen_shirt");
        wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = itemDatabase::getItem("item_leather_trousers");
        wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_cotton_boxers");
        wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = itemDatabase::getItem("item_dress_shoes");
    }
    else if (presetName == "Evening Dress")
    {
        wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] = itemDatabase::getItem("item_evening_dress");
        wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_silk_panties");
        wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] = itemDatabase::getItem("item_silk_bra");
        wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = itemDatabase::getItem("item_high_heels");
    }
    else if (presetName == "Smart Casual")
    {
        wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = itemDatabase::getItem("item_linen_shirt");
        wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = itemDatabase::getItem("item_leather_trousers");
        wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_cotton_boxers");
        wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = itemDatabase::getItem("item_leather_boots");
    }
    else if (presetName == "Pleated Skirt & Shirt")
    {
        wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = itemDatabase::getItem("item_linen_shirt");
        wardrobeEquipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = itemDatabase::getItem("item_leather_skirt");
        wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_silk_panties");
        wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] = itemDatabase::getItem("item_silk_bra");
        wardrobeEquipped[static_cast<size_t>(equipSlot::FEET)] = itemDatabase::getItem("item_high_heels");
    }
    else if (presetName == "Undergarments")
    {
        bool isFem = (gender == "Female" || femininity == "Feminine" || femininity == "Very Feminine");
        if (isFem)
        {
            wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_silk_panties");
            wardrobeEquipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] = itemDatabase::getItem("item_silk_bra");
        }
        else
        {
            wardrobeEquipped[static_cast<size_t>(equipSlot::GROIN_OVER)] = itemDatabase::getItem("item_cotton_boxers");
        }
    }
    // "Nude / Exposed" leaves all nullptr

    rebuildAvailableWardrobe();
}

bool characterCreationState::equipWardrobeItem(size_t wardrobeIndex)
{
    if (wardrobeIndex >= availableWardrobe.size()) return false;
    auto itemToEquip = availableWardrobe[wardrobeIndex];
    if (!itemToEquip || !itemToEquip->isEquippable) return false;

    equipSlot target = itemToEquip->targetSlot;
    if (target == equipSlot::NONE) return false;

    // Special handling for full-body evening dress
    if (itemToEquip->id == "item_evening_dress")
    {
        unequipWardrobeItem(equipSlot::TORSO_UNDER);
        unequipWardrobeItem(equipSlot::LEGS_OUTER);
    }
    else if (target == equipSlot::TORSO_UNDER || target == equipSlot::LEGS_OUTER)
    {
        if (wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)] &&
            wardrobeEquipped[static_cast<size_t>(equipSlot::TORSO_OVER)]->id == "item_evening_dress")
        {
            unequipWardrobeItem(equipSlot::TORSO_OVER);
        }
    }

    // Erase from available pile (locating by pointer in case unequip modified indices)
    auto it = std::find(availableWardrobe.begin(), availableWardrobe.end(), itemToEquip);
    if (it != availableWardrobe.end())
    {
        availableWardrobe.erase(it);
    }

    size_t slotIdx = static_cast<size_t>(target);
    auto existingItem = wardrobeEquipped[slotIdx];
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

void characterCreationState::applyToEntity(entity* player)
{
    if (!player) return;

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

    player->genderArchetype = (gender == "Female") ? GenderArchetype::FEMALE : GenderArchetype::MALE;
    player->orientation = (orientation == "Androphilic") ? SexualOrientation::HOMOSEXUAL : ((orientation == "Gynephilic") ? SexualOrientation::HETEROSEXUAL : SexualOrientation::BISEXUAL);

    player->stats.setBaseStat("appeared_age", static_cast<float>(birthAge));
    player->anatomy.heightMeters = static_cast<float>(heightCm) / 100.0f;
    player->anatomy.bodySize = bodySize;
    player->anatomy.muscleTone = muscleDefinition;

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
    head.style = facialHair;
    player->anatomy.setPart(bodySlot::HEAD, head);

    bodyPart eyes;
    eyes.id = "eyes_human";
    eyes.name = "Eyes";
    eyes.race = "Human";
    eyes.primaryColor = eyeColor;
    eyes.style = "Round";
    eyes.secondaryColor = "Pure White";
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
    mouth.orifice.wetnessLevel = 2;
    mouth.style = (lipSize >= 0 && lipSize < static_cast<int>(EditorConfig::Catalogs::lipSizes().size())) ? EditorConfig::Catalogs::lipSizes()[lipSize] : "Average";
    if (puffyLips) mouth.tags.push_back("puffy_lips");
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
    breasts.style = breastShape;
    breasts.primaryColor = skinPrimaryColor;
    breasts.isLactating = (lactationTier > 0 || isLactating);
    breasts.maxFluidMl = (breasts.isLactating && milkCapacityMl <= 0.0f) ? 1000.0f : milkCapacityMl;
    breasts.currentFluidMl = breasts.isLactating ? breasts.maxFluidMl * 0.5f : 0.0f;
    player->anatomy.setPart(bodySlot::BREASTS, breasts);

    bodyPart nipples;
    nipples.id = "nipples_human";
    nipples.name = "Nipples";
    nipples.race = "Human";
    nipples.primaryColor = skinPrimaryColor;
    nipples.style = (nippleSize >= 0 && nippleSize < static_cast<int>(EditorConfig::Catalogs::nippleShapes().size())) ? EditorConfig::Catalogs::nippleShapes()[nippleSize] : "Normal";
    nipples.secondaryColor = (areolaeSize >= 0 && areolaeSize < static_cast<int>(EditorConfig::Catalogs::areolaeShapes().size())) ? EditorConfig::Catalogs::areolaeShapes()[areolaeSize] : "Round";
    if (puffyNipples) nipples.tags.push_back("puffy_nipples");
    player->anatomy.setPart(bodySlot::NIPPLES, nipples);

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
    hips.style = (hipSize >= 0 && hipSize < static_cast<int>(EditorConfig::Catalogs::size5().size())) ? EditorConfig::Catalogs::size5()[hipSize] : "Average";
    player->anatomy.setPart(bodySlot::HIPS, hips);

    bodyPart ass;
    ass.id = "ass_human";
    ass.name = "Ass";
    ass.race = "Human";
    ass.primaryColor = skinPrimaryColor;
    ass.style = (assSize >= 0 && assSize < static_cast<int>(EditorConfig::Catalogs::size5().size())) ? EditorConfig::Catalogs::size5()[assSize] : "Average";
    ass.orifice.exists = true;
    ass.orifice.elasticity = anusElasticity;
    ass.orifice.maxCapacityMl = 80.0f;
    ass.orifice.depthCm = 18.0f;
    if (anusBleached) ass.tags.push_back("bleached");
    player->anatomy.setPart(bodySlot::ASS, ass);

    // Genitals
    if (gender == "Female")
    {
        bodyPart vagina;
        vagina.id = "vagina_human";
        vagina.name = "Vagina";
        vagina.race = "Human";
        vagina.tags.push_back("vagina");
        vagina.tags.push_back("has_vagina");
        vagina.tags.push_back("female_genitalia");
        vagina.orifice.exists = true;
        vagina.orifice.elasticity = vaginaElasticity;
        vagina.orifice.wetnessLevel = vaginaWetness;
        vagina.orifice.maxCapacityMl = 120.0f;
        vagina.orifice.depthCm = 14.0f + (vaginaCapacity * 2.0f);
        vagina.style = (labiaSize >= 0 && labiaSize < static_cast<int>(EditorConfig::Catalogs::size5().size())) ? EditorConfig::Catalogs::size5()[labiaSize] : "Average";
        vagina.secondaryColor = (clitorisSize == 0) ? "Tiny" : ((clitorisSize < static_cast<int>(EditorConfig::Catalogs::size5().size())) ? EditorConfig::Catalogs::size5()[clitorisSize] : "Average");
        if (isVirgin) vagina.tags.push_back("virgin_hymen");
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
        penis.count = 2;
        penis.secondaryColor = (testicleSize >= 0 && testicleSize < static_cast<int>(EditorConfig::Catalogs::size5().size())) ? EditorConfig::Catalogs::size5()[testicleSize] : "Average";
        penis.tags.push_back("penis");
        penis.tags.push_back("has_penis");
        player->anatomy.setPart(bodySlot::GROIN, penis);
    }

    bodyPart legs;
    legs.id = "legs_human";
    legs.name = "Legs";
    legs.race = "Human";
    legs.primaryColor = skinPrimaryColor;
    legs.style = "Bipedal";
    legs.covering = stringToCoveringType(skinCovering);
    player->anatomy.setPart(bodySlot::LEGS, legs);

    bodyPart feet;
    feet.id = "feet_human";
    feet.name = "Feet";
    feet.race = "Human";
    feet.primaryColor = skinPrimaryColor;
    feet.style = "Plantigrade";
    feet.covering = stringToCoveringType(skinCovering);
    player->anatomy.setPart(bodySlot::FEET, feet);

    // Appendages
    if (hornsType != "None")
    {
        bodyPart horns;
        horns.id = "horns_" + hornsType;
        horns.name = "Horns";
        horns.race = hornsType;
        horns.count = 2;
        horns.length = 15.0f;
        player->anatomy.setPart(bodySlot::HORNS, horns);
    }
    else
    {
        player->anatomy.removePart(bodySlot::HORNS);
    }

    if (wingsType != "None")
    {
        bodyPart wings;
        wings.id = "wings_" + wingsType;
        wings.name = "Wings";
        wings.race = wingsType;
        wings.count = 2;
        wings.style = "Average";
        player->anatomy.setPart(bodySlot::WINGS, wings);
    }
    else
    {
        player->anatomy.removePart(bodySlot::WINGS);
    }

    if (tailsType != "None")
    {
        bodyPart tail;
        tail.id = "tail_" + tailsType;
        tail.name = "Tail";
        tail.race = tailsType;
        tail.count = tailsCount;
        tail.length = 60.0f;
        player->anatomy.setPart(bodySlot::TAIL, tail);
    }
    else
    {
        player->anatomy.removePart(bodySlot::TAIL);
    }

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
    player->personalityTraits.clear();
    for (const auto& tr : personalityTraits) player->personalityTraits.push_back(tr);
    player->startingOccupation = startingOccupation;

    // Starting core stats & currency
    player->stats.setBaseStat("currency", 5000.0f);
    player->stats.setBaseStat("health", 100.0f);
    player->stats.setBaseStat("max_health", 100.0f);
    player->stats.setBaseStat("mana", 100.0f);
    player->stats.setBaseStat("max_mana", 100.0f);
    player->stats.setBaseStat("lust", 0.0f);
    player->stats.setBaseStat("max_lust", 100.0f);
    player->stats.setBaseStat("arousal", 0.0f);
    player->stats.setBaseStat("max_arousal", 100.0f);
    player->stats.setBaseStat("arcaneEssence", 20.0f);
    player->stats.setBaseStat("physique", 10.0f);
    player->stats.setBaseStat("agility", 10.0f);
    player->stats.setBaseStat("arcane", 10.0f);
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
        applyToEntity(player);

        // Clear all ground clothes
        availableWardrobe.clear();

        // Strip any unequipped clothes from backpack to prevent selling boost exploitation
        std::erase_if(player->inventory.backpack, [](const auto& it) {
            return it && it->isEquippable;
        });

        // Ensure player retains their starting quest pendant
        auto pendant = itemDatabase::getItem("item_golden_pendant");
        bool hasPendant = false;
        for (const auto& it : player->inventory.backpack)
        {
            if (it && it->id == "item_golden_pendant") { hasPendant = true; break; }
        }
        if (pendant && !hasPendant) player->inventory.addItem(pendant);

        // Grant starting quests (Main Quest + Side Quests)
        if (!player->quests.hasQuest("root_delivery"))
        {
            player->quests.setQuestStage("root_delivery", 0);
            player->quests.setTrackedQuest("root_delivery");
        }
        if (!player->quests.hasQuest("cottage_investigation"))
        {
            player->quests.setQuestStage("cottage_investigation", 0);
        }
        if (!player->quests.hasQuest("patrol_duty"))
        {
            player->quests.setQuestStage("patrol_duty", 0);
        }
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
