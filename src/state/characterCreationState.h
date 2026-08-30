#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <SDL3/SDL.h>

#include "state/iGameState.h"
#include "state/editorConfig.h"

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
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;

    // Granular Configuration
    EditorConfig config = EditorConfig::newGamePreset();

    // Tab Navigation Helpers
    std::vector<EditorTabId> getActiveTabs() const;
    int getActiveTabCount() const;
    EditorTabId getCurrentTabId() const;
    std::string getTabName(EditorTabId tab) const;

    // Current active step index into getActiveTabs()
    int step = 0;

    // Step 0 properties
    std::string startMonth = "August";
    int startMonthIdx = 7; // August
    std::string gender = "Male"; // Male or Female
    std::string femininity = "Masculine"; // Androgynous, Masculine, Very Masculine

    // Step 1 properties
    int birthDay = 29;
    std::string birthMonth = "August";
    int birthMonthIdx = 7;
    int birthAge = 22;
    std::string orientation = "Ambiphilic"; // Androphilic, Ambiphilic, Gynephilic
    std::set<std::string> personalityTraits;

    // Step 2 properties
    std::string masculineName = "Rudy";
    std::string androgynousName = "Rudy";
    std::string feminineName = "Rudy";
    std::string surname = "";
    int activeNameField = 0; // 0 = Masc, 1 = Andro, 2 = Fem, 3 = Surname

    // Step 3 (Customization Overview & Sub-views) properties
    int subView = 0; // 0 = Overview ("In the Museum"), 1 = Core, 2 = Face, 3 = Hair, 4 = Breasts, 5 = Ass/Hips, 6 = Genitals, 7 = Makeup, 8 = Piercings, 9 = Tattoos, 10 = Extra Hair
    int heightCm = 180;
    std::string skinPattern = "Plain"; // Plain, Freckled (face), Freckled
    std::string skinCovering = "Skin"; // Skin, Fur, Scales, Feathers
    std::string skinPrimaryColor = "Fair"; // Fair, Pale, Tan, Olive, Dark, Ebony, Pale Blue, Green, Lilac, Deep Red
    int skinColorIdx = 0;
    std::string bodySize = "Average"; // Skinny, Slender, Average, Muscular, Chubby
    std::string muscleDefinition = "Lightly muscled"; // Soft, Lightly muscled, Toned, Muscular, Ripped
    
    // Face & Head
    std::string eyeColor = "Blue"; // Blue, Green, Brown, Amber, Hazel, Red, Violet, Black
    int lipSize = 1; // 0 = Thin, 1 = Average, 2 = Full, 3 = Plump
    std::string earType = "Human"; // Human, Cat, Dog, Elf, Demon
    
    // Hair
    int hairLengthCm = 15;
    std::string hairStyle = "Short"; // Short, Bob, Shoulder-length, Long, Braided, Ponytail, Messy
    std::string hairColor = "Brown"; // Black, Dark Brown, Auburn, Blonde, Platinum, Silver, Red, Blue, Pink, Purple
    
    // Breasts
    int breastCupSize = 0; // 0 = Flat, 1 = A, 2 = B, 3 = C, 4 = D, 5 = DD, 6 = E, 7 = F, 8 = G, 9 = H
    int nippleSize = 1; // 0 = Small, 1 = Normal, 2 = Puffy, 3 = Large
    bool isLactating = false;
    float milkCapacityMl = 0.0f;
    
    // Ass & Hips
    int hipSize = 2; // 0 = Very narrow, 1 = Narrow, 2 = Average, 3 = Wide, 4 = Very wide
    int assSize = 2; // 0 = Flat, 1 = Small, 2 = Average, 3 = Plump, 4 = Enormous
    float anusElasticity = 60.0f;
    
    // Genitals
    float penisLengthCm = 16.0f;
    float penisDiameterCm = 3.8f;
    float cumCapacityMl = 20.0f;
    int testicleSize = 2; // 1 = Small, 2 = Average, 3 = Large, 4 = Enormous
    float clitSizeCm = 1.0f;
    int labiaSize = 1;
    int vaginaWetness = 2;
    float vaginaElasticity = 70.0f;
    bool isVirgin = true;

    // Appendages
    std::string hornsType = "None"; // None, Demon, Dragon, Goat, Bull, Unicorn
    std::string wingsType = "None"; // None, Feathered, Leathery, Insectoid, Energy
    std::string tailsType = "None"; // None, Cat, Dog, Fox, Horse, Demon, Dragon
    int tailsCount = 1;

    // Markings & Extra
    std::string makeupStyle = "None"; // None, Subtle, Glamour, Goth, Festive
    bool hasEarPiercings = false;
    bool hasNosePiercing = false;
    bool hasNavelPiercing = false;
    bool hasNipplePiercings = false;
    std::string tattooLocation = "None"; // None, Back, Chest, Arm, Thigh, Lower Back
    std::string pubicHair = "Natural"; // Hairless, Trimmed, Natural, Bushy
    std::string underarmHair = "Hairless"; // Hairless, Stubble, Natural

    // Step 4 (Evening's Attire) properties
    int activeBagSlot = 0;
    int wardrobePage = 1;

    void randomizeFirstNames();
    void randomizeSurname();
    void randomizeAll();
    void finalizeCharacter(game* gameContext);
};
