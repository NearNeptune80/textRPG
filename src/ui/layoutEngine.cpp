#include "ui/layoutEngine.h"

#include <algorithm>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

layoutEngine::layoutEngine()
{
    loadDefaultLayout();
}

PanelAnchor layoutEngine::stringToAnchor(const std::string& str)
{
    if (str == "TOP_BAR" || str == "TOP")             return PanelAnchor::TOP_BAR;
    if (str == "BOTTOM_BAR" || str == "BOTTOM")       return PanelAnchor::BOTTOM_BAR;
    if (str == "LEFT_SIDEBAR" || str == "LEFT")       return PanelAnchor::LEFT_SIDEBAR;
    if (str == "RIGHT_SIDEBAR" || str == "RIGHT")     return PanelAnchor::RIGHT_SIDEBAR;
    if (str == "FLOATING_RECT" || str == "FLOATING")  return PanelAnchor::FLOATING_RECT;
    return PanelAnchor::CENTER_FLEX;
}

std::string layoutEngine::anchorToString(PanelAnchor anchor)
{
    switch (anchor)
    {
        case PanelAnchor::TOP_BAR:       return "TOP_BAR";
        case PanelAnchor::BOTTOM_BAR:    return "BOTTOM_BAR";
        case PanelAnchor::LEFT_SIDEBAR:  return "LEFT_SIDEBAR";
        case PanelAnchor::RIGHT_SIDEBAR: return "RIGHT_SIDEBAR";
        case PanelAnchor::FLOATING_RECT: return "FLOATING_RECT";
        case PanelAnchor::CENTER_FLEX:
        default:                         return "CENTER_FLEX";
    }
}

void layoutEngine::parseStudioNode(const json& j, StudioLayoutNode& node)
{
    node.id = j.value("id", "box_" + std::to_string(rand() % 1000));
    node.type = j.value("type", "LEAF");
    node.name = j.value("name", "");
    node.direction = j.value("direction", "ROW");

    if (j.contains("sizes") && j["sizes"].is_array())
    {
        node.sizes = j["sizes"].get<std::vector<float>>();
    }

    if (j.contains("widgets") && j["widgets"].is_array())
    {
        node.widgets = j["widgets"].get<std::vector<std::string>>();
    }

    if (j.contains("children") && j["children"].is_array())
    {
        for (const auto& cJson : j["children"])
        {
            StudioLayoutNode childNode;
            parseStudioNode(cJson, childNode);
            node.children.push_back(childNode);
        }
    }
}

void layoutEngine::loadDefaultLayout()
{
    m_layoutName = "Default Responsive 5-Pane";
    m_globalMargin = 6.0f;
    m_hasRootNode = false;
    m_panels.clear();

    // 1. Top Header Bar
    PanelConfig topBar;
    topBar.id = "top_bar";
    topBar.anchor = PanelAnchor::TOP_BAR;
    topBar.fixedHeight = 38.0f;
    topBar.backgroundColor = "bgHeader";
    topBar.borderColor = "borderNormal";
    topBar.widgets = { "TOP_STATUS_BAR", "widget_top_bar_full" };
    m_panels.push_back(topBar);

    // 2. Bottom Action Grid
    PanelConfig bottomGrid;
    bottomGrid.id = "bottom_action_grid";
    bottomGrid.anchor = PanelAnchor::BOTTOM_BAR;
    bottomGrid.fixedHeight = 140.0f;
    bottomGrid.backgroundColor = "bgPanel";
    bottomGrid.borderColor = "borderNormal";
    bottomGrid.widgets = { "ACTION_GRID", "widget_action_commands" };
    m_panels.push_back(bottomGrid);

    // 3. Left Character & Stats Sidebar
    PanelConfig leftPane;
    leftPane.id = "left_pane";
    leftPane.anchor = PanelAnchor::LEFT_SIDEBAR;
    leftPane.fixedWidth = 320.0f;
    leftPane.backgroundColor = "bgPanel";
    leftPane.borderColor = "borderNormal";
    leftPane.widgets = { "CHARACTER_OVERVIEW", "STAT_BARS", "PAPERDOLL_SOCKETS", "widget_char_overview", "widget_vitals_gauges", "widget_attributes_table" };
    m_panels.push_back(leftPane);

    // 4. Right Target & Minimap Sidebar
    PanelConfig rightPane;
    rightPane.id = "right_pane";
    rightPane.anchor = PanelAnchor::RIGHT_SIDEBAR;
    rightPane.fixedWidth = 300.0f;
    rightPane.backgroundColor = "bgPanel";
    rightPane.borderColor = "borderNormal";
    rightPane.widgets = { "MINIMAP_RADAR", "TARGET_INSPECTOR", "widget_minimap_radar" };
    m_panels.push_back(rightPane);

    // 5. Center Dynamic View
    PanelConfig centerPane;
    centerPane.id = "center_pane";
    centerPane.anchor = PanelAnchor::CENTER_FLEX;
    centerPane.backgroundColor = "bgPanel";
    centerPane.borderColor = "borderNormal";
    centerPane.widgets = { "SCENE_NARRATIVE", "INTERACTIVE_SEX", "COMBAT_VIEW", "BACKPACK_INVENTORY", "RESOLUTION_HUB", "widget_narrative_story" };
    m_panels.push_back(centerPane);
}

bool layoutEngine::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json j;
        file >> j;

        m_layoutName = j.value("layoutName", "Custom Layout");
        m_globalMargin = j.value("margin", 6.0f);
        m_panels.clear();
        m_hasRootNode = false;

        // Check for textRPG-studio N-way container format
        if (j.contains("rootNode") && j["rootNode"].is_object())
        {
            m_hasRootNode = true;
            parseStudioNode(j["rootNode"], m_rootNode);
            return true;
        }

        // Legacy panels format
        if (j.contains("panels"))
        {
            for (const auto& pJson : j["panels"])
            {
                PanelConfig p;
                p.id = pJson.value("id", "panel_" + std::to_string(m_panels.size()));
                p.anchor = stringToAnchor(pJson.value("anchor", "CENTER_FLEX"));
                p.fixedWidth = pJson.value("fixedWidth", 0.0f);
                p.fixedHeight = pJson.value("fixedHeight", 0.0f);
                p.minWidth = pJson.value("minWidth", 100.0f);
                p.maxWidth = pJson.value("maxWidth", 2000.0f);
                p.margin = pJson.value("margin", m_globalMargin);
                p.backgroundColor = pJson.value("backgroundColor", "bgPanel");
                p.borderColor = pJson.value("borderColor", "borderNormal");
                p.widgets = pJson.value("widgets", std::vector<std::string>{});
                p.visibleInStates = pJson.value("visibleInStates", std::vector<std::string>{});
                m_panels.push_back(p);
            }
            return true;
        }
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[LayoutEngine] Error parsing " << filePath << ": " << e.what() << "\n";
        return false;
    }
}

const PanelConfig* layoutEngine::getPanelConfig(const std::string& id) const
{
    for (const auto& p : m_panels)
    {
        if (p.id == id) return &p;
    }
    return nullptr;
}

void layoutEngine::computeStudioNode(const StudioLayoutNode& node, const SDL_FRect& bounds, float margin, std::vector<PanelComputedBounds>& out) const
{
    if (node.type == "LEAF" || node.children.empty())
    {
        PanelComputedBounds leafBounds;
        leafBounds.id = node.id;
        leafBounds.rect = {
            bounds.x + (margin / 2.0f),
            bounds.y + (margin / 2.0f),
            std::max(10.0f, bounds.w - margin),
            std::max(10.0f, bounds.h - margin)
        };
        leafBounds.backgroundColor = "bgPanel";
        leafBounds.borderColor = "borderNormal";
        leafBounds.widgets = node.widgets;
        out.push_back(leafBounds);
        return;
    }

    // Container node: split bounds among children
    const bool isRow = (node.direction == "ROW");
    const size_t numChildren = node.children.size();
    float totalSize = 0.0f;
    for (size_t i = 0; i < numChildren; ++i)
    {
        totalSize += (i < node.sizes.size()) ? node.sizes[i] : (100.0f / numChildren);
    }
    if (totalSize <= 0.0f) totalSize = 100.0f;

    float currentOffset = 0.0f;
    for (size_t i = 0; i < numChildren; ++i)
    {
        const float sizePercent = (i < node.sizes.size()) ? node.sizes[i] : (100.0f / numChildren);
        const float fraction = sizePercent / totalSize;

        SDL_FRect childBounds;
        if (isRow)
        {
            const float childWidth = bounds.w * fraction;
            childBounds = { bounds.x + currentOffset, bounds.y, childWidth, bounds.h };
            currentOffset += childWidth;
        }
        else
        {
            const float childHeight = bounds.h * fraction;
            childBounds = { bounds.x, bounds.y + currentOffset, bounds.w, childHeight };
            currentOffset += childHeight;
        }

        computeStudioNode(node.children[i], childBounds, margin, out);
    }
}

std::vector<PanelComputedBounds> layoutEngine::computeLayout(float windowWidth, float windowHeight, float uiScale) const
{
    std::vector<PanelComputedBounds> results;
    float margin = m_globalMargin * uiScale;

    // Handle Studio RootNode format
    if (m_hasRootNode)
    {
        SDL_FRect rootBounds = { margin / 2.0f, margin / 2.0f, windowWidth - margin, windowHeight - margin };
        computeStudioNode(m_rootNode, rootBounds, margin, results);
        return results;
    }

    // Legacy Panel Anchor calculation
    float topBarHeight = 0.0f;
    float bottomBarHeight = 0.0f;
    float leftSidebarWidth = 0.0f;
    float rightSidebarWidth = 0.0f;

    for (const auto& p : m_panels)
    {
        if (p.anchor == PanelAnchor::TOP_BAR)
        {
            topBarHeight = p.fixedHeight * uiScale;
        }
        else if (p.anchor == PanelAnchor::BOTTOM_BAR)
        {
            bottomBarHeight = p.fixedHeight * uiScale;
        }
        else if (p.anchor == PanelAnchor::LEFT_SIDEBAR)
        {
            leftSidebarWidth = std::clamp(p.fixedWidth * uiScale, p.minWidth * uiScale, p.maxWidth * uiScale);
        }
        else if (p.anchor == PanelAnchor::RIGHT_SIDEBAR)
        {
            rightSidebarWidth = std::clamp(p.fixedWidth * uiScale, p.minWidth * uiScale, p.maxWidth * uiScale);
        }
    }

    float contentTopY = margin;
    if (topBarHeight > 0.0f)
    {
        contentTopY += topBarHeight + margin;
    }

    float contentBottomY = windowHeight - margin;
    if (bottomBarHeight > 0.0f)
    {
        contentBottomY -= (bottomBarHeight + margin);
    }

    float contentHeight = std::max(100.0f, contentBottomY - contentTopY);

    float contentLeftX = margin;
    if (leftSidebarWidth > 0.0f)
    {
        contentLeftX += leftSidebarWidth + margin;
    }

    float contentRightX = windowWidth - margin;
    if (rightSidebarWidth > 0.0f)
    {
        contentRightX -= (rightSidebarWidth + margin);
    }

    float centerWidth = std::max(100.0f, contentRightX - contentLeftX);

    for (const auto& p : m_panels)
    {
        PanelComputedBounds bounds;
        bounds.id = p.id;
        bounds.backgroundColor = p.backgroundColor;
        bounds.borderColor = p.borderColor;
        bounds.widgets = p.widgets;

        switch (p.anchor)
        {
            case PanelAnchor::TOP_BAR:
                bounds.rect = { margin, margin, windowWidth - (2.0f * margin), topBarHeight };
                break;

            case PanelAnchor::BOTTOM_BAR:
                bounds.rect = { margin, windowHeight - margin - bottomBarHeight, windowWidth - (2.0f * margin), bottomBarHeight };
                break;

            case PanelAnchor::LEFT_SIDEBAR:
                bounds.rect = { margin, contentTopY, leftSidebarWidth, contentHeight };
                break;

            case PanelAnchor::RIGHT_SIDEBAR:
                bounds.rect = { windowWidth - margin - rightSidebarWidth, contentTopY, rightSidebarWidth, contentHeight };
                break;

            case PanelAnchor::CENTER_FLEX:
            default:
                bounds.rect = { contentLeftX, contentTopY, centerWidth, contentHeight };
                break;
        }

        results.push_back(bounds);
    }

    return results;
}
