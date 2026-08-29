#include "input/inputHandler.h"

#include "core/game.h"
#include "state/explorationState.h"
#include "state/iGameState.h"
#include "state/loadGameState.h"

void inputHandler::update(game* g)
{
    if (!g) return;

    m_leftMouseJustClicked = false;
    m_mouseWheelY = 0.0f;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            g->isRunning = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            m_mousePosition.x = event.motion.x;
            m_mousePosition.y = event.motion.y;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            m_leftMouseDown = true;
            m_leftMouseJustClicked = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
        {
            m_leftMouseDown = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            m_mouseWheelY += event.wheel.y;
        }

        processKeyEvent(event);

        if (g->getActiveState())
        {
            g->getActiveState()->handleInput(g, event);
        }

        // Global hotkey routing for Action Grid buttons (1..5, SHIFT+1..5, CTRL+1..5, Q, E)
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
        {
            auto loadState = dynamic_cast<loadGameState*>(g->getActiveState());
            if (!loadState || !loadState->isEditingSaveName)
            {
                SDL_Keymod mod = SDL_GetModState();
                bool isShift = (mod & SDL_KMOD_SHIFT) != 0;
                bool isCtrl = (mod & SDL_KMOD_CTRL) != 0;

                int slotIdx = -1;
                switch (event.key.key)
                {
                    case SDLK_1: slotIdx = isCtrl ? 10 : (isShift ? 5 : 0); break;
                    case SDLK_2: slotIdx = isCtrl ? 11 : (isShift ? 6 : 1); break;
                    case SDLK_3: slotIdx = isCtrl ? 12 : (isShift ? 7 : 2); break;
                    case SDLK_4: slotIdx = isCtrl ? 13 : (isShift ? 8 : 3); break;
                    case SDLK_5: slotIdx = isCtrl ? 14 : (isShift ? 9 : 4); break;
                    case SDLK_Q:
                        if (!isCtrl)
                        {
                            g->previousActionPage();
                        }
                        break;
                    case SDLK_E:
                        if (!dynamic_cast<explorationState*>(g->getActiveState()))
                        {
                            g->nextActionPage();
                        }
                        break;
                    default:
                        break;
                }

                if (slotIdx >= 0)
                {
                    g->triggerActionButton(slotIdx);
                }
            }
        }
    }
}

void inputHandler::processKeyEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (!event.key.repeat)
        {
            switch (event.key.key)
            {
                case SDLK_W:
                case SDLK_UP:
                    m_actionStates[keyAction::moveUp] = buttonState::pressed;
                    break;
                case SDLK_S:
                case SDLK_DOWN:
                    m_actionStates[keyAction::moveDown] = buttonState::pressed;
                    break;
                case SDLK_A:
                case SDLK_LEFT:
                    m_actionStates[keyAction::moveLeft] = buttonState::pressed;
                    break;
                case SDLK_D:
                case SDLK_RIGHT:
                    m_actionStates[keyAction::moveRight] = buttonState::pressed;
                    break;
                case SDLK_E:
                case SDLK_RETURN:
                case SDLK_SPACE:
                    m_actionStates[keyAction::confirm] = buttonState::pressed;
                    break;
                case SDLK_ESCAPE:
                    m_actionStates[keyAction::pause] = buttonState::pressed;
                    break;
                case SDLK_I:
                    m_actionStates[keyAction::toggleInventory] = buttonState::pressed;
                    break;
                default:
                    break;
            }

            if (event.key.scancode == SDL_SCANCODE_F5)
            {
                m_actionStates[keyAction::quickSave] = buttonState::pressed;
            }
            else if (event.key.scancode == SDL_SCANCODE_F9)
            {
                m_actionStates[keyAction::quickLoad] = buttonState::pressed;
            }
        }
    }
    else if (event.type == SDL_EVENT_KEY_UP)
    {
        switch (event.key.key)
        {
            case SDLK_W:
            case SDLK_UP:
                m_actionStates[keyAction::moveUp] = buttonState::released;
                break;
            case SDLK_S:
            case SDLK_DOWN:
                m_actionStates[keyAction::moveDown] = buttonState::released;
                break;
            case SDLK_A:
            case SDLK_LEFT:
                m_actionStates[keyAction::moveLeft] = buttonState::released;
                break;
            case SDLK_D:
            case SDLK_RIGHT:
                m_actionStates[keyAction::moveRight] = buttonState::released;
                break;
            case SDLK_E:
            case SDLK_RETURN:
            case SDLK_SPACE:
                m_actionStates[keyAction::confirm] = buttonState::released;
                break;
            case SDLK_ESCAPE:
                m_actionStates[keyAction::pause] = buttonState::released;
                break;
            case SDLK_I:
                m_actionStates[keyAction::toggleInventory] = buttonState::released;
                break;
            default:
                break;
        }

        if (event.key.scancode == SDL_SCANCODE_F5)
        {
            m_actionStates[keyAction::quickSave] = buttonState::released;
        }
        else if (event.key.scancode == SDL_SCANCODE_F9)
        {
            m_actionStates[keyAction::quickLoad] = buttonState::released;
        }
    }
}

bool inputHandler::isActionActive(keyAction action) const
{
    auto it = m_actionStates.find(action);
    if (it != m_actionStates.end())
    {
        return it->second == buttonState::pressed || it->second == buttonState::held;
    }
    return false;
}

bool inputHandler::isActionJustPressed(keyAction action) const
{
    auto it = m_actionStates.find(action);
    if (it != m_actionStates.end())
    {
        return it->second == buttonState::pressed;
    }
    return false;
}