#pragma once
#include <string>
#include <vector>

struct gameEffect
{
    std::string action;
    // Supported: "GIVE_ITEM", "REMOVE_ITEM", "ADD_STAT", "SET_QUEST", 
    //            "TELEPORT_MAP", "MOVE_NPC", "TELEPORT_WITH_NPC"

    std::string target;   // e.g., "item_canis_root", "overworld,8,1", "npc_bandit_1"
    int amount = 0;       // e.g., 1, 250, 0
    int extraX = 0;       // Optional target X for NPC moves
    int extraY = 0;       // Optional target Y for NPC moves
};

struct gameCondition
{
    std::string type;
    // Supported: "HAS_ITEM", "QUEST_STAGE", "TIME_PHASE", "STAT_MIN", "HAS_TAG"

    std::string target;   // e.g., "root_delivery", "NIGHT", "physique", "digitigrade"
    int requiredValue = 0;
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

// Data structure for dynamic map triggers
struct MapTrigger
{
    std::string id;
    std::string mapId;  // e.g. "overworld"
    int x;
    int y;
    std::string label;  // e.g. "Talk to Stranger"
    std::string sceneId;
    std::vector<gameCondition> conditions;
};