#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

/**
 * Single-source-of-truth helper utility for calculating exact widget bounds
 * and grid slot geometries. Used by both rendering logic and mouse hit-testing.
 */
class UIGridHelper
{
public:
    static bool contains(SDL_FRect rect, float x, float y)
    {
        return (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h);
    }

    static SDL_FRect getActionGridBounds(const SDL_FRect& bounds, float arrowWRatio = 0.03f, float padding = 8.0f)
    {
        float arrowW = bounds.w * arrowWRatio;
        float gridX = padding + arrowW + (padding * 0.5f);
        float gridW = bounds.w - (2.0f * (padding + arrowW + (padding * 0.5f)));
        return { gridX, 0.0f, gridW, bounds.h };
    }

    static SDL_FRect getEquipmentSlotRect(const SDL_FRect& bounds, int col, int row, int cols, int rows, float slotPadding, float outerPadding)
    {
        float contentX = bounds.x + outerPadding;
        float contentY = bounds.y + outerPadding;
        float contentW = bounds.w - (2.0f * outerPadding);
        float contentH = bounds.h - (2.0f * outerPadding);

        float slotW = (contentW - (cols - 1) * slotPadding) / cols;
        float slotH = (contentH - (rows - 1) * slotPadding) / rows;

        return {
            contentX + col * (slotW + slotPadding),
            contentY + row * (slotH + slotPadding),
            slotW,
            slotH
        };
    }

    /**
 * Calculates square layout geometry for twin inventory grids (Player left, Target right).
 */
    static SDL_FRect getInventorySlotRect(const SDL_FRect& bounds, int index, int cols = 6, int rows = 5, float headerHeight = 24.0f)
    {
        float padding = 6.0f;
        float tabW = 28.0f; // Square tab button width
        float halfW = bounds.w / 2.0f;

        int side = index / (cols * rows); // 0 = Left Grid, 1 = Right Grid
        int localIdx = index % (cols * rows);
        int col = localIdx % cols;
        int row = localIdx / cols;

        // Determine grid sub-region (accounting for left tab bar on Left side, right tab bar on Right side)
        float pageX = bounds.x + (side * halfW) + padding;
        if (side == 0) pageX += tabW + padding; // Push left grid past left tabs

        float pageW = halfW - (tabW + (3.0f * padding));
        float pageY = bounds.y + headerHeight + padding;
        float pageH = bounds.h - headerHeight - (2.0f * padding);

        float rawW = (pageW - (cols - 1) * padding) / cols;
        float rawH = (pageH - (rows - 1) * padding) / rows;
        float squareSize = std::min(rawW, rawH);

        float gridW = (squareSize * cols) + ((cols - 1) * padding);
        float gridH = (squareSize * rows) + ((rows - 1) * padding);

        float offsetX = pageX + (pageW - gridW) * 0.5f;
        float offsetY = pageY + (pageH - gridH) * 0.5f;

        return {
            offsetX + col * (squareSize + padding),
            offsetY + row * (squareSize + padding),
            squareSize,
            squareSize
        };
    }

    /**
 * Single-source-of-truth for square tab button geometry.
 */
    static SDL_FRect getInventoryTabRect(const SDL_FRect& bounds, int side, int tabIndex)
    {
        float padding = 6.0f;
        float tabW = 28.0f;
        float headerH = 24.0f;
        float halfW = bounds.w / 2.0f;

        float tabX = (side == 0)
            ? (bounds.x + padding)
            : (bounds.x + bounds.w - tabW - padding);

        float startY = bounds.y + headerH + padding;

        return {
            tabX,
            startY + tabIndex * (tabW + padding),
            tabW,
            tabW // Strictly 1:1 Square Tab
        };
    }

    static SDL_FRect getMapTileRect(const SDL_FRect& bounds, int col, int row, float padding)
    {
        float contentX = bounds.x + padding;
        float contentY = bounds.y + padding;
        float contentW = bounds.w - (2.0f * padding);
        float contentH = bounds.h - (2.0f * padding);

        float tileW = (contentW - (4.0f * 4.0f)) / 5.0f;
        float tileH = (contentH - (4.0f * 4.0f)) / 5.0f;

        return {
            contentX + col * (tileW + 4.0f),
            contentY + row * (tileH + 4.0f),
            tileW,
            tileH
        };
    }

    static SDL_FRect getActionButtonRect(const SDL_FRect& bounds, int col, int row, int cols, int rows, float padding = 8.0f)
    {
        float contentW = bounds.w - (2.0f * padding);
        float contentH = bounds.h - (2.0f * padding);

        float btnW = (contentW - (cols - 1) * padding) / cols;
        float btnH = (contentH - (rows - 1) * padding) / rows;

        return {
            bounds.x + padding + col * (btnW + padding),
            bounds.y + padding + row * (btnH + padding),
            btnW,
            btnH
        };
    }
};