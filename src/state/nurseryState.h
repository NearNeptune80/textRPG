#pragma once

#include <memory>
#include <string>
#include <vector>

#include "state/iGameState.h"

class game;

class nurseryState : public iGameState
{
public:
    std::string selectedOffspringId = "";
    std::string statusMessage = "";
    std::unique_ptr<iGameState> previousState = nullptr;

    explicit nurseryState(std::unique_ptr<iGameState> prevState = nullptr);
    ~nurseryState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void selectOffspring(const std::string& id, game* gameContext);
    void renameSelected(const std::string& newName, game* gameContext);
    void promoteToCompanion(game* gameContext);
    void promoteToServant(game* gameContext);
};

