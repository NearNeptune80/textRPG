#pragma once
#include <string>
#include <vector>

struct gameEffect
{
    std::string action;   // e.g., "GIVE_ITEM", "REMOVE_ITEM", "ADD_STAT", "TELEPORT", "SET_QUEST"
    std::string target;   // e.g., "item_canis_root", "experience", "x_4_y_4", "root_delivery"
    int amount;           // e.g., 1, 250, 0
};

struct gameCondition
{
    std::string type;     // e.g., "HAS_ITEM", "QUEST_STAGE"
    std::string target;   // e.g., "item_canis_root", "root_delivery"
    int requiredValue;    // e.g., 1
};

struct dialogueChoice
{
    std::string label;
    std::vector<gameCondition> requirements;
    std::vector<gameEffect> results;
    std::string nextSceneId; // "EXIT" to close
};

struct questScene
{
    std::string id;
    std::string speakerName;
    std::string bodyText;
    std::vector<dialogueChoice> choices;
};