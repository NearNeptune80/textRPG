#pragma once
#include <string>
#include <functional>

struct actionButton
{
    std::string label;
    int slotIndex = -1; // -1 = auto-flow to next open slot. 0..14 = pinned
    bool pinnedAllPages = false;
    bool isEnabled = true; // Set to false to grey out and disable clicks
    std::function<void()> onClick = nullptr;
};