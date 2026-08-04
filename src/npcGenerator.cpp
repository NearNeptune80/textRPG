#include "npcGenerator.h"
#include "entity.h"
#include "itemDatabase.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

std::unordered_map<std::string, NPCTemplate> npcGenerator::registry;

bool npcGenerator::loadTemplates(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json data;
        file >> data;
        registry.clear();

        for (const auto& tJson : data.at("templates"))
        {
            NPCTemplate tpl;
            tpl.id = tJson.at("id").get<std::string>();
            tpl.name = tJson.value("name", "Unknown NPC");
            tpl.levelMin = tJson.value("levelMin", 1);
            tpl.levelMax = tJson.value("levelMax", 1);

            if (tJson.contains("baseStats"))
            {
                for (auto& [key, val] : tJson["baseStats"].items())
                {
                    tpl.baseStats[key] = val.get<float>();
                }
            }

            tpl.tags = tJson.value("tags", std::vector<std::string>{});
            tpl.possibleRaces = tJson.value("possibleRaces", std::vector<std::string>{"Human"});
            tpl.guaranteedItems = tJson.value("guaranteedItems", std::vector<std::string>{});
            tpl.randomItems = tJson.value("randomItems", std::vector<std::string>{});

            registry[tpl.id] = tpl;
        }
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "NPC Template JSON Error (" << filePath << "): " << e.what() << "\n";
        return false;
    }
}

std::shared_ptr<entity> npcGenerator::generateFromTemplate(const std::string& templateId)
{
    auto it = registry.find(templateId);
    if (it == registry.end()) return nullptr;

    const auto& tpl = it->second;
    static int genCounter = 1;

    auto npc = std::make_shared<entity>("npc_gen_" + std::to_string(genCounter++), tpl.name);

    // Roll Level
    int lvlRange = (tpl.levelMax - tpl.levelMin) + 1;
    npc->stats.level = tpl.levelMin + (rand() % std::max(1, lvlRange));

    // Assign Base Stats
    for (const auto& [sName, val] : tpl.baseStats)
    {
        npc->stats.setBaseStat(sName, val);
    }

    // Assign Base Human/Race Anatomy Parts
    std::string race = tpl.possibleRaces.empty() ? "Human" : tpl.possibleRaces[rand() % tpl.possibleRaces.size()];
    
    bodyPart torso; torso.id = "part_torso_" + race; torso.name = "Torso"; torso.race = race;
    npc->anatomy.setPart(bodySlot::TORSO, torso);

    bodyPart head; head.id = "part_head_" + race; head.name = "Face"; head.race = race;
    npc->anatomy.setPart(bodySlot::HEAD, head);

    bodyPart legs; legs.id = "part_legs_" + race; legs.name = "Legs"; legs.race = race; legs.count = 2;
    npc->anatomy.setPart(bodySlot::LEGS, legs);

    // Add Guaranteed Items & Equip
    std::vector<std::string> bodyTags = npc->anatomy.getAllTags();
    for (const auto& itemId : tpl.guaranteedItems)
    {
        auto itemPtr = itemDatabase::getItem(itemId);
        if (itemPtr)
        {
            npc->inventory.addItem(itemPtr);
            if (itemPtr->isEquippable && itemPtr->targetSlot != equipSlot::NONE)
            {
                size_t backpackIdx = npc->inventory.backpack.size() - 1;
                npc->inventory.equipItem(backpackIdx, itemPtr->targetSlot, bodyTags);
            }
        }
    }

    // Add Random Items
    for (const auto& itemId : tpl.randomItems)
    {
        if ((rand() % 100) < 50)
        {
            auto itemPtr = itemDatabase::getItem(itemId);
            if (itemPtr) npc->inventory.addItem(itemPtr);
        }
    }

    return npc;
}

std::shared_ptr<entity> npcGenerator::generateRandomNPC()
{
    if (registry.empty()) return nullptr;

    auto it = registry.begin();
    std::advance(it, rand() % registry.size());
    return generateFromTemplate(it->first);
}