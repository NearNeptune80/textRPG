#pragma once
#include <vector>
#include "actionButton.h"

class game;

/**
 * Handles action button generation and slot placement for the bottom grid.
 */
class ActionGridManager
{
public:
    static void refresh(game* g);

private:
    static void buildInventoryActions(game* g);
    static void buildExplorationActions(game* g);

    static void buildEquipmentActions(game* g);
    static void buildPlayerItemActions(game* g);
    static void buildRightInventoryActions(game* g);
};