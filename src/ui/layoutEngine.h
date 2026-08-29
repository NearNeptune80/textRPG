#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

/**
 * Anchor types for legacy layout panel positioning.
 */
enum class PanelAnchor
{
    TOP_BAR,
    BOTTOM_BAR,
    LEFT_SIDEBAR,
    RIGHT_SIDEBAR,
    CENTER_FLEX,
    FLOATING_RECT
};

/**
 * Configuration descriptor for a single UI panel.
 */
struct PanelConfig
{
    std::string id;
    PanelAnchor anchor = PanelAnchor::CENTER_FLEX;
    float fixedWidth = 0.0f;
    float fixedHeight = 0.0f;
    float minWidth = 100.0f;
    float maxWidth = 2000.0f;
    float margin = 6.0f;
    std::string backgroundColor = "bgPanel";
    std::string borderColor = "borderNormal";
    std::vector<std::string> widgets;
    std::vector<std::string> visibleInStates;
};

/**
 * N-Way Container Node (exported from textRPG-studio).
 */
struct StudioLayoutNode
{
    std::string id;
    std::string type = "LEAF"; // "CONTAINER" or "LEAF"
    std::string direction = "ROW"; // "ROW" or "COLUMN"
    std::vector<float> sizes;
    std::vector<StudioLayoutNode> children;
    std::vector<std::string> widgets;
    std::string name;
};

/**
 * Calculated pixel bounding box for rendering at native screen resolution.
 */
struct PanelComputedBounds
{
    std::string id;
    SDL_FRect rect{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::string backgroundColor = "bgPanel";
    std::string borderColor = "borderNormal";
    std::vector<std::string> widgets;
};

/**
 * Responsive Layout Engine.
 * Loads layout definitions from JSON (both textRPG-studio N-way container format and legacy panel format)
 * and calculates crisp, responsive pixel bounds for all panels at any screen resolution or aspect ratio.
 */
class layoutEngine
{
public:
    layoutEngine();
    ~layoutEngine() = default;

    bool loadFromFile(const std::string& filePath);
    void loadDefaultLayout();

    [[nodiscard]] std::vector<PanelComputedBounds> computeLayout(float windowWidth, float windowHeight, float uiScale = 1.0f, const std::string& activeState = "") const;
    [[nodiscard]] const PanelConfig* getPanelConfig(const std::string& id) const;

    float getMargin() const { return m_globalMargin; }
    void setMargin(float margin) { m_globalMargin = margin; }

private:
    std::string m_layoutName = "Default Responsive Layout";
    float m_globalMargin = 6.0f;
    bool m_hasRootNode = false;
    StudioLayoutNode m_rootNode;
    std::unordered_map<std::string, StudioLayoutNode> m_stateOverrides;
    std::vector<PanelConfig> m_panels;

    void parseStudioNode(const nlohmann::json& j, StudioLayoutNode& node);
    void computeStudioNode(const StudioLayoutNode& node, const SDL_FRect& bounds, float margin, std::vector<PanelComputedBounds>& out) const;

    static PanelAnchor stringToAnchor(const std::string& str);
    static std::string anchorToString(PanelAnchor anchor);
};
