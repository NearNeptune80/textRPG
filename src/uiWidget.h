#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <utility>

/**
 * Shared layout calculation helpers for UI grids and components.
 */
class UIGridHelper
{
public:
    static bool contains(SDL_FRect rect, float x, float y)
    {
        return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
    }

    static std::pair<SDL_FRect, SDL_FRect> getNavigationArrows(SDL_FRect bounds, float padding = 8.0f)
    {
        float arrowW = bounds.w * 0.03f;
        float arrowH = bounds.h - (2.0f * padding);
        SDL_FRect left = { bounds.x + padding, bounds.y + padding, arrowW, arrowH };
        SDL_FRect right = { bounds.x + bounds.w - arrowW - padding, bounds.y + padding, arrowW, arrowH };
        return { left, right };
    }

    static SDL_FRect getActionGridBounds(SDL_FRect bounds, float padding = 8.0f)
    {
        float arrowW = bounds.w * 0.03f;
        float gridX = bounds.x + padding + arrowW + (padding * 0.5f);
        float gridW = bounds.w - (2.0f * (padding + arrowW + (padding * 0.5f)));
        return { gridX, bounds.y, gridW, bounds.h };
    }

    static SDL_FRect getActionButtonRect(SDL_FRect gridBounds, int col, int row, int cols, int rows, float padding = 8.0f)
    {
        float cellW = (gridBounds.w - (padding * (cols + 1))) / cols;
        float cellH = (gridBounds.h - (padding * (rows + 1))) / rows;
        return {
            gridBounds.x + padding + col * (cellW + padding),
            gridBounds.y + padding + row * (cellH + padding),
            cellW, cellH
        };
    }

    static SDL_FRect getMapTileRect(SDL_FRect bounds, int col, int row, float padding = 12.0f, float gap = 4.0f)
    {
        float availableW = bounds.w - (padding * 2.0f);
        float availableH = bounds.h - (padding * 2.0f);
        float tileSize = (std::min(availableW, availableH) - (gap * 4.0f)) / 5.0f;

        float gridTotalW = (tileSize * 5.0f) + (gap * 4.0f);
        float gridTotalH = (tileSize * 5.0f) + (gap * 4.0f);

        float startX = bounds.x + (bounds.w - gridTotalW) * 0.5f;
        float startY = bounds.y + (bounds.h - gridTotalH) * 0.5f;

        return { startX + col * (tileSize + gap), startY + row * (tileSize + gap), tileSize, tileSize };
    }

    static SDL_FRect getEquipmentSlotRect(SDL_FRect bounds, int col, int row, int cols, int rows, float slotGap = 4.0f, float padding = 12.0f)
    {
        float cellW = (bounds.w - (2.0f * padding) - ((cols - 1) * slotGap)) / cols;
        float cellH = (bounds.h - (2.0f * padding) - ((rows - 1) * slotGap)) / rows;
        return {
            bounds.x + padding + col * (cellW + slotGap),
            bounds.y + padding + row * (cellH + slotGap),
            cellW, cellH
        };
    }

    // Side-aligned vertical tabs (Player = Left column, Storage/NPC = Right column)
    static SDL_FRect getInventoryTabRect(SDL_FRect bounds, int side, int tabIndex)
    {
        float halfW = bounds.w * 0.5f;
        float tabW = bounds.w * 0.045f;
        float startY = bounds.h * 0.12f;
        float availH = bounds.h - startY - 12.0f;
        float tabH = (availH - (6.0f * 3.0f)) / 7.0f;

        float tabX = (side == 0)
            ? (bounds.x + 8.0f)                                  // Left side for Player
            : (bounds.x + halfW + halfW - tabW - 8.0f);          // Right side for NPC/Storage

        return { tabX, startY + (tabIndex * (tabH + 3.0f)), tabW, tabH };
    }

    static SDL_FRect getInventorySlotRect(SDL_FRect bounds, int gridSlotIndex, int cols, int rows)
    {
        float halfW = bounds.w * 0.5f;
        int side = gridSlotIndex / (cols * rows);
        int localIdx = gridSlotIndex % (cols * rows);
        int col = localIdx % cols;
        int row = localIdx / cols;

        float tabMargin = bounds.w * 0.052f;
        float startX = (side == 0) ? (bounds.x + tabMargin + 6.0f) : (bounds.x + (side * halfW) + 8.0f);
        float startY = bounds.h * 0.12f;

        float availW = halfW - tabMargin - 14.0f;
        float availH = bounds.h - startY - 12.0f;

        float gap = 4.0f;
        float maxCellW = (availW - ((cols - 1) * gap)) / cols;
        float maxCellH = (availH - ((rows - 1) * gap)) / rows;

        float squareSize = std::min(maxCellW, maxCellH);

        float offsetX = (availW - ((cols * squareSize) + ((cols - 1) * gap))) * 0.5f;
        float offsetY = (availH - ((rows * squareSize) + ((rows - 1) * gap))) * 0.5f;

        return {
            startX + offsetX + col * (squareSize + gap),
            startY + offsetY + row * (squareSize + gap),
            squareSize, squareSize
        };
    }
};