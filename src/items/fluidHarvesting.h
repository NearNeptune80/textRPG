#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "common/enums.h"

class entity;
class inventoryComponent;
struct bodyPart;

struct FluidConversionRule
{
    std::string id;
    std::string name;
    std::optional<bodySlot> slot;
    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;
    float mlPerUnit = 50.0f;
    std::string itemId = "item_milk_bottle";
    bool requiresLactating = false;
};

struct HarvestResult
{
    int bottlesHarvested = 0;
    std::string itemId = "";
    float fluidMlDrained = 0.0f;
};

class fluidHarvesting
{
public:
    static bool loadConfigFromFile(const std::string& path = "data/config/fluid_harvesting.json");
    static void clear();
    static bool isConfigLoaded();
    static const std::vector<FluidConversionRule>& getConversionRules();
    static const FluidConversionRule* findMatchingRule(bodySlot slot, const bodyPart* part);

    static HarvestResult drainAndBottle(entity* target, bodySlot slot, float maxAmount = -1.0f, inventoryComponent* destinationInv = nullptr);
    static std::vector<HarvestResult> harvestAllFluids(entity* target, inventoryComponent* destinationInv = nullptr);

private:
    static void ensureConfigLoaded();
    static std::vector<FluidConversionRule> s_conversionRules;
    static bool s_loaded;
};
