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

void layoutEngine::loadDefaultLayout()
{
    m_layoutName = "Default Responsive 5-Pane";
    m_globalMargin = 6.0f;
    m_panels.clear();

    // 1. Top Header Bar (Full width, fixed 38px height)
    PanelConfig topBar;
    topBar.id = "top_bar";
    topBar.anchor = PanelAnchor::TOP_BAR;
    topBar.fixedHeight = 38.0f;
    topBar.backgroundColor = "bgHeader";
    topBar.borderColor = "borderNormal";
    topBar.widgets = { "TOP_STATUS_BAR" };
    m_panels.push_back(topBar);

    // 2. Bottom Action Grid (Full width, fixed 140px height)
    PanelConfig bottomGrid;
    bottomGrid.id = "bottom_action_grid";
    bottomGrid.anchor = PanelAnchor::BOTTOM_BAR;
    bottomGrid.fixedHeight = 140.0f;
    bottomGrid.backgroundColor = "bgPanel";
    bottomGrid.borderColor = "borderNormal";
    bottomGrid.widgets = { "ACTION_GRID" };
    m_panels.push_back(bottomGrid);

    // 3. Left Character & Stats Sidebar (Fixed 320px width)
    PanelConfig leftPane;
    leftPane.id = "left_pane";
    leftPane.anchor = PanelAnchor::LEFT_SIDEBAR;
    leftPane.fixedWidth = 320.0f;
    leftPane.backgroundColor = "bgPanel";
    leftPane.borderColor = "borderNormal";
    leftPane.widgets = { "CHARACTER_OVERVIEW", "STAT_BARS", "PAPERDOLL_SOCKETS" };
    m_panels.push_back(leftPane);

    // 4. Right Target & Minimap Sidebar (Fixed 300px width)
    PanelConfig rightPane;
    rightPane.id = "right_pane";
    rightPane.anchor = PanelAnchor::RIGHT_SIDEBAR;
    rightPane.fixedWidth = 300.0f;
    rightPane.backgroundColor = "bgPanel";
    rightPane.borderColor = "borderNormal";
    rightPane.widgets = { "MINIMAP_RADAR", "TARGET_INSPECTOR" };
    m_panels.push_back(rightPane);

    // 5. Center Dynamic View (Auto-Flex: Takes all remaining width and height)
    PanelConfig centerPane;
    centerPane.id = "center_pane";
    centerPane.anchor = PanelAnchor::CENTER_FLEX;
    centerPane.backgroundColor = "bgPanel";
    centerPane.borderColor = "borderNormal";
    centerPane.widgets = { "SCENE_NARRATIVE", "INTERACTIVE_SEX", "COMBAT_VIEW", "BACKPACK_INVENTORY", "RESOLUTION_HUB" };
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
        }
        return true;
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

std::vector<PanelComputedBounds> layoutEngine::computeLayout(float windowWidth, float windowHeight, float uiScale) const
{
    std::vector<PanelComputedBounds> results;
    float margin = m_globalMargin * uiScale;

    float topBarHeight = 0.0f;
    float bottomBarHeight = 0.0f;
    float leftSidebarWidth = 0.0f;
    float rightSidebarWidth = 0.0f;

    // Step 1: Query dimensions of fixed/anchored bars
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

    // Step 2: Compute bounding rectangles for every panel
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
                bounds.rect = { contentLeftX, contentTopY, centerWidth, contentHeight };
                break;

            case PanelAnchor::FLOATING_RECT:
            default:
                bounds.rect = { margin, contentTopY, centerWidth, contentHeight };
                break;
        }

        results.push_back(bounds);
    }

    return results;
}
