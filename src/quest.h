#pragma once

#include "conditionNode.h"
#include <string>
#include <vector>

// Effect struct applied when making a dialogue choice
struct gameEffect {
    std::string action; // e.g. "ADD_GOLD", "SET_FLAG"
    std::string target;
    int amount{0};
};

struct dialogueChoice {
    std::string text;
    std::string label;       // <--- Added label
    std::string nextSceneId; // <--- Added nextSceneId
    int nextNodeId{-1};
    std::vector<conditionNode> requirements;
    std::vector<gameEffect> results; // <--- Added results
};

// Scene struct for dialogue / quest nodes
struct questScene {
    std::string id;
    std::string speakerName;
    std::string bodyText;
    std::vector<dialogueChoice> choices;
};

// Trigger placed on map tiles
struct MapTrigger {
    std::string id;       // <--- Added id
    std::string mapId;    // <--- Added mapId
    std::string triggerId;
    std::string label;    // <--- Added label
    std::string sceneId;  // <--- Added sceneId
    int x{0};             // <--- Added x
    int y{0};             // <--- Added y
    std::vector<conditionNode> conditions;
};

struct QuestStep {
    int stepId;
    std::string description;
    conditionNode completionCondition;
};

struct Quest {
    std::string id;
    std::string title;
    std::vector<QuestStep> steps;
};