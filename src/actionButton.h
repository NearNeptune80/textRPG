#pragma once
#include <string>

struct actionButton
{
    std::string label;

    // The broad category of the action (e.g., "DIALOGUE", "LOOT", "QUEST_EVENT")
    std::string command;

    // The specific target (e.g., "npc_maid", "item_sword", "quest_start_01")
    std::string payload;
};