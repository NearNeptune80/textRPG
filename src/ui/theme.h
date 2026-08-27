#pragma once

#include <string>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

/**
 * Colour palette data structure loaded from JSON.
 */
struct ThemeColors
{
    SDL_Color bgDark{ 30, 30, 30, 255 };
    SDL_Color bgPanel{ 30, 28, 35, 255 };
    SDL_Color bgHeader{ 45, 45, 52, 255 };
    SDL_Color bgSlot{ 40, 38, 48, 255 };
    SDL_Color bgSlotOccupied{ 50, 55, 75, 255 };
    SDL_Color bgSlotSelected{ 70, 60, 95, 255 };
    SDL_Color bgButton{ 70, 100, 140, 255 };
    SDL_Color bgButtonDisabled{ 45, 45, 52, 255 };

    SDL_Color borderNormal{ 60, 55, 65, 255 };
    SDL_Color borderSelected{ 255, 215, 0, 255 };
    SDL_Color borderButton{ 100, 140, 190, 255 };
    SDL_Color borderButtonDisabled{ 65, 65, 75, 255 };

    SDL_Color textPrimary{ 255, 255, 255, 255 };
    SDL_Color textSecondary{ 220, 225, 240, 255 };
    SDL_Color textMuted{ 130, 130, 145, 255 };
    SDL_Color textGold{ 255, 215, 0, 255 };
    SDL_Color textAccent{ 180, 150, 220, 255 };

    SDL_Color health{ 255, 60, 90, 255 };
    SDL_Color mana{ 220, 130, 255, 255 };
    SDL_Color lust{ 230, 50, 150, 255 };
    SDL_Color physique{ 255, 50, 120, 255 };
    SDL_Color arcane{ 180, 110, 255, 255 };
    SDL_Color corruption{ 100, 200, 255, 255 };
    SDL_Color currency{ 255, 215, 0, 255 };
    SDL_Color gems{ 255, 100, 220, 255 };

    SDL_Color enemy{ 255, 120, 170, 255 };
    SDL_Color friendly{ 100, 210, 255, 255 };
    SDL_Color companion{ 120, 240, 150, 255 };
};

/**
 * UI Theme Engine.
 * Loads and manages global theme palettes and colour configurations.
 */
class Theme
{
public:
    static ThemeColors colors;
    static bool loadFromFile(const std::string& filePath);
};