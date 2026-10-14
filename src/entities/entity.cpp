#include "entities/entity.h"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "entities/perkDatabase.h"
#include "items/itemDatabase.h"
#include "core/timeManager.h"
#include "state/editorConfig.h"

using json = nlohmann::json;

entity::entity(std::string entityId, std::string entityName) : id(entityId), name(entityName)
{
    stats.setBaseStat("perk_points", 3.0f);
}

void entity::addStatusEffect(const StatusEffect& effect)
{
    for (auto& fx : statusEffects)
    {
        if (fx.id == effect.id)
        {
            fx.durationTurns = effect.durationTurns;
            fx.durationMinutes = effect.durationMinutes;
            fx.maxDurationMinutes = effect.maxDurationMinutes;
            if (!effect.iconId.empty()) fx.iconId = effect.iconId;
            invalidateStatCache();
            return;
        }
    }
    statusEffects.push_back(effect);
    invalidateStatCache();
}

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

void entity::updateStatusEffectsOnTurn()
{
    std::erase_if(statusEffects, [](StatusEffect& fx) {
        if (fx.durationTurns > 0)
        {
            fx.durationTurns--;
            if (fx.durationTurns <= 0) return true;
        }
        else if (fx.durationMinutes > 0)
        {
            fx.durationMinutes--;
            if (fx.durationMinutes <= 0) return true;
        }
        return false;
    });
    invalidateStatCache();
}

void entity::updateStatusEffectsOnTime(int minutesPassed)
{
    if (minutesPassed <= 0) return;
    std::erase_if(statusEffects, [minutesPassed](StatusEffect& fx) {
        if (fx.durationMinutes > 0)
        {
            fx.durationMinutes -= minutesPassed;
            if (fx.durationMinutes <= 0) return true;
        }
        else if (fx.durationTurns > 0)
        {
            int turnDecay = std::max(1, minutesPassed / 5);
            fx.durationTurns -= turnDecay;
            if (fx.durationTurns <= 0) return true;
        }
        return false;
    });
    invalidateStatCache();
}

void entity::clearStatusEffects()
{
    if (!statusEffects.empty())
    {
        statusEffects.clear();
        invalidateStatCache();
    }
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

    if (statName == "affection")
    {
        return affection;
    }
    if (statName == "corruption")
    {
        return corruption;
    }

    float val = stats.getEffectiveStat(statName, statusEffects);

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

    if (statName == "charm" || statName == "charisma")
    {
        for (const auto& eqItem : inventory.equipped)
        {
            if (eqItem && eqItem->soiling.isDirty)
            {
                flatEquipment -= 2.0f;
            }
        }
    }

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
    j["affection"] = affection;
    j["corruption"] = corruption;
    j["merchantAffinity"] = merchantAffinity;
    j["lastRestockDay"] = lastRestockDay;
    j["baseMerchantGold"] = baseMerchantGold;
    j["restockItemIds"] = restockItemIds;
    j["buyMarkup"] = buyMarkup;
    j["sellMarkdown"] = sellMarkdown;
    j["tradePerkModifier"] = tradePerkModifier;
    j["unlockedPerks"] = unlockedPerks;
    j["birthDay"] = birthDay;
    j["birthMonth"] = birthMonth;
    j["birthYear"] = birthYear;
    j["age"] = age;
    j["fetishDesires"] = fetishDesires;
    j["personalityTraits"] = personalityTraits;

    j["isElemental"] = isElemental;
    j["elementalType"] = elementalType;
    j["bodyMaterial"] = bodyMaterial;
    j["summonSpellId"] = summonSpellId;
    j["spiritRank"] = spiritRank;
    j["bondLevel"] = bondLevel;
    j["resonance"] = resonance;
    j["canSelfTransform"] = canSelfTransform;

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
        fxJson["durationMinutes"] = fx.durationMinutes;
        fxJson["maxDurationMinutes"] = fx.maxDurationMinutes;
        fxJson["iconId"] = fx.iconId;
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
    j["isServant"] = isServant;
    j["servant"] = servant.toJson();

    json backpackJson = json::array();
    for (const auto& itemPtr : inventory.backpack)
    {
        if (itemPtr)
        {
            if (itemPtr->soiling.hasStains())
            {
                json itemObj;
                itemObj["id"] = itemPtr->id;
                itemObj["count"] = itemPtr->count;
                itemObj["soiling"] = itemPtr->soiling.toJson();
                backpackJson.push_back(itemObj);
            }
            else
            {
                backpackJson.push_back(itemPtr->id);
            }
        }
    }
    j["backpack"] = backpackJson;

    json equippedJson = json::object();
    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        if (inventory.equipped[i])
        {
            if (inventory.equipped[i]->soiling.hasStains())
            {
                json itemObj;
                itemObj["id"] = inventory.equipped[i]->id;
                itemObj["soiling"] = inventory.equipped[i]->soiling.toJson();
                equippedJson[equipSlotToString(static_cast<equipSlot>(i))] = itemObj;
            }
            else
            {
                equippedJson[equipSlotToString(static_cast<equipSlot>(i))] = inventory.equipped[i]->id;
            }
        }
    }
    j["equipped"] = equippedJson;

    json dispJson = json::object();
    for (const auto& [slot, mode] : inventory.activeDisplacements)
    {
        dispJson[equipSlotToString(slot)] = displacementModeToString(mode);
    }
    j["activeDisplacements"] = dispJson;
    j["revealedBodySlots"] = std::vector<std::string>(revealedBodySlots.begin(), revealedBodySlots.end());
    j["quests"] = quests.toJson();

    return j;
}

void entity::fromJson(const json& j)
{
    id = j.value("id", "entity_unknown");
    name = j.value("name", "Unknown");

    if (j.contains("orientation")) orientation = stringToSexualOrientation(j["orientation"].get<std::string>());
    if (j.contains("genderArchetype")) genderArchetype = stringToGenderArchetype(j["genderArchetype"].get<std::string>());

    affection = std::clamp(j.value("affection", 0.0f), -100.0f, 100.0f);
    corruption = std::clamp(j.value("corruption", 0.0f), 0.0f, 100.0f);

    merchantAffinity = j.value("merchantAffinity", 1.0f);
    lastRestockDay = j.value("lastRestockDay", -1);
    baseMerchantGold = j.value("baseMerchantGold", 500.0f);
    restockItemIds = j.value("restockItemIds", std::vector<std::string>{});
    buyMarkup = j.value("buyMarkup", 1.25f);
    sellMarkdown = j.value("sellMarkdown", 0.50f);
    tradePerkModifier = j.value("tradePerkModifier", 0.0f);
    birthDay = j.value("birthDay", 29);
    birthMonth = j.value("birthMonth", 8);
    birthYear = j.value("birthYear", 1);
    age = j.value("age", 24);

    isElemental = j.value("isElemental", false);
    elementalType = j.value("elementalType", "physical");
    bodyMaterial = j.value("bodyMaterial", "flesh");
    summonSpellId = j.value("summonSpellId", "");
    spiritRank = j.value("spiritRank", 1);
    bondLevel = j.value("bondLevel", 10);
    resonance = j.value("resonance", 80.0f);
    canSelfTransform = j.value("canSelfTransform", false);

    if (j.contains("fetishDesires") && j["fetishDesires"].is_object())
    {
        fetishDesires = j["fetishDesires"].get<std::unordered_map<std::string, int>>();
    }
    if (j.contains("personalityTraits") && j["personalityTraits"].is_array())
    {
        personalityTraits = j["personalityTraits"].get<std::vector<std::string>>();
    }
    if (j.contains("unlockedPerks") && j["unlockedPerks"].is_array())
    {
        unlockedPerks = j["unlockedPerks"].get<std::vector<std::string>>();
        recalculatePerkModifiers();
    }

    if (j.contains("revealedBodySlots") && j["revealedBodySlots"].is_array())
    {
        revealedBodySlots.clear();
        for (const auto& item : j["revealedBodySlots"])
        {
            revealedBodySlots.insert(item.get<std::string>());
        }
    }

    if (j.contains("gestation"))
    {
        gestation.fromJson(j["gestation"]);
    }
    if (j.contains("isServant"))
    {
        isServant = j["isServant"].get<bool>();
    }
    if (j.contains("servant"))
    {
        servant.fromJson(j["servant"]);
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
            fx.durationMinutes = fxJson.value("durationMinutes", -1);
            fx.maxDurationMinutes = fxJson.value("maxDurationMinutes", 1440);
            fx.iconId = fxJson.value("iconId", "");
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
        for (const auto& itemEntry : j["backpack"])
        {
            std::string itemId;
            int count = 1;
            json soilingJson;
            if (itemEntry.is_string())
            {
                itemId = itemEntry.get<std::string>();
            }
            else if (itemEntry.is_object())
            {
                itemId = itemEntry.value("id", "");
                count = itemEntry.value("count", 1);
                if (itemEntry.contains("soiling"))
                {
                    soilingJson = itemEntry["soiling"];
                }
            }
            if (!itemId.empty())
            {
                auto itemPtr = itemDatabase::getItem(itemId);
                if (itemPtr)
                {
                    itemPtr->count = count;
                    if (!soilingJson.is_null())
                    {
                        itemPtr->soiling.fromJson(soilingJson);
                    }
                    inventory.addItem(itemPtr);
                }
            }
        }
    }

    inventory.equipped.fill(nullptr);
    if (j.contains("equipped"))
    {
        for (auto& [slotStr, itemVal] : j["equipped"].items())
        {
            equipSlot slot = stringToEquipSlot(slotStr);
            size_t slotIdx = static_cast<size_t>(slot);
            if (slot != equipSlot::NONE && slotIdx < EQUIP_SLOT_COUNT)
            {
                std::string itemId;
                json soilingJson;
                if (itemVal.is_string())
                {
                    itemId = itemVal.get<std::string>();
                }
                else if (itemVal.is_object())
                {
                    itemId = itemVal.value("id", "");
                    if (itemVal.contains("soiling"))
                    {
                        soilingJson = itemVal["soiling"];
                    }
                }

                if (!itemId.empty())
                {
                    auto itemPtr = itemDatabase::getItem(itemId);
                    if (itemPtr)
                    {
                        if (!soilingJson.is_null())
                        {
                            itemPtr->soiling.fromJson(soilingJson);
                        }
                        inventory.equipped[slotIdx] = itemPtr;
                    }
                }
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

int entity::getFetishDesire(const std::string& fetishKey) const
{
    auto it = fetishDesires.find(fetishKey);
    return (it != fetishDesires.end()) ? it->second : 2;
}

void entity::setFetishDesire(const std::string& fetishKey, int level)
{
    fetishDesires[fetishKey] = level;
}

bool entity::hasFetish(const std::string& fetishKey) const
{
    auto it = fetishDesires.find(fetishKey);
    return (it != fetishDesires.end() && it->second >= 3);
}

bool entity::hasTrait(const std::string& traitName) const
{
    for (const auto& t : personalityTraits)
    {
        if (t.size() == traitName.size())
        {
            bool match = true;
            for (size_t i = 0; i < t.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(t[i])) != std::tolower(static_cast<unsigned char>(traitName[i])))
                {
                    match = false;
                    break;
                }
            }
            if (match) return true;
        }
    }
    return false;
}

void entity::addTrait(const std::string& traitName)
{
    if (!hasTrait(traitName))
    {
        personalityTraits.push_back(traitName);
    }
}

void entity::removeTrait(const std::string& traitName)
{
    std::erase_if(personalityTraits, [&](const std::string& t) {
        if (t.size() != traitName.size()) return false;
        for (size_t i = 0; i < t.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(t[i])) != std::tolower(static_cast<unsigned char>(traitName[i])))
            {
                return false;
            }
        }
        return true;
    });
}

std::string entity::handleMasturbation(const std::string& actionId, int baseDurationMinutes, timeManager* timeMgr)
{
    if (timeMgr && baseDurationMinutes > 0)
    {
        timeMgr->advanceTime(baseDurationMinutes);
    }

    float currentArousal = getStat("arousal");
    float currentLust = getStat("lust");
    float maxArousal = 100.0f;
    std::string narrative = "";

    bool hasBreasts = anatomy.hasBreasts();
    bool hasVagina = anatomy.hasVagina();
    bool hasPenis = anatomy.hasPenis();

    if (actionId == "CARESS_CHEST")
    {
        float gain = 20.0f;
        stats.setBaseStat("arousal", std::min(maxArousal, currentArousal + gain));
        if (hasBreasts)
        {
            narrative = "Your fingertips trace circular patterns over your tender breasts, squeezing gently as your nipples harden to the sensitive touch.";
        }
        else
        {
            narrative = "You run your fingers slowly across your firm chest, teasing your sensitive nipples as your breath quickens.";
        }
    }
    else if (actionId == "FONDLE_GROIN")
    {
        float gain = 30.0f;
        stats.setBaseStat("arousal", std::min(maxArousal, currentArousal + gain));
        if (hasPenis && hasVagina)
        {
            narrative = "You slide your hands between your thighs, caressing your throbbing shaft while your fingertips circle your soaking slit.";
        }
        else if (hasPenis)
        {
            narrative = "You wrap your fingers firmly around your hardening shaft, stroking with deliberate rhythm as pre-cum beads at the tip.";
        }
        else if (hasVagina)
        {
            narrative = "You slip your slick fingers into your folds, rubbing your swollen clitoris in rhythmic, breathless circles.";
        }
        else
        {
            narrative = "You explore your sensitive groin, caressing your trembling flesh as a wave of heat spreads through your hips.";
        }
    }
    else if (actionId == "TEASE_CLIMAX")
    {
        float gain = 35.0f;
        stats.setBaseStat("arousal", std::min(maxArousal, currentArousal + gain));
        narrative = "You accelerate the pace, deliberately teasing right on the brink of ecstasy before pausing, shivering with exquisite frustration.";
    }
    else if (actionId == "CLIMAX_RELIEF")
    {
        if (currentArousal >= 60.0f || currentLust >= 40.0f)
        {
            stats.setBaseStat("arousal", 0.0f);
            stats.setBaseStat("lust", std::max(0.0f, currentLust - 50.0f));
            narrative = "A shattering, full-body convulsion ripples through you as you surrender completely to orgasm, panting in blissful, trembling release.";
        }
        else
        {
            stats.setBaseStat("arousal", std::min(maxArousal, currentArousal + 15.0f));
            narrative = "You push toward climax, but your desire is not yet fully peaked; your body shivers with unfulfilled anticipation.";
        }
    }
    else
    {
        stats.setBaseStat("arousal", std::min(maxArousal, currentArousal + 15.0f));
        narrative = "You take a quiet moment for yourself, letting your hands explore your changing body.";
    }

    return narrative;
}

std::string entity::handleFamiliarAction(const std::string& actionId, int baseDurationMinutes, timeManager* timeMgr)
{
    if (timeMgr && baseDurationMinutes > 0)
    {
        timeMgr->advanceTime(baseDurationMinutes);
    }

    std::string narrative = "";
    if (actionId == "PET")
    {
        bondLevel = std::min(100, bondLevel + 2);
        float currentMana = getStat("mana");
        float maxMana = getStat("max_mana");
        stats.setBaseStat("mana", std::min(maxMana, currentMana + 8.0f));
        narrative = "You gently stroke your elemental familiar. It hums with affectionate magical resonance, restoring your mana.";
    }
    else if (actionId == "COMMUNE")
    {
        resonance = std::min(100.0f, resonance + 5.0f);
        bondLevel = std::min(100, bondLevel + 4);
        narrative = "You sit in deep meditation with your familiar, harmonizing your arcane core with its elemental currents.";
    }
    else
    {
        narrative = "You spend time interacting with your loyal companion.";
    }

    return narrative;
}

bool entity::canTransform() const
{
    if (canSelfTransform) return true;
    if (hasPerk("metamorphosis") || hasPerk("shapeshifter") || hasPerk("transformation")) return true;
    if (hasPerkFlag("can_transform") || hasPerkFlag("shapeshifter")) return true;

    const auto& validTraits = EditorConfig::Catalogs::innateTransformTraits();
    for (const auto& trait : personalityTraits)
    {
        if (std::find(validTraits.begin(), validTraits.end(), trait) != validTraits.end())
        {
            return true;
        }
    }

    std::string race = anatomy.getDominantRace();
    for (char& c : race) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const auto& validRaces = EditorConfig::Catalogs::innateTransformRaces();
    if (std::find(validRaces.begin(), validRaces.end(), race) != validRaces.end())
    {
        return true;
    }

    return false;
}

bool entity::isSlotRevealed(bodySlot slot) const
{
    if (inventory.isSlotExposed(slot))
    {
        const_cast<entity*>(this)->revealSlot(slot);
        return true;
    }
    return revealedBodySlots.contains(bodySlotToString(slot));
}

void entity::revealSlot(bodySlot slot)
{
    revealedBodySlots.insert(bodySlotToString(slot));
}

void entity::revealAllSlots()
{
    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        revealedBodySlots.insert(bodySlotToString(static_cast<bodySlot>(i)));
    }
}

GenderArchetype entity::getPerceivedGender(const entity* observer) const
{
    if (observer == this || isSlotRevealed(bodySlot::GROIN))
    {
        return genderArchetype;
    }

    int femininePoints = 0;
    int masculinePoints = 0;

    // 1. Breast / Bust silhouette under clothing
    if (const bodyPart* breasts = anatomy.getPart(bodySlot::BREASTS))
    {
        if (breasts->cupSize >= 3) femininePoints += 4;
        else if (breasts->cupSize >= 1) femininePoints += 2;
        else masculinePoints += 2;
    }
    else
    {
        masculinePoints += 2;
    }

    // 2. Groin silhouette / bulge under clothing
    if (const bodyPart* groin = anatomy.getPart(bodySlot::GROIN))
    {
        if (groin->length >= 14.0f) masculinePoints += 3;
        else if (groin->length >= 8.0f) masculinePoints += 1;
        else if (groin->orifice.exists) femininePoints += 1;
    }

    // 3. Hair length & style
    if (const bodyPart* hair = anatomy.getPart(bodySlot::HAIR))
    {
        std::string style = hair->style;
        for (char& c : style) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (style.find("long") != std::string::npos ||
            style.find("braid") != std::string::npos ||
            style.find("ponytail") != std::string::npos ||
            style.find("pigtail") != std::string::npos ||
            style.find("twin") != std::string::npos ||
            style.find("bob") != std::string::npos ||
            style.find("curls") != std::string::npos)
        {
            femininePoints += 2;
        }
        else if (style.find("short") != std::string::npos ||
                 style.find("buzz") != std::string::npos ||
                 style.find("crew") != std::string::npos ||
                 style.find("bald") != std::string::npos ||
                 style.find("crop") != std::string::npos ||
                 style.find("fade") != std::string::npos)
        {
            masculinePoints += 2;
        }
    }

    // 4. Height & Build
    if (anatomy.heightMeters >= 1.83f) masculinePoints += 1;
    else if (anatomy.heightMeters <= 1.62f) femininePoints += 1;

    std::string bSize = anatomy.bodySize;
    for (char& c : bSize) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::string mTone = anatomy.muscleTone;
    for (char& c : mTone) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (bSize.find("petite") != std::string::npos ||
        bSize.find("slender") != std::string::npos ||
        bSize.find("curvy") != std::string::npos ||
        bSize.find("hourglass") != std::string::npos ||
        mTone.find("soft") != std::string::npos ||
        mTone.find("delicate") != std::string::npos)
    {
        femininePoints += 2;
    }
    else if (bSize.find("broad") != std::string::npos ||
             bSize.find("heavy") != std::string::npos ||
             bSize.find("towering") != std::string::npos ||
             mTone.find("muscular") != std::string::npos ||
             mTone.find("rugged") != std::string::npos ||
             mTone.find("ripped") != std::string::npos ||
             mTone.find("bulk") != std::string::npos)
    {
        masculinePoints += 2;
    }

    // 5. Facial hair and cosmetics
    auto itHair = cosmetics.find("facial_hair");
    if (itHair != cosmetics.end() && !itHair->second.empty() && itHair->second != "None")
    {
        masculinePoints += 4;
    }
    auto itBeard = cosmetics.find("beard");
    if (itBeard != cosmetics.end() && !itBeard->second.empty() && itBeard->second != "None")
    {
        masculinePoints += 4;
    }
    auto itMakeup = cosmetics.find("makeup");
    if (itMakeup != cosmetics.end() && !itMakeup->second.empty() && itMakeup->second != "None")
    {
        femininePoints += 2;
    }

    // 6. Personality Traits & Tags
    for (const auto& trait : personalityTraits)
    {
        std::string t = trait;
        for (char& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (t == "feminine" || t == "woman" || t == "female" || t == "maiden" || t == "seductress" || t == "lady")
        {
            femininePoints += 3;
        }
        else if (t == "masculine" || t == "man" || t == "male" || t == "warrior" || t == "brute" || t == "lord")
        {
            masculinePoints += 3;
        }
    }

    // 7. Visual presentation from anatomy
    BodyPresentation pres = anatomy.getVisualPresentation();
    if (pres == BodyPresentation::FEMININE) femininePoints += 2;
    else if (pres == BodyPresentation::MASCULINE) masculinePoints += 2;

    if (femininePoints > masculinePoints)
    {
        return GenderArchetype::FEMALE;
    }
    else if (masculinePoints > femininePoints)
    {
        return GenderArchetype::MALE;
    }
    else
    {
        return GenderArchetype::ANDROMORPH;
    }
}

void entity::ensureSexualityStatusEffect()
{
    std::string expectedId;
    std::string expectedName;
    std::string expectedIcon;
    std::string expectedDesc;

    switch (orientation)
    {
        case SexualOrientation::HETEROSEXUAL:
            expectedId = "status_sexuality_hetero";
            expectedName = "Heterosexual";
            expectedIcon = "HET";
            expectedDesc = "Attracted exclusively to individuals of the opposite gender presentation. In social and erotic encounters, seduction from opposite-gender partners carries natural appeal.";
            break;
        case SexualOrientation::HOMOSEXUAL:
            expectedId = "status_sexuality_homo";
            expectedName = "Homosexual";
            expectedIcon = "HOM";
            expectedDesc = "Attracted exclusively to individuals of the same gender presentation. In social and erotic encounters, seduction from same-gender partners carries natural appeal.";
            break;
        case SexualOrientation::BISEXUAL:
            expectedId = "status_sexuality_bi";
            expectedName = "Bisexual";
            expectedIcon = "BI";
            expectedDesc = "Attracted to individuals of both masculine and feminine gender presentations. Open to romantic and sexual advances across all presentations.";
            break;
        case SexualOrientation::ASEXUAL:
            expectedId = "status_sexuality_ace";
            expectedName = "Asexual";
            expectedIcon = "ACE";
            expectedDesc = "Experiences no inherent sexual attraction towards others. Naturally resistant or indifferent to seductive advances and lust-based manipulation.";
            break;
    }

    bool found = false;
    for (auto it = statusEffects.begin(); it != statusEffects.end(); )
    {
        if (it->id == "status_sexuality_hetero" ||
            it->id == "status_sexuality_homo" ||
            it->id == "status_sexuality_bi" ||
            it->id == "status_sexuality_ace")
        {
            if (it->id == expectedId)
            {
                found = true;
                ++it;
            }
            else
            {
                it = statusEffects.erase(it);
            }
        }
        else
        {
            ++it;
        }
    }

    if (!found)
    {
        StatusEffect eff;
        eff.id = expectedId;
        eff.name = expectedName;
        eff.iconId = expectedIcon;
        eff.description = expectedDesc;
        eff.durationTurns = -1;
        eff.durationMinutes = -1;
        eff.isDebuff = false;
        statusEffects.push_back(eff);
    }
}

void entity::modifyAffection(float delta)
{
    affection = std::clamp(affection + delta, -100.0f, 100.0f);
    invalidateStatCache();
}

void entity::modifyCorruption(float delta)
{
    corruption = std::clamp(corruption + delta, 0.0f, 100.0f);
    invalidateStatCache();
}