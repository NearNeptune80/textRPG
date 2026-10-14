#include "items/fluidHarvesting.h"
#include "entities/entity.h"
#include "items/itemDatabase.h"
#include <algorithm>

HarvestResult fluidHarvesting::drainAndBottle(entity* target, bodySlot slot, float maxAmount, inventoryComponent* destinationInv)
{
    HarvestResult result;
    if (!target) return result;

    bodyPart* part = target->anatomy.getPart(slot);
    if (!part || part->currentFluidMl <= 0.0f) return result;

    inventoryComponent* inv = destinationInv ? destinationInv : &target->inventory;

    if (slot == bodySlot::BREASTS)
    {
        result.itemId = "item_milk_bottle";
        float volumeToDrain = (maxAmount > 0.0f) ? std::min(part->currentFluidMl, maxAmount) : part->currentFluidMl;
        result.bottlesHarvested = static_cast<int>(volumeToDrain / 50.0f);
        result.fluidMlDrained = target->anatomy.drainFluid(bodySlot::BREASTS, volumeToDrain);

        for (int i = 0; i < result.bottlesHarvested; ++i)
        {
            auto bottle = itemDatabase::getItem("item_milk_bottle");
            if (bottle && inv) inv->addItem(bottle);
        }
    }
    else if (slot == bodySlot::GROIN)
    {
        bool isMale = part->hasTag("penis") || (part->name.find("Penis") != std::string::npos);
        result.itemId = isMale ? "item_cum_flask" : "item_girlcum_vial";
        float volumeToDrain = (maxAmount > 0.0f) ? std::min(part->currentFluidMl, maxAmount) : part->currentFluidMl;
        result.bottlesHarvested = static_cast<int>(volumeToDrain / 15.0f);
        result.fluidMlDrained = target->anatomy.drainFluid(bodySlot::GROIN, volumeToDrain);

        for (int i = 0; i < result.bottlesHarvested; ++i)
        {
            auto bottle = itemDatabase::getItem(result.itemId);
            if (bottle && inv) inv->addItem(bottle);
        }
    }

    return result;
}

std::vector<HarvestResult> fluidHarvesting::harvestAllFluids(entity* target, inventoryComponent* destinationInv)
{
    std::vector<HarvestResult> results;
    if (!target) return results;

    auto bRes = drainAndBottle(target, bodySlot::BREASTS, -1.0f, destinationInv);
    if (bRes.bottlesHarvested > 0 || bRes.fluidMlDrained > 0.0f)
    {
        results.push_back(bRes);
    }

    auto gRes = drainAndBottle(target, bodySlot::GROIN, -1.0f, destinationInv);
    if (gRes.bottlesHarvested > 0 || gRes.fluidMlDrained > 0.0f)
    {
        results.push_back(gRes);
    }

    return results;
}

