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

struct TooltipStatRow
{
    std::string key;
    std::string value;
    bool isMuted{ false };
    bool hasCustomColor{ false };
    SDL_Color color{ 218, 165, 32, 255 };

    TooltipStatRow() = default;
    TooltipStatRow(std::string k, std::string v, bool muted = false)
        : key(std::move(k)), value(std::move(v)), isMuted(muted) {}
    TooltipStatRow(std::pair<std::string, std::string> p)
        : key(std::move(p.first)), value(std::move(p.second)), isMuted(false) {}
    TooltipStatRow(std::string k, std::string v, SDL_Color col)
        : key(std::move(k)), value(std::move(v)), isMuted(false), hasCustomColor(true), color(col) {}
};

struct TooltipData
{
    std::string title;
    std::string subtitle;       // e.g. "[Torso Under] • Value: 120 ¤"
    std::string description;    // Multi-line wrapped text
    std::string hotkey;         // e.g. "[ 1 ]", "[ I ]", "[ ESC ]"
    std::vector<TooltipStatRow> stats; // Key-value pairs with mute and color support
    SDL_Color titleColor = { 218, 165, 32, 255 }; // textGold
    SDL_Color subtitleColor = { 100, 180, 240, 255 }; // companion / accent
    TooltipPoint customAnchor = { 0.0f, 0.0f };
    bool hasCustomAnchor = false;
    std::optional<SDL_FRect> hoverBounds = std::nullopt;
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
