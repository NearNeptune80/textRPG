#pragma once
#include <string>

struct actionButton
{
    std::string label;
    std::string command; // e.g., "SCENE_CHOICE", "START_SCENE"
    std::string payload; // Choice index or scene ID
};