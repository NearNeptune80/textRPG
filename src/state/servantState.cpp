#include "state/servantState.h"

#include <format>
#include "core/game.h"
#include "core/servantManager.h"
#include "entities/entity.h"
#include "state/explorationState.h"

servantState::servantState(std::unique_ptr<iGameState> prevState)
    : previousState(std::move(prevState))
{
}

void servantState::initialise(game* gameContext)
{
    if (selectedServantId.empty())
    {
        auto servants = servantManager::getAllServants();
        if (!servants.empty() && servants.front())
        {
            selectedServantId = servants.front()->id;
        }
    }
}

void servantState::handleCommand(game* gameContext, const UICommand& cmd)
{
    (void)gameContext;
    (void)cmd;
}

void servantState::update(game* gameContext, float deltaTime)
{
    (void)gameContext;
    (void)deltaTime;
}

void servantState::onEnter(game* gameContext)
{
    initialise(gameContext);
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void servantState::onExit(game* gameContext)
{
    (void)gameContext;
}

void servantState::selectServant(const std::string& id, game* gameContext)
{
    selectedServantId = id;
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void servantState::assignJobToSelected(int hour, const std::string& jobId, game* gameContext)
{
    auto s = servantManager::getServant(selectedServantId);
    if (!s) return;

    s->servant.setJobForHour(hour, jobId);
    statusMessage = std::format("Assigned {} to hour {:02d}:00.", jobId, hour);
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void servantState::setFullDayJob(const std::string& jobId, game* gameContext)
{
    auto s = servantManager::getServant(selectedServantId);
    if (!s) return;

    s->servant.fillSchedule(jobId);
    statusMessage = std::format("Set 24-hour schedule for {} to {}.", s->name, jobId);
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void servantState::releaseServant(game* gameContext)
{
    if (selectedServantId.empty()) return;

    auto s = servantManager::getServant(selectedServantId);
    std::string name = s ? s->name : selectedServantId;
    servantManager::removeServant(selectedServantId);

    if (gameContext)
    {
        gameContext->addLogEntry("[SERVANTS]", std::format("Released {} from servitude.", name), { 180, 180, 180, 255 });
    }

    auto servants = servantManager::getAllServants();
    if (!servants.empty() && servants.front())
    {
        selectedServantId = servants.front()->id;
        if (gameContext) gameContext->refreshActionGrid();
    }
    else
    {
        selectedServantId = "";
        if (gameContext)
        {
            if (previousState)
            {
                gameContext->changeState(std::move(previousState));
            }
            else
            {
                gameContext->changeState(std::make_unique<explorationState>());
            }
        }
    }
}

