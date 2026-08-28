#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

/**
 * Anchor types for responsive layout panel positioning.
 */
enum class PanelAnchor
{
    TOP_BAR,        // Spans full width at top, fixed height
    BOTTOM_BAR,     // Spans full width at bottom, fixed height
    LEFT_SIDEBAR,   // Anchored left between top & bottom bars, fixed/proportional width
    RIGHT_SIDEBAR,  // Anchored right between top & bottom bars, fixed/proportional width
    CENTER_FLEX,    // Dynamically fills all remaining space between sidebars and bars
    FLOATING_RECT   // Custom absolute/percentage bounds
};

/**
 * Configuration descriptor for a single UI panel.
 */
struct PanelConfig
{
    std::string id;
    PanelAnchor anchor = PanelAnchor::CENTER_FLEX;
    float fixedWidth = 0.0f;       // In base units (e.g. 320px)
    float fixedHeight = 0.0f;      // In base units (e.g. 40px)
    float minWidth = 100.0f;
    float maxWidth = 2000.0f;
    float margin = 6.0f;
    std::string backgroundColor = "bgPanel";
    std::string borderColor = "borderNormal";
    std::vector<std::string> widgets;
    std::vector<std::string> visibleInStates; // Empty = visible in all states
};

/**
 * Calculated pixel bounding box for rendering at native screen resolution.
 */
struct PanelComputedBounds
{
    std::string id;
    SDL_FRect rect{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::string backgroundColor;
    std::string borderColor;
    std::vector<std::string> widgets;
};

/**
 * Responsive Layout Engine.
 * Loads layout definitions from JSON and calculates non-distorting, crisp pixel bounds
 * for all panels at any screen resolution or aspect ratio (16:9, 21:9 ultrawide, 4:3).
 */
class layoutEngine
{
public:
    layoutEngine();
    ~layoutEngine() = default;

    bool loadFromFile(const std::string& filePath);
    void loadDefaultLayout();

    [[nodiscard]] std::vector<PanelComputedBounds> computeLayout(float windowWidth, float windowHeight, float uiScale = 1.0f) const;
    [[nodiscard]] const PanelConfig* getPanelConfig(const std::string& id) const;

    float getMargin() const { return m_globalMargin; }
    void setMargin(float margin) { m_globalMargin = margin; }

private:
    std::string m_layoutName = "Default Responsive 5-Pane";
    float m_globalMargin = 6.0f;
    std::vector<PanelConfig> m_panels;

    static PanelAnchor stringToAnchor(const std::string& str);
    static std::string anchorToString(PanelAnchor anchor);
};
