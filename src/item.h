#pragma once
#include <string>
#include <vector>
#include "enums.h"      
#include "enchantment.h" 

struct item
{
    std::string id;
    std::string name;

    bool isConsumable;
    equipSlot targetSlot;
    std::string baseRace;

    std::vector<enchantment> enchantments;
};