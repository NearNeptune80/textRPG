#pragma once

#include <array>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/enums.h"
#include "items/item.h"
#include "state/iGameState.h"
#include "state/editorConfig.h"

class entity;

class characterCreationState : public iGameState
{
public:
    explicit characterCreationState(int startStep = 0);
    characterCreationState(EditorConfig cfg, int startStep = 0);
    ~characterCreationState() override = default;

    void initialise(game* gameContext) override;
    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;
    void update(game* gameContext, float deltaTime) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void syncPreviewEntity(game* gameContext);

    EditorConfig config = EditorConfig::newGamePreset();

    std::vector<EditorTabId> getActiveTabs() const;
    int getActiveTabCount() const;
    EditorTabId getCurrentTabId() const;
    std::string getTabName(EditorTabId tab) const;

    int step = 0;

    std::string startMonth = "August";
    int startMonthIdx = 7;
    std::string gender = "Male";;
    std::string femininity = "Masculine";;

    int birthDay = 29;
    std::string birthMonth = "August";
    int birthMonthIdx = 7;
    int birthAge = 22;
    std::string orientation = "Ambiphilic";;
    std::set<std::string> personalityTraits;

    std::string masculineName = "Rudy";
    std::string androgynousName = "Rudy";
    std::string feminineName = "Rudy";
    std::string surname = "";
    int activeNameField = 0; // 0 = Masc, 1 = Andro, 2 = Fem, 3 = Surname

    int subView = 0; // 0 = Overview ("In the Museum"), 1 = Core, 2 = Face, 3 = Hair, 4 = Breasts, 5 = Ass/Hips, 6 = Genitals, 7 = Makeup, 8 = Piercings, 9 = Tattoos, 10 = Extra Hair
    int heightCm = 180;
    std::string skinPattern = "Plain";;
    std::string skinCovering = "Skin";;
    std::string skinPrimaryColor = "Fair";;
    int skinColorIdx = 0;
    std::string bodySize = "Average";;
    std::string muscleDefinition = "Lightly muscled"; // Soft, Lightly muscled, Toned, Muscular, Ripped
    
    std::string eyeColor = "Blue"; // Blue, Green, Brown, Amber, Hazel, Red, Violet, Black
    int lipSize = 1; // 0 = Thin, 1 = Average, 2 = Full, 3 = Plump
    std::string earType = "Human"; // Human, Cat, Dog, Elf, Demon
    
    int hairLengthCm = 15;
    std::string hairStyle = "Short"; // Short, Bob, Shoulder-length, Long, Braided, Ponytail, Messy
    std::string hairColor = "Brown"; // Black, Dark Brown, Auburn, Blonde, Platinum, Silver, Red, Blue, Pink, Purple
    
    int breastCupSize = 0; // 0 = Flat, 1 = A, 2 = B, 3 = C, 4 = D, 5 = DD, 6 = E, 7 = F, 8 = G, 9 = H
    std::string breastShape = "Round"; // Round, Pointy, Perky, Wide, Side-set, Narrow
    int nippleSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int areolaeSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    bool puffyNipples = false;
    bool puffyLips = false;
    int lactationTier = 0; // 0 = None .. 7 = Monstrous
    bool isLactating = false;
    float milkCapacityMl = 0.0f;
    
    int hipSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int assSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    bool anusBleached = false;
    float anusElasticity = 60.0f;
    
    float penisLengthCm = 16.0f;
    float penisDiameterCm = 3.8f;
    float cumCapacityMl = 20.0f;
    int testicleSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int cumProductionTier = 3; // 0 = None .. 7 = Monstrous
    int vaginaCapacity = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int labiaSize = 2; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int clitorisSize = 0; // 0 = Tiny, 1 = Small, 2 = Average-sized, 3 = Large, 4 = Huge
    int vaginaWetness = 2;
    float vaginaElasticity = 70.0f;
    bool isVirgin = true;

    std::string hornsType = "None"; // None, Demon, Dragon, Goat, Bull, Unicorn
    std::string wingsType = "None";;
    std::string tailsType = "None"; // None, Cat, Dog, Fox, Horse, Demon, Dragon
    int tailsCount = 1;

    std::string blusher = "none";
    std::string lipstick = "none";
    std::string eyeliner = "none";
    std::string eyeshadow = "none";
    std::string nailPolish = "none";
    std::string toenailPolish = "none";

    std::unordered_map<std::string, bool> piercings = {
        {"ear", false}, {"nose", false}, {"lip", false}, {"tongue", false},
        {"navel", false}, {"nipple", false}, {"vagina", false}, {"penis", false}
    };

    std::unordered_map<std::string, std::string> tattoos;

    std::string bodyHairColour = "brown";
    std::string facialHair = "none"; // none, stubble, short beard, full beard, goatee, moustache
    std::string pubicHair = "natural"; // none, stubble, manicured, trimmed, natural, unkempt, bushy, wild
    std::string underarmHair = "none"; // none, stubble, manicured, trimmed, natural, unkempt, bushy, wild
    std::string assHair = "none"; // none, stubble, manicured, trimmed, natural, unkempt, bushy, wild

    std::string startingOccupation = "Student"; // Student, Office Worker, Athlete, Bartender, Artist, Unemployed
    bool skipPrologue = false;

    std::string generateAppearanceDescription() const;

    std::vector<std::shared_ptr<item>> availableWardrobe;
    std::array<std::shared_ptr<item>, EQUIP_SLOT_COUNT> wardrobeEquipped{};
    bool wardrobeInitialized = false;

    void initializeWardrobe();
    void rebuildAvailableWardrobe();
    void applyWardrobePreset(const std::string& presetName);
    std::string getEquippedItemName(equipSlot slot) const;
    bool equipWardrobeItem(size_t wardrobeIndex);
    bool unequipWardrobeItem(equipSlot slot);
    bool isClothedEnough() const;
    std::string getDecencyStatus() const;

    void randomizeFirstNames();
    void randomizeSurname();
    void randomizeAll();
    void applyToEntity(entity* player);
    void finalizeCharacter(game* gameContext);
};
