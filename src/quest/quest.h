#pragma once

#include <string>
#include <vector>

#include "quest/conditionNode.h"

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

struct MapTrigger {
    std::string id;
    std::string mapId;
    std::string label;
    std::string tooltip;
    std::string sceneId;
    int x{0};
    int y{0};
    std::vector<conditionNode> conditions;
};