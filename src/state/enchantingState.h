#pragma once

#include <memory>
#include <string>
#include <vector>

#include "state/iGameState.h"
#include "items/enchantmentAspects.h"
#include "items/infusionTier.h"
#include "items/infusionEffect.h"
#include "items/enchantingEngine.h"

class game;

class enchantingState : public iGameState
{
public:
    int selectedBackpackIndex = -1;
    std::shared_ptr<item> targetItemPtr = nullptr;
    EnchantmentFocus selectedFocus = EnchantmentFocus::HEAD_FEATURE;
    AspectProperty selectedProperty = AspectProperty::AGILITY_STAT;
    InfusionTier selectedTier = InfusionTier::GREATER_BOON;
    int limitValue = -1;

    std::vector<InfusionEffect> stagedEffects;
    std::string customOutputName = "";
    std::string statusMessage = "";

    std::unique_ptr<iGameState> previousState;

    explicit enchantingState(int initialBackpackIndex = -1, std::unique_ptr<iGameState> prevState = nullptr, std::shared_ptr<item> initialItem = nullptr);
    ~enchantingState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void selectBackpackItem(int index, game* gameContext);
    void cycleBackpackItem(game* gameContext);
    const item* getSelectedBaseItem(const game* gameContext) const;
    std::shared_ptr<item> getSelectedBaseItemPtr(const game* gameContext) const;
    void setFocus(EnchantmentFocus focus);
    void setProperty(AspectProperty prop);
    void setTier(InfusionTier tier);

    void stageCurrentEffect();
    void removeStagedEffect(size_t index);
    void clearStagedEffects();

    InfusionEffect getCurrentPreviewEffect() const;
    int getTotalCost(game* gameContext) const;
    bool canAffordCraft(game* gameContext) const;

    void craft(game* gameContext);
    void exitAltar(game* gameContext);
};
