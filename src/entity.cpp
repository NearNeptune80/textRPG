#include "entity.h"
#include <iostream>
#include <cmath>

// --- anatomyComponent Methods ---

void anatomyComponent::setPart(bodySlot slot, const bodyPart& part)
{
    parts[slot] = part;
}

void anatomyComponent::removePart(bodySlot slot)
{
    parts.erase(slot);
}

bool anatomyComponent::hasPart(bodySlot slot) const
{
    return parts.find(slot) != parts.end();
}

bodyPart* anatomyComponent::getPart(bodySlot slot)
{
    if (hasPart(slot)) return &parts[slot];
    return nullptr;
}

const bodyPart* anatomyComponent::getPart(bodySlot slot) const
{
    if (hasPart(slot)) return &parts.at(slot);
    return nullptr;
}

bool anatomyComponent::hasTag(bodySlot slot, const std::string& tag) const
{
    if (!hasPart(slot)) return false;
    const auto& tags = parts.at(slot).tags;
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool anatomyComponent::hasGlobalTag(const std::string& tag) const
{
    for (const auto& [slot, part] : parts)
    {
        if (std::find(part.tags.begin(), part.tags.end(), tag) != part.tags.end())
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string> anatomyComponent::getAllTags() const
{
    std::vector<std::string> allTags;
    for (const auto& [slot, part] : parts)
    {
        for (const auto& t : part.tags)
        {
            if (std::find(allTags.begin(), allTags.end(), t) == allTags.end())
            {
                allTags.push_back(t);
            }
        }
    }
    return allTags;
}

void anatomyComponent::setTattoo(tattooSlot slot, const tattoo& tat)
{
    tattoos[slot] = tat;
}

void anatomyComponent::removeTattoo(tattooSlot slot)
{
    tattoos.erase(slot);
}

bool anatomyComponent::hasTattoo(tattooSlot slot) const
{
    return tattoos.find(slot) != tattoos.end();
}

tattoo* anatomyComponent::getTattoo(tattooSlot slot)
{
    if (hasTattoo(slot)) return &tattoos[slot];
    return nullptr;
}

std::string getSlotName(bodySlot slot)
{
    switch (slot)
    {
        case bodySlot::HAIR:      return "Hair";
        case bodySlot::HEAD:      return "Head";
        case bodySlot::EYES:      return "Eyes";
        case bodySlot::EARS:      return "Ears";
        case bodySlot::MOUTH:     return "Mouth";
        case bodySlot::NECK:      return "Neck";
        case bodySlot::HORNS:     return "Horns";
        case bodySlot::ANTENNAE:  return "Antennae";
        case bodySlot::TORSO:     return "Torso";
        case bodySlot::BREASTS:   return "Breasts";
        case bodySlot::NIPPLES:   return "Nipples";
        case bodySlot::STOMACH:   return "Stomach";
        case bodySlot::BACK:      return "Back";
        case bodySlot::ARMS:      return "Arms";
        case bodySlot::HANDS:     return "Hands";
        case bodySlot::FINGERS:   return "Fingers";
        case bodySlot::HIPS:      return "Hips";
        case bodySlot::GROIN:     return "Groin";
        case bodySlot::ASS:       return "Ass";
        case bodySlot::TAIL:      return "Tail";
        case bodySlot::LEGS:      return "Legs";
        case bodySlot::FEET:      return "Feet";
        case bodySlot::WINGS:     return "Wings";
        case bodySlot::TENTACLES: return "Tentacles";
        default:                  return "Unknown Slot";
    }
}

void anatomyComponent::printDebug() const
{
    std::cout << "\n=== ANATOMY DEBUG ===\n";
    if (parts.empty())
    {
        std::cout << "No body parts attached.\n";
    }
    else
    {
        for (const auto& pair : parts)
        {
            std::cout << "[" << getSlotName(pair.first) << "] "
                << pair.second.name
                << "\n  Race: " << pair.second.race
                //<< " | Covering: " << pair.second.covering
                << " | Color: " << pair.second.primaryColor << "\n";

            std::cout << "  Tags: ";
            for (const auto& tag : pair.second.tags)
            {
                std::cout << tag << " ";
            }
            std::cout << "\n\n";
        }
    }
    std::cout << "=====================\n\n";
}

// --- statsComponent Methods ---

bool statsComponent::addXp(float amount)
{
    currentXp += amount;
    bool leveledUp = false;

    while (currentXp >= getRequiredXp())
    {
        currentXp -= getRequiredXp();
        level++;
        leveledUp = true;
        std::cout << "[Level Up] Entity reached level " << level << "!\n";
    }

    return leveledUp;
}

void statsComponent::setBaseStat(const std::string& name, float value)
{
    baseValues[name] = value;
}

float statsComponent::getBaseStat(const std::string& name) const
{
    if (baseValues.find(name) != baseValues.end())
    {
        return baseValues.at(name);
    }
    return 0.0f;
}

void statsComponent::modifyBaseStat(const std::string& name, float amount)
{
    baseValues[name] += amount;
}

float statsComponent::getEffectiveStat(const std::string& name, const std::vector<StatusEffect>& activeEffects) const
{
    float base = getBaseStat(name);
    float flatMod = 0.0f;
    float percentMod = 0.0f;

    for (const auto& fx : activeEffects)
    {
        for (const auto& mod : fx.statModifiers)
        {
            if (mod.statName == name)
            {
                flatMod += mod.flatValue;
                percentMod += mod.percentValue;
            }
        }
    }

    float result = (base + flatMod) * (1.0f + percentMod);
    return std::max(0.0f, result);
}

void statsComponent::printDebug() const
{
    std::cout << "\n=== STATS DEBUG ===\n";
    std::cout << "Level: " << level << " | XP: " << currentXp << "/" << getRequiredXp() << "\n";
    for (const auto& pair : baseValues)
    {
        std::cout << pair.first << ": " << pair.second << "\n";
    }
    std::cout << "===================\n\n";
}

// --- questComponent Methods ---

void questComponent::setQuestStage(const std::string& questId, int stage)
{
    activeQuests[questId] = stage;
}

int questComponent::getQuestStage(const std::string& questId) const
{
    if (activeQuests.find(questId) != activeQuests.end())
    {
        return activeQuests.at(questId);
    }
    return 0;
}

// --- entity Core Methods ---

entity::entity(std::string entityId, std::string entityName)
    : id(entityId), name(entityName)
{
}

void entity::addStatusEffect(const StatusEffect& effect)
{
    for (auto& fx : statusEffects)
    {
        if (fx.id == effect.id)
        {
            fx.durationTurns = effect.durationTurns; // Refresh duration
            return;
        }
    }
    statusEffects.push_back(effect);
}

void entity::removeStatusEffect(const std::string& effectId)
{
    statusEffects.erase(
        std::remove_if(statusEffects.begin(), statusEffects.end(),
            [&](const StatusEffect& fx) { return fx.id == effectId; }),
        statusEffects.end()
    );
}

bool entity::hasStatusEffect(const std::string& effectId) const
{
    for (const auto& fx : statusEffects)
    {
        if (fx.id == effectId) return true;
    }
    return false;
}

void entity::updateStatusEffectsOnTurn()
{
    for (auto it = statusEffects.begin(); it != statusEffects.end(); )
    {
        if (it->durationTurns > 0)
        {
            it->durationTurns--;
        }

        if (it->durationTurns == 0)
        {
            std::cout << "[" << name << "] Status effect expired: " << it->name << "\n";
            it = statusEffects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

using json = nlohmann::json;

json entity::toJson() const
{
    json j;
    j["id"] = id;
    j["name"] = name;

    // 1. Stats
    json statsJson;
    statsJson["level"] = stats.level;
    statsJson["currentXp"] = stats.currentXp;

    json baseStats;
    baseStats["health"] = stats.getBaseStat("health");
    baseStats["mana"] = stats.getBaseStat("mana");
    baseStats["lust"] = stats.getBaseStat("lust");
    baseStats["physique"] = stats.getBaseStat("physique");
    baseStats["arcane"] = stats.getBaseStat("arcane");
    baseStats["corruption"] = stats.getBaseStat("corruption");
    baseStats["currency"] = stats.getBaseStat("currency");
    baseStats["gems"] = stats.getBaseStat("gems");
    statsJson["baseValues"] = baseStats;
    j["stats"] = statsJson;

    // 2. Status Effects
    json fxArray = json::array();
    for (const auto& fx : statusEffects)
    {
        json fxJson;
        fxJson["id"] = fx.id;
        fxJson["name"] = fx.name;
        fxJson["description"] = fx.description;
        fxJson["durationTurns"] = fx.durationTurns;
        fxJson["isDebuff"] = fx.isDebuff;
        fxJson["grantedTags"] = fx.grantedTags;

        json modsArray = json::array();
        for (const auto& mod : fx.statModifiers)
        {
            json modJson;
            modJson["statName"] = mod.statName;
            modJson["flatValue"] = mod.flatValue;
            modJson["percentValue"] = mod.percentValue;
            modsArray.push_back(modJson);
        }
        fxJson["statModifiers"] = modsArray;
        fxArray.push_back(fxJson);
    }
    j["statusEffects"] = fxArray;

    // 3. Quests
    j["quests"] = quests.activeQuests;

    // 4. Anatomy
    json anatomyJson = json::object();
    anatomyJson["heightMeters"] = anatomy.heightMeters;
    for (const auto& [slot, part] : anatomy.getAllParts())
    {
        json pJson;
        pJson["id"] = part.id;
        pJson["name"] = part.name;
        pJson["race"] = part.race;
        pJson["count"] = part.count;
        pJson["covering"] = coveringTypeToString(part.covering);
        pJson["primaryColor"] = part.primaryColor;
        pJson["secondaryColor"] = part.secondaryColor;
        pJson["length"] = part.length;
        pJson["diameter"] = part.diameter;
        pJson["cupSize"] = part.cupSize;
        pJson["style"] = part.style;
        pJson["tags"] = part.tags;
        anatomyJson[std::to_string(static_cast<int>(slot))] = pJson;
    }
    j["anatomy"] = anatomyJson;

    // 5. Inventory Backpack
    json backpackJson = json::array();
    for (const auto& itemPtr : inventory.backpack)
    {
        if (itemPtr) backpackJson.push_back(itemPtr->id);
    }
    j["backpack"] = backpackJson;

    // 6. Inventory Equipped
    json equippedJson = json::object();
    for (const auto& [slot, itemPtr] : inventory.equipped)
    {
        if (itemPtr)
        {
            equippedJson[std::to_string(static_cast<int>(slot))] = itemPtr->id;
        }
    }
    j["equipped"] = equippedJson;

    return j;
}

void entity::fromJson(const json& j)
{
    id = j.value("id", "entity_unknown");
    name = j.value("name", "Unknown");

    // 1. Stats
    if (j.contains("stats"))
    {
        const auto& s = j["stats"];
        stats.level = s.value("level", 1); // Sets member variable level
        stats.currentXp = s.value("currentXp", 0.0f);

        if (s.contains("baseValues"))
        {
            for (auto& [key, val] : s["baseValues"].items())
            {
                stats.setBaseStat(key, val.get<float>());
            }
        }
    }

    // 2. Status Effects
    statusEffects.clear();
    if (j.contains("statusEffects"))
    {
        for (const auto& fxJson : j["statusEffects"])
        {
            StatusEffect fx;
            fx.id = fxJson.value("id", "");
            fx.name = fxJson.value("name", "");
            fx.description = fxJson.value("description", "");
            fx.durationTurns = fxJson.value("durationTurns", -1);
            fx.isDebuff = fxJson.value("isDebuff", false);
            fx.grantedTags = fxJson.value("grantedTags", std::vector<std::string>{});

            if (fxJson.contains("statModifiers"))
            {
                for (const auto& modJson : fxJson["statModifiers"])
                {
                    StatModifier mod;
                    mod.statName = modJson.value("statName", "");
                    mod.flatValue = modJson.value("flatValue", 0.0f);
                    mod.percentValue = modJson.value("percentValue", 0.0f);
                    fx.statModifiers.push_back(mod);
                }
            }
            statusEffects.push_back(fx);
        }
    }

    // 3. Quests
    if (j.contains("quests"))
    {
        quests.activeQuests = j["quests"].get<std::unordered_map<std::string, int>>();
    }

    // 4. Anatomy
    if (j.contains("anatomy"))
    {
        const auto& aJson = j["anatomy"];

        // 1. Extract overall height (with 1.75f default fallback)
        anatomy.heightMeters = aJson.value("heightMeters", 1.75f);

        // 2. Loop through body part slots
        for (auto& [slotStr, pJson] : aJson.items())
        {
            // Skip the metadata key so std::stoi doesn't throw an exception!
            if (slotStr == "heightMeters") continue;

            bodySlot slot = static_cast<bodySlot>(std::stoi(slotStr));
            bodyPart part;
            part.id = pJson.value("id", "");
            part.name = pJson.value("name", "");
            part.race = pJson.value("race", "Human");
            part.count = pJson.value("count", 1);
            part.covering = stringToCoveringType(pJson.value("covering", "SKIN"));
            part.primaryColor = pJson.value("primaryColor", "Fair");
            part.secondaryColor = pJson.value("secondaryColor", "");
            part.length = pJson.value("length", 0.0f);
            part.diameter = pJson.value("diameter", 0.0f);
            part.cupSize = pJson.value("cupSize", 0);
            part.style = pJson.value("style", "");
            part.tags = pJson.value("tags", std::vector<std::string>{});
            anatomy.setPart(slot, part);
        }
    }

    // 5. Inventory Backpack
    inventory.backpack.clear();
    if (j.contains("backpack"))
    {
        for (const auto& itemId : j["backpack"])
        {
            auto itemPtr = itemDatabase::getItem(itemId.get<std::string>());
            if (itemPtr) inventory.addItem(itemPtr);
        }
    }

    // 6. Inventory Equipped
    inventory.equipped.clear();
    if (j.contains("equipped"))
    {
        for (auto& [slotStr, itemId] : j["equipped"].items())
        {
            equipSlot slot = static_cast<equipSlot>(std::stoi(slotStr));
            auto itemPtr = itemDatabase::getItem(itemId.get<std::string>());
            if (itemPtr) inventory.equipped[slot] = itemPtr;
        }
    }
}