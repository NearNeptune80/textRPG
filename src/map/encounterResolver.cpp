#include "map/encounterResolver.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>

#include "common/randomEngine.h"
#include "core/game.h"
#include "entities/entity.h"
#include "entities/npcGenerator.h"

bool encounterResolver::shouldTriggerEncounter(int dangerLevel, TimePhase phase, float playerStealth)
{
    if (dangerLevel <= 0) return false;

    float baseChance = static_cast<float>(dangerLevel) * 15.0f;

    if (phase == TimePhase::NIGHT) baseChance += 15.0f;
    else if (phase == TimePhase::DUSK) baseChance += 5.0f;

    baseChance -= (playerStealth * 0.5f);

    float finalChance = std::clamp(baseChance, 5.0f, 85.0f);
    return dice::rollPercent(finalChance);
}

std::shared_ptr<entity> encounterResolver::createEncounterNPC(int dangerLevel, const GameSettings& settings)
{
    auto npc = npcGenerator::generateRandomNPC(&settings);
    if (!npc)
    {
        npc = std::make_shared<entity>("npc_ambush", "Shadowy Ambush");
    }

    npc->stats.level = std::max(1, npc->stats.level + (dangerLevel - 1));
    float hpScale = 50.0f + (dangerLevel * 15.0f);
    npc->stats.setBaseStat("health", hpScale);
    npc->stats.setBaseStat("physique", 10.0f + (dangerLevel * 2.0f));
    npc->stats.setBaseStat("currency", 20.0f + (dangerLevel * 10.0f));

    return npc;
}

static std::map<std::string, std::string> loadFlavorTexts()
{
    std::map<std::string, std::string> flavors = {
        { "forest", "You overwhelm {}, sending them stumbling backward into the dense undergrowth. They yield, unable to continue fighting." },
        { "house", "You corner {} inside the wooden room. Disarmed and panting, they drop their weapon in submission." },
        { "default", "You stand victorious over {}. They are defeated and at your mercy." }
    };

    std::ifstream file("data/scenes/encounters.json");
    if (file.is_open())
    {
        try
        {
            nlohmann::json j;
            file >> j;
            if (j.contains("flavor_texts") && j["flavor_texts"].is_object())
            {
                for (auto& [key, val] : j["flavor_texts"].items())
                {
                    if (val.is_string()) flavors[key] = val.get<std::string>();
                }
            }
        }
        catch (const std::exception& e)
        {
            // Fall back to default flavor texts
        }
    }
    return flavors;
}

questScene encounterResolver::buildEncounterScene(game* g, std::shared_ptr<entity> npc)
{
    questScene scene;
    if (!npc) return scene;

    if (questDatabase::exists("scene_procedural_encounter"))
    {
        scene = questDatabase::getScene("scene_procedural_encounter");
    }
    else
    {
        scene.id = "scene_procedural_encounter";
        dialogueChoice fightChoice;
        fightChoice.label = "Fight";
        fightChoice.nextSceneId = "ENCOUNTER_FIGHT";
        scene.choices.push_back(fightChoice);

        dialogueChoice payChoice;
        payChoice.label = "Bribe (25¤)";
        payChoice.nextSceneId = "ENCOUNTER_BRIBE";
        scene.choices.push_back(payChoice);

        dialogueChoice fleeChoice;
        fleeChoice.label = "Evade / Flee";
        fleeChoice.nextSceneId = "ENCOUNTER_FLEE";
        scene.choices.push_back(fleeChoice);

        dialogueChoice surrenderChoice;
        surrenderChoice.label = "Surrender";
        surrenderChoice.nextSceneId = "ENCOUNTER_SURRENDER";
        scene.choices.push_back(surrenderChoice);
    }

    scene.speakerName = npc->name;
    std::string archetypeName = genderArchetypeToString(npc->genderArchetype);
    std::string bodyTemplate = scene.bodyText.empty() ?
        "A {target.name} ({target.archetype}, Level {target.level}) steps out from the shadows and blocks your path with clear intent!" : scene.bodyText;

    std::string formatted = bodyTemplate;
    auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    };
    replaceAll(formatted, "{target.name}", npc->name);
    replaceAll(formatted, "{target.archetype}", archetypeName);
    replaceAll(formatted, "{target.level}", std::to_string(npc->stats.level));
    scene.bodyText = formatted;

    return scene;
}

std::string encounterResolver::selectFlavorText(const std::string& mapId, entity* target)
{
    static const auto flavors = loadFlavorTexts();
    std::string npcName = (target != nullptr) ? target->name : "your opponent";

    std::string key = "default";
    if (mapId.find("forest") != std::string::npos || mapId.find("woods") != std::string::npos)
    {
        key = "forest";
    }
    else if (mapId.find("house") != std::string::npos || mapId.find("cottage") != std::string::npos)
    {
        key = "house";
    }

    auto it = flavors.find(key);
    std::string templateStr = (it != flavors.end()) ? it->second : flavors.at("default");
    try
    {
        return std::vformat(templateStr, std::make_format_args(npcName));
    }
    catch (...)
    {
        return std::format("You stand victorious over {}. They are defeated and at your mercy.", npcName);
    }
}

questScene encounterResolver::buildVictoryScene(game* g, entity* targetNPC, const std::string& mapId)
{
    questScene scene;
    if (questDatabase::exists("scene_procedural_victory"))
    {
        scene = questDatabase::getScene("scene_procedural_victory");
    }
    else
    {
        scene.id = "scene_procedural_victory";
        dialogueChoice leaveChoice;
        leaveChoice.label = "Leave / Spare";
        leaveChoice.nextSceneId = "EXIT";
        scene.choices.push_back(leaveChoice);

        dialogueChoice removeChoice;
        removeChoice.label = "Permanently Remove";
        removeChoice.nextSceneId = "ENCOUNTER_PERMANENT_REMOVE";
        scene.choices.push_back(removeChoice);

        dialogueChoice lootChoice;
        lootChoice.label = "Loot";
        lootChoice.nextSceneId = "VICTORY_INVENTORY";
        scene.choices.push_back(lootChoice);
    }

    scene.speakerName = (targetNPC != nullptr) ? targetNPC->name : "Defeated Enemy";
    scene.bodyText = selectFlavorText(mapId, targetNPC);

    if (targetNPC != nullptr && targetNPC->anatomy.hasGlobalTag("can_be_interrogated"))
    {
        bool hasInterrogate = false;
        for (const auto& ch : scene.choices)
        {
            if (ch.nextSceneId == "scene_interrogate_npc")
            {
                hasInterrogate = true;
                break;
            }
        }
        if (!hasInterrogate)
        {
            dialogueChoice interrogateChoice;
            interrogateChoice.label = "Interrogate";
            interrogateChoice.nextSceneId = "scene_interrogate_npc";
            scene.choices.push_back(interrogateChoice);
        }
    }
    else
    {
        std::erase_if(scene.choices, [](const dialogueChoice& ch) {
            return ch.nextSceneId == "scene_interrogate_npc";
        });
    }

    return scene;
}

std::string encounterResolver::getDefaultTemplateForDanger(int dangerLevel)
{
    static std::string defaultTpl = "tpl_alley_bandit";
    static std::map<int, std::string> dangerTemplates = {
        { 1, "tpl_alley_bandit" },
        { 2, "tpl_rogue_mage" }
    };
    static bool loaded = false;
    if (!loaded)
    {
        loaded = true;
        std::ifstream f("data/config/danger_encounters.json");
        if (f.is_open())
        {
            try
            {
                nlohmann::json j;
                f >> j;
                if (j.contains("defaultTemplate"))
                {
                    defaultTpl = j["defaultTemplate"].get<std::string>();
                }
                if (j.contains("dangerLevelTemplates") && j["dangerLevelTemplates"].is_object())
                {
                    dangerTemplates.clear();
                    for (auto& [key, val] : j["dangerLevelTemplates"].items())
                    {
                        dangerTemplates[std::stoi(key)] = val.get<std::string>();
                    }
                }
            }
            catch (...) {}
        }
    }

    if (dangerTemplates.count(dangerLevel))
    {
        return dangerTemplates[dangerLevel];
    }
    if (!dangerTemplates.empty() && dangerLevel >= dangerTemplates.rbegin()->first)
    {
        return dangerTemplates.rbegin()->second;
    }
    return defaultTpl;
}