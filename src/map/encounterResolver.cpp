#include "map/encounterResolver.h"

#include <algorithm>
#include <cstdlib>
#include <format>

#include "core/game.h"
#include "entities/entity.h"
#include "entities/npcGenerator.h"

bool encounterResolver::shouldTriggerEncounter(int dangerLevel, TimePhase phase, float playerStealth)
{
    if (dangerLevel <= 0) return false;

    float baseChance = static_cast<float>(dangerLevel) * 15.0f;

    // Time phase danger modifiers
    if (phase == TimePhase::NIGHT) baseChance += 15.0f;
    else if (phase == TimePhase::DUSK) baseChance += 5.0f;

    // Reduce chance based on player stealth / agility
    baseChance -= (playerStealth * 0.5f);

    float finalChance = std::clamp(baseChance, 5.0f, 85.0f);
    int roll = rand() % 100;
    return (roll < static_cast<int>(finalChance));
}

std::shared_ptr<entity> encounterResolver::createEncounterNPC(int dangerLevel, const GameSettings& settings)
{
    auto npc = npcGenerator::generateRandomNPC(&settings);
    if (!npc)
    {
        npc = std::make_shared<entity>("npc_ambush", "Shadowy Ambush");
    }

    // Scale stats with tile danger level
    npc->stats.level = std::max(1, npc->stats.level + (dangerLevel - 1));
    float hpScale = 50.0f + (dangerLevel * 15.0f);
    npc->stats.setBaseStat("health", hpScale);
    npc->stats.setBaseStat("physique", 10.0f + (dangerLevel * 2.0f));
    npc->stats.setBaseStat("currency", 20.0f + (dangerLevel * 10.0f));

    return npc;
}

questScene encounterResolver::buildEncounterScene(game* g, std::shared_ptr<entity> npc)
{
    questScene scene;
    if (!npc) return scene;

    scene.id = "scene_procedural_encounter";
    scene.speakerName = npc->name;

    std::string archetypeName = genderArchetypeToString(npc->genderArchetype);
    std::string orientationName = sexualOrientationToString(npc->orientation);

    scene.bodyText = std::format("A {} ({}, Level {}) steps out from the shadows and blocks your path with clear intent!",
                                  npc->name, archetypeName, npc->stats.level);

    dialogueChoice fightChoice;
    fightChoice.label = "Fight";
    fightChoice.nextSceneId = "ENCOUNTER_FIGHT";
    scene.choices.push_back(fightChoice);

    dialogueChoice payChoice;
    payChoice.label = "Bribe (25¤)";
    payChoice.nextSceneId = "ENCOUNTER_BRIBE";
    scene.choices.push_back(payChoice);

    dialogueChoice surrenderChoice;
    surrenderChoice.label = "Surrender";
    surrenderChoice.nextSceneId = "ENCOUNTER_SURRENDER";
    scene.choices.push_back(surrenderChoice);

    return scene;
}

std::string encounterResolver::selectFlavorText(const std::string& mapId, entity* target)
{
    std::string npcName = (target != nullptr) ? target->name : "your opponent";

    if (mapId.find("forest") != std::string::npos || mapId.find("woods") != std::string::npos)
    {
        return "You overwhelm " + npcName + ", sending them stumbling backward into the dense undergrowth. They yield, unable to continue fighting.";
    }
    else if (mapId.find("house") != std::string::npos || mapId.find("cottage") != std::string::npos)
    {
        return "You corner " + npcName + " inside the wooden room. Disarmed and panting, they drop their weapon in submission.";
    }

    return "You stand victorious over " + npcName + ". They are defeated and at your mercy.";
}

questScene encounterResolver::buildVictoryScene(game* g, entity* targetNPC, const std::string& mapId)
{
    questScene scene;
    scene.id = "scene_procedural_victory";
    scene.speakerName = (targetNPC != nullptr) ? targetNPC->name : "Defeated Enemy";
    scene.bodyText = selectFlavorText(mapId, targetNPC);

    dialogueChoice leaveChoice;
    leaveChoice.label = "Leave";
    leaveChoice.nextSceneId = "EXIT";
    scene.choices.push_back(leaveChoice);

    dialogueChoice lootChoice;
    lootChoice.label = "Loot";
    lootChoice.nextSceneId = "VICTORY_INVENTORY";
    scene.choices.push_back(lootChoice);

    if (targetNPC != nullptr && targetNPC->anatomy.hasGlobalTag("can_be_interrogated"))
    {
        dialogueChoice interrogateChoice;
        interrogateChoice.label = "Interrogate";
        interrogateChoice.nextSceneId = "scene_interrogate_npc";
        scene.choices.push_back(interrogateChoice);
    }

    return scene;
}