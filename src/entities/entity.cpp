#include "entities/entity.h"

#include <algorithm>
#include <iostream>

#include "entities/perkDatabase.h"
#include "items/itemDatabase.h"

using json = nlohmann::json;

entity::entity(std::string entityId, std::string entityName) : id(entityId), name(entityName)
{
    stats.setBaseStat("perk_points", 3.0f);
}

/**
 * Applies or refreshes duration of a status effect.
 */
void entity::addStatusEffect(const StatusEffect& effect)
{
    for (auto& fx : statusEffects)
    {
        if (fx.id == effect.id)
        {
            fx.durationTurns = effect.durationTurns;
            invalidateStatCache();
            return;
        }
    }
    statusEffects.push_back(effect);
    invalidateStatCache();
}

/**
 * Removes all active status effects with the matching ID.
 */
void entity::removeStatusEffect(const std::string& effectId)
{
    std::erase_if(statusEffects, [&](const StatusEffect& fx) { return fx.id == effectId; });
    invalidateStatCache();
}

bool entity::hasStatusEffect(const std::string& effectId) const
{
    for (const auto& fx : statusEffects)
    {
        if (fx.id == effectId) return true;
    }
    return false;
}

/**
 * Decrements turn-based durations and prunes expired status effects.
 */
void entity::updateStatusEffectsOnTurn()
{
    std::erase_if(statusEffects, [](StatusEffect& fx) {
        if (fx.durationTurns > 0) fx.durationTurns--;
        return fx.durationTurns == 0;
    });
    invalidateStatCache();
}

void entity::invalidateStatCache() const
{
    m_statsDirty = true;
    m_statCache.clear();
}

float entity::getStat(const std::string& statName) const
{
    if (m_cachedEquipVersion != inventory.equipVersion || m_cachedStatsVersion != stats.statsVersion)
    {
        m_cachedEquipVersion = inventory.equipVersion;
        m_cachedStatsVersion = stats.statsVersion;
        m_statsDirty = true;
        m_statCache.clear();
    }

    if (!m_statsDirty)
    {
        auto it = m_statCache.find(statName);
        if (it != m_statCache.end())
        {
            return it->second;
        }
    }
    else
    {
        m_statCache.clear();
        m_statsDirty = false;
    }

    float val = stats.getEffectiveStat(statName, statusEffects);

    // Sum flat and percent bonuses from equipped items
    float flatEquipment = 0.0f;
    float percentEquipment = 0.0f;

    for (const auto& eqItem : inventory.equipped)
    {
        if (!eqItem) continue;
        for (const auto& mod : eqItem->statModifiers)
        {
            if (mod.statName == statName)
            {
                flatEquipment += mod.flatValue;
                percentEquipment += mod.percentValue;
            }
        }
    }

    // Sum bonuses from unlocked perks
    float flatPerks = 0.0f;
    float percentPerks = 0.0f;
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (perk)
        {
            auto itFlat = perk->modifiers.statModifiers.find(statName);
            if (itFlat != perk->modifiers.statModifiers.end())
            {
                flatPerks += itFlat->second;
            }
            auto itPct = perk->modifiers.percentStatModifiers.find(statName);
            if (itPct != perk->modifiers.percentStatModifiers.end())
            {
                percentPerks += itPct->second;
            }
        }
    }

    float result = std::max(0.0f, (val + flatEquipment + flatPerks) * (1.0f + percentEquipment + percentPerks));
    m_statCache[statName] = result;
    return result;
}

json entity::toJson() const
{
    json j;
    j["id"] = id;
    j["name"] = name;
    j["orientation"] = sexualOrientationToString(orientation);
    j["genderArchetype"] = genderArchetypeToString(genderArchetype);
    j["merchantAffinity"] = merchantAffinity;
    j["lastRestockDay"] = lastRestockDay;
    j["baseMerchantGold"] = baseMerchantGold;
    j["buyMarkup"] = buyMarkup;
    j["sellMarkdown"] = sellMarkdown;
    j["tradePerkModifier"] = tradePerkModifier;
    j["unlockedPerks"] = unlockedPerks;
    j["birthDay"] = birthDay;
    j["birthMonth"] = birthMonth;
    j["birthYear"] = birthYear;

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
            pJson["currentFluidMl"] = part.currentFluidMl;
            pJson["maxFluidMl"] = part.maxFluidMl;
            pJson["fluidRegenPerHour"] = part.fluidRegenPerHour;
            pJson["isLactating"] = part.isLactating;

            if (part.orifice.exists)
            {
                json orfJson;
                orfJson["exists"] = part.orifice.exists;
                orfJson["elasticity"] = part.orifice.elasticity;
                orfJson["currentStretch"] = part.orifice.currentStretch;
                orfJson["maxCapacityMl"] = part.orifice.maxCapacityMl;
                orfJson["depthCm"] = part.orifice.depthCm;
                orfJson["wetnessLevel"] = part.orifice.wetnessLevel;
                orfJson["storedFluids"] = part.orifice.storedFluids;
                pJson["orifice"] = orfJson;
            }

            anatomyJson[bodySlotToString(static_cast<bodySlot>(i))] = pJson;
        }
    }
    j["anatomy"] = anatomyJson;
    j["gestation"] = gestation.toJson();

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

    json dispJson = json::object();
    for (const auto& [slot, mode] : inventory.activeDisplacements)
    {
        dispJson[equipSlotToString(slot)] = displacementModeToString(mode);
    }
    j["activeDisplacements"] = dispJson;
    j["quests"] = quests.toJson();

    return j;
}

void entity::fromJson(const json& j)
{
    id = j.value("id", "entity_unknown");
    name = j.value("name", "Unknown");

    if (j.contains("orientation")) orientation = stringToSexualOrientation(j["orientation"].get<std::string>());
    if (j.contains("genderArchetype")) genderArchetype = stringToGenderArchetype(j["genderArchetype"].get<std::string>());

    merchantAffinity = j.value("merchantAffinity", 1.0f);
    lastRestockDay = j.value("lastRestockDay", -1);
    baseMerchantGold = j.value("baseMerchantGold", 500.0f);
    buyMarkup = j.value("buyMarkup", 1.25f);
    sellMarkdown = j.value("sellMarkdown", 0.50f);
    tradePerkModifier = j.value("tradePerkModifier", 0.0f);
    birthDay = j.value("birthDay", 29);
    birthMonth = j.value("birthMonth", 8);
    birthYear = j.value("birthYear", 1);
    if (j.contains("unlockedPerks") && j["unlockedPerks"].is_array())
    {
        unlockedPerks = j["unlockedPerks"].get<std::vector<std::string>>();
        recalculatePerkModifiers();
    }

    if (j.contains("gestation"))
    {
        gestation.fromJson(j["gestation"]);
    }

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
        quests.fromJson(j["quests"]);
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

            part.currentFluidMl = pJson.value("currentFluidMl", 0.0f);
            part.maxFluidMl = pJson.value("maxFluidMl", 0.0f);
            part.fluidRegenPerHour = pJson.value("fluidRegenPerHour", 0.0f);
            part.isLactating = pJson.value("isLactating", false);

            if (pJson.contains("orifice"))
            {
                const auto& orfJson = pJson["orifice"];
                part.orifice.exists = orfJson.value("exists", true);
                part.orifice.elasticity = orfJson.value("elasticity", 50.0f);
                part.orifice.currentStretch = orfJson.value("currentStretch", 0.0f);
                part.orifice.maxCapacityMl = orfJson.value("maxCapacityMl", 100.0f);
                part.orifice.depthCm = orfJson.value("depthCm", 15.0f);
                part.orifice.wetnessLevel = orfJson.value("wetnessLevel", 1);
                if (orfJson.contains("storedFluids"))
                {
                    part.orifice.storedFluids = orfJson["storedFluids"].get<std::unordered_map<std::string, float>>();
                }
            }

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

    inventory.activeDisplacements.clear();
    if (j.contains("activeDisplacements"))
    {
        for (auto& [slotStr, modeStr] : j["activeDisplacements"].items())
        {
            equipSlot slot = stringToEquipSlot(slotStr);
            DisplacementMode mode = stringToDisplacementMode(modeStr.get<std::string>());
            if (slot != equipSlot::NONE && mode != DisplacementMode::NONE)
            {
                inventory.activeDisplacements[slot] = mode;
            }
        }
    }

    invalidateStatCache();
}

void entity::unlockPerk(const std::string& perkId)
{
    if (std::find(unlockedPerks.begin(), unlockedPerks.end(), perkId) == unlockedPerks.end())
    {
        unlockedPerks.push_back(perkId);
        recalculatePerkModifiers();
    }
}

bool entity::hasPerk(const std::string& perkId) const
{
    return std::find(unlockedPerks.begin(), unlockedPerks.end(), perkId) != unlockedPerks.end();
}

void entity::resetPerks()
{
    unlockedPerks.clear();
    recalculatePerkModifiers();
}

void entity::recalculatePerkModifiers()
{
    tradePerkModifier = 0.0f;
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (perk)
        {
            tradePerkModifier += perk->modifiers.tradeDiscount;
        }
        else if (perkId == "silver_tongue")
        {
            tradePerkModifier += 0.10f;
        }
        else if (perkId == "master_trader")
        {
            tradePerkModifier += 0.15f;
        }
    }
    invalidateStatCache();
}

float entity::getPerkDamageMultiplier(const std::string& targetRace, const std::string& attackType) const
{
    float mult = 0.0f;
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (!perk) continue;

        if (!targetRace.empty())
        {
            auto itR = perk->modifiers.damageBoostByRace.find(targetRace);
            if (itR != perk->modifiers.damageBoostByRace.end()) mult += itR->second;
        }
        if (!attackType.empty())
        {
            auto itA = perk->modifiers.damageBoostByAttackType.find(attackType);
            if (itA != perk->modifiers.damageBoostByAttackType.end()) mult += itA->second;
        }
    }
    return mult;
}

float entity::getPerkDefenseMultiplier(const std::string& attackerRace, const std::string& attackType) const
{
    float mult = 0.0f;
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (!perk) continue;

        if (!attackerRace.empty())
        {
            auto itR = perk->modifiers.defenseBoostByRace.find(attackerRace);
            if (itR != perk->modifiers.defenseBoostByRace.end()) mult += itR->second;
        }
        if (!attackType.empty())
        {
            auto itA = perk->modifiers.defenseBoostByAttackType.find(attackType);
            if (itA != perk->modifiers.defenseBoostByAttackType.end()) mult += itA->second;
        }
    }
    return mult;
}

bool entity::hasPerkFlag(const std::string& flag) const
{
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (!perk) continue;

        for (const auto& f : perk->modifiers.flags)
        {
            if (f == flag) return true;
        }
    }
    return false;
}

std::vector<std::string> entity::getAllPerkFlags() const
{
    std::vector<std::string> result;
    for (const auto& perkId : unlockedPerks)
    {
        const auto* perk = PerkDatabase::getPerk(perkId);
        if (!perk) continue;

        for (const auto& f : perk->modifiers.flags)
        {
            if (std::find(result.begin(), result.end(), f) == result.end())
            {
                result.push_back(f);
            }
        }
    }
    return result;
}