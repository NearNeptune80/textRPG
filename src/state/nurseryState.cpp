#include "state/nurseryState.h"

#include <format>
#include "core/game.h"
#include "entities/offspringManager.h"
#include "core/servantManager.h"
#include "entities/entity.h"
#include "state/explorationState.h"

nurseryState::nurseryState(std::unique_ptr<iGameState> prevState)
    : previousState(std::move(prevState))
{
}

void nurseryState::initialise(game* gameContext)
{
    if (selectedOffspringId.empty())
    {
        const auto& records = offspringManager::getAllOffspring();
        if (!records.empty())
        {
            selectedOffspringId = records.front().id;
        }
    }
}

void nurseryState::handleCommand(game* gameContext, const UICommand& cmd)
{
    (void)gameContext;
    (void)cmd;
}

void nurseryState::update(game* gameContext, float deltaTime)
{
    (void)gameContext;
    (void)deltaTime;
}

void nurseryState::onEnter(game* gameContext)
{
    initialise(gameContext);
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void nurseryState::onExit(game* gameContext)
{
    (void)gameContext;
}

void nurseryState::selectOffspring(const std::string& id, game* gameContext)
{
    selectedOffspringId = id;
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void nurseryState::renameSelected(const std::string& newName, game* gameContext)
{
    auto* rec = offspringManager::getOffspring(selectedOffspringId);
    if (rec && !newName.empty())
    {
        rec->name = newName;
        statusMessage = "Renamed offspring to: " + newName;
    }
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void nurseryState::promoteToCompanion(game* gameContext)
{
    auto* rec = offspringManager::getOffspring(selectedOffspringId);
    if (!rec || !rec->isAdult())
    {
        statusMessage = "Cannot recruit: Offspring is not yet an adult.";
        if (gameContext) gameContext->refreshActionGrid();
        return;
    }

    std::string childId = selectedOffspringId;
    auto adult = offspringManager::promoteToAdultNPC(childId);
    if (adult && gameContext)
    {
        gameContext->addCompanion(adult);
        statusMessage = std::format("{} has matured into an adult and joined your adventuring party!", adult->name);
        selectedOffspringId = "";
        initialise(gameContext);
        gameContext->refreshActionGrid();
    }
}

void nurseryState::promoteToServant(game* gameContext)
{
    auto* rec = offspringManager::getOffspring(selectedOffspringId);
    if (!rec || !rec->isAdult())
    {
        statusMessage = "Cannot assign: Offspring is not yet an adult.";
        if (gameContext) gameContext->refreshActionGrid();
        return;
    }

    std::string childId = selectedOffspringId;
    auto adult = offspringManager::promoteToAdultNPC(childId);
    if (adult && gameContext)
    {
        servantManager::registerServant(adult);
        statusMessage = std::format("{} has matured into an adult and been assigned as an estate servant!", adult->name);
        selectedOffspringId = "";
        initialise(gameContext);
        gameContext->refreshActionGrid();
    }
}

