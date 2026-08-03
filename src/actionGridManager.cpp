#include "actionGridManager.h"
#include "game.h"
#include "state/inventoryState.h"
#include "state/explorationState.h"

static void pushBtn(game* g, const std::string& label, int slotIndex, std::function<void()> onClick = nullptr, bool isEnabled = true, bool pinnedAllPages = false)
{
    if (g->activeButtons.size() >= g->activeButtons.capacity()) return;

    actionButton btn;
    btn.label = label;
    btn.slotIndex = slotIndex;
    btn.onClick = onClick;
    btn.isEnabled = isEnabled;
    btn.pinnedAllPages = pinnedAllPages;
    g->activeButtons.push_back(btn);
}

static void addQuantityButtons(game* g, const std::string& verb, int totalCount, int startSlot, std::function<void(int)> onAction)
{
    struct QConfig { std::string label; int count; int slotOffset; };
    QConfig configs[] = {
        { verb + " (1)", 1, 0 },
        { verb + " (5)", 5, 1 },
        { verb + " (All)", totalCount, 2 }
    };

    for (const auto& cfg : configs)
    {
        pushBtn(g, cfg.label, startSlot + cfg.slotOffset, [onAction, cfg]() { onAction(cfg.count); }, (totalCount >= cfg.count));
    }
}

void ActionGridManager::refresh(game* g)
{
    g->activeButtons.clear();

    if (dynamic_cast<inventoryState*>(g->getActiveState())) buildInventoryActions(g);
    else if (dynamic_cast<explorationState*>(g->getActiveState())) buildExplorationActions(g);
}

void ActionGridManager::buildInventoryActions(game* g)
{
    pushBtn(g, "Close inventory", 14, [g]()
        {
            g->selectedInventoryIndex = -1;
            g->selectedEquipmentSlot = equipSlot::NONE;

            if (g->activeTargetNPC)
            {
                dialogueChoice fightSim;
                fightSim.nextSceneId = "ENCOUNTER_FIGHT";
                g->processChoice(fightSim);
            }
            else
            {
                g->changeState(std::make_unique<explorationState>());
            }
        }, true, true);

    if (g->selectedEquipmentSlot != equipSlot::NONE)
    {
        buildEquipmentActions(g);
    }
    else if (g->selectedInventorySide == 0 && g->selectedInventoryIndex >= 0 && g->Player)
    {
        buildPlayerItemActions(g);
    }
    else if (g->selectedInventorySide == 1 && g->selectedInventoryIndex >= 0)
    {
        buildRightInventoryActions(g);
    }
}

void ActionGridManager::buildEquipmentActions(game* g)
{
    entity* targetChar = (g->selectedInventorySide == 1 && g->activeTargetNPC) ? g->activeTargetNPC : g->Player;
    if (!targetChar || !targetChar->inventory.isEquipped(g->selectedEquipmentSlot)) return;

    auto unequipCb = [g, targetChar]()
        {
            if (targetChar->inventory.unequipItem(g->selectedEquipmentSlot))
            {
                g->selectedEquipmentSlot = equipSlot::NONE;
                g->refreshActionGrid();
            }
        };

    pushBtn(g, (targetChar == g->Player) ? "Drop" : "Take (1)", 0, unequipCb);
    pushBtn(g, "Dye", 3);
    pushBtn(g, "Enchant", 4);
    pushBtn(g, "Unequip", 5, unequipCb);
    pushBtn(g, "Pull down", 10);
    pushBtn(g, "Shift aside", 11);
}

void ActionGridManager::buildPlayerItemActions(game* g)
{
    InventorySlotInfo slotData = g->getInventorySlotItem(0, g->selectedInventoryIndex);
    if (!slotData.isValid) return;

    int idx = g->selectedInventoryIndex;
    auto selItem = slotData.itemPtr;

    addQuantityButtons(g, "Drop", slotData.count, 0, [g, idx](int qty) { g->handleDropAction(idx, qty); });
    pushBtn(g, "Enchant", 4);

    if (selItem->isEquippable)
    {
        auto stackedView = g->Player->inventory.getStackedView();
        int firstBackpackIndex = stackedView[idx].firstBackpackIndex;

        pushBtn(g, "Equip: " + g->formatEquipSlotName(selItem->targetSlot), 5, [g, firstBackpackIndex]()
            {
                g->handleEquipAction(firstBackpackIndex);
            });
    }
    else if (selItem->isConsumable)
    {
        pushBtn(g, "Eat (Self)", 5, [g, selItem]()
            {
                g->Player->inventory.removeItem(selItem->id, 1);
                eventBus::getInstance().publishEvent({ gameEvent::itemUsed, 1, selItem->id, g->Player });
                g->selectedInventoryIndex = -1;
                g->refreshActionGrid();
            });

        pushBtn(g, "Eat all (Self)", 6, [g, selItem, count = slotData.count]()
            {
                g->Player->inventory.removeItem(selItem->id, count);
                eventBus::getInstance().publishEvent({ gameEvent::itemUsed, count, selItem->id, g->Player });
                g->selectedInventoryIndex = -1;
                g->refreshActionGrid();
            }, (slotData.count >= 1));
    }
}

void ActionGridManager::buildRightInventoryActions(game* g)
{
    InventorySlotInfo slotData = g->getInventorySlotItem(1, g->selectedInventoryIndex);
    if (!slotData.isValid) return;

    auto selItem = slotData.itemPtr;
    int idx = g->selectedInventoryIndex;

    if (g->activeTargetNPC)
    {
        addQuantityButtons(g, "Take", slotData.count, 0, [g, selItem](int qty)
            {
                if (g->activeTargetNPC && g->activeTargetNPC->inventory.removeItem(selItem->id, qty))
                {
                    auto copy = std::make_shared<item>(*selItem); copy->count = qty;
                    g->Player->inventory.addItem(copy);
                    g->selectedInventoryIndex = -1; g->refreshActionGrid();
                }
            });

        if (selItem->isEquippable)
        {
            pushBtn(g, "Equip: " + g->formatEquipSlotName(selItem->targetSlot), 5, [g, selItem]()
                {
                    if (g->activeTargetNPC && g->activeTargetNPC->inventory.removeItem(selItem->id, 1))
                    {
                        auto copy = std::make_shared<item>(*selItem); copy->count = 1;
                        g->Player->inventory.addItem(copy);
                        g->handleEquipAction(static_cast<int>(g->Player->inventory.backpack.size()) - 1);
                    }
                });
        }
    }
    else
    {
        addQuantityButtons(g, "Take", slotData.count, 0, [g, idx](int qty) { g->handlePickupAction(idx, qty); });

        if (selItem->isEquippable)
        {
            pushBtn(g, "Equip: " + g->formatEquipSlotName(selItem->targetSlot), 5, [g, idx]()
                {
                    TileRuntimeData& tData = g->map->getRuntimeData(g->gridX, g->gridY);
                    if (idx < static_cast<int>(tData.droppedItems.size()))
                    {
                        auto groundItem = tData.droppedItems[idx];
                        tData.droppedItems.erase(tData.droppedItems.begin() + idx);
                        g->Player->inventory.addItem(groundItem);
                        g->handleEquipAction(static_cast<int>(g->Player->inventory.backpack.size()) - 1);
                    }
                });
        }
    }
}

void ActionGridManager::buildExplorationActions(game* g)
{
    for (const auto& trig : questDatabase::getTriggersForLocation(g->map->getId(), g->gridX, g->gridY))
    {
        if (g->checkConditions(trig.conditions))
        {
            std::string sId = trig.sceneId;
            pushBtn(g, trig.label, -1, [g, sId]() { g->loadScene(sId); });
        }
    }

    MapWarp w;
    if (g->map->checkWarp(g->gridX, g->gridY, w))
    {
        pushBtn(g, "Enter Door", -1, [g, w]() { g->loadMap(w.targetMap, w.targetX, w.targetY); });
    }
}