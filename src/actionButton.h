#pragma once
#include <string>
#include <functional>

struct actionButton
{
    std::string label;
    std::function<void()> onClick = nullptr;
};