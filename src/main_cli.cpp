#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "core/characterDescription.h"
#include "core/game.h"
#include "core/uiCommand.h"
#include "save/saveManager.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/eventState.h"
#include "state/combatState.h"

void printHeader(const game& engine)
{
    std::cout << "\n======================================================================\n";
    std::cout << " [textRPG Headless CLI Test Harness]\n";
    std::cout << " Time: " << engine.getTime().getFormattedTime() << " (" << engine.getTime().getFormattedDate() << ")\n";
    
    if (const entity* p = engine.getPlayer())
    {
        std::cout << " Player: " << p->name << " | Race: " << p->anatomy.getDominantRace() << "\n";
        std::cout << " HP: " << p->getStat("health")
                  << " | Mana: " << p->getStat("mana")
                  << " | Lust: " << p->getStat("lust")
                  << " | Gold: " << p->getStat("currency") << "¤\n";
    }

    if (const gameMap* m = engine.getActiveMap())
    {
        std::cout << " Map: " << m->getId() << " (" << m->getName() << ") at [" << engine.gridX << ", " << engine.gridY << "]\n";
    }
    std::cout << "----------------------------------------------------------------------\n";
}

void printStateDetails(game& engine)
{
    iGameState* state = engine.getActiveState();
    if (!state)
    {
        std::cout << " [State: None]\n";
        return;
    }

    if (dynamic_cast<eventState*>(state))
    {
        const questScene& scene = engine.getCurrentScene();
        std::cout << " [SCENE: " << scene.id << "]\n";
        if (!scene.speakerName.empty())
        {
            std::cout << " <" << scene.speakerName << ">: ";
        }
        std::cout << scene.bodyText << "\n\n Choices:\n";
        for (size_t i = 0; i < scene.choices.size(); ++i)
        {
            std::cout << "   [" << i << "] " << scene.choices[i].label << "\n";
        }
    }
    else if (dynamic_cast<inventoryState*>(state))
    {
        std::cout << " [INVENTORY VIEW]\n";
        auto backpack = engine.getPlayerInventoryStacked();
        std::cout << " Backpack (" << backpack.size() << " items):\n";
        for (size_t i = 0; i < backpack.size(); ++i)
        {
            if (backpack[i].itemPtr)
            {
                std::cout << "   [" << i << "] " << backpack[i].itemPtr->name 
                          << " (x" << backpack[i].totalCount << ")"
                          << (backpack[i].itemPtr->isEquippable ? " [Equippable]" : "") << "\n";
            }
        }

        auto ground = engine.getTileInventoryStacked();
        std::cout << " Ground Items (" << ground.size() << " items):\n";
        for (size_t i = 0; i < ground.size(); ++i)
        {
            if (ground[i].itemPtr)
            {
                std::cout << "   [" << i << "] " << ground[i].itemPtr->name 
                          << " (x" << ground[i].totalCount << ")\n";
            }
        }
    }
    else if (dynamic_cast<explorationState*>(state))
    {
        std::cout << " [EXPLORATION]\n";
        const auto& buttons = engine.getActiveActionButtons();
        if (!buttons.empty())
        {
            std::cout << " Actions:\n";
            for (size_t i = 0; i < buttons.size(); ++i)
            {
                std::cout << "   (" << buttons[i].label << ")";
            }
            std::cout << "\n";
        }
        std::cout << " Movement: Use 'w', 'a', 's', 'd' or 'move <x> <y>'\n";
    }
    else
    {
        std::cout << " [STATE: Active]\n";
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Booting textRPG Headless CLI Test Harness...\n";

    game engine;
    engine.init();

    std::cout << "Engine initialized successfully. Type 'help' for command list.\n";

    std::string line;
    while (engine.isRunning)
    {
        printHeader(engine);
        printStateDetails(engine);

        std::cout << "\nCLI> ";
        if (!std::getline(std::cin, line))
        {
            break;
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "quit" || cmd == "exit")
        {
            engine.isRunning = false;
            break;
        }
        else if (cmd == "help")
        {
            std::cout << "\nAvailable Commands:\n"
                      << "  w, a, s, d        - Move player on the grid\n"
                      << "  move <x> <y>      - Move to specific coordinate\n"
                      << "  <number>          - Select dialogue choice index\n"
                      << "  inv               - Toggle inventory state\n"
                      << "  equip <index>     - Equip item from backpack index\n"
                      << "  unequip <slot>    - Unequip slot (e.g., TORSO_OVER, WEAPON_MAIN)\n"
                      << "  drop <index> <qty>- Drop item from backpack to ground\n"
                      << "  pickup <idx> <qty>- Pickup item from ground to backpack\n"
                      << "  time <minutes>    - Advance in-game time\n"
                      << "  stats             - Display full player stats and race percentages\n"
                      << "  save <name>       - Save named game\n"
                      << "  load <name>       - Load named save\n"
                      << "  win / defeat      - Simulate combat outcome if in combat\n"
                      << "  quit              - Exit test harness\n";
        }
        else if (cmd == "w")
        {
            engine.handleCommand({CommandType::MOVE_PLAYER, engine.gridX, engine.gridY - 1, ""});
        }
        else if (cmd == "s")
        {
            engine.handleCommand({CommandType::MOVE_PLAYER, engine.gridX, engine.gridY + 1, ""});
        }
        else if (cmd == "a")
        {
            engine.handleCommand({CommandType::MOVE_PLAYER, engine.gridX - 1, engine.gridY, ""});
        }
        else if (cmd == "d")
        {
            engine.handleCommand({CommandType::MOVE_PLAYER, engine.gridX + 1, engine.gridY, ""});
        }
        else if (cmd == "move")
        {
            int x, y;
            if (iss >> x >> y)
            {
                engine.handleCommand({CommandType::MOVE_PLAYER, x, y, ""});
            }
            else
            {
                std::cout << "Usage: move <x> <y>\n";
            }
        }
        else if (isdigit(cmd[0]))
        {
            int choiceIdx = std::stoi(cmd);
            engine.handleCommand({CommandType::SELECT_DIALOGUE_CHOICE, choiceIdx, 0, ""});
        }
        else if (cmd == "inv")
        {
            if (dynamic_cast<inventoryState*>(engine.getActiveState()))
            {
                engine.handleCommand({CommandType::CLOSE_MENU, 0, 0, ""});
            }
            else
            {
                engine.changeState(std::make_unique<inventoryState>());
            }
        }
        else if (cmd == "equip")
        {
            int idx;
            if (iss >> idx)
            {
                engine.handleEquipAction(idx);
            }
            else
            {
                std::cout << "Usage: equip <backpack_index>\n";
            }
        }
        else if (cmd == "drop")
        {
            int idx, qty = 1;
            if (iss >> idx)
            {
                iss >> qty;
                engine.handleDropAction(idx, qty);
            }
            else
            {
                std::cout << "Usage: drop <backpack_index> [quantity]\n";
            }
        }
        else if (cmd == "pickup")
        {
            int idx, qty = 1;
            if (iss >> idx)
            {
                iss >> qty;
                engine.handlePickupAction(idx, qty);
            }
            else
            {
                std::cout << "Usage: pickup <ground_index> [quantity]\n";
            }
        }
        else if (cmd == "time")
        {
            int mins;
            if (iss >> mins)
            {
                engine.gameTime.advanceTime(mins);
                std::cout << "Advanced time by " << mins << " minutes.\n";
            }
            else
            {
                std::cout << "Usage: time <minutes>\n";
            }
        }
        else if (cmd == "desc" || cmd == "inspect")
        {
            if (const entity* p = engine.getPlayer())
            {
                std::cout << "\n================ [CHARACTER INSPECT] ================\n"
                          << characterDescription::generateFullDescription(p)
                          << "======================================================\n";
            }
        }
        else if (cmd == "stats")
        {
            if (const entity* p = engine.getPlayer())
            {
                std::cout << "\n--- Character Stats ---\n";
                for (const auto& [stat, val] : p->stats.getAllBaseStats())
                {
                    std::cout << "  " << stat << ": " << val << " (effective: " << p->getStat(stat) << ")\n";
                }
                std::cout << "\n--- Racial Composition ---\n";
                std::cout << " Archetype: " << genderArchetypeToString(p->anatomy.getGenderArchetype()) << "\n";
                std::cout << " Title: " << p->anatomy.getRacialTitle() << "\n";
                for (const auto& [race, pct] : p->anatomy.calculateRacePercentages())
                {
                    std::cout << "  " << race << ": " << pct << "%\n";
                }
            }
        }
        else if (cmd == "save")
        {
            std::string saveName;
            if (iss >> saveName)
            {
                if (saveManager::saveNamedGame(&engine, saveName))
                {
                    std::cout << "Saved game as: " << saveName << "\n";
                }
                else
                {
                    std::cout << "Failed to save game.\n";
                }
            }
            else
            {
                std::cout << "Usage: save <saveName>\n";
            }
        }
        else if (cmd == "load")
        {
            std::string saveName;
            if (iss >> saveName)
            {
                std::string filename = saveName;
                if (filename.find(".json") == std::string::npos)
                {
                    std::string charName = (engine.getPlayer() && !engine.getPlayer()->name.empty()) ? engine.getPlayer()->name : "Hero";
                    filename = charName + "_" + saveName + ".json";
                }
                if (saveManager::loadFromFile(&engine, filename))
                {
                    std::cout << "Loaded game from: " << filename << "\n";
                }
                else
                {
                    std::cout << "Failed to load save file: " << filename << "\n";
                }
            }
            else
            {
                std::cout << "Usage: load <saveName>\n";
            }
        }
        else if (cmd == "win")
        {
            engine.handleCommand({CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "WIN"});
        }
        else if (cmd == "defeat")
        {
            engine.handleCommand({CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DEFEAT"});
        }
        else
        {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for command list.\n";
        }
    }

    engine.clean();
    std::cout << "textRPG Headless CLI terminated cleanly.\n";
    return 0;
}
