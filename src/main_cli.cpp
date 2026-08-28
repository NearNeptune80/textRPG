#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "core/characterDescription.h"
#include "core/game.h"
#include "core/uiCommand.h"
#include "entities/npcGenerator.h"
#include "save/saveManager.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/eventState.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/sexState.h"

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

    if (auto sex = dynamic_cast<sexState*>(state))
    {
        std::cout << " [INTERACTIVE CYOA SEX ENGINE]\n";
        std::cout << " Partner: " << (sex->getPartner() ? sex->getPartner()->name : "Partner")
                  << " | Stance: " << sexStanceToString(sex->getStance()) << "\n";
        std::cout << " Player Arousal: " << sex->getPlayerArousal() << "/100"
                  << " | Partner Arousal: " << sex->getPartnerArousal() << "/100\n";
        std::cout << " Dominance: " << sex->getPlayerDominance()
                  << " (" << (sex->isPlayerDominant() ? "Dominant" : "Submissive") << ")\n\n";

        std::cout << " Narrative Log:\n" << sex->getNarrativeLog() << "\n\n";

        const auto& buttons = engine.getActiveActionButtons();
        if (!buttons.empty())
        {
            std::cout << " Sex Actions & Commands:\n";
            for (size_t i = 0; i < buttons.size(); ++i)
            {
                std::cout << "   [" << i << "] " << buttons[i].label << "\n";
            }
            std::cout << "\n";
        }
    }
    else if (dynamic_cast<eventState*>(state))
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
    else if (auto resState = dynamic_cast<encounterResolutionState*>(state))
    {
        std::cout << " [ENCOUNTER RESOLUTION HUB]\n";
        std::cout << " Log: " << resState->getResolutionLog() << "\n\n";

        const auto& records = resState->getDefeatedRecords();
        size_t selected = resState->getSelectedIndex();

        std::cout << " Defeated Enemies (" << records.size() << "):\n";
        for (size_t i = 0; i < records.size(); ++i)
        {
            const auto& rec = records[i];
            std::cout << "  " << (i == selected ? "-> " : "   ")
                      << "[" << i << "] " << (rec.npc ? rec.npc->name : "Enemy")
                      << " | Looted: " << (rec.isLooted ? "Yes" : "No")
                      << " | Stripped: " << (rec.isStripped ? "Yes" : "No")
                      << " | Sex: " << (rec.hadSex ? "Yes" : "No")
                      << " | Subjugated: " << (rec.isSubjugated ? "Yes" : "No")
                      << " | Released: " << (rec.isReleased ? "Yes" : "No") << "\n";
        }
        std::cout << " Resolution Commands: loot, strip, sex, subjugate, release, target <idx>, leave\n";
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
                      << "  <number>          - Select action/choice index\n"
                      << "  scene <sceneId>   - Load and start a dialogue/quest scene\n"
                      << "  spawn <template>  - Spawn NPC on current tile (e.g. tpl_alley_bandit)\n"
                      << "  startsex <tpl>    - Start interactive CYOA sex state with NPC\n"
                      << "  fight <tpl>       - Start turn-based combat with NPC\n"
                      << "  teleport <m> <x><y>- Warp to map (e.g. overworld 1 1)\n"
                      << "  stance <name>     - Change sex stance\n"
                      << "  endsex            - End current sex encounter\n"
                      << "  inv               - Toggle inventory state\n"
                      << "  equip <index>     - Equip item from backpack index\n"
                      << "  unequip <slot>    - Unequip slot\n"
                      << "  drop <index> <qty>- Drop item from backpack to ground\n"
                      << "  pickup <idx> <qty>- Pickup item from ground to backpack\n"
                      << "  time <minutes>    - Advance in-game time\n"
                      << "  stats             - Display full player stats and race percentages\n"
                      << "  desc / inspect    - Procedural head-to-toe character inspect\n"
                      << "  save <name>       - Save named game\n"
                      << "  load <name>       - Load named save\n"
                      << "  win / defeat      - Simulate combat outcome if in combat\n"
                      << "  loot / strip / sex / subjugate / release - Resolution hub sub-actions\n"
                      << "  leave             - Leave resolution hub\n"
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
        else if (cmd == "scene")
        {
            std::string sceneId;
            if (iss >> sceneId)
            {
                engine.loadScene(sceneId);
            }
            else
            {
                std::cout << "Usage: scene <sceneId>\n";
            }
        }
        else if (cmd == "spawn")
        {
            std::string tplId;
            if (iss >> tplId)
            {
                auto npc = npcGenerator::generateFromTemplate(tplId, &engine.settings);
                if (npc && engine.map)
                {
                    engine.map->getRuntimeData(engine.gridX, engine.gridY).persistentNPC = npc;
                    std::cout << "Spawned " << npc->name << " at [" << engine.gridX << ", " << engine.gridY << "].\n";
                    engine.refreshActionGrid();
                }
                else
                {
                    std::cout << "Template not found: " << tplId << "\n";
                }
            }
            else
            {
                std::cout << "Usage: spawn <templateId>\n";
            }
        }
        else if (cmd == "startsex")
        {
            std::string tplId;
            std::shared_ptr<entity> partner = nullptr;
            if (iss >> tplId)
            {
                partner = npcGenerator::generateFromTemplate(tplId, &engine.settings);
            }
            if (!partner)
            {
                partner = npcGenerator::generateRandomNPC(&engine.settings);
            }
            if (partner)
            {
                engine.changeState(std::make_unique<sexState>(partner, SexStance::MISSIONARY, 20.0f));
            }
        }
        else if (cmd == "fight")
        {
            std::string tplId;
            std::shared_ptr<entity> enemy = nullptr;
            if (iss >> tplId)
            {
                enemy = npcGenerator::generateFromTemplate(tplId, &engine.settings);
            }
            if (!enemy)
            {
                enemy = npcGenerator::generateRandomNPC(&engine.settings);
            }
            if (enemy)
            {
                std::vector<std::shared_ptr<entity>> playerParty = { engine.getPlayerShared() };
                std::vector<std::shared_ptr<entity>> enemyParty = { enemy };
                engine.changeState(std::make_unique<CombatState>(playerParty, enemyParty));
            }
        }
        else if (cmd == "teleport")
        {
            std::string mId;
            int x = 1, y = 1;
            if (iss >> mId >> x >> y)
            {
                engine.loadMap(mId, x, y);
            }
            else
            {
                std::cout << "Usage: teleport <mapId> <x> <y>\n";
            }
        }
        else if (isdigit(cmd[0]))
        {
            int choiceIdx = std::stoi(cmd);
            if (dynamic_cast<sexState*>(engine.getActiveState()))
            {
                engine.handleCommand({CommandType::EXECUTE_SEX_ACTION, choiceIdx, 0, ""});
            }
            else if (dynamic_cast<eventState*>(engine.getActiveState()))
            {
                engine.handleCommand({CommandType::SELECT_DIALOGUE_CHOICE, choiceIdx, 0, ""});
            }
            else
            {
                const auto& btns = engine.getActiveActionButtons();
                if (choiceIdx >= 0 && static_cast<size_t>(choiceIdx) < btns.size() && btns[choiceIdx].onClick)
                {
                    btns[choiceIdx].onClick();
                }
            }
        }
        else if (cmd == "stance")
        {
            std::string stanceStr;
            if (std::getline(iss, stanceStr))
            {
                if (!stanceStr.empty() && stanceStr[0] == ' ') stanceStr.erase(0, 1);
                SexStance st = stringToSexStance(stanceStr);
                engine.handleCommand({CommandType::CHANGE_SEX_STANCE, static_cast<int>(st), 0, ""});
            }
            else
            {
                std::cout << "Usage: stance <Missionary|From Behind|Kneeling|Standing|Lap Sitting>\n";
            }
        }
        else if (cmd == "endsex")
        {
            engine.handleCommand({CommandType::END_SEX_SCENE, 0, 0, ""});
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
        else if (cmd == "loot")
        {
            engine.handleCommand({CommandType::LOOT_ENEMY, 0, 0, ""});
        }
        else if (cmd == "strip")
        {
            engine.handleCommand({CommandType::STRIP_ENEMY, 0, 0, ""});
        }
        else if (cmd == "sex")
        {
            engine.handleCommand({CommandType::INTERACTIVE_SEX, 0, 0, ""});
        }
        else if (cmd == "subjugate")
        {
            engine.handleCommand({CommandType::SUBJUGATE_ENEMY, 0, 0, ""});
        }
        else if (cmd == "release")
        {
            engine.handleCommand({CommandType::RELEASE_ENEMY, 0, 0, ""});
        }
        else if (cmd == "target")
        {
            int idx;
            if (iss >> idx)
            {
                engine.handleCommand({CommandType::SELECT_RESOLUTION_TARGET, idx, 0, ""});
            }
            else
            {
                std::cout << "Usage: target <index>\n";
            }
        }
        else if (cmd == "leave")
        {
            engine.handleCommand({CommandType::CLOSE_MENU, 0, 0, ""});
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