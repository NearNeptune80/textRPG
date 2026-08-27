#pragma once

#include <unordered_map>
#include <SDL3/SDL.h>

class game;

enum class keyAction {
    moveUp,
    moveDown,
    moveLeft,
    moveRight,
    interact,
    confirm,
    back,
    pause,
    toggleInventory,
    quickSave,
    quickLoad
};

enum class buttonState {
    released,
    pressed,
    held
};

/**
 * Headless Hardware Input Processor.
 * Maps physical keyboard and mouse events into logical actions and states.
 * Contains ZERO rendering, window coordinate projection, or UI grid layout math.
 */
class inputHandler {
public:
    inputHandler() = default;
    ~inputHandler() = default;

    // Poll event queue and dispatch inputs to active game state
    void update(game* g);

    [[nodiscard]] bool isActionActive(keyAction action) const;
    [[nodiscard]] bool isActionJustPressed(keyAction action) const;

    struct mousePosition {
        float x{0.0f};
        float y{0.0f};
    };
    [[nodiscard]] mousePosition getMousePosition() const { return m_mousePosition; }
    [[nodiscard]] bool isLeftMouseDown() const { return m_leftMouseDown; }
    [[nodiscard]] bool isLeftMouseJustClicked() const { return m_leftMouseJustClicked; }

    void consumeMouseClick() { m_leftMouseJustClicked = false; }

private:
    std::unordered_map<keyAction, buttonState> m_actionStates;
    mousePosition m_mousePosition{0.0f, 0.0f};
    bool m_leftMouseDown{false};
    bool m_leftMouseJustClicked{false};

    void processKeyEvent(const SDL_Event& event);
};