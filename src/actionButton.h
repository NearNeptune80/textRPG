#pragma once
#include <string>
#include <functional>

struct actionButton
{
    std::string label;
    int slotIndex = -1; // -1 = auto-flow to next open slot. 0..14 = pinned to specific slot on page
    bool pinnedAllPages = false; // If true, stays in this slot on ALL pages (e.g. Close Inventory at slot 14)
    std::function<void()> onClick = nullptr;
};