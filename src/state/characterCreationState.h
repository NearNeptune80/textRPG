#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <SDL3/SDL.h>

#include "state/iGameState.h"

class characterCreationState : public iGameState
{
public:
    explicit characterCreationState(int startStep = 0);
    ~characterCreationState() override = default;

    void initialise(game* gameContext) override;
    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;
    void update(game* gameContext, float deltaTime) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;

    // Step: 0 = Start Date/Gender/Femininity, 1 = Birthday/Orientation/Personality, 2 = Names/Surname
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

    // Step 3 (Customization Overview / Core Body) properties
    int subView = 0; // 0 = Overview ("In the Museum"), 1 = Core Body Appearance, 2 = Face, etc.
    int heightCm = 180;
    std::string skinPattern = "Plain"; // Plain, Freckled (face), Freckled
    std::string skinPrimaryColor = "Light";
    int skinColorIdx = 3; // Light beige
    std::string bodySize = "Average"; // Skinny, Slender, Average, Large, Huge
    std::string muscleDefinition = "Lightly muscled"; // Soft, Lightly muscled, Toned, Muscular, Ripped
    std::string hairLength = "short";
    std::string hairColor = "brown";
    std::string eyeColor = "brown";

    // Step 4 (Evening's Attire) properties
    int activeBagSlot = 0;
    int wardrobePage = 1;

    void randomizeFirstNames();
    void randomizeSurname();
    void finalizeCharacter(game* gameContext);
};
