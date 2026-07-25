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
    // Helper to test if a point (x, y) resides inside an SDL_FRect
    static inline bool contains(const SDL_FRect& rect, float x, float y)
    {
        return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
    }

    // Helper to test if a point (x, y) resides inside an SDL_Rect
    static inline bool contains(const SDL_Rect& rect, float x, float y)
    {
        return (x >= (float)rect.x && x <= (float)(rect.x + rect.w) &&
            y >= (float)rect.y && y <= (float)(rect.y + rect.h));
    }

    // 1. Calculate 5x5 Map Tile Bounding Rectangle
    static inline SDL_FRect getMapTileRect(const SDL_Rect& mapRect, int gridCol, int gridRow, int padding = 12)
    {
        int tileGap = 2;
        int availableForTiles = mapRect.w - (2 * padding) - (4 * tileGap);
        int drawnTileSize = availableForTiles / 5;
        int totalGridSize = (drawnTileSize * 5) + (tileGap * 4);

        int offsetX = mapRect.x + (mapRect.w - totalGridSize) / 2;
        int offsetY = mapRect.y + (mapRect.h - totalGridSize) / 2;

        return SDL_FRect{
            (float)(offsetX + (gridCol * (drawnTileSize + tileGap))),
            (float)(offsetY + (gridRow * (drawnTileSize + tileGap))),
            (float)drawnTileSize,
            (float)drawnTileSize
        };
    }

    // 2. Calculate Action Grid Button Bounding Rectangle (5 Cols x 3 Rows)
    static inline SDL_FRect getActionButtonRect(const SDL_FRect& actionRect, int col, int row,
        int cols = 5, int rows = 3,
        float gap = 8.0f, float vertPad = 15.0f, float horizPad = 40.0f)
    {
        float availableW = actionRect.w - (horizPad * 2.0f) - (gap * (cols - 1));
        float availableH = actionRect.h - (vertPad * 2.0f) - (gap * (rows - 1));
        float btnW = availableW / (float)cols;
        float btnH = availableH / (float)rows;

        return SDL_FRect{
            actionRect.x + horizPad + (col * (btnW + gap)),
            actionRect.y + vertPad + (row * (btnH + gap)),
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