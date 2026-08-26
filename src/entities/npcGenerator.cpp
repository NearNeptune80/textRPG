#include "entities/npcGenerator.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "entities/entity.h"
#include "items/itemDatabase.h"

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

void npcGenerator::applyDemographicConfiguration(entity* npc, const GameSettings* settings)
{
    if (!npc) return;

    DemographicSettings defaultDemo;
    const DemographicSettings& demo = settings ? settings->demographics : defaultDemo;

    float rollSex = static_cast<float>(rand() % 1000) / 1000.0f;
    float rollArch = static_cast<float>(rand() % 1000) / 1000.0f;

    npc->orientation = demo.rollSexuality(rollSex);
    npc->genderArchetype = demo.rollGenderArchetype(rollArch);

    std::string race = npc->anatomy.getDominantRace();
    if (race.empty()) race = "Human";

    // Build anatomical parts matching gender archetype
    bodyPart torso; torso.id = "part_torso_" + race; torso.name = "Torso"; torso.race = race; torso.primaryColor = "Fair"; torso.covering = CoveringType::SKIN;
    bodyPart head; head.id = "part_head_" + race; head.name = "Face"; head.race = race; head.primaryColor = "Fair"; head.covering = CoveringType::SKIN;
    bodyPart legs; legs.id = "part_legs_" + race; legs.name = "Legs"; legs.race = race; legs.count = 2; legs.covering = CoveringType::SKIN;
    bodyPart feet; feet.id = "part_feet_" + race; feet.name = "Feet"; feet.race = race; feet.count = 2; feet.covering = CoveringType::SKIN;
    bodyPart arms; arms.id = "part_arms_" + race; arms.name = "Arms"; arms.race = race; arms.count = 2; arms.covering = CoveringType::SKIN;
    bodyPart hair; hair.id = "part_hair_" + race; hair.name = "Hair"; hair.race = race; hair.primaryColor = "Brown"; hair.covering = CoveringType::HAIR_COVERING;
    bodyPart eyes; eyes.id = "part_eyes_" + race; eyes.name = "Eyes"; eyes.race = race; eyes.primaryColor = "Blue"; eyes.covering = CoveringType::IRIS;

    bodyPart breasts; breasts.id = "part_breasts_" + race; breasts.name = "Breasts"; breasts.race = race;
    bodyPart groin; groin.id = "part_groin_" + race; groin.name = "Groin"; groin.race = race;
    bodyPart ass; ass.id = "part_ass_" + race; ass.name = "Ass"; ass.race = race;
    ass.orifice.exists = true; ass.orifice.elasticity = 60.0f; ass.orifice.maxCapacityMl = 80.0f;

    switch (npc->genderArchetype)
    {
        case GenderArchetype::MALE:
            breasts.cupSize = 0;
            groin.name = "Penis"; groin.length = 15.0f; groin.diameter = 3.5f;
            groin.currentFluidMl = 10.0f; groin.maxFluidMl = 20.0f; groin.fluidRegenPerHour = 2.0f;
            groin.tags.push_back("penis"); groin.tags.push_back("has_penis");
            torso.tags.push_back("masculine");
            break;

        case GenderArchetype::FEMALE:
            breasts.cupSize = 3; // C cup
            groin.name = "Vagina"; groin.orifice.exists = true; groin.orifice.elasticity = 75.0f; groin.orifice.maxCapacityMl = 120.0f;
            groin.tags.push_back("vagina"); groin.tags.push_back("has_vagina");
            torso.tags.push_back("feminine");
            break;

        case GenderArchetype::HERMAPHRODITE:
            breasts.cupSize = 3;
            groin.name = "Hermaphrodite Genitals"; groin.length = 14.0f; groin.diameter = 3.2f;
            groin.currentFluidMl = 10.0f; groin.maxFluidMl = 20.0f; groin.fluidRegenPerHour = 2.0f;
            groin.orifice.exists = true; groin.orifice.elasticity = 75.0f; groin.orifice.maxCapacityMl = 120.0f;
            groin.tags.push_back("penis"); groin.tags.push_back("vagina"); groin.tags.push_back("hermaphrodite");
            torso.tags.push_back("feminine"); torso.tags.push_back("masculine");
            break;

        case GenderArchetype::GYNOMORPH:
            breasts.cupSize = 4; // D cup
            groin.name = "Penis"; groin.length = 16.0f; groin.diameter = 3.6f;
            groin.currentFluidMl = 12.0f; groin.maxFluidMl = 25.0f; groin.fluidRegenPerHour = 2.5f;
            groin.tags.push_back("penis"); groin.tags.push_back("gynomorph");
            torso.tags.push_back("feminine");
            break;

        case GenderArchetype::ANDROMORPH:
            breasts.cupSize = 0;
            groin.name = "Vagina"; groin.orifice.exists = true; groin.orifice.elasticity = 70.0f; groin.orifice.maxCapacityMl = 100.0f;
            groin.tags.push_back("vagina"); groin.tags.push_back("andromorph");
            torso.tags.push_back("masculine");
            break;

        case GenderArchetype::ASEXUAL_NULL:
        default:
            breasts.cupSize = 0;
            groin.name = "Smooth Groin";
            groin.tags.push_back("null");
            break;
    }

    npc->anatomy.setPart(bodySlot::HEAD, head);
    npc->anatomy.setPart(bodySlot::HAIR, hair);
    npc->anatomy.setPart(bodySlot::EYES, eyes);
    npc->anatomy.setPart(bodySlot::TORSO, torso);
    npc->anatomy.setPart(bodySlot::BREASTS, breasts);
    npc->anatomy.setPart(bodySlot::ARMS, arms);
    npc->anatomy.setPart(bodySlot::GROIN, groin);
    npc->anatomy.setPart(bodySlot::ASS, ass);
    npc->anatomy.setPart(bodySlot::LEGS, legs);
    npc->anatomy.setPart(bodySlot::FEET, feet);
}

std::shared_ptr<entity> npcGenerator::generateFromTemplate(const std::string& templateId, const GameSettings* settings)
{
    auto it = registry.find(templateId);
    if (it == registry.end()) return nullptr;

    const auto& tpl = it->second;
    static int genCounter = 1;

    auto npc = std::make_shared<entity>("npc_gen_" + std::to_string(genCounter++), tpl.name);

    int lvlRange = (tpl.levelMax - tpl.levelMin) + 1;
    npc->stats.level = tpl.levelMin + (rand() % std::max(1, lvlRange));

    for (const auto& [sName, val] : tpl.baseStats)
    {
        npc->stats.setBaseStat(sName, val);
    }

    std::string race = tpl.possibleRaces.empty() ? "Human" : tpl.possibleRaces[rand() % tpl.possibleRaces.size()];
    bodyPart torso; torso.id = "part_torso_" + race; torso.name = "Torso"; torso.race = race;
    npc->anatomy.setPart(bodySlot::TORSO, torso);

    // Apply demographic settings (Sexuality & Gender Archetype)
    applyDemographicConfiguration(npc.get(), settings);

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

std::shared_ptr<entity> npcGenerator::generateRandomNPC(const GameSettings* settings)
{
    if (registry.empty()) return nullptr;

    auto it = registry.begin();
    std::advance(it, rand() % registry.size());
    return generateFromTemplate(it->first, settings);
}