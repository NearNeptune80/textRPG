#include "state/characterCreationState.h"

#include <algorithm>
#include <random>
#include "core/game.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

static const std::vector<std::string> MASC_NAMES = {
    "Arthur", "James", "Thomas", "William", "Alexander", "Edward",
    "Henry", "Charles", "Oliver", "George", "Harry", "Jack", "Samuel", "David"
};

static const std::vector<std::string> ANDRO_NAMES = {
    "Alex", "Sam", "Chris", "Taylor", "Jordan", "Morgan",
    "Riley", "Robin", "Casey", "Jamie", "Quinn", "Avery"
};

static const std::vector<std::string> FEM_NAMES = {
    "Lily", "Victoria", "Alice", "Charlotte", "Eleanor", "Grace",
    "Sophia", "Emma", "Olivia", "Rose", "Emily", "Isabella"
};

static const std::vector<std::string> SURNAMES = {
    "Blackwood", "Sterling", "Ashford", "Montgomery", "Pemberton",
    "Sinclair", "Vance", "Harrington", "Kensington", "Thorne"
};

characterCreationState::characterCreationState()
{
}

void characterCreationState::initialise(game* gameContext)
{
    if (gameContext)
    {
        gameContext->gameTime.day = 29;
        gameContext->gameTime.hour = 20;
        gameContext->gameTime.minute = 34;
    }
}

void characterCreationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void characterCreationState::onExit(game* gameContext)
{
}

void characterCreationState::update(game* gameContext, float deltaTime)
{
}

void characterCreationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (step == 2) // Step 3: Name text input
    {
        if (event.type == SDL_EVENT_TEXT_INPUT)
        {
            std::string text = event.text.text;
            // Filter out '[' ']' '.'
            text.erase(std::remove_if(text.begin(), text.end(), [](char c) {
                return c == '[' || c == ']' || c == '.';
            }), text.end());

            if (activeNameField == 0)
            {
                if (masculineName == "Unknown") masculineName = "";
                if (masculineName.size() + text.size() <= 32) masculineName += text;
            }
            else if (activeNameField == 1)
            {
                if (androgynousName == "Unknown") androgynousName = "";
                if (androgynousName.size() + text.size() <= 32) androgynousName += text;
            }
            else if (activeNameField == 2)
            {
                if (feminineName == "Unknown") feminineName = "";
                if (feminineName.size() + text.size() <= 32) feminineName += text;
            }
            else if (activeNameField == 3)
            {
                if (surname.size() + text.size() <= 32) surname += text;
            }
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_BACKSPACE)
            {
                if (activeNameField == 0 && !masculineName.empty()) masculineName.pop_back();
                else if (activeNameField == 1 && !androgynousName.empty()) androgynousName.pop_back();
                else if (activeNameField == 2 && !feminineName.empty()) feminineName.pop_back();
                else if (activeNameField == 3 && !surname.empty()) surname.pop_back();
            }
            else if (event.key.key == SDLK_TAB)
            {
                activeNameField = (activeNameField + 1) % 4;
            }
        }
    }
}

void characterCreationState::handleCommand(game* gameContext, const UICommand& cmd)
{
}

void characterCreationState::randomizeFirstNames()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> distM(0, MASC_NAMES.size() - 1);
    std::uniform_int_distribution<size_t> distA(0, ANDRO_NAMES.size() - 1);
    std::uniform_int_distribution<size_t> distF(0, FEM_NAMES.size() - 1);

    masculineName = MASC_NAMES[distM(gen)];
    androgynousName = ANDRO_NAMES[distA(gen)];
    feminineName = FEM_NAMES[distF(gen)];
}

void characterCreationState::randomizeSurname()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> distS(0, SURNAMES.size() - 1);
    surname = SURNAMES[distS(gen)];
}

void characterCreationState::finalizeCharacter(game* gameContext)
{
    if (!gameContext) return;

    entity* player = gameContext->getPlayer();
    if (player)
    {
        std::string chosenName = masculineName;
        if (femininity == "Androgynous") chosenName = androgynousName;
        else if (femininity == "Feminine" || femininity == "Very Feminine") chosenName = feminineName;

        if (chosenName == "Unknown" || chosenName.empty())
        {
            chosenName = (gender == "Female") ? "Lily" : "Alex";
        }

        if (!surname.empty())
        {
            player->name = chosenName + " " + surname;
        }
        else
        {
            player->name = chosenName;
        }

        // Apply anatomy based on gender selection
        if (gender == "Female")
        {
            bodyPart breasts;
            breasts.id = "breasts_human";
            breasts.name = "Breasts";
            breasts.race = "Human";
            breasts.cupSize = 2; // B-Cup
            breasts.primaryColor = "fair";
            player->anatomy.setPart(bodySlot::BREASTS, breasts);

            bodyPart vagina;
            vagina.id = "vagina_human";
            vagina.name = "Vagina";
            vagina.race = "Human";
            vagina.tags.push_back("vagina");
            player->anatomy.setPart(bodySlot::GROIN, vagina);
        }
        else
        {
            bodyPart breasts;
            breasts.id = "breasts_human";
            breasts.name = "Breasts";
            breasts.race = "Human";
            breasts.cupSize = 0;
            breasts.primaryColor = "fair";
            player->anatomy.setPart(bodySlot::BREASTS, breasts);

            bodyPart penis;
            penis.id = "penis_human";
            penis.name = "Penis";
            penis.race = "Human";
            penis.length = 16.0f;
            penis.diameter = 3.8f;
            penis.tags.push_back("penis");
            player->anatomy.setPart(bodySlot::GROIN, penis);
        }
    }

    gameContext->changeState(std::make_unique<explorationState>());
}
