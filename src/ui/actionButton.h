#pragma once

#include <functional>
#include <string>

struct actionButton
{
    std::string label;
    int slotIndex = -1;
    bool pinnedAllPages = false;
    bool isEnabled = true;
    std::function<void()> onClick = nullptr;
};