#include "items/fluidHarvesting.h"
#include "entities/entity.h"
#include "items/itemDatabase.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::vector<FluidConversionRule> fluidHarvesting::s_conversionRules;
bool fluidHarvesting::s_loaded = false;

void fluidHarvesting::clear()
{
    s_conversionRules.clear();
    s_loaded = false;
}

bool fluidHarvesting::isConfigLoaded()
{
    return s_loaded && !s_conversionRules.empty();
}

const std::vector<FluidConversionRule>& fluidHarvesting::getConversionRules()
{
    ensureConfigLoaded();
    return s_conversionRules;
}

void fluidHarvesting::ensureConfigLoaded()
{
    if (!s_loaded || s_conversionRules.empty())
    {
        loadConfigFromFile("data/config/fluid_harvesting.json");
    }
}

bool fluidHarvesting::loadConfigFromFile(const std::string& path)
{
    std::string targetPath = path;
    if (!fs::exists(targetPath))
    {
        if (fs::exists("data/config/fluid_harvesting.json"))
        {
            targetPath = "data/config/fluid_harvesting.json";
        }
        else
        {
            return false;
        }
    }

    std::ifstream file(targetPath);
    if (!file.is_open()) return false;

    try
    {
        json j;
        file >> j;
        s_conversionRules.clear();

        if (j.contains("conversions") && j["conversions"].is_array())
        {
            for (const auto& item : j["conversions"])
            {
                FluidConversionRule rule;
                rule.id = item.value("id", "");
                rule.name = item.value("name", "");
                std::string slotStr = item.value("slot", "");
                rule.slot = stringToBodySlot(slotStr);
                rule.mlPerUnit = item.value("mlPerUnit", 50.0f);
                rule.itemId = item.value("itemId", "");
                rule.requiresLactating = item.value("requiresLactating", false);

                if (item.contains("requiredTags") && item["requiredTags"].is_array())
                {
                    rule.requiredTags = item["requiredTags"].get<std::vector<std::string>>();
                }
                if (item.contains("forbiddenTags") && item["forbiddenTags"].is_array())
                {
                    rule.forbiddenTags = item["forbiddenTags"].get<std::vector<std::string>>();
                }

                s_conversionRules.push_back(rule);
            }
        }

        s_loaded = true;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[fluidHarvesting] Error loading fluid conversion config: " << e.what() << "\n";
        return false;
    }
}

const FluidConversionRule* fluidHarvesting::findMatchingRule(bodySlot slot, const bodyPart* part)
{
    if (!part) return nullptr;
    ensureConfigLoaded();

    auto partHasIndicator = [&](const std::string& indicator) {
        if (part->hasTag(indicator)) return true;
        std::string lowerName = part->name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::string lowerInd = indicator;
        std::transform(lowerInd.begin(), lowerInd.end(), lowerInd.begin(), ::tolower);
        return lowerName.find(lowerInd) != std::string::npos;
    };

    for (const auto& rule : s_conversionRules)
    {
        if (!rule.slot.has_value() || rule.slot.value() != slot) continue;
        if (rule.requiresLactating && !part->isLactating) continue;

        bool forbiddenFound = false;
        for (const auto& fTag : rule.forbiddenTags)
        {
            if (partHasIndicator(fTag))
            {
                forbiddenFound = true;
                break;
            }
        }
        if (forbiddenFound) continue;

        bool requiredMissing = false;
        for (const auto& rTag : rule.requiredTags)
        {
            if (!partHasIndicator(rTag))
            {
                requiredMissing = true;
                break;
            }
        }
        if (requiredMissing) continue;

        return &rule;
    }

    return nullptr;
}

HarvestResult fluidHarvesting::drainAndBottle(entity* target, bodySlot slot, float maxAmount, inventoryComponent* destinationInv)
{
    HarvestResult result;
    if (!target) return result;

    bodyPart* part = target->anatomy.getPart(slot);
    if (!part || part->currentFluidMl <= 0.0f) return result;

    const FluidConversionRule* rule = findMatchingRule(slot, part);
    if (!rule || rule->mlPerUnit <= 0.0f || rule->itemId.empty()) return result;

    inventoryComponent* inv = destinationInv ? destinationInv : &target->inventory;

    result.itemId = rule->itemId;
    float volumeToDrain = (maxAmount > 0.0f) ? std::min(part->currentFluidMl, maxAmount) : part->currentFluidMl;
    result.bottlesHarvested = static_cast<int>(volumeToDrain / rule->mlPerUnit);
    result.fluidMlDrained = target->anatomy.drainFluid(slot, volumeToDrain);

    for (int i = 0; i < result.bottlesHarvested; ++i)
    {
        auto bottle = itemDatabase::getItem(rule->itemId);
        if (bottle && inv) inv->addItem(bottle);
    }

    return result;
}

std::vector<HarvestResult> fluidHarvesting::harvestAllFluids(entity* target, inventoryComponent* destinationInv)
{
    std::vector<HarvestResult> results;
    if (!target) return results;

    ensureConfigLoaded();

    std::vector<bodySlot> checkedSlots;
    for (const auto& rule : s_conversionRules)
    {
        if (rule.slot.has_value() && std::find(checkedSlots.begin(), checkedSlots.end(), rule.slot.value()) == checkedSlots.end())
        {
            checkedSlots.push_back(rule.slot.value());
            auto res = drainAndBottle(target, rule.slot.value(), -1.0f, destinationInv);
            if (res.bottlesHarvested > 0 || res.fluidMlDrained > 0.0f)
            {
                results.push_back(res);
            }
        }
    }

    return results;
}
