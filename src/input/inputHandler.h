#pragma once

#include <unordered_map>
#include <cstdint>

// Logical input actions mapped away from raw hardware keys
enum class keyAction {
    moveUp,
    moveDown,
    moveLeft,
    moveRight,
    interact,
    confirm,
    back,
    pause
};

enum class buttonState {
    released,
    pressed,
    held
};

class inputHandler {
public:
    inputHandler() = default;
    ~inputHandler() = default;

    // Process raw window events without engine rendering logic
    void update();

    // Query states by logical action
    [[nodiscard]] bool isActionActive(keyAction action) const;
    [[nodiscard]] bool isActionJustPressed(keyAction action) const;

    // Mouse details in purely logical screen coordinates
    struct mousePosition {
        int x{0};
        int y{0};
    };
    [[nodiscard]] mousePosition getMousePosition() const { return m_mousePosition; }

private:
    std::unordered_map<keyAction, buttonState> m_actionStates;
    mousePosition m_mousePosition;
};