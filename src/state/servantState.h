#pragma once

#include <memory>
#include <string>
#include <vector>

#include "state/iGameState.h"

class game;

class servantState : public iGameState
{
public:
    std::string selectedServantId = "";
    int selectedHour = 0; // 0..23
    std::string statusMessage = "";
    std::unique_ptr<iGameState> previousState = nullptr;

    explicit servantState(std::unique_ptr<iGameState> prevState = nullptr);
    ~servantState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void selectServant(const std::string& id, game* gameContext);
    void assignJobToSelected(int hour, const std::string& jobId, game* gameContext);
    void setFullDayJob(const std::string& jobId, game* gameContext);
    void releaseServant(game* gameContext);
};

