#pragma once
#include <string>
#include <vector>

/**
 * Mutative effect resulting from dialogue choices or quest events.
 */
struct gameEffect
{
    std::string action; // "GIVE_ITEM", "REMOVE_ITEM", "ADD_STAT", "SET_QUEST", "TELEPORT_MAP"
    std::string target; // Identifier or parameter string
    int amount = 0;
    int extraX = 0;
    int extraY = 0;
};

/**
 * Prerequisite condition check for choices, triggers, or map warps.
 */
struct gameCondition
{
    std::string type;   // "HAS_ITEM", "QUEST_STAGE", "TIME_PHASE", "STAT_MIN", "HAS_TAG"
    std::string target;
    int requiredValue = 0;
};

/**
 * Individual choice option presented within a dialogue scene.
 */
struct dialogueChoice
{
    std::string label;
    std::vector<gameCondition> requirements;
    std::vector<gameEffect> results;
    std::string nextSceneId; // "EXIT" or target scene ID
};

/**
 * Full narrative scene block containing dialogue text and choices.
 */
struct questScene
{
    std::string id;
    std::string speakerName;
    std::string bodyText;
    std::vector<dialogueChoice> choices;
};

/**
 * Dynamic trigger bound to a map location.
 */
struct MapTrigger
{
    std::string id;
    std::string mapId;
    int x = 0;
    int y = 0;
    std::string label;
    std::string sceneId;
    std::vector<gameCondition> conditions;
};