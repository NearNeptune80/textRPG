#include "entity.h"
#include <iostream>
#include <cmath>

// --- anatomyComponent Implementation ---

void anatomyComponent::setPart(bodySlot slot, const bodyPart& part)
{
    size_t idx = static_cast<size_t>(slot);
    if (idx < BODY_SLOT_COUNT) parts[idx] = part;
}

void anatomyComponent::removePart(bodySlot slot)
{
    size_t idx = static_cast<size_t>(slot);
    if (idx < BODY_SLOT_COUNT) parts[idx].reset();
}

bool anatomyComponent::hasPart(bodySlot slot) const
{
    size_t idx = static_cast<size_t>(slot);
    return idx < BODY_SLOT_COUNT && parts[idx].has_value();
}

bodyPart* anatomyComponent::getPart(bodySlot slot)
{
    size_t idx = static_cast<size_t>(slot);
    if (idx < BODY_SLOT_COUNT && parts[idx].has_value()) return &parts[idx].value();
    return nullptr;
}

const bodyPart* anatomyComponent::getPart(bodySlot slot) const
{
    size_t idx = static_cast<size_t>(slot);
    if (idx < BODY_SLOT_COUNT && parts[idx].has_value()) return &parts[idx].value();
    return nullptr;
}

bool anatomyComponent::hasTag(bodySlot slot, const std::string& tag) const
{
    const bodyPart* part = getPart(slot);
    if (!part) return false;
    return std::find(part->tags.begin(), part->tags.end(), tag) != part->tags.end();
}

bool anatomyComponent::hasGlobalTag(const std::string& tag) const
{
    for (const auto& optPart : parts)
    {
        if (optPart.has_value())
        {
            const auto& tags = optPart.value().tags;
            if (std::find(tags.begin(), tags.end(), tag) != tags.end()) return true;
        }
    }
    return false;
}

std::vector<std::string> anatomyComponent::getAllTags() const
{
    std::vector<std::string> allTags;
    for (const auto& optPart : parts)
    {
        if (optPart.has_value())
        {
            for (const auto& t : optPart.value().tags)
            {
                if (std::find(allTags.begin(), allTags.end(), t) == allTags.end()) allTags.push_back(t);
            }
        }
    }
    return allTags;
}

void anatomyComponent::setTattoo(tattooSlot slot, const tattoo& tat) { tattoos[slot] = tat; }
void anatomyComponent::removeTattoo(tattooSlot slot) { tattoos.erase(slot); }
bool anatomyComponent::hasTattoo(tattooSlot slot) const { return tattoos.find(slot) != tattoos.end(); }

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
    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        if (parts[i].has_value())
        {
            std::cout << "[" << getSlotName(static_cast<bodySlot>(i)) << "] " << parts[i].value().name << "\n";
        }
    }
    std::cout << "=====================\n\n";
}

// --- statsComponent Implementation ---

bool statsComponent::addXp(float amount)
{
    currentXp += amount;
    bool leveledUp = false;
    while (currentXp >= getRequiredXp())
    {
        currentXp -= getRequiredXp();
        level++;
        leveledUp = true;
    }
    return leveledUp;
}

void statsComponent::setBaseStat(const std::string& name, float value) { baseValues[name] = value; }

float statsComponent::getBaseStat(const std::string& name) const
{
    if (baseValues.find(name) != baseValues.end()) return baseValues.at(name);
    return 0.0f;
}

void statsComponent::modifyBaseStat(const std::string& name, float amount) { baseValues[name] += amount; }

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
    return std::max(0.0f, (base + flatMod) * (1.0f + percentMod));
}

void statsComponent::printDebug() const
{
    std::cout << "\n=== STATS DEBUG ===\nLevel: " << level << " | XP: " << currentXp << "\n===================\n\n";
}

// --- questComponent Implementation ---

void questComponent::setQuestStage(const std::string& questId, int stage) { activeQuests[questId] = stage; }

int questComponent::getQuestStage(const std::string& questId) const
{
    if (activeQuests.find(questId) != activeQuests.end()) return activeQuests.at(questId);
    return 0;
}

// --- entity Core Implementation ---

entity::entity(std::string entityId, std::string entityName) : id(entityId), name(entityName) {}

void entity::addStatusEffect(const StatusEffect& effect)
{
    for (auto& fx : statusEffects)
    {
        if (fx.id == effect.id)
        {
            fx.durationTurns = effect.durationTurns;
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
        if (it->durationTurns > 0) it->durationTurns--;
        if (it->durationTurns == 0) it = statusEffects.erase(it);
        else ++it;
    }
}

using json = nlohmann::json;

json entity::toJson() const
{
    json j;
    j["id"] = id;
    j["name"] = name;

    json statsJson;
    statsJson["level"] = stats.level;
    statsJson["currentXp"] = stats.currentXp;

    json baseStats;
    for (const auto& [statName, val] : stats.getAllBaseStats())
    {
        baseStats[statName] = val;
    }
    statsJson["baseValues"] = baseStats;
    j["stats"] = statsJson;

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
    j["quests"] = quests.activeQuests;
    j["essences"] = essences;

    json anatomyJson = json::object();
    anatomyJson["heightMeters"] = anatomy.heightMeters;
    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        if (anatomy.getAllParts()[i].has_value())
        {
            const auto& part = anatomy.getAllParts()[i].value();
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
            anatomyJson[bodySlotToString(static_cast<bodySlot>(i))] = pJson;
        }
    }
    j["anatomy"] = anatomyJson;

    json backpackJson = json::array();
    for (const auto& itemPtr : inventory.backpack)
    {
        if (itemPtr) backpackJson.push_back(itemPtr->id);
    }
    j["backpack"] = backpackJson;

    json equippedJson = json::object();
    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        if (inventory.equipped[i])
        {
            equippedJson[equipSlotToString(static_cast<equipSlot>(i))] = inventory.equipped[i]->id;
        }
    }
    j["equipped"] = equippedJson;

    return j;
}

void entity::fromJson(const json& j)
{
    id = j.value("id", "entity_unknown");
    name = j.value("name", "Unknown");

    if (j.contains("stats"))
    {
        const auto& s = j["stats"];
        stats.level = s.value("level", 1);
        stats.currentXp = s.value("currentXp", 0.0f);

        if (s.contains("baseValues"))
        {
            for (auto& [key, val] : s["baseValues"].items())
            {
                stats.setBaseStat(key, val.get<float>());
            }
        }
    }

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

    if (j.contains("quests"))
    {
        quests.activeQuests = j["quests"].get<std::unordered_map<std::string, int>>();
    }

    if (j.contains("essences"))
    {
        essences = j["essences"].get<std::unordered_map<std::string, int>>();
    }

    if (j.contains("anatomy"))
    {
        const auto& aJson = j["anatomy"];
        anatomy.heightMeters = aJson.value("heightMeters", 1.75f);

        for (auto& [slotStr, pJson] : aJson.items())
        {
            if (slotStr == "heightMeters") continue;

            bodySlot slot = stringToBodySlot(slotStr);
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

    inventory.backpack.clear();
    if (j.contains("backpack"))
    {
        for (const auto& itemId : j["backpack"])
        {
            auto itemPtr = itemDatabase::getItem(itemId.get<std::string>());
            if (itemPtr) inventory.addItem(itemPtr);
        }
    }

    inventory.equipped.fill(nullptr);
    if (j.contains("equipped"))
    {
        for (auto& [slotStr, itemId] : j["equipped"].items())
        {
            equipSlot slot = stringToEquipSlot(slotStr);
            size_t slotIdx = static_cast<size_t>(slot);
            if (slot != equipSlot::NONE && slotIdx < EQUIP_SLOT_COUNT)
            {
                auto itemPtr = itemDatabase::getItem(itemId.get<std::string>());
                if (itemPtr) inventory.equipped[slotIdx] = itemPtr;
            }
        }
    }
}