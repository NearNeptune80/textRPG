#include "items/enchantmentAspects.h"

#include <unordered_map>
#include "items/item.h"
#include "items/infusionTier.h"

static const AspectDefinition s_noneDef = { "none", "None", "No modifier selected.", "", ItemRarity::COMMON };

static const std::unordered_map<EnchantmentFocus, AspectDefinition> s_focusDefs = {
    { EnchantmentFocus::HEAD_FEATURE,        { "focus_head", "Head Feature", "Focuses infusion upon the head, facial structure, or predatory senses.", "Predator's", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::HORNS,               { "focus_horns", "Horns & Crown", "Focuses infusion upon cranial horn growths and demonic crests.", "Curved", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::HAIR,                { "focus_hair", "Mane & Follicles", "Focuses infusion upon hair volume, length, and coloration.", "Flowing", ItemRarity::COMMON } },
    { EnchantmentFocus::EYES,                { "focus_eyes", "Ocular Senses", "Focuses infusion upon eye hue, pupil configuration, and darkvision.", "Piercing", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::EARS,                { "focus_ears", "Auditory Organ", "Focuses infusion upon ear morphology and heightened auditory perception.", "Keen", ItemRarity::COMMON } },
    { EnchantmentFocus::FACE,                { "focus_face", "Facial Countenance", "Focuses infusion upon visage contours, softness, or feral muzzle shape.", "Feral", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::SKIN,                { "focus_skin", "Dermis & Scales", "Focuses infusion upon epidermal texture, skin sheen, scales, or fur.", "Silken", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::ARMS,                { "focus_arms", "Upper Limbs & Claws", "Focuses infusion upon forearm power, claws, and grip strength.", "Mighty", ItemRarity::COMMON } },
    { EnchantmentFocus::TORSO,               { "focus_torso", "Torso & Core", "Focuses infusion upon the torso, abdominal wall, and muscular stature.", "Titan's", ItemRarity::RARE } },
    { EnchantmentFocus::BREASTS,             { "focus_breasts", "Mammary Glands", "Focuses infusion upon chest volume, areolae, and lactation capacity.", "Voluptuous", ItemRarity::RARE } },
    { EnchantmentFocus::WINGS,               { "focus_wings", "Wings & Membrane", "Focuses infusion upon bat or feather wing structures.", "Soaring", ItemRarity::EPIC } },
    { EnchantmentFocus::TAIL,                { "focus_tail", "Caudal Tail", "Focuses infusion upon tail morphology, fur tufts, or prehensile grace.", "Sinewy", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::GENITALIA_PRIMARY,   { "focus_genitalia_primary", "Phallus & Seed", "Focuses infusion upon virile member length, girth, and fluid emission.", "Stallion's", ItemRarity::RARE } },
    { EnchantmentFocus::GENITALIA_SECONDARY, { "focus_genitalia_secondary", "Yoni & Womb", "Focuses infusion upon vaginal elasticity, depth, and fertile receptivity.", "Lush", ItemRarity::RARE } },
    { EnchantmentFocus::HIPS_ASS,            { "focus_hips_ass", "Hips & Derriere", "Focuses infusion upon pelvic width, gluteal curvature, and posterior fullness.", "Curvaceous", ItemRarity::UNCOMMON } },

    { EnchantmentFocus::ARMOR_REINFORCEMENT, { "focus_armor", "Armor Reinforcement", "Focuses infusion upon garment resilience, weave density, and deflection.", "Warded", ItemRarity::RARE } },
    { EnchantmentFocus::WEAPON_LETHALITY,    { "focus_weapon", "Weapon Lethality", "Focuses infusion upon edge sharpness, impact velocity, and critical trauma.", "Striking", ItemRarity::RARE } },
    { EnchantmentFocus::ARCANE_AMPLIFICATION,{ "focus_arcane", "Arcane Amplification", "Focuses infusion upon magical resonance, spell potency, and aura capacity.", "Eldritch", ItemRarity::EPIC } },
    { EnchantmentFocus::RESISTANCE_WARDING,  { "focus_resistance", "Elemental Warding", "Focuses infusion upon shielding against elemental and physical harm.", "Impenetrable", ItemRarity::RARE } },
    { EnchantmentFocus::BINDING_SPECIAL,     { "focus_binding", "Soulbound Binding", "Focuses infusion upon inescapable binding, servitude seals, and arousal stimulation.", "Runic", ItemRarity::LEGENDARY } }
};

static const std::unordered_map<AspectProperty, AspectDefinition> s_propDefs = {
    { AspectProperty::SCALE_SIZE,          { "prop_scale_size", "Scale & Dimensions", "Magnifies or shrinks the primary dimension.", "Greater", ItemRarity::COMMON } },
    { AspectProperty::SECONDARY_SIZE,      { "prop_secondary_size", "Secondary Dimensions", "Modifies secondary dimensions such as width, thickness, or nipple girth.", "Broadened", ItemRarity::COMMON } },
    { AspectProperty::VOLUME_CAPACITY,     { "prop_volume_capacity", "Volume Capacity", "Expands fluid holding capacity or internal elasticity.", "Expansive", ItemRarity::UNCOMMON } },
    { AspectProperty::DEPTH,               { "prop_depth", "Depth & Accommodative Reach", "Increases depth of orifices or structural cavity reach.", "Deepened", ItemRarity::UNCOMMON } },
    { AspectProperty::ELASTICITY,          { "prop_elasticity", "Tissue Elasticity", "Enhances pliability and elastic rebound of bodily tissue.", "Pliant", ItemRarity::UNCOMMON } },
    { AspectProperty::FLUID_PRODUCTION,    { "prop_fluid_production", "Fluid Production", "Increases volume of lactation, essence fluids, or natural lubrication.", "Overflowing", ItemRarity::RARE } },
    { AspectProperty::REGENERATION_RATE,   { "prop_regeneration", "Fluid Regeneration", "Accelerates the biological replenishment rate of fluids.", "Quenched", ItemRarity::RARE } },
    { AspectProperty::HAIR_GROWTH,         { "prop_hair_growth", "Follicle Growth", "Accelerates follicle growth or shifts hair density.", "Luxuriant", ItemRarity::COMMON } },

    { AspectProperty::PHYSIQUE_STAT,       { "prop_physique", "Physique & Might", "Imbues brute muscle density, fortitude, and physical power.", "Might", ItemRarity::RARE } },
    { AspectProperty::ARCANE_STAT,         { "prop_arcane", "Arcane Resonance", "Attunes the essence to arcane channels and magical spellcasting.", "Clarity", ItemRarity::RARE } },
    { AspectProperty::AGILITY_STAT,        { "prop_agility", "Agility & Reflexes", "Enhances movement speed, balance, and critical evasion.", "Instinct", ItemRarity::RARE } },
    { AspectProperty::HEALTH_CEILING,      { "prop_health_ceiling", "Maximum Vitality", "Bolsters maximum health reserves.", "Vitality", ItemRarity::EPIC } },
    { AspectProperty::MANA_CEILING,        { "prop_mana_ceiling", "Maximum Aura", "Expands maximum mana pool and magical reserves.", "Aura", ItemRarity::EPIC } },
    { AspectProperty::VIRILITY_FACTOR,     { "prop_virility", "Virile Potency", "Dramatically surges virility, masculine arousal, and potency.", "Virility", ItemRarity::RARE } },
    { AspectProperty::FERTILITY_FACTOR,    { "prop_fertility", "Fertile Receptivity", "Elevates fertile receptivity, feminine hormonal response, and bloom.", "Fertility", ItemRarity::RARE } },
    { AspectProperty::CORRUPTION_AURA,     { "prop_corruption", "Demonic Corruption", "Induces seductive corruption and unrestrained libido.", "Sin", ItemRarity::EPIC } },

    { AspectProperty::SOULBOUND_SEAL,      { "prop_soulbound", "Soulbound Seal", "Affixes clothing permanently onto wearer until purged with essence.", "Soulbound", ItemRarity::EPIC } },
    { AspectProperty::SERVITUDE_INHIBITION, { "prop_servitude", "Servitude Binding", "Suppresses uninhibited transformations and seals willpower.", "Servitude", ItemRarity::EPIC } },
    { AspectProperty::SENSORY_VIBRATION,   { "prop_vibration", "Sensory Vibration", "Generates continuous rhythmic sensory stimulation.", "Buzzing", ItemRarity::RARE } }
};

const AspectDefinition& getFocusDefinition(EnchantmentFocus focus)
{
    auto it = s_focusDefs.find(focus);
    if (it != s_focusDefs.end()) return it->second;
    return s_noneDef;
}

const AspectDefinition& getPropertyDefinition(AspectProperty prop)
{
    auto it = s_propDefs.find(prop);
    if (it != s_propDefs.end()) return it->second;
    return s_noneDef;
}

std::vector<EnchantmentFocus> getAllEnchantmentFocuses()
{
    return {
        EnchantmentFocus::HEAD_FEATURE,
        EnchantmentFocus::HORNS,
        EnchantmentFocus::HAIR,
        EnchantmentFocus::EYES,
        EnchantmentFocus::EARS,
        EnchantmentFocus::FACE,
        EnchantmentFocus::SKIN,
        EnchantmentFocus::ARMS,
        EnchantmentFocus::TORSO,
        EnchantmentFocus::BREASTS,
        EnchantmentFocus::WINGS,
        EnchantmentFocus::TAIL,
        EnchantmentFocus::GENITALIA_PRIMARY,
        EnchantmentFocus::GENITALIA_SECONDARY,
        EnchantmentFocus::HIPS_ASS,
        EnchantmentFocus::ARMOR_REINFORCEMENT,
        EnchantmentFocus::WEAPON_LETHALITY,
        EnchantmentFocus::ARCANE_AMPLIFICATION,
        EnchantmentFocus::RESISTANCE_WARDING,
        EnchantmentFocus::BINDING_SPECIAL
    };
}

std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HEAD_FEATURE:
        case EnchantmentFocus::FACE:
        case EnchantmentFocus::EARS:
        case EnchantmentFocus::EYES:
            return { AspectProperty::SCALE_SIZE, AspectProperty::AGILITY_STAT, AspectProperty::CORRUPTION_AURA };

        case EnchantmentFocus::HAIR:
            return { AspectProperty::HAIR_GROWTH, AspectProperty::SCALE_SIZE };

        case EnchantmentFocus::HORNS:
        case EnchantmentFocus::WINGS:
        case EnchantmentFocus::TAIL:
            return { AspectProperty::SCALE_SIZE, AspectProperty::SECONDARY_SIZE, AspectProperty::CORRUPTION_AURA, AspectProperty::AGILITY_STAT };

        case EnchantmentFocus::ARMS:
        case EnchantmentFocus::TORSO:
            return { AspectProperty::SCALE_SIZE, AspectProperty::PHYSIQUE_STAT, AspectProperty::HEALTH_CEILING };

        case EnchantmentFocus::BREASTS:
            return { AspectProperty::SCALE_SIZE, AspectProperty::SECONDARY_SIZE, AspectProperty::VOLUME_CAPACITY, AspectProperty::FLUID_PRODUCTION, AspectProperty::REGENERATION_RATE, AspectProperty::FERTILITY_FACTOR };

        case EnchantmentFocus::HIPS_ASS:
            return { AspectProperty::SCALE_SIZE, AspectProperty::SECONDARY_SIZE, AspectProperty::VOLUME_CAPACITY, AspectProperty::DEPTH, AspectProperty::ELASTICITY };

        case EnchantmentFocus::GENITALIA_PRIMARY:
            return { AspectProperty::SCALE_SIZE, AspectProperty::SECONDARY_SIZE, AspectProperty::VOLUME_CAPACITY, AspectProperty::FLUID_PRODUCTION, AspectProperty::REGENERATION_RATE, AspectProperty::VIRILITY_FACTOR };

        case EnchantmentFocus::GENITALIA_SECONDARY:
            return { AspectProperty::SCALE_SIZE, AspectProperty::SECONDARY_SIZE, AspectProperty::VOLUME_CAPACITY, AspectProperty::DEPTH, AspectProperty::ELASTICITY, AspectProperty::FLUID_PRODUCTION, AspectProperty::FERTILITY_FACTOR };

        case EnchantmentFocus::ARMOR_REINFORCEMENT:
        case EnchantmentFocus::RESISTANCE_WARDING:
            return { AspectProperty::PHYSIQUE_STAT, AspectProperty::HEALTH_CEILING, AspectProperty::ARCANE_STAT };

        case EnchantmentFocus::WEAPON_LETHALITY:
            return { AspectProperty::PHYSIQUE_STAT, AspectProperty::AGILITY_STAT, AspectProperty::ARCANE_STAT };

        case EnchantmentFocus::ARCANE_AMPLIFICATION:
            return { AspectProperty::ARCANE_STAT, AspectProperty::MANA_CEILING, AspectProperty::CORRUPTION_AURA };

        case EnchantmentFocus::BINDING_SPECIAL:
            return { AspectProperty::SOULBOUND_SEAL, AspectProperty::SERVITUDE_INHIBITION, AspectProperty::SENSORY_VIBRATION };

        default:
            return { AspectProperty::PHYSIQUE_STAT, AspectProperty::ARCANE_STAT, AspectProperty::AGILITY_STAT };
    }
}

std::string enchantmentFocusToString(EnchantmentFocus focus)
{
    return getFocusDefinition(focus).id;
}

EnchantmentFocus stringToEnchantmentFocus(std::string_view str)
{
    for (const auto& [f, def] : s_focusDefs)
    {
        if (def.id == str) return f;
    }
    return EnchantmentFocus::NONE;
}

std::string aspectPropertyToString(AspectProperty prop)
{
    return getPropertyDefinition(prop).id;
}

AspectProperty stringToAspectProperty(std::string_view str)
{
    for (const auto& [p, def] : s_propDefs)
    {
        if (def.id == str) return p;
    }
    return AspectProperty::NONE;
}

bool isAnatomicalRacialFocus(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HORNS:
        case EnchantmentFocus::WINGS:
        case EnchantmentFocus::TAIL:
            return true;
        default:
            return false;
    }
}

bool isAnatomicalSizingFocus(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HEAD_FEATURE:
        case EnchantmentFocus::HAIR:
        case EnchantmentFocus::EYES:
        case EnchantmentFocus::EARS:
        case EnchantmentFocus::FACE:
        case EnchantmentFocus::SKIN:
        case EnchantmentFocus::ARMS:
        case EnchantmentFocus::TORSO:
        case EnchantmentFocus::BREASTS:
        case EnchantmentFocus::GENITALIA_PRIMARY:
        case EnchantmentFocus::GENITALIA_SECONDARY:
        case EnchantmentFocus::HIPS_ASS:
            return true;
        default:
            return false;
    }
}

bool isCombatEquipmentFocus(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::WEAPON_LETHALITY:
        case EnchantmentFocus::ARMOR_REINFORCEMENT:
        case EnchantmentFocus::ARCANE_AMPLIFICATION:
        case EnchantmentFocus::RESISTANCE_WARDING:
        case EnchantmentFocus::BINDING_SPECIAL:
            return true;
        default:
            return false;
    }
}

bool isFocusCompatibleWithItem(EnchantmentFocus focus, const item* baseItem)
{
    if (!baseItem)
    {
        return !isAnatomicalRacialFocus(focus);
    }

    bool isWeapon = (baseItem->category == ItemCategory::WEAPON ||
                     baseItem->targetSlot == equipSlot::WEAPON_MAIN ||
                     baseItem->targetSlot == equipSlot::WEAPON_OFF);

    bool isApparel = (baseItem->category == ItemCategory::CLOTHING ||
                      baseItem->category == ItemCategory::UNDERWEAR ||
                      baseItem->category == ItemCategory::ACCESSORY ||
                      (baseItem->isEquippable && !isWeapon));

    if (isWeapon)
    {
        // Weapons can ONLY receive combat equipment focuses (excluding armor reinforcement)
        return (focus == EnchantmentFocus::WEAPON_LETHALITY ||
                focus == EnchantmentFocus::ARCANE_AMPLIFICATION ||
                focus == EnchantmentFocus::RESISTANCE_WARDING ||
                focus == EnchantmentFocus::BINDING_SPECIAL);
    }

    if (isApparel)
    {
        // Apparel cannot receive racial transformations (horns, wings, tails)
        if (isAnatomicalRacialFocus(focus)) return false;
        // Apparel cannot receive weapon lethality
        if (focus == EnchantmentFocus::WEAPON_LETHALITY) return false;
        // Apparel can receive defensive/warding/special and anatomical sizing/modifiers
        return (isCombatEquipmentFocus(focus) || isAnatomicalSizingFocus(focus));
    }

    if (baseItem->isConsumable || baseItem->isFood)
    {
        // Racial transformations on consumables require a racial reagent/food
        if (isAnatomicalRacialFocus(focus))
        {
            return baseItem->isRacialReagent();
        }
        return true;
    }

    return !isAnatomicalRacialFocus(focus) || baseItem->isRacialReagent();
}

std::string getFocusLockReason(EnchantmentFocus focus, const item* baseItem)
{
    if (baseItem)
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
            return "Weapons cannot receive bodily transformatives. Weapons focus strictly on combat lethality, wards, and combat stats.";
        }

        if (isApparel && isAnatomicalRacialFocus(focus))
        {
            return "Apparel cannot hold racial body transformations (horns, wings, tails). Use for gradual part sizing, modifiers, or defense.";
        }

        if (isApparel && focus == EnchantmentFocus::WEAPON_LETHALITY)
        {
            return "Weapon lethality can only be applied to weapons.";
        }
    }

    if (isAnatomicalRacialFocus(focus))
    {
        return "Racial Transformation Locked: Requires a race-specific food or reagent (found in racial settlements or from vendors) to awaken racial resonance.";
    }

    return "";
}

std::string getFocusShortLabel(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HEAD_FEATURE:        return "Head";
        case EnchantmentFocus::HORNS:               return "Horns";
        case EnchantmentFocus::HAIR:                return "Hair";
        case EnchantmentFocus::EYES:                return "Eyes";
        case EnchantmentFocus::EARS:                return "Ears";
        case EnchantmentFocus::FACE:                return "Face";
        case EnchantmentFocus::SKIN:                return "Skin";
        case EnchantmentFocus::ARMS:                return "Arms";
        case EnchantmentFocus::TORSO:               return "Torso";
        case EnchantmentFocus::BREASTS:             return "Chest";
        case EnchantmentFocus::WINGS:               return "Wings";
        case EnchantmentFocus::TAIL:                return "Tail";
        case EnchantmentFocus::GENITALIA_PRIMARY:   return "Phallus";
        case EnchantmentFocus::GENITALIA_SECONDARY: return "Yoni";
        case EnchantmentFocus::HIPS_ASS:            return "Hips";
        case EnchantmentFocus::ARMOR_REINFORCEMENT: return "Armor";
        case EnchantmentFocus::WEAPON_LETHALITY:    return "Weapon";
        case EnchantmentFocus::ARCANE_AMPLIFICATION:return "Arcane";
        case EnchantmentFocus::RESISTANCE_WARDING:  return "Wards";
        case EnchantmentFocus::BINDING_SPECIAL:     return "Seals";
        default:                                    return "Focus";
    }
}

std::string getFocusIconGlyph(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HEAD_FEATURE:        return "HD";
        case EnchantmentFocus::HORNS:               return "HN";
        case EnchantmentFocus::HAIR:                return "HR";
        case EnchantmentFocus::EYES:                return "EY";
        case EnchantmentFocus::EARS:                return "ER";
        case EnchantmentFocus::FACE:                return "FC";
        case EnchantmentFocus::SKIN:                return "SK";
        case EnchantmentFocus::ARMS:                return "AM";
        case EnchantmentFocus::TORSO:               return "TR";
        case EnchantmentFocus::BREASTS:             return "BS";
        case EnchantmentFocus::WINGS:               return "WG";
        case EnchantmentFocus::TAIL:                return "TL";
        case EnchantmentFocus::GENITALIA_PRIMARY:   return "PH";
        case EnchantmentFocus::GENITALIA_SECONDARY: return "YN";
        case EnchantmentFocus::HIPS_ASS:            return "HP";
        case EnchantmentFocus::ARMOR_REINFORCEMENT: return "SH";
        case EnchantmentFocus::WEAPON_LETHALITY:    return "SW";
        case EnchantmentFocus::ARCANE_AMPLIFICATION:return "MG";
        case EnchantmentFocus::RESISTANCE_WARDING:  return "WD";
        case EnchantmentFocus::BINDING_SPECIAL:     return "SL";
        default:                                    return "??";
    }
}

std::string getPropertyShortLabel(AspectProperty prop)
{
    switch (prop)
    {
        case AspectProperty::SCALE_SIZE:          return "Scale";
        case AspectProperty::SECONDARY_SIZE:      return "Width";
        case AspectProperty::VOLUME_CAPACITY:     return "Volume";
        case AspectProperty::DEPTH:               return "Depth";
        case AspectProperty::ELASTICITY:          return "Elastic";
        case AspectProperty::FLUID_PRODUCTION:    return "Fluids";
        case AspectProperty::REGENERATION_RATE:   return "Regen";
        case AspectProperty::HAIR_GROWTH:         return "Growth";
        case AspectProperty::PHYSIQUE_STAT:       return "Physique";
        case AspectProperty::ARCANE_STAT:         return "Arcane";
        case AspectProperty::AGILITY_STAT:        return "Agility";
        case AspectProperty::HEALTH_CEILING:      return "Health";
        case AspectProperty::MANA_CEILING:        return "Mana";
        case AspectProperty::VIRILITY_FACTOR:     return "Virility";
        case AspectProperty::FERTILITY_FACTOR:    return "Fertility";
        case AspectProperty::CORRUPTION_AURA:     return "Corrupt";
        case AspectProperty::SOULBOUND_SEAL:      return "Soulbound";
        case AspectProperty::SERVITUDE_INHIBITION: return "Servitude";
        case AspectProperty::SENSORY_VIBRATION:   return "Sensory";
        default:                                  return "Prop";
    }
}

std::string getGradualTimeInterval(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::GREATER_BOON:
        case InfusionTier::MAJOR_HEX:
            return "Hourly gradual shift (Ticks every hour worn)";
        case InfusionTier::BOON:
        case InfusionTier::HEX:
            return "Daily gradual shift (Ticks every day worn)";
        case InfusionTier::MINOR_BOON:
        case InfusionTier::MINOR_HEX:
        default:
            return "Weekly gradual shift (Ticks every week worn)";
    }
}

