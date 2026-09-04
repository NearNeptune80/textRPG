#include "ui/theme.h"

#include <fstream>
#include <iostream>

ThemeColors Theme::colors;

static SDL_Color parseJsonColor(const nlohmann::json& parentJson, const std::string& key, SDL_Color fallback)
{
    const nlohmann::json* target = nullptr;
    if (parentJson.contains("colors") && parentJson["colors"].is_object() && parentJson["colors"].contains(key))
    {
        target = &parentJson["colors"][key];
    }
    else if (parentJson.contains(key))
    {
        target = &parentJson[key];
    }

    if (!target) return fallback;

    // 1. Array [r, g, b, (a)]
    if (target->is_array() && target->size() >= 3)
    {
        return SDL_Color{
            (*target)[0].get<Uint8>(),
            (*target)[1].get<Uint8>(),
            (*target)[2].get<Uint8>(),
            target->size() > 3 ? (*target)[3].get<Uint8>() : static_cast<Uint8>(255)
        };
    }

    // 2. Object { "r": r, "g": g, "b": b, ("a": a) }
    if (target->is_object())
    {
        Uint8 r = target->value("r", fallback.r);
        Uint8 g = target->value("g", fallback.g);
        Uint8 b = target->value("b", fallback.b);
        Uint8 a = target->value("a", fallback.a);
        return SDL_Color{ r, g, b, a };
    }

    return fallback;
}

bool Theme::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::ifstream altFile("data/themes/theme.json");
        if (!altFile.is_open())
        {
            std::cout << "[Theme] Could not initialise theme from " << filePath << " or data/themes/theme.json. Using default palette.\n";
            return false;
        }
        return loadFromFile("data/themes/theme.json");
    }

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
        colors.bgInput = parseJsonColor(j, "bgInput", colors.bgInput);
        colors.bgTooltip = parseJsonColor(j, "bgTooltip", colors.bgTooltip);
        colors.bgButtonHover = parseJsonColor(j, "bgButtonHover", colors.bgButtonHover);

        colors.borderNormal = parseJsonColor(j, "borderNormal", colors.borderNormal);
        colors.borderSelected = parseJsonColor(j, "borderSelected", colors.borderSelected);
        colors.borderButton = parseJsonColor(j, "borderButton", colors.borderButton);
        colors.borderButtonDisabled = parseJsonColor(j, "borderButtonDisabled", colors.borderButtonDisabled);
        colors.borderButtonHover = parseJsonColor(j, "borderButtonHover", colors.borderButtonHover);
        colors.borderMuted = parseJsonColor(j, "borderMuted", colors.borderMuted);
        colors.slotEmptyBorder = parseJsonColor(j, "slotEmptyBorder", colors.slotEmptyBorder);
        colors.borderTooltip = parseJsonColor(j, "borderTooltip", colors.borderTooltip);

        colors.toggleOn = parseJsonColor(j, "toggleOn", colors.toggleOn);
        colors.toggleOff = parseJsonColor(j, "toggleOff", colors.toggleOff);

        colors.textPrimary = parseJsonColor(j, "textPrimary", colors.textPrimary);
        colors.textSecondary = parseJsonColor(j, "textSecondary", colors.textSecondary);
        colors.textMuted = parseJsonColor(j, "textMuted", colors.textMuted);
        colors.textGold = parseJsonColor(j, "textGold", colors.textGold);
        colors.textAccent = parseJsonColor(j, "textAccent", colors.textAccent);
        colors.textDisabled = parseJsonColor(j, "textDisabled", colors.textDisabled);

        colors.badgeBackground = parseJsonColor(j, "badgeBackground", colors.badgeBackground);

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

        std::cout << "[Theme] Initialised UI colour palette successfully from " << filePath << ".\n";
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Theme] JSON parse error in " << filePath << ": " << e.what() << "\n";
        return false;
    }
}

bool Theme::applyTheme(const std::string& themeNameOrPath)
{
    if (themeNameOrPath.ends_with(".json"))
    {
        return loadFromFile(themeNameOrPath);
    }

    if (themeNameOrPath == "theme_dark_fantasy" || themeNameOrPath == "dark_fantasy" || themeNameOrPath == "Dark Fantasy")
    {
        return loadFromFile("data/themes/theme_dark_fantasy.json");
    }
    if (themeNameOrPath == "theme_cyber_neon" || themeNameOrPath == "cyber_neon" || themeNameOrPath == "Cyber Neon")
    {
        return loadFromFile("data/themes/theme_cyber_neon.json");
    }
    if (themeNameOrPath == "theme_parchment" || themeNameOrPath == "parchment" || themeNameOrPath == "Arcane Parchment" || themeNameOrPath == "Parchment")
    {
        return loadFromFile("data/themes/theme_parchment.json");
    }

    return loadFromFile("data/themes/theme.json");
}