#include "ui/theme.h"

#include <algorithm>
#include <fstream>
#include <iostream>

ThemeColors Theme::colors;

static SDL_Color parseJsonColor(const nlohmann::json& j, const std::string& key, SDL_Color fallback)
{
    if (j.contains(key) && j[key].is_array() && j[key].size() >= 3)
    {
        return SDL_Color{
            j[key][0].get<Uint8>(),
            j[key][1].get<Uint8>(),
            j[key][2].get<Uint8>(),
            j[key].size() > 3 ? j[key][3].get<Uint8>() : static_cast<Uint8>(255)
        };
    }
    return fallback;
}

bool Theme::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        nlohmann::json j;
        file >> j;

        colors.bgDark = parseJsonColor(j, "bgDark", colors.bgDark);
        colors.bgPanel = parseJsonColor(j, "bgPanel", colors.bgPanel);
        colors.bgHeader = parseJsonColor(j, "bgHeader", colors.bgHeader);
        colors.bgSlot = parseJsonColor(j, "bgSlot", colors.bgSlot);
        colors.bgSlotOccupied = parseJsonColor(j, "bgSlotOccupied", colors.bgSlotOccupied);
        colors.bgSlotSelected = parseJsonColor(j, "bgSlotSelected", colors.bgSlotSelected);
        colors.bgButton = parseJsonColor(j, "bgButton", colors.bgButton);
        colors.bgButtonDisabled = parseJsonColor(j, "bgButtonDisabled", colors.bgButtonDisabled);

        colors.borderNormal = parseJsonColor(j, "borderNormal", colors.borderNormal);
        colors.borderSelected = parseJsonColor(j, "borderSelected", colors.borderSelected);
        colors.borderButton = parseJsonColor(j, "borderButton", colors.borderButton);
        colors.borderButtonDisabled = parseJsonColor(j, "borderButtonDisabled", colors.borderButtonDisabled);

        colors.textPrimary = parseJsonColor(j, "textPrimary", colors.textPrimary);
        colors.textSecondary = parseJsonColor(j, "textSecondary", colors.textSecondary);
        colors.textMuted = parseJsonColor(j, "textMuted", colors.textMuted);
        colors.textGold = parseJsonColor(j, "textGold", colors.textGold);
        colors.textAccent = parseJsonColor(j, "textAccent", colors.textAccent);

        colors.health = parseJsonColor(j, "health", colors.health);
        colors.mana = parseJsonColor(j, "mana", colors.mana);
        colors.lust = parseJsonColor(j, "lust", colors.lust);
        colors.physique = parseJsonColor(j, "physique", colors.physique);
        colors.arcane = parseJsonColor(j, "arcane", colors.arcane);
        colors.corruption = parseJsonColor(j, "corruption", colors.corruption);
        colors.currency = parseJsonColor(j, "currency", colors.currency);
        colors.gems = parseJsonColor(j, "gems", colors.gems);

        colors.enemy = parseJsonColor(j, "enemy", colors.enemy);
        colors.friendly = parseJsonColor(j, "friendly", colors.friendly);
        colors.companion = parseJsonColor(j, "companion", colors.companion);

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Theme System] Warning: Failed to parse theme file (" << e.what() << "). Using defaults.\n";
        return false;
    }
}

SDL_Color Theme::getColorFromName(const std::string& colorName)
{
    std::string lower = colorName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "fair" || lower == "flesh") return { 240, 190, 170, 255 };
    if (lower == "brown") return { 160, 90, 44, 255 };
    if (lower == "blue") return { 80, 160, 255, 255 };
    if (lower == "pink") return { 255, 130, 180, 255 };
    if (lower == "scarlet" || lower == "red") return colors.health;
    if (lower == "yellow") return colors.textGold;
    if (lower == "purple") return colors.arcane;
    if (lower == "green") return { 60, 200, 80, 255 };
    if (lower == "black" || lower == "dark") return { 100, 100, 110, 255 };
    if (lower == "white") return colors.textPrimary;

    return colors.textSecondary;
}