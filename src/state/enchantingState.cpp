#include "state/enchantingState.h"

#include <algorithm>
#include <format>
#include <iostream>
#include "core/game.h"
#include "state/inventoryState.h"
#include "state/explorationState.h"

enchantingState::enchantingState(int initialBackpackIndex, std::unique_ptr<iGameState> prevState)
    : selectedBackpackIndex(initialBackpackIndex),
      previousState(std::move(prevState))
{
}

void enchantingState::initialise(game* gameContext)
{
    if (gameContext && gameContext->Player)
    {
        if (selectedBackpackIndex >= 0 && static_cast<size_t>(selectedBackpackIndex) < gameContext->Player->inventory.backpack.size())
        {
            auto itPtr = gameContext->Player->inventory.backpack[selectedBackpackIndex];
            if (itPtr)
            {
                stagedEffects = itPtr->infusionEffects;
                customOutputName = itPtr->name;
            }
        }
    }
}

void enchantingState::handleCommand(game* gameContext, const UICommand& cmd)
{
    // Subscribed state commands can be handled here if needed
}

void enchantingState::update(game* gameContext, float deltaTime)
{
}

void enchantingState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void enchantingState::onExit(game* gameContext)
{
}

void enchantingState::selectBackpackItem(int index, game* gameContext)
{
    selectedBackpackIndex = index;
    stagedEffects.clear();
    customOutputName.clear();

    if (gameContext && gameContext->Player)
    {
        if (selectedBackpackIndex >= 0 && static_cast<size_t>(selectedBackpackIndex) < gameContext->Player->inventory.backpack.size())
        {
            auto itPtr = gameContext->Player->inventory.backpack[selectedBackpackIndex];
            if (itPtr)
            {
                stagedEffects = itPtr->infusionEffects;
                customOutputName = itPtr->name;
            }
        }
    }
}

void enchantingState::setFocus(EnchantmentFocus focus)
{
    selectedFocus = focus;
    auto available = getAvailablePropertiesForFocus(selectedFocus);
    if (!available.empty())
    {
        bool currentValid = false;
        for (auto p : available)
        {
            if (p == selectedProperty)
            {
                currentValid = true;
                break;
            }
        }
        if (!currentValid)
        {
            selectedProperty = available.front();
        }
    }
}

void enchantingState::setProperty(AspectProperty prop)
{
    selectedProperty = prop;
}

void enchantingState::setTier(InfusionTier tier)
{
    selectedTier = tier;
}

void enchantingState::stageCurrentEffect()
{
    InfusionEffect eff = getCurrentPreviewEffect();
    for (const auto& exist : stagedEffects)
    {
        if (exist == eff)
        {
            statusMessage = "Effect is already staged.";
            return;
        }
    }
    stagedEffects.push_back(eff);
    statusMessage = "Effect staged in infusion recipe.";
}

void enchantingState::removeStagedEffect(size_t index)
{
    if (index < stagedEffects.size())
    {
        stagedEffects.erase(stagedEffects.begin() + index);
        statusMessage = "Removed effect from recipe.";
    }
}

void enchantingState::clearStagedEffects()
{
    stagedEffects.clear();
    statusMessage = "Cleared all staged effects.";
}

InfusionEffect enchantingState::getCurrentPreviewEffect() const
{
    InfusionTargetType targetType = InfusionTargetType::CONSUMABLE_TONIC;

    // We can infer target type or let it default to tonic
    if (selectedFocus == EnchantmentFocus::ARMOR_REINFORCEMENT || selectedFocus == EnchantmentFocus::BINDING_SPECIAL)
    {
        targetType = InfusionTargetType::APPAREL;
    }
    else if (selectedFocus == EnchantmentFocus::WEAPON_LETHALITY)
    {
        targetType = InfusionTargetType::WEAPONRY;
    }

    return InfusionEffect{
        targetType,
        selectedFocus,
        selectedProperty,
        selectedTier,
        limitValue
    };
}

int enchantingState::getTotalCost(game* gameContext) const
{
    const item* baseItem = nullptr;
    if (gameContext && gameContext->Player)
    {
        if (selectedBackpackIndex >= 0 && static_cast<size_t>(selectedBackpackIndex) < gameContext->Player->inventory.backpack.size())
        {
            baseItem = gameContext->Player->inventory.backpack[selectedBackpackIndex].get();
        }
        return EnchantingEngine::calculateInfusionCost(baseItem, stagedEffects, gameContext->Player);
    }
    return EnchantingEngine::calculateInfusionCost(nullptr, stagedEffects, nullptr);
}

bool enchantingState::canAffordCraft(game* gameContext) const
{
    if (!gameContext || !gameContext->Player) return false;
    float currentEssence = gameContext->Player->getStat("arcaneEssence");
    int cost = getTotalCost(gameContext);
    return currentEssence >= cost && !stagedEffects.empty();
}

void enchantingState::craft(game* gameContext)
{
    if (!gameContext || !gameContext->Player) return;

    int cost = getTotalCost(gameContext);
    if (!canAffordCraft(gameContext))
    {
        statusMessage = "Insufficient Arcane Essence to perform infusion!";
        return;
    }

    std::shared_ptr<item> baseItem = nullptr;
    if (selectedBackpackIndex >= 0 && static_cast<size_t>(selectedBackpackIndex) < gameContext->Player->inventory.backpack.size())
    {
        baseItem = gameContext->Player->inventory.backpack[selectedBackpackIndex];
    }

    // Deduct essence
    gameContext->Player->stats.modifyBaseStat("arcaneEssence", -static_cast<float>(cost));

    // Craft new item
    auto crafted = EnchantingEngine::craftInfusedItem(baseItem.get(), stagedEffects, customOutputName);

    // Consume base item from backpack if stackable or remove
    if (baseItem)
    {
        if (baseItem->isStackable && baseItem->count > 1)
        {
            baseItem->count--;
        }
        else
        {
            gameContext->Player->inventory.backpack.erase(gameContext->Player->inventory.backpack.begin() + selectedBackpackIndex);
            selectedBackpackIndex = -1;
        }
    }

    // Add crafted item to backpack
    gameContext->Player->inventory.backpack.push_back(crafted);
    stagedEffects.clear();
    statusMessage = std::format("Infusion complete: Created {}!", crafted->name);
    std::cout << "[Enchanting] " << statusMessage << " (Essence spent: " << cost << ")\n";

    gameContext->refreshActionGrid();
}

void enchantingState::exitAltar(game* gameContext)
{
    if (!gameContext) return;
    if (previousState)
    {
        gameContext->changeState(std::move(previousState));
    }
    else
    {
        gameContext->changeState(std::make_unique<inventoryState>());
    }
}
