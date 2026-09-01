#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <optional>

struct TooltipPoint
{
    float x{ 0.0f };
    float y{ 0.0f };
    TooltipPoint() = default;
    TooltipPoint(float _x, float _y) : x(_x), y(_y) {}
    TooltipPoint(const SDL_FPoint& pt) : x(pt.x), y(pt.y) {}
    template<typename T>
    TooltipPoint(const T& pt) : x(static_cast<float>(pt.x)), y(static_cast<float>(pt.y)) {}
    operator SDL_FPoint() const { return { x, y }; }
};

struct TooltipData
{
    std::string title;
    std::string subtitle;       // e.g. "[Torso Under] • Value: 120 ¤"
    std::string description;    // Multi-line wrapped text
    std::string hotkey;         // e.g. "[ 1 ]", "[ I ]", "[ ESC ]"
    std::vector<std::pair<std::string, std::string>> stats; // Key-value pairs
    SDL_Color titleColor = { 218, 165, 32, 255 }; // textGold
    SDL_Color subtitleColor = { 100, 180, 240, 255 }; // companion / accent
    TooltipPoint customAnchor = { 0.0f, 0.0f };
    bool hasCustomAnchor = false;
};

class TooltipManager
{
public:
    static void setTooltip(const TooltipData& data);
    
    static void setTooltip(const std::string& title,
                           const std::string& description = "",
                           const std::string& subtitle = "",
                           const std::string& hotkey = "",
                           const SDL_Color* titleColor = nullptr);

    static bool setHoverTooltip(const SDL_FRect& hoverRect,
                                const TooltipPoint& mousePos,
                                const std::string& title,
                                const std::string& description = "",
                                const std::string& subtitle = "",
                                const std::string& hotkey = "",
                                const SDL_Color* titleColor = nullptr);

    static bool setHoverTooltip(const SDL_FRect& hoverRect,
                                const TooltipPoint& mousePos,
                                const TooltipData& data);

    static void clear();
    static bool hasActiveTooltip();

    static void render(SDL_Renderer* renderer, float uiScale, float windowW, float windowH, const TooltipPoint& mousePos);

private:
    static std::optional<TooltipData> s_activeTooltip;
};
