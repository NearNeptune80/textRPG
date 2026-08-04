#pragma once

#include <vector>

#include "ui/actionButton.h"

class game;

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