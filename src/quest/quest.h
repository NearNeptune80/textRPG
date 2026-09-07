#pragma once

#include <string>
#include <vector>
#include <map>

#include "quest/conditionNode.h"

struct QuestDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::string category = "Main Quest";
    std::string giver = "Unknown";
    std::string location = "Unknown";
    std::string rewardsDescription = "";
    std::map<int, std::string> stages;
    int completionStage = 1;
};

struct gameEffect {
    std::string action;
    std::string target;
    int amount{0};
    int x{0};
    int y{0};
    float floatAmount{0.0f};
    std::string secondaryTarget;
    std::string stringVal;
    std::string extraString;
    std::vector<int> weights;
    std::vector<std::string> branches;
};

struct dialogueChoice {
    std::string label;
    std::string tooltip;
    std::string nextSceneId;
    std::vector<conditionNode> requirements;
    std::vector<gameEffect> results;
};

struct questScene {
    std::string id;
    std::string speakerName;
    std::string bodyText;
    std::vector<dialogueChoice> choices;
};

struct QuestNPCRelocation {
    std::string questId;
    std::string npcId;
    int minStage{ 0 };
    int maxStage{ 999 };
    std::string mapId;
    int x{ 0 };
    int y{ 0 };
    std::string activity;
    std::string overrideSceneId;
    std::vector<conditionNode> conditions;
};

struct MapTrigger {
    std::string id;
    std::string mapId;
    std::string npcId;
    std::string label;
    std::string tooltip;
    std::string description;
    std::string sceneId;
    int x{0};
    int y{0};
    std::vector<conditionNode> conditions;
};