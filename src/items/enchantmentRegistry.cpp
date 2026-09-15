#include "items/enchantmentRegistry.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "items/item.h"

namespace fs = std::filesystem;

EnchantmentRegistry& EnchantmentRegistry::getInstance()
{
    static EnchantmentRegistry s_instance;
    if (!s_instance.isLoaded())
    {
        s_instance.load();
    }
    return s_instance;
}

static ItemRarity stringToRarity(const std::string& str)
{
    if (str == "UNCOMMON") return ItemRarity::UNCOMMON;
    if (str == "RARE")     return ItemRarity::RARE;
    if (str == "EPIC")     return ItemRarity::EPIC;
    if (str == "LEGENDARY")return ItemRarity::LEGENDARY;
    return ItemRarity::COMMON;
}

bool EnchantmentRegistry::load(const std::string& aspectsPath, const std::string& groupsPath)
{
    // Try provided paths first, then fallback to relative or source paths
    std::string actualAspectsPath = aspectsPath;
    if (!fs::exists(actualAspectsPath) && fs::exists("../" + aspectsPath))
    {
        actualAspectsPath = "../" + aspectsPath;
    }
    std::string actualGroupsPath = groupsPath;
    if (!fs::exists(actualGroupsPath) && fs::exists("../" + groupsPath))
    {
        actualGroupsPath = "../" + groupsPath;
    }

    std::ifstream aFile(actualAspectsPath);
    if (!aFile.is_open())
    {
        std::cerr << "[EnchantmentRegistry] Warning: Could not open " << actualAspectsPath << "\n";
        return false;
    }

    try
    {
        nlohmann::json aJson;
        aFile >> aJson;

        m_focusesById.clear();
        m_focusesByEnum.clear();
        m_propsById.clear();
        m_propsByEnum.clear();
        m_propShortLabels.clear();
        m_propContinuous.clear();
        m_propLimitSteps.clear();
        m_focusToProps.clear();

        if (aJson.contains("focuses") && aJson["focuses"].is_array())
        {
            for (const auto& fj : aJson["focuses"])
            {
                std::string id = fj.value("id", "");
                std::string enumStr = fj.value("enum", "");
                std::string name = fj.value("name", "");
                std::string desc = fj.value("description", "");
                std::string affix = fj.value("affix", "");
                ItemRarity rar = stringToRarity(fj.value("rarity", "COMMON"));

                AspectDefinition def{ id, name, desc, affix, rar };
                if (!id.empty()) m_focusesById[id] = def;
                if (!enumStr.empty()) m_focusesByEnum[enumStr] = def;

                if (fj.contains("properties") && fj["properties"].is_array())
                {
                    std::vector<std::string> pList;
                    for (const auto& pVal : fj["properties"])
                    {
                        pList.push_back(pVal.get<std::string>());
                    }
                    if (!enumStr.empty()) m_focusToProps[enumStr] = pList;
                }
            }
        }

        if (aJson.contains("properties") && aJson["properties"].is_array())
        {
            for (const auto& pj : aJson["properties"])
            {
                std::string id = pj.value("id", "");
                std::string enumStr = pj.value("enum", "");
                std::string name = pj.value("name", "");
                std::string desc = pj.value("description", "");
                std::string affix = pj.value("affix", "");
                ItemRarity rar = stringToRarity(pj.value("rarity", "COMMON"));

                AspectDefinition def{ id, name, desc, affix, rar };
                if (!id.empty()) m_propsById[id] = def;
                if (!enumStr.empty()) m_propsByEnum[enumStr] = def;

                if (pj.contains("shortLabel"))
                {
                    m_propShortLabels[enumStr] = pj["shortLabel"].get<std::string>();
                    m_propShortLabels[id] = pj["shortLabel"].get<std::string>();
                }
                if (pj.contains("isContinuous"))
                {
                    bool cont = pj["isContinuous"].get<bool>();
                    m_propContinuous[enumStr] = cont;
                    m_propContinuous[id] = cont;
                }
                if (pj.contains("limitSteps") && pj["limitSteps"].is_array())
                {
                    std::vector<std::string> steps;
                    for (const auto& st : pj["limitSteps"])
                    {
                        steps.push_back(st.get<std::string>());
                    }
                    m_propLimitSteps[enumStr] = steps;
                    m_propLimitSteps[id] = steps;
                }

                if (pj.contains("focus"))
                {
                    std::string fEnum = pj["focus"].get<std::string>();
                    auto& list = m_focusToProps[fEnum];
                    if (std::find(list.begin(), list.end(), id) == list.end())
                    {
                        list.push_back(id);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[EnchantmentRegistry] JSON parse error in aspects: " << e.what() << "\n";
        return false;
    }

    std::ifstream gFile(actualGroupsPath);
    if (!gFile.is_open())
    {
        std::cerr << "[EnchantmentRegistry] Warning: Could not open " << actualGroupsPath << "\n";
        return false;
    }

    try
    {
        nlohmann::json gJson;
        gFile >> gJson;

        m_groups.clear();
        for (auto it = gJson.begin(); it != gJson.end(); ++it)
        {
            EnchantmentGroupDef gDef;
            gDef.id = it.key();
            const auto& gj = it.value();
            gDef.name = gj.value("name", gDef.id);
            gDef.defaultMaxEnchantments = gj.value("defaultMaxEnchantments", 999);

            if (gj.contains("allowedFocuses") && gj["allowedFocuses"].is_array())
            {
                for (const auto& af : gj["allowedFocuses"])
                {
                    gDef.allowedFocuses.push_back(af.get<std::string>());
                }
            }
            if (gj.contains("allowedProperties") && gj["allowedProperties"].is_array())
            {
                for (const auto& ap : gj["allowedProperties"])
                {
                    gDef.allowedProperties.push_back(ap.get<std::string>());
                }
            }
            m_groups[gDef.id] = gDef;
        }

        m_loaded = true;
        std::cout << "[EnchantmentRegistry] Initialised JSON enchantment database successfully ("
                  << m_focusesById.size() << " focuses, " << m_propsById.size() << " properties, "
                  << m_groups.size() << " groups).\n";
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[EnchantmentRegistry] JSON parse error in groups: " << e.what() << "\n";
        return false;
    }
}

const AspectDefinition* EnchantmentRegistry::getFocusDef(const std::string& idOrEnum) const
{
    auto it1 = m_focusesById.find(idOrEnum);
    if (it1 != m_focusesById.end()) return &it1->second;
    auto it2 = m_focusesByEnum.find(idOrEnum);
    if (it2 != m_focusesByEnum.end()) return &it2->second;
    return nullptr;
}

const AspectDefinition* EnchantmentRegistry::getPropertyDef(const std::string& idOrEnum) const
{
    auto it1 = m_propsById.find(idOrEnum);
    if (it1 != m_propsById.end()) return &it1->second;
    auto it2 = m_propsByEnum.find(idOrEnum);
    if (it2 != m_propsByEnum.end()) return &it2->second;
    return nullptr;
}

const EnchantmentGroupDef* EnchantmentRegistry::getGroup(const std::string& groupId) const
{
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) return &it->second;
    return nullptr;
}

int EnchantmentRegistry::getDefaultLimitForGroup(const std::string& groupId) const
{
    const auto* grp = getGroup(groupId);
    if (grp) return grp->defaultMaxEnchantments;
    return 999;
}

std::vector<EnchantmentFocus> EnchantmentRegistry::getCompatibleFocuses(const item* baseItem) const
{
    std::string targetGroupId = "";
    if (baseItem && !baseItem->enchantmentGroup.empty())
    {
        targetGroupId = baseItem->enchantmentGroup;
    }
    else if (!baseItem)
    {
        targetGroupId = "";
    }
    else
    {
        bool isWeapon = (baseItem->category == ItemCategory::WEAPON ||
                         baseItem->targetSlot == equipSlot::WEAPON_MAIN ||
                         baseItem->targetSlot == equipSlot::WEAPON_OFF);

        bool isApparel = (baseItem->category == ItemCategory::CLOTHING ||
                          baseItem->category == ItemCategory::UNDERWEAR ||
                          baseItem->category == ItemCategory::ACCESSORY ||
                          (baseItem->isEquippable && !isWeapon));

        if (isWeapon)
        {
            targetGroupId = "weapon";
        }
        else if (isApparel)
        {
            if (baseItem->category == ItemCategory::ACCESSORY)
            {
                // Accessories can use apparel or jewelry
                targetGroupId = "apparel";
            }
            else
            {
                targetGroupId = "apparel";
            }
        }
        else if (baseItem->isRacialReagent())
        {
            targetGroupId = "consumable_reagent_canine";
        }
        else
        {
            targetGroupId = "consumable_plain";
        }
    }

    const auto* grp = getGroup(targetGroupId);
    if (grp && !grp->allowedFocuses.empty())
    {
        std::vector<EnchantmentFocus> result;
        for (const auto& fStr : grp->allowedFocuses)
        {
            EnchantmentFocus f = enumStringToFocus(fStr);
            if (f != EnchantmentFocus::NONE)
            {
                result.push_back(f);
            }
        }
        return result;
    }

    // Fallback: all primary focuses
    return {
        EnchantmentFocus::CORE_ATTRIBUTES,
        EnchantmentFocus::GENERAL_ATTRIBUTES,
        EnchantmentFocus::SPECIAL_EFFECTS,
        EnchantmentFocus::RETENTION_FLUIDS,
        EnchantmentFocus::BODY_DESIRES,
        EnchantmentFocus::BEHAVIORAL_DESIRES,
        EnchantmentFocus::HEAD_FEATURE,
        EnchantmentFocus::HAIR,
        EnchantmentFocus::EYES,
        EnchantmentFocus::EARS,
        EnchantmentFocus::MOUTH,
        EnchantmentFocus::FACE,
        EnchantmentFocus::SKIN,
        EnchantmentFocus::ARMS,
        EnchantmentFocus::TORSO,
        EnchantmentFocus::BREASTS,
        EnchantmentFocus::CROTCH_MAMMARIES,
        EnchantmentFocus::GENITALIA_PRIMARY,
        EnchantmentFocus::GENITALIA_SECONDARY,
        EnchantmentFocus::HIPS_ASS,
        EnchantmentFocus::LEGS_FEET,
        EnchantmentFocus::ARCANE_AMPLIFICATION,
        EnchantmentFocus::BINDING_SPECIAL
    };
}

std::vector<AspectProperty> EnchantmentRegistry::getAvailablePropertiesForFocus(EnchantmentFocus focus, const item* baseItem) const
{
    std::string fEnum = focusToEnumString(focus);
    auto it = m_focusToProps.find(fEnum);
    if (it != m_focusToProps.end() && !it->second.empty())
    {
        bool isRacial = baseItem && baseItem->isRacialReagent();
        std::vector<AspectProperty> result;

        for (const auto& pIdOrEnum : it->second)
        {
            AspectProperty prop = enumStringToProp(pIdOrEnum);
            if (prop == AspectProperty::NONE)
            {
                // Try looking up by ID in m_propsById
                auto pDef = m_propsById.find(pIdOrEnum);
                if (pDef != m_propsById.end())
                {
                    prop = enumStringToProp(pDef->second.id);
                }
            }
            if (prop == AspectProperty::NONE) continue;

            // Filter racial morph properties if not a racial reagent
            if (!isRacial)
            {
                if (prop == AspectProperty::CROTCH_MAMMARY_MORPH ||
                    prop == AspectProperty::RACIAL_PHALLUS_MORPH ||
                    prop == AspectProperty::RACIAL_SHEATH_MORPH ||
                    prop == AspectProperty::RACIAL_YONI_MORPH ||
                    prop == AspectProperty::RACIAL_LEGS_BIPED ||
                    prop == AspectProperty::RACIAL_STANCE_MORPH ||
                    prop == AspectProperty::RACIAL_BODY_CONFIG ||
                    prop == AspectProperty::FOOT_MORPH ||
                    prop == AspectProperty::RACIAL_FACIAL_STRUCTURE ||
                    prop == AspectProperty::RACIAL_MUZZLE_MORPH ||
                    prop == AspectProperty::RACIAL_DENTITION ||
                    prop == AspectProperty::RACIAL_TONGUE ||
                    prop == AspectProperty::RACIAL_MANE_MORPH ||
                    prop == AspectProperty::RACIAL_EAR_MORPH ||
                    prop == AspectProperty::RACIAL_COVERING_TYPE ||
                    prop == AspectProperty::RACIAL_PATTERN_COLOR ||
                    prop == AspectProperty::RACIAL_HORN_PRIMARY ||
                    prop == AspectProperty::RACIAL_HORN_VARIANT ||
                    prop == AspectProperty::RACIAL_WING_PRIMARY ||
                    prop == AspectProperty::RACIAL_WING_VARIANT ||
                    prop == AspectProperty::RACIAL_TAIL_PRIMARY ||
                    prop == AspectProperty::RACIAL_TAIL_VARIANT ||
                    prop == AspectProperty::ANTENNAE_MORPH)
                {
                    continue;
                }
            }

            result.push_back(prop);
        }

        if (!result.empty()) return result;
    }

    // Default fallback to legacy C++ properties if not found in JSON
    return {};
}

std::string EnchantmentRegistry::focusToEnumString(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HEAD_FEATURE:        return "HEAD_FEATURE";
        case EnchantmentFocus::HORNS:               return "HORNS";
        case EnchantmentFocus::HAIR:                return "HAIR";
        case EnchantmentFocus::EYES:                return "EYES";
        case EnchantmentFocus::EARS:                return "EARS";
        case EnchantmentFocus::MOUTH:               return "MOUTH";
        case EnchantmentFocus::FACE:                return "FACE";
        case EnchantmentFocus::SKIN:                return "SKIN";
        case EnchantmentFocus::ARMS:                return "ARMS";
        case EnchantmentFocus::TORSO:               return "TORSO";
        case EnchantmentFocus::BREASTS:             return "BREASTS";
        case EnchantmentFocus::WINGS:               return "WINGS";
        case EnchantmentFocus::TAIL:                return "TAIL";
        case EnchantmentFocus::GENITALIA_PRIMARY:   return "GENITALIA_PRIMARY";
        case EnchantmentFocus::GENITALIA_SECONDARY: return "GENITALIA_SECONDARY";
        case EnchantmentFocus::HIPS_ASS:            return "HIPS_ASS";
        case EnchantmentFocus::LEGS_FEET:           return "LEGS_FEET";
        case EnchantmentFocus::ARMOR_REINFORCEMENT: return "ARMOR_REINFORCEMENT";
        case EnchantmentFocus::WEAPON_LETHALITY:    return "WEAPON_LETHALITY";
        case EnchantmentFocus::ARCANE_AMPLIFICATION:return "ARCANE_AMPLIFICATION";
        case EnchantmentFocus::RESISTANCE_WARDING:  return "RESISTANCE_WARDING";
        case EnchantmentFocus::BINDING_SPECIAL:     return "BINDING_SPECIAL";
        case EnchantmentFocus::CORE_ATTRIBUTES:     return "CORE_ATTRIBUTES";
        case EnchantmentFocus::GENERAL_ATTRIBUTES:  return "GENERAL_ATTRIBUTES";
        case EnchantmentFocus::SPECIAL_EFFECTS:     return "SPECIAL_EFFECTS";
        case EnchantmentFocus::RETENTION_FLUIDS:    return "RETENTION_FLUIDS";
        case EnchantmentFocus::BODY_DESIRES:        return "BODY_DESIRES";
        case EnchantmentFocus::BEHAVIORAL_DESIRES:  return "BEHAVIORAL_DESIRES";
        case EnchantmentFocus::CROTCH_MAMMARIES:    return "CROTCH_MAMMARIES";
        case EnchantmentFocus::ANTENNAE:            return "ANTENNAE";
        case EnchantmentFocus::FLUIDS_CUM:          return "FLUIDS_CUM";
        case EnchantmentFocus::FLUIDS_MILK:         return "FLUIDS_MILK";
        case EnchantmentFocus::FLUIDS_GIRLCUM:      return "FLUIDS_GIRLCUM";
        case EnchantmentFocus::RACE_AWAKENING:      return "RACE_AWAKENING";
        default:                                    return "NONE";
    }
}

EnchantmentFocus EnchantmentRegistry::enumStringToFocus(const std::string& str)
{
    if (str == "HEAD_FEATURE" || str == "focus_head") return EnchantmentFocus::HEAD_FEATURE;
    if (str == "HORNS" || str == "focus_horns") return EnchantmentFocus::HORNS;
    if (str == "HAIR" || str == "focus_hair") return EnchantmentFocus::HAIR;
    if (str == "EYES" || str == "focus_eyes") return EnchantmentFocus::EYES;
    if (str == "EARS" || str == "focus_ears") return EnchantmentFocus::EARS;
    if (str == "MOUTH" || str == "focus_mouth") return EnchantmentFocus::MOUTH;
    if (str == "FACE" || str == "focus_face") return EnchantmentFocus::FACE;
    if (str == "SKIN" || str == "focus_skin") return EnchantmentFocus::SKIN;
    if (str == "ARMS" || str == "focus_arms") return EnchantmentFocus::ARMS;
    if (str == "TORSO" || str == "focus_torso") return EnchantmentFocus::TORSO;
    if (str == "BREASTS" || str == "focus_breasts") return EnchantmentFocus::BREASTS;
    if (str == "WINGS" || str == "focus_wings") return EnchantmentFocus::WINGS;
    if (str == "TAIL" || str == "focus_tail") return EnchantmentFocus::TAIL;
    if (str == "GENITALIA_PRIMARY" || str == "focus_genitalia_primary") return EnchantmentFocus::GENITALIA_PRIMARY;
    if (str == "GENITALIA_SECONDARY" || str == "focus_genitalia_secondary") return EnchantmentFocus::GENITALIA_SECONDARY;
    if (str == "HIPS_ASS" || str == "focus_hips_ass") return EnchantmentFocus::HIPS_ASS;
    if (str == "LEGS_FEET" || str == "focus_legs_feet") return EnchantmentFocus::LEGS_FEET;
    if (str == "ARMOR_REINFORCEMENT" || str == "focus_armor") return EnchantmentFocus::ARMOR_REINFORCEMENT;
    if (str == "WEAPON_LETHALITY" || str == "focus_weapon") return EnchantmentFocus::WEAPON_LETHALITY;
    if (str == "ARCANE_AMPLIFICATION" || str == "focus_arcane") return EnchantmentFocus::ARCANE_AMPLIFICATION;
    if (str == "RESISTANCE_WARDING" || str == "focus_resistance") return EnchantmentFocus::RESISTANCE_WARDING;
    if (str == "BINDING_SPECIAL" || str == "focus_binding") return EnchantmentFocus::BINDING_SPECIAL;
    if (str == "CORE_ATTRIBUTES" || str == "focus_core_attr") return EnchantmentFocus::CORE_ATTRIBUTES;
    if (str == "GENERAL_ATTRIBUTES" || str == "focus_gen_attr") return EnchantmentFocus::GENERAL_ATTRIBUTES;
    if (str == "SPECIAL_EFFECTS" || str == "focus_special_fx") return EnchantmentFocus::SPECIAL_EFFECTS;
    if (str == "RETENTION_FLUIDS" || str == "focus_retention") return EnchantmentFocus::RETENTION_FLUIDS;
    if (str == "BODY_DESIRES" || str == "focus_body_desires") return EnchantmentFocus::BODY_DESIRES;
    if (str == "BEHAVIORAL_DESIRES" || str == "focus_behav_desires") return EnchantmentFocus::BEHAVIORAL_DESIRES;
    if (str == "CROTCH_MAMMARIES" || str == "focus_crotch_mammaries") return EnchantmentFocus::CROTCH_MAMMARIES;
    if (str == "ANTENNAE" || str == "focus_antennae") return EnchantmentFocus::ANTENNAE;
    if (str == "FLUIDS_CUM" || str == "focus_fluids_cum") return EnchantmentFocus::FLUIDS_CUM;
    if (str == "FLUIDS_MILK" || str == "focus_fluids_milk") return EnchantmentFocus::FLUIDS_MILK;
    if (str == "FLUIDS_GIRLCUM" || str == "focus_fluids_girlcum") return EnchantmentFocus::FLUIDS_GIRLCUM;
    if (str == "RACE_AWAKENING" || str == "focus_race_awaken") return EnchantmentFocus::RACE_AWAKENING;
    return EnchantmentFocus::NONE;
}

std::string EnchantmentRegistry::propToEnumString(AspectProperty prop)
{
    return aspectPropertyToString(prop);
}

AspectProperty EnchantmentRegistry::enumStringToProp(const std::string& str)
{
    return stringToAspectProperty(str);
}
