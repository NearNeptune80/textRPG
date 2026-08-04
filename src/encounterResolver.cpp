#include "encounterResolver.h"
#include "game.h"
#include "entity.h"

std::string encounterResolver::selectFlavorText(const std::string& mapId, entity* target)
{
    std::string npcName = (target != nullptr) ? target->name : "your opponent";

    if (mapId.find("forest") != std::string::npos || mapId.find("woods") != std::string::npos)
    {
        return "You overwhelm " + npcName + ", sending them stumbling backward into the dense undergrowth. They yield, unable to continue fighting.";
    }
    else if (mapId.find("city") != std::string::npos || mapId.find("alley") != std::string::npos)
    {
        return "You pin " + npcName + " against the damp stone wall. Disarmed and breathless, they raise their hands in defeat.";
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

    // Only inspect anatomy if targetNPC is valid
    if (targetNPC != nullptr && targetNPC->anatomy.hasGlobalTag("can_be_interrogated"))
    {
        dialogueChoice interrogateChoice;
        interrogateChoice.label = "Interrogate";
        interrogateChoice.nextSceneId = "scene_interrogate_npc";
        scene.choices.push_back(interrogateChoice);
    }

    return scene;
}