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

    static SDL_FRect getEquipmentSlotRect(const SDL_FRect& bounds, int col, int row, int cols, int rows, int slotPadding, int outerPadding)
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

    static SDL_FRect getInventorySlotRect(const SDL_FRect& bounds, int index, int cols, int rows)
    {
        float padding = 8.0f;
        float halfW = bounds.w / 2.0f;

        int side = index / (cols * rows); // 0 = left page, 1 = right page
        int localIdx = index % (cols * rows);
        int col = localIdx % cols;
        int row = localIdx / cols;

        float startX = bounds.x + (side * halfW) + padding;
        float startY = bounds.y + padding;
        float gridW = halfW - (2.0f * padding);
        float gridH = bounds.h - (2.0f * padding);

        float slotW = (gridW - (cols - 1) * padding) / cols;
        float slotH = (gridH - (rows - 1) * padding) / rows;

        return {
            startX + col * (slotW + padding),
            startY + row * (slotH + padding),
            slotW,
            slotH
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

    // 3. Calculate 6x6 Equipment Grid Slot Rectangle
    static inline SDL_FRect getEquipmentSlotRect(const SDL_Rect& equipRect, int col, int row,
        int cols = 6, int rows = 6,
        int slotGap = 4, int padding = 12)
    {
        int internalPadding = padding + 6;
        int availableW = equipRect.w - (2 * internalPadding) - ((cols - 1) * slotGap);
        int availableH = equipRect.h - (2 * internalPadding) - ((rows - 1) * slotGap);
        int slotSize = std::min(availableW / cols, availableH / rows);

        int gridW = (slotSize * cols) + (slotGap * (cols - 1));
        int gridH = (slotSize * rows) + (slotGap * (rows - 1));

        int offsetX = equipRect.x + (equipRect.w - gridW) / 2;
        int offsetY = equipRect.y + (equipRect.h - gridH) / 2;

        return SDL_FRect{
            (float)(offsetX + (col * (slotSize + slotGap))),
            (float)(offsetY + (row * (slotSize + slotGap))),
            (float)slotSize,
            (float)slotSize
        };
    }

    // 4. Calculate Inventory Slot Rectangle (6 Cols x 5 Rows x 2 Columns)
    static inline SDL_FRect getInventorySlotRect(const SDL_Rect& invRect, int slotIndex,
        int cols = 6, int rows = 5,
        int slotGap = 4, int sidePad = 20, int topPad = 60)
    {
        int halfWidth = invRect.w / 2;
        int availableW = halfWidth - (2 * sidePad) - ((cols - 1) * slotGap);
        int availableH = invRect.h - topPad - sidePad - ((rows - 1) * slotGap);
        int slotSize = std::min(availableW / cols, availableH / rows);

        int gridW = (slotSize * cols) + (slotGap * (cols - 1));
        int leftOffsetX = invRect.x + (halfWidth - gridW) / 2;
        int rightOffsetX = invRect.x + halfWidth + (halfWidth - gridW) / 2;
        int gridOffsetY = invRect.y + topPad;

        int maxSlotsPerSide = cols * rows;
        int side = slotIndex / maxSlotsPerSide;
        int localIndex = slotIndex % maxSlotsPerSide;
        int r = localIndex / cols;
        int c = localIndex % cols;
        float originX = (side == 0) ? (float)leftOffsetX : (float)rightOffsetX;

        return SDL_FRect{
            originX + (c * (slotSize + slotGap)),
            (float)(gridOffsetY + (r * (slotSize + slotGap))),
            (float)slotSize,
            (float)slotSize
        };
    }
};