#pragma once
#include <string>

/**
 * Represents an actionable UI button definition bound to a dynamic command.
 */
struct actionButton
{
    std::string label;   // Display text on the button
    std::string command; // Command type (e.g., "SCENE_CHOICE", "START_SCENE", "MAP_WARP", "EQUIP_ITEM", "UNEQUIP_ITEM")
    std::string payload; // Context data payload associated with the command
};