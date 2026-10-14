#pragma once

#include <string>
#include <vector>
#include <memory>
#include "common/enums.h"

class entity;
class inventoryComponent;

struct HarvestResult
{
    int bottlesHarvested = 0;
    std::string itemId = "";
    float fluidMlDrained = 0.0f;
};

class fluidHarvesting
{
public:
    static HarvestResult drainAndBottle(entity* target, bodySlot slot, float maxAmount = -1.0f, inventoryComponent* destinationInv = nullptr);
    static std::vector<HarvestResult> harvestAllFluids(entity* target, inventoryComponent* destinationInv = nullptr);
};

