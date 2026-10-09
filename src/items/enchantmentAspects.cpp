#include "items/enchantmentAspects.h"
#include "items/enchantmentRegistry.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include "items/item.h"
#include "items/infusionTier.h"

static const AspectDefinition s_noneDef = { "none", "None", "No modifier selected.", "", ItemRarity::COMMON };

const AspectDefinition& getFocusDefinition(EnchantmentFocus focus)
{
    const auto* def = EnchantmentRegistry::getInstance().getFocusDef(focus);
    if (def) return *def;
    return s_noneDef;
}

const AspectDefinition& getPropertyDefinition(AspectProperty prop)
{
    const auto* def = EnchantmentRegistry::getInstance().getPropertyDef(prop);
    if (def) return *def;
    return s_noneDef;
}

std::vector<EnchantmentFocus> getAllEnchantmentFocuses()
{
    return {
        EnchantmentFocus::CORE_ATTRIBUTES,
        EnchantmentFocus::GENERAL_ATTRIBUTES,
        EnchantmentFocus::SPECIAL_EFFECTS,
        EnchantmentFocus::RETENTION_FLUIDS,
        EnchantmentFocus::BODY_DESIRES,
        EnchantmentFocus::BEHAVIORAL_DESIRES,
        EnchantmentFocus::FACE,
        EnchantmentFocus::TORSO,
        EnchantmentFocus::ARMS,
        EnchantmentFocus::HAIR,
        EnchantmentFocus::HIPS_ASS,
        EnchantmentFocus::BREASTS,
        EnchantmentFocus::CROTCH_MAMMARIES,
        EnchantmentFocus::GENITALIA_PRIMARY,
        EnchantmentFocus::GENITALIA_SECONDARY,
        EnchantmentFocus::LEGS_FEET,
        EnchantmentFocus::HEAD_FEATURE,
        EnchantmentFocus::HORNS,
        EnchantmentFocus::ANTENNAE,
        EnchantmentFocus::WINGS,
        EnchantmentFocus::TAIL,
        EnchantmentFocus::EYES,
        EnchantmentFocus::EARS,
        EnchantmentFocus::MOUTH,
        EnchantmentFocus::SKIN,
        EnchantmentFocus::FLUIDS_CUM,
        EnchantmentFocus::FLUIDS_MILK,
        EnchantmentFocus::FLUIDS_GIRLCUM,
        EnchantmentFocus::RACE_AWAKENING,
        EnchantmentFocus::ARMOR_REINFORCEMENT,
        EnchantmentFocus::WEAPON_LETHALITY,
        EnchantmentFocus::ARCANE_AMPLIFICATION,
        EnchantmentFocus::RESISTANCE_WARDING,
        EnchantmentFocus::BINDING_SPECIAL
    };
}

std::vector<EnchantmentFocus> getCompatibleFocuses(const item* baseItem)
{
    return EnchantmentRegistry::getInstance().getCompatibleFocuses(baseItem);
}

std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus, const item* baseItem)
{
    return EnchantmentRegistry::getInstance().getAvailablePropertiesForFocus(focus, baseItem);
}

std::string enchantmentFocusToString(EnchantmentFocus focus)
{
    return getFocusDefinition(focus).id;
}

EnchantmentFocus stringToEnchantmentFocus(std::string_view str)
{
    return EnchantmentRegistry::enumStringToFocus(std::string(str));
}

std::string aspectPropertyToString(AspectProperty prop)
{
    return getPropertyDefinition(prop).id;
}

AspectProperty stringToAspectProperty(std::string_view str)
{
    static const std::unordered_map<std::string_view, AspectProperty> s_propMap = {
        { "BREAST_SIZE", AspectProperty::BREAST_SIZE },
        { "prop_breast_size", AspectProperty::BREAST_SIZE },
        { "BREAST_SHAPE", AspectProperty::BREAST_SHAPE },
        { "prop_breast_shape", AspectProperty::BREAST_SHAPE },
        { "NIPPLE_LENGTH", AspectProperty::NIPPLE_LENGTH },
        { "prop_nipple_length", AspectProperty::NIPPLE_LENGTH },
        { "NIPPLE_GIRTH", AspectProperty::NIPPLE_GIRTH },
        { "prop_nipple_girth", AspectProperty::NIPPLE_GIRTH },
        { "NIPPLE_TYPE", AspectProperty::NIPPLE_TYPE },
        { "prop_nipple_type", AspectProperty::NIPPLE_TYPE },
        { "NIPPLE_CAPACITY", AspectProperty::NIPPLE_CAPACITY },
        { "prop_nipple_capacity", AspectProperty::NIPPLE_CAPACITY },
        { "LACTATION_VOLUME", AspectProperty::LACTATION_VOLUME },
        { "prop_lactation_volume", AspectProperty::LACTATION_VOLUME },
        { "LACTATION_REGEN", AspectProperty::LACTATION_REGEN },
        { "prop_lactation_regen", AspectProperty::LACTATION_REGEN },
        { "FLUID_TYPE", AspectProperty::FLUID_TYPE },
        { "prop_fluid_type", AspectProperty::FLUID_TYPE },
        { "CROTCH_MAMMARY_MORPH", AspectProperty::CROTCH_MAMMARY_MORPH },
        { "prop_crotch_mammary", AspectProperty::CROTCH_MAMMARY_MORPH },
        { "PENIS_LENGTH", AspectProperty::PENIS_LENGTH },
        { "prop_penis_length", AspectProperty::PENIS_LENGTH },
        { "PENIS_GIRTH", AspectProperty::PENIS_GIRTH },
        { "prop_penis_girth", AspectProperty::PENIS_GIRTH },
        { "KNOT_SIZE", AspectProperty::KNOT_SIZE },
        { "prop_knot_size", AspectProperty::KNOT_SIZE },
        { "TESTES_SIZE", AspectProperty::TESTES_SIZE },
        { "prop_testes_size", AspectProperty::TESTES_SIZE },
        { "CUM_VOLUME", AspectProperty::CUM_VOLUME },
        { "prop_cum_volume", AspectProperty::CUM_VOLUME },
        { "CUM_REGEN", AspectProperty::CUM_REGEN },
        { "prop_cum_regen", AspectProperty::CUM_REGEN },
        { "VIRILITY_POTENCY", AspectProperty::VIRILITY_POTENCY },
        { "prop_virility_potency", AspectProperty::VIRILITY_POTENCY },
        { "RACIAL_PHALLUS_MORPH", AspectProperty::RACIAL_PHALLUS_MORPH },
        { "prop_racial_phallus", AspectProperty::RACIAL_PHALLUS_MORPH },
        { "RACIAL_SHEATH_MORPH", AspectProperty::RACIAL_SHEATH_MORPH },
        { "prop_racial_sheath", AspectProperty::RACIAL_SHEATH_MORPH },
        { "VAGINA_DEPTH", AspectProperty::VAGINA_DEPTH },
        { "prop_vagina_depth", AspectProperty::VAGINA_DEPTH },
        { "VAGINA_TIGHTNESS", AspectProperty::VAGINA_TIGHTNESS },
        { "prop_vagina_tightness", AspectProperty::VAGINA_TIGHTNESS },
        { "CLIT_SIZE", AspectProperty::CLIT_SIZE },
        { "prop_clit_size", AspectProperty::CLIT_SIZE },
        { "LABIA_SIZE", AspectProperty::LABIA_SIZE },
        { "prop_labia_size", AspectProperty::LABIA_SIZE },
        { "LUBRICATION_WETNESS", AspectProperty::LUBRICATION_WETNESS },
        { "prop_lube_wetness", AspectProperty::LUBRICATION_WETNESS },
        { "FERTILITY_RECEPTIVITY", AspectProperty::FERTILITY_RECEPTIVITY },
        { "prop_fertility_receptivity", AspectProperty::FERTILITY_RECEPTIVITY },
        { "RACIAL_YONI_MORPH", AspectProperty::RACIAL_YONI_MORPH },
        { "prop_racial_yoni", AspectProperty::RACIAL_YONI_MORPH },
        { "BUTT_SIZE", AspectProperty::BUTT_SIZE },
        { "prop_butt_size", AspectProperty::BUTT_SIZE },
        { "HIP_WIDTH", AspectProperty::HIP_WIDTH },
        { "prop_hip_width", AspectProperty::HIP_WIDTH },
        { "ANUS_CAPACITY", AspectProperty::ANUS_CAPACITY },
        { "prop_anus_capacity", AspectProperty::ANUS_CAPACITY },
        { "ANUS_DEPTH", AspectProperty::ANUS_DEPTH },
        { "prop_anus_depth", AspectProperty::ANUS_DEPTH },
        { "ANUS_ELASTICITY", AspectProperty::ANUS_ELASTICITY },
        { "prop_anus_elasticity", AspectProperty::ANUS_ELASTICITY },
        { "LEG_LENGTH", AspectProperty::LEG_LENGTH },
        { "prop_leg_length", AspectProperty::LEG_LENGTH },
        { "THIGH_FULLNESS", AspectProperty::THIGH_FULLNESS },
        { "prop_thigh_fullness", AspectProperty::THIGH_FULLNESS },
        { "RACIAL_LEGS_BIPED", AspectProperty::RACIAL_LEGS_BIPED },
        { "prop_racial_legs_biped", AspectProperty::RACIAL_LEGS_BIPED },
        { "RACIAL_STANCE_MORPH", AspectProperty::RACIAL_STANCE_MORPH },
        { "prop_racial_stance", AspectProperty::RACIAL_STANCE_MORPH },
        { "RACIAL_BODY_CONFIG", AspectProperty::RACIAL_BODY_CONFIG },
        { "prop_racial_body_config", AspectProperty::RACIAL_BODY_CONFIG },
        { "SPRINT_AGILITY", AspectProperty::SPRINT_AGILITY },
        { "prop_sprint_agility", AspectProperty::SPRINT_AGILITY },
        { "FACE_SHAPE", AspectProperty::FACE_SHAPE },
        { "prop_face_shape", AspectProperty::FACE_SHAPE },
        { "RACIAL_FACIAL_STRUCTURE", AspectProperty::RACIAL_FACIAL_STRUCTURE },
        { "prop_racial_face", AspectProperty::RACIAL_FACIAL_STRUCTURE },
        { "RACIAL_MUZZLE_MORPH", AspectProperty::RACIAL_MUZZLE_MORPH },
        { "prop_racial_muzzle", AspectProperty::RACIAL_MUZZLE_MORPH },
        { "PREDATORY_PERCEPTION", AspectProperty::PREDATORY_PERCEPTION },
        { "prop_predatory_perception", AspectProperty::PREDATORY_PERCEPTION },
        { "LIP_FULLNESS", AspectProperty::LIP_FULLNESS },
        { "prop_lip_fullness", AspectProperty::LIP_FULLNESS },
        { "RACIAL_DENTITION", AspectProperty::RACIAL_DENTITION },
        { "prop_racial_dentition", AspectProperty::RACIAL_DENTITION },
        { "RACIAL_TONGUE", AspectProperty::RACIAL_TONGUE },
        { "prop_racial_tongue", AspectProperty::RACIAL_TONGUE },
        { "THROAT_DEPTH", AspectProperty::THROAT_DEPTH },
        { "prop_throat_depth", AspectProperty::THROAT_DEPTH },
        { "SALIVA_PRODUCTION", AspectProperty::SALIVA_PRODUCTION },
        { "prop_saliva_prod", AspectProperty::SALIVA_PRODUCTION },
        { "HAIR_GROWTH_RATE", AspectProperty::HAIR_GROWTH_RATE },
        { "prop_hair_growth_rate", AspectProperty::HAIR_GROWTH_RATE },
        { "HAIR_LENGTH", AspectProperty::HAIR_LENGTH },
        { "prop_hair_length", AspectProperty::HAIR_LENGTH },
        { "HAIR_VOLUME", AspectProperty::HAIR_VOLUME },
        { "prop_hair_volume", AspectProperty::HAIR_VOLUME },
        { "HAIR_STYLE", AspectProperty::HAIR_STYLE },
        { "prop_hair_style", AspectProperty::HAIR_STYLE },
        { "HAIR_COLOR", AspectProperty::HAIR_COLOR },
        { "prop_hair_color", AspectProperty::HAIR_COLOR },
        { "RACIAL_MANE_MORPH", AspectProperty::RACIAL_MANE_MORPH },
        { "prop_racial_mane", AspectProperty::RACIAL_MANE_MORPH },
        { "EYE_PUPIL_SHAPE", AspectProperty::EYE_PUPIL_SHAPE },
        { "prop_eye_pupils", AspectProperty::EYE_PUPIL_SHAPE },
        { "EYE_IRIS_COLOR", AspectProperty::EYE_IRIS_COLOR },
        { "prop_eye_color", AspectProperty::EYE_IRIS_COLOR },
        { "DARKVISION_AURA", AspectProperty::DARKVISION_AURA },
        { "prop_darkvision", AspectProperty::DARKVISION_AURA },
        { "ALLURING_GAZE", AspectProperty::ALLURING_GAZE },
        { "prop_alluring_gaze", AspectProperty::ALLURING_GAZE },
        { "EAR_SIZE", AspectProperty::EAR_SIZE },
        { "prop_ear_size", AspectProperty::EAR_SIZE },
        { "RACIAL_EAR_MORPH", AspectProperty::RACIAL_EAR_MORPH },
        { "prop_racial_ears", AspectProperty::RACIAL_EAR_MORPH },
        { "KEEN_HEARING", AspectProperty::KEEN_HEARING },
        { "prop_keen_hearing", AspectProperty::KEEN_HEARING },
        { "STATURE_HEIGHT", AspectProperty::STATURE_HEIGHT },
        { "prop_height", AspectProperty::STATURE_HEIGHT },
        { "MUSCLE_PHYSIQUE", AspectProperty::MUSCLE_PHYSIQUE },
        { "prop_muscle_physique", AspectProperty::MUSCLE_PHYSIQUE },
        { "WAIST_TAPER", AspectProperty::WAIST_TAPER },
        { "prop_waist_taper", AspectProperty::WAIST_TAPER },
        { "STOMACH_FIRMNESS", AspectProperty::STOMACH_FIRMNESS },
        { "prop_stomach_tone", AspectProperty::STOMACH_FIRMNESS },
        { "HEALTH_VITALITY", AspectProperty::HEALTH_VITALITY },
        { "prop_health_vitality", AspectProperty::HEALTH_VITALITY },
        { "RACIAL_COVERING_TYPE", AspectProperty::RACIAL_COVERING_TYPE },
        { "prop_racial_covering", AspectProperty::RACIAL_COVERING_TYPE },
        { "RACIAL_PATTERN_COLOR", AspectProperty::RACIAL_PATTERN_COLOR },
        { "prop_racial_pattern", AspectProperty::RACIAL_PATTERN_COLOR },
        { "DERMIS_ELASTICITY", AspectProperty::DERMIS_ELASTICITY },
        { "prop_dermis_elasticity", AspectProperty::DERMIS_ELASTICITY },
        { "NATURAL_ARMOR", AspectProperty::NATURAL_ARMOR },
        { "prop_natural_armor", AspectProperty::NATURAL_ARMOR },
        { "ARM_MUSCLE", AspectProperty::ARM_MUSCLE },
        { "prop_arm_muscle", AspectProperty::ARM_MUSCLE },
        { "CLAWS_NAILS", AspectProperty::CLAWS_NAILS },
        { "prop_claws_nails", AspectProperty::CLAWS_NAILS },
        { "MANUAL_DEXTERITY", AspectProperty::MANUAL_DEXTERITY },
        { "prop_manual_dexterity", AspectProperty::MANUAL_DEXTERITY },
        { "HORN_SIZE", AspectProperty::HORN_SIZE },
        { "prop_horn_size", AspectProperty::HORN_SIZE },
        { "HORN_SHAPE", AspectProperty::HORN_SHAPE },
        { "prop_horn_shape", AspectProperty::HORN_SHAPE },
        { "HORN_TEXTURE", AspectProperty::HORN_TEXTURE },
        { "prop_horn_texture", AspectProperty::HORN_TEXTURE },
        { "RACIAL_HORN_PRIMARY", AspectProperty::RACIAL_HORN_PRIMARY },
        { "prop_racial_horn_primary", AspectProperty::RACIAL_HORN_PRIMARY },
        { "RACIAL_HORN_VARIANT", AspectProperty::RACIAL_HORN_VARIANT },
        { "prop_racial_horn_variant", AspectProperty::RACIAL_HORN_VARIANT },
        { "WING_SIZE", AspectProperty::WING_SIZE },
        { "prop_wing_size", AspectProperty::WING_SIZE },
        { "WING_TYPE", AspectProperty::WING_TYPE },
        { "prop_wing_type", AspectProperty::WING_TYPE },
        { "GLIDING_FLIGHT", AspectProperty::GLIDING_FLIGHT },
        { "prop_gliding_flight", AspectProperty::GLIDING_FLIGHT },
        { "RACIAL_WING_PRIMARY", AspectProperty::RACIAL_WING_PRIMARY },
        { "prop_racial_wing_primary", AspectProperty::RACIAL_WING_PRIMARY },
        { "RACIAL_WING_VARIANT", AspectProperty::RACIAL_WING_VARIANT },
        { "prop_racial_wing_variant", AspectProperty::RACIAL_WING_VARIANT },
        { "TAIL_LENGTH", AspectProperty::TAIL_LENGTH },
        { "prop_tail_length", AspectProperty::TAIL_LENGTH },
        { "TAIL_GIRTH", AspectProperty::TAIL_GIRTH },
        { "prop_tail_girth", AspectProperty::TAIL_GIRTH },
        { "TAIL_TYPE", AspectProperty::TAIL_TYPE },
        { "prop_tail_type", AspectProperty::TAIL_TYPE },
        { "RACIAL_TAIL_PRIMARY", AspectProperty::RACIAL_TAIL_PRIMARY },
        { "prop_racial_tail_primary", AspectProperty::RACIAL_TAIL_PRIMARY },
        { "RACIAL_TAIL_VARIANT", AspectProperty::RACIAL_TAIL_VARIANT },
        { "prop_racial_tail_variant", AspectProperty::RACIAL_TAIL_VARIANT },
        { "PART_REMOVAL", AspectProperty::PART_REMOVAL },
        { "prop_part_removal", AspectProperty::PART_REMOVAL },
        { "DAMAGE_PHYSICAL", AspectProperty::DAMAGE_PHYSICAL },
        { "prop_damage_physical", AspectProperty::DAMAGE_PHYSICAL },
        { "ATTACK_POWER", AspectProperty::ATTACK_POWER },
        { "prop_attack_power", AspectProperty::ATTACK_POWER },
        { "STRIKE_VELOCITY", AspectProperty::STRIKE_VELOCITY },
        { "prop_strike_velocity", AspectProperty::STRIKE_VELOCITY },
        { "CRITICAL_POWER", AspectProperty::CRITICAL_POWER },
        { "prop_critical_power", AspectProperty::CRITICAL_POWER },
        { "LIFE_LEECH", AspectProperty::LIFE_LEECH },
        { "prop_life_leech", AspectProperty::LIFE_LEECH },
        { "DAMAGE_ELEMENTAL", AspectProperty::DAMAGE_ELEMENTAL },
        { "prop_damage_elemental", AspectProperty::DAMAGE_ELEMENTAL },
        { "ARCANE_STAT", AspectProperty::ARCANE_STAT },
        { "prop_arcane", AspectProperty::ARCANE_STAT },
        { "MANA_CEILING", AspectProperty::MANA_CEILING },
        { "prop_mana_ceiling", AspectProperty::MANA_CEILING },
        { "MANA_REGENERATION", AspectProperty::MANA_REGENERATION },
        { "prop_mana_regen", AspectProperty::MANA_REGENERATION },
        { "ARMOR_RATING", AspectProperty::ARMOR_RATING },
        { "prop_armor_rating", AspectProperty::ARMOR_RATING },
        { "FORTITUDE_STAT", AspectProperty::FORTITUDE_STAT },
        { "prop_fortitude", AspectProperty::FORTITUDE_STAT },
        { "WARD_RESISTANCE", AspectProperty::WARD_RESISTANCE },
        { "prop_ward_resistance", AspectProperty::WARD_RESISTANCE },
        { "MIND_WARD", AspectProperty::MIND_WARD },
        { "prop_mind_ward", AspectProperty::MIND_WARD },
        { "SOULBOUND_SEAL", AspectProperty::SOULBOUND_SEAL },
        { "prop_soulbound", AspectProperty::SOULBOUND_SEAL },
        { "SERVITUDE_INHIBITION", AspectProperty::SERVITUDE_INHIBITION },
        { "prop_servitude", AspectProperty::SERVITUDE_INHIBITION },
        { "SENSORY_VIBRATION", AspectProperty::SENSORY_VIBRATION },
        { "prop_vibration", AspectProperty::SENSORY_VIBRATION },
        { "SCALE_SIZE", AspectProperty::SCALE_SIZE },
        { "prop_scale_size", AspectProperty::SCALE_SIZE },
        { "SECONDARY_SIZE", AspectProperty::SECONDARY_SIZE },
        { "prop_secondary_size", AspectProperty::SECONDARY_SIZE },
        { "VOLUME_CAPACITY", AspectProperty::VOLUME_CAPACITY },
        { "prop_volume_capacity", AspectProperty::VOLUME_CAPACITY },
        { "DEPTH", AspectProperty::DEPTH },
        { "prop_depth", AspectProperty::DEPTH },
        { "ELASTICITY", AspectProperty::ELASTICITY },
        { "prop_elasticity", AspectProperty::ELASTICITY },
        { "FLUID_PRODUCTION", AspectProperty::FLUID_PRODUCTION },
        { "prop_fluid_production", AspectProperty::FLUID_PRODUCTION },
        { "REGENERATION_RATE", AspectProperty::REGENERATION_RATE },
        { "prop_regeneration", AspectProperty::REGENERATION_RATE },
        { "HAIR_GROWTH", AspectProperty::HAIR_GROWTH },
        { "prop_hair_growth", AspectProperty::HAIR_GROWTH },
        { "PHYSIQUE_STAT", AspectProperty::PHYSIQUE_STAT },
        { "prop_physique", AspectProperty::PHYSIQUE_STAT },
        { "AGILITY_STAT", AspectProperty::AGILITY_STAT },
        { "prop_agility", AspectProperty::AGILITY_STAT },
        { "HEALTH_CEILING", AspectProperty::HEALTH_CEILING },
        { "prop_health_ceiling", AspectProperty::HEALTH_CEILING },
        { "VIRILITY_FACTOR", AspectProperty::VIRILITY_FACTOR },
        { "prop_virility", AspectProperty::VIRILITY_FACTOR },
        { "FERTILITY_FACTOR", AspectProperty::FERTILITY_FACTOR },
        { "prop_fertility", AspectProperty::FERTILITY_FACTOR },
        { "CORRUPTION_AURA", AspectProperty::CORRUPTION_AURA },
        { "prop_corruption", AspectProperty::CORRUPTION_AURA },
        { "RACIAL_TRANSFORMATION", AspectProperty::RACIAL_TRANSFORMATION },
        { "prop_racial_tf", AspectProperty::RACIAL_TRANSFORMATION },
        { "CORE_HEALTH", AspectProperty::CORE_HEALTH },
        { "prop_core_health", AspectProperty::CORE_HEALTH },
        { "CORE_MANA", AspectProperty::CORE_MANA },
        { "prop_core_mana", AspectProperty::CORE_MANA },
        { "CORE_STAMINA", AspectProperty::CORE_STAMINA },
        { "prop_core_stamina", AspectProperty::CORE_STAMINA },
        { "CORE_PHYSIQUE", AspectProperty::CORE_PHYSIQUE },
        { "prop_core_physique", AspectProperty::CORE_PHYSIQUE },
        { "CORE_ARCANE", AspectProperty::CORE_ARCANE },
        { "prop_core_arcane", AspectProperty::CORE_ARCANE },
        { "CORE_CORRUPTION", AspectProperty::CORE_CORRUPTION },
        { "prop_core_corruption", AspectProperty::CORE_CORRUPTION },
        { "ATTR_PHYSICAL_DAMAGE", AspectProperty::ATTR_PHYSICAL_DAMAGE },
        { "prop_attr_phys_dmg", AspectProperty::ATTR_PHYSICAL_DAMAGE },
        { "ATTR_SPELL_POWER", AspectProperty::ATTR_SPELL_POWER },
        { "prop_attr_spell_pwr", AspectProperty::ATTR_SPELL_POWER },
        { "ATTR_ARMOR", AspectProperty::ATTR_ARMOR },
        { "prop_attr_armor", AspectProperty::ATTR_ARMOR },
        { "ATTR_WARDING", AspectProperty::ATTR_WARDING },
        { "prop_attr_warding", AspectProperty::ATTR_WARDING },
        { "ATTR_FERTILITY", AspectProperty::ATTR_FERTILITY },
        { "prop_attr_fertility", AspectProperty::ATTR_FERTILITY },
        { "ATTR_VIRILITY", AspectProperty::ATTR_VIRILITY },
        { "prop_attr_virility", AspectProperty::ATTR_VIRILITY },
        { "ATTR_CRITICAL", AspectProperty::ATTR_CRITICAL },
        { "prop_attr_critical", AspectProperty::ATTR_CRITICAL },
        { "ATTR_AGILITY", AspectProperty::ATTR_AGILITY },
        { "prop_attr_agility", AspectProperty::ATTR_AGILITY },
        { "CONCEAL_IDENTITY", AspectProperty::CONCEAL_IDENTITY },
        { "prop_conceal_identity", AspectProperty::CONCEAL_IDENTITY },
        { "RETENTION_STOMACH", AspectProperty::RETENTION_STOMACH },
        { "prop_retention_stomach", AspectProperty::RETENTION_STOMACH },
        { "RETENTION_MAMMARY", AspectProperty::RETENTION_MAMMARY },
        { "prop_retention_mammary", AspectProperty::RETENTION_MAMMARY },
        { "RETENTION_YONI", AspectProperty::RETENTION_YONI },
        { "prop_retention_yoni", AspectProperty::RETENTION_YONI },
        { "RETENTION_ANAL", AspectProperty::RETENTION_ANAL },
        { "prop_retention_anal", AspectProperty::RETENTION_ANAL },
        { "DESIRE_ANAL", AspectProperty::DESIRE_ANAL },
        { "prop_desire_anal", AspectProperty::DESIRE_ANAL },
        { "DESIRE_MAMMARY", AspectProperty::DESIRE_MAMMARY },
        { "prop_desire_mammary", AspectProperty::DESIRE_MAMMARY },
        { "DESIRE_PHALLIC", AspectProperty::DESIRE_PHALLIC },
        { "prop_desire_phallic", AspectProperty::DESIRE_PHALLIC },
        { "DESIRE_YONI", AspectProperty::DESIRE_YONI },
        { "prop_desire_yoni", AspectProperty::DESIRE_YONI },
        { "DESIRE_FEET", AspectProperty::DESIRE_FEET },
        { "prop_desire_feet", AspectProperty::DESIRE_FEET },
        { "DESIRE_DOMINANT", AspectProperty::DESIRE_DOMINANT },
        { "prop_desire_dominant", AspectProperty::DESIRE_DOMINANT },
        { "DESIRE_SUBMISSIVE", AspectProperty::DESIRE_SUBMISSIVE },
        { "prop_desire_submissive", AspectProperty::DESIRE_SUBMISSIVE },
        { "DESIRE_BONDAGE", AspectProperty::DESIRE_BONDAGE },
        { "prop_desire_bondage", AspectProperty::DESIRE_BONDAGE },
        { "DESIRE_EXHIBITIONISM", AspectProperty::DESIRE_EXHIBITIONISM },
        { "prop_desire_exhibitionism", AspectProperty::DESIRE_EXHIBITIONISM },
        { "DESIRE_CHASTITY", AspectProperty::DESIRE_CHASTITY },
        { "prop_desire_chastity", AspectProperty::DESIRE_CHASTITY },
        { "DESIRE_MASOCHISM", AspectProperty::DESIRE_MASOCHISM },
        { "prop_desire_masochism", AspectProperty::DESIRE_MASOCHISM },
        { "DESIRE_SADISM", AspectProperty::DESIRE_SADISM },
        { "prop_desire_sadism", AspectProperty::DESIRE_SADISM },
        { "NOSE_SIZE", AspectProperty::NOSE_SIZE },
        { "prop_nose_size", AspectProperty::NOSE_SIZE },
        { "FACIAL_BEARD", AspectProperty::FACIAL_BEARD },
        { "prop_facial_beard", AspectProperty::FACIAL_BEARD },
        { "EYEBROW_SHAPE", AspectProperty::EYEBROW_SHAPE },
        { "prop_eyebrow_shape", AspectProperty::EYEBROW_SHAPE },
        { "FEMININITY_MASCULINITY", AspectProperty::FEMININITY_MASCULINITY },
        { "prop_fem_masc", AspectProperty::FEMININITY_MASCULINITY },
        { "BODY_HAIR", AspectProperty::BODY_HAIR },
        { "prop_body_hair", AspectProperty::BODY_HAIR },
        { "UNDERARM_HAIR", AspectProperty::UNDERARM_HAIR },
        { "prop_underarm_hair", AspectProperty::UNDERARM_HAIR },
        { "GRIP_STRENGTH", AspectProperty::GRIP_STRENGTH },
        { "prop_grip_strength", AspectProperty::GRIP_STRENGTH },
        { "ANUS_WETNESS", AspectProperty::ANUS_WETNESS },
        { "prop_anus_wetness", AspectProperty::ANUS_WETNESS },
        { "AREOLA_SIZE", AspectProperty::AREOLA_SIZE },
        { "prop_areola_size", AspectProperty::AREOLA_SIZE },
        { "MILK_FLAVOR", AspectProperty::MILK_FLAVOR },
        { "prop_milk_flavor", AspectProperty::MILK_FLAVOR },
        { "UDDER_SIZE", AspectProperty::UDDER_SIZE },
        { "prop_udder_size", AspectProperty::UDDER_SIZE },
        { "TEAT_LENGTH", AspectProperty::TEAT_LENGTH },
        { "prop_teat_length", AspectProperty::TEAT_LENGTH },
        { "TEAT_COUNT", AspectProperty::TEAT_COUNT },
        { "prop_teat_count", AspectProperty::TEAT_COUNT },
        { "UDDER_CAPACITY", AspectProperty::UDDER_CAPACITY },
        { "prop_udder_capacity", AspectProperty::UDDER_CAPACITY },
        { "UDDER_REGEN", AspectProperty::UDDER_REGEN },
        { "prop_udder_regen", AspectProperty::UDDER_REGEN },
        { "ANTENNAE_MORPH", AspectProperty::ANTENNAE_MORPH },
        { "prop_antennae_morph", AspectProperty::ANTENNAE_MORPH },
        { "ANTENNAE_LENGTH", AspectProperty::ANTENNAE_LENGTH },
        { "prop_antennae_length", AspectProperty::ANTENNAE_LENGTH },
        { "ANTENNAE_TYPE", AspectProperty::ANTENNAE_TYPE },
        { "prop_antennae_type", AspectProperty::ANTENNAE_TYPE },
        { "FOOT_MORPH", AspectProperty::FOOT_MORPH },
        { "prop_foot_morph", AspectProperty::FOOT_MORPH },
        { "LEG_HAIR", AspectProperty::LEG_HAIR },
        { "prop_leg_hair", AspectProperty::LEG_HAIR },
        { "ARMOR_PIERCING", AspectProperty::ARMOR_PIERCING },
        { "prop_armor_piercing", AspectProperty::ARMOR_PIERCING },
        { "WEAPON_BLEED", AspectProperty::WEAPON_BLEED },
        { "prop_weapon_bleed", AspectProperty::WEAPON_BLEED },
    };
    auto it = s_propMap.find(str);
    if (it != s_propMap.end()) return it->second;
    return AspectProperty::NONE;
}

bool isAnatomicalRacialFocus(EnchantmentFocus focus)
{
    switch (focus)
    {
        case EnchantmentFocus::HORNS:
        case EnchantmentFocus::WINGS:
        case EnchantmentFocus::TAIL:
        case EnchantmentFocus::ANTENNAE:
        case EnchantmentFocus::RACE_AWAKENING:
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
        case EnchantmentFocus::MOUTH:
        case EnchantmentFocus::FACE:
        case EnchantmentFocus::SKIN:
        case EnchantmentFocus::ARMS:
        case EnchantmentFocus::TORSO:
        case EnchantmentFocus::BREASTS:
        case EnchantmentFocus::CROTCH_MAMMARIES:
        case EnchantmentFocus::GENITALIA_PRIMARY:
        case EnchantmentFocus::GENITALIA_SECONDARY:
        case EnchantmentFocus::HIPS_ASS:
        case EnchantmentFocus::LEGS_FEET:
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
    auto comp = getCompatibleFocuses(baseItem);
    return std::find(comp.begin(), comp.end(), focus) != comp.end();
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
            return "Apparel cannot hold racial body transformations (horns, wings, tails, antennae). Use for gradual part sizing, modifiers, or defense.";
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
        case EnchantmentFocus::CORE_ATTRIBUTES:     return "Core";
        case EnchantmentFocus::GENERAL_ATTRIBUTES:  return "Attrib";
        case EnchantmentFocus::SPECIAL_EFFECTS:     return "Special";
        case EnchantmentFocus::RETENTION_FLUIDS:    return "Retent";
        case EnchantmentFocus::BODY_DESIRES:        return "B-Desire";
        case EnchantmentFocus::BEHAVIORAL_DESIRES:  return "P-Desire";
        case EnchantmentFocus::FACE:                return "Face";
        case EnchantmentFocus::TORSO:               return "Torso";
        case EnchantmentFocus::ARMS:                return "Arms";
        case EnchantmentFocus::HAIR:                return "Hair";
        case EnchantmentFocus::HIPS_ASS:            return "Hips";
        case EnchantmentFocus::BREASTS:             return "Chest";
        case EnchantmentFocus::CROTCH_MAMMARIES:    return "Udders";
        case EnchantmentFocus::GENITALIA_PRIMARY:   return "Phallus";
        case EnchantmentFocus::GENITALIA_SECONDARY: return "Yoni";
        case EnchantmentFocus::LEGS_FEET:           return "Legs";
        case EnchantmentFocus::HEAD_FEATURE:        return "Head";
        case EnchantmentFocus::HORNS:               return "Horns";
        case EnchantmentFocus::ANTENNAE:            return "Antenna";
        case EnchantmentFocus::WINGS:               return "Wings";
        case EnchantmentFocus::TAIL:                return "Tail";
        case EnchantmentFocus::EYES:                return "Eyes";
        case EnchantmentFocus::EARS:                return "Ears";
        case EnchantmentFocus::MOUTH:               return "Mouth";
        case EnchantmentFocus::SKIN:                return "Skin";
        case EnchantmentFocus::FLUIDS_CUM:          return "Cum";
        case EnchantmentFocus::FLUIDS_MILK:         return "Milk";
        case EnchantmentFocus::FLUIDS_GIRLCUM:      return "Girlcum";
        case EnchantmentFocus::RACE_AWAKENING:      return "Awaken";
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
        case EnchantmentFocus::CORE_ATTRIBUTES:     return "CR";
        case EnchantmentFocus::GENERAL_ATTRIBUTES:  return "AT";
        case EnchantmentFocus::SPECIAL_EFFECTS:     return "SP";
        case EnchantmentFocus::RETENTION_FLUIDS:    return "RT";
        case EnchantmentFocus::BODY_DESIRES:        return "BD";
        case EnchantmentFocus::BEHAVIORAL_DESIRES:  return "PD";
        case EnchantmentFocus::FACE:                return "FC";
        case EnchantmentFocus::TORSO:               return "TR";
        case EnchantmentFocus::ARMS:                return "AM";
        case EnchantmentFocus::HAIR:                return "HR";
        case EnchantmentFocus::HIPS_ASS:            return "HP";
        case EnchantmentFocus::BREASTS:             return "BS";
        case EnchantmentFocus::CROTCH_MAMMARIES:    return "UD";
        case EnchantmentFocus::GENITALIA_PRIMARY:   return "PH";
        case EnchantmentFocus::GENITALIA_SECONDARY: return "YN";
        case EnchantmentFocus::LEGS_FEET:           return "LG";
        case EnchantmentFocus::HEAD_FEATURE:        return "HD";
        case EnchantmentFocus::HORNS:               return "HN";
        case EnchantmentFocus::ANTENNAE:            return "AN";
        case EnchantmentFocus::WINGS:               return "WG";
        case EnchantmentFocus::TAIL:                return "TL";
        case EnchantmentFocus::EYES:                return "EY";
        case EnchantmentFocus::EARS:                return "ER";
        case EnchantmentFocus::MOUTH:               return "MT";
        case EnchantmentFocus::SKIN:                return "SK";
        case EnchantmentFocus::FLUIDS_CUM:          return "CM";
        case EnchantmentFocus::FLUIDS_MILK:         return "MK";
        case EnchantmentFocus::FLUIDS_GIRLCUM:      return "GC";
        case EnchantmentFocus::RACE_AWAKENING:      return "AW";
        case EnchantmentFocus::ARMOR_REINFORCEMENT: return "SH";
        case EnchantmentFocus::WEAPON_LETHALITY:    return "SW";
        case EnchantmentFocus::ARCANE_AMPLIFICATION:return "MG";
        case EnchantmentFocus::RESISTANCE_WARDING:  return "WD";
        case EnchantmentFocus::BINDING_SPECIAL:     return "SL";
        default:                                    return "??";
    }
}

static std::string getCapitalizedRace(const item* baseItem)
{
    if (!baseItem || baseItem->baseRace.empty()) return "Racial";
    std::string r = baseItem->baseRace;
    r[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[0])));
    return r;
}

std::string getPropertyDisplayName(AspectProperty prop, const item* baseItem)
{
    std::string race = getCapitalizedRace(baseItem);

    switch (prop)
    {
        case AspectProperty::RACIAL_LEGS_BIPED:         return race + " Biped Legs";
        case AspectProperty::RACIAL_STANCE_MORPH:       return race + " Stance Morph";
        case AspectProperty::RACIAL_BODY_CONFIG:        return race + " Taur / Multi-Body";
        case AspectProperty::RACIAL_PHALLUS_MORPH:      return race + " Phallus Morph";
        case AspectProperty::RACIAL_SHEATH_MORPH:       return race + " Sheath Morph";
        case AspectProperty::RACIAL_YONI_MORPH:         return race + " Yoni Contours";
        case AspectProperty::RACIAL_FACIAL_STRUCTURE:   return race + " Visage";
        case AspectProperty::RACIAL_MUZZLE_MORPH:       return race + " Muzzle & Snout";
        case AspectProperty::RACIAL_DENTITION:          return race + " Fangs & Dentition";
        case AspectProperty::RACIAL_TONGUE:             return race + " Tongue Morph";
        case AspectProperty::RACIAL_MANE_MORPH:         return race + " Mane Morph";
        case AspectProperty::RACIAL_EAR_MORPH:          return race + " Ears Morph";
        case AspectProperty::RACIAL_COVERING_TYPE:      return race + " Fur / Covering";
        case AspectProperty::RACIAL_PATTERN_COLOR:      return race + " Pattern & Hue";
        case AspectProperty::RACIAL_HORN_PRIMARY:       return race + " Horns";
        case AspectProperty::RACIAL_HORN_VARIANT:       return race + " Horn Variant";
        case AspectProperty::ANTENNAE_MORPH:            return race + " Antennae";
        case AspectProperty::RACIAL_WING_PRIMARY:       return race + " Wings";
        case AspectProperty::RACIAL_WING_VARIANT:       return race + " Wing Variant";
        case AspectProperty::RACIAL_TAIL_PRIMARY:       return race + " Tail";
        case AspectProperty::RACIAL_TAIL_VARIANT:       return race + " Tail Variant";
        case AspectProperty::CROTCH_MAMMARY_MORPH:      return race + " Crotch Mammaries";
        case AspectProperty::FOOT_MORPH:                return race + " Foot Morph";
        default:
            return getPropertyDefinition(prop).displayName;
    }
}

std::string getPropertyShortLabel(AspectProperty prop, const item* baseItem)
{
    switch (prop)
    {
        case AspectProperty::CORE_HEALTH:          return "Health";
        case AspectProperty::CORE_MANA:            return "Mana";
        case AspectProperty::CORE_STAMINA:         return "Stamina";
        case AspectProperty::CORE_PHYSIQUE:        return "Physiq";
        case AspectProperty::CORE_ARCANE:          return "Arcane";
        case AspectProperty::CORE_CORRUPTION:      return "Corrupt";

        case AspectProperty::ATTR_PHYSICAL_DAMAGE: return "Damage";
        case AspectProperty::ATTR_SPELL_POWER:     return "Sp Power";
        case AspectProperty::ATTR_ARMOR:           return "Armor";
        case AspectProperty::ATTR_WARDING:         return "Wards";
        case AspectProperty::ATTR_FERTILITY:       return "Fertil";
        case AspectProperty::ATTR_VIRILITY:        return "Viril";
        case AspectProperty::ATTR_CRITICAL:        return "Crit";
        case AspectProperty::ATTR_AGILITY:         return "Agility";

        case AspectProperty::CONCEAL_IDENTITY:     return "Shroud";

        case AspectProperty::RETENTION_STOMACH:    return "Stomach";
        case AspectProperty::RETENTION_MAMMARY:    return "Mammary";
        case AspectProperty::RETENTION_YONI:       return "Yoni";
        case AspectProperty::RETENTION_ANAL:       return "Anal";

        case AspectProperty::DESIRE_ANAL:          return "Anal";
        case AspectProperty::DESIRE_MAMMARY:       return "Mammary";
        case AspectProperty::DESIRE_PHALLIC:       return "Phallus";
        case AspectProperty::DESIRE_YONI:          return "Yoni";
        case AspectProperty::DESIRE_FEET:          return "Feet";
        case AspectProperty::DESIRE_DOMINANT:      return "Dominant";
        case AspectProperty::DESIRE_SUBMISSIVE:    return "Submiss";
        case AspectProperty::DESIRE_BONDAGE:       return "Bondage";
        case AspectProperty::DESIRE_EXHIBITIONISM: return "Exhibit";
        case AspectProperty::DESIRE_CHASTITY:      return "Chastity";
        case AspectProperty::DESIRE_MASOCHISM:     return "Masoch";
        case AspectProperty::DESIRE_SADISM:        return "Sadism";

        case AspectProperty::BREAST_SIZE:          return "Size";
        case AspectProperty::BREAST_SHAPE:         return "Shape";
        case AspectProperty::NIPPLE_LENGTH:        return "Nip Len";
        case AspectProperty::NIPPLE_GIRTH:         return "Nip Girth";
        case AspectProperty::NIPPLE_TYPE:          return "Nip Type";
        case AspectProperty::NIPPLE_CAPACITY:      return "Nip Cap";
        case AspectProperty::AREOLA_SIZE:          return "Areola";
        case AspectProperty::LACTATION_VOLUME:     return "Milk Vol";
        case AspectProperty::LACTATION_REGEN:      return "Milk Regen";
        case AspectProperty::FLUID_TYPE:           return "Fluid";
        case AspectProperty::MILK_FLAVOR:          return "Flavor";
        case AspectProperty::CROTCH_MAMMARY_MORPH: return "Udder";

        case AspectProperty::UDDER_SIZE:           return "Udder";
        case AspectProperty::TEAT_LENGTH:          return "TeatLen";
        case AspectProperty::TEAT_COUNT:           return "Teats";
        case AspectProperty::UDDER_CAPACITY:       return "UddrCap";
        case AspectProperty::UDDER_REGEN:          return "UddrReg";

        case AspectProperty::PENIS_LENGTH:         return "Length";
        case AspectProperty::PENIS_GIRTH:          return "Girth";
        case AspectProperty::KNOT_SIZE:            return "Knot";
        case AspectProperty::TESTES_SIZE:          return "Testes";
        case AspectProperty::CUM_VOLUME:           return "Cum Vol";
        case AspectProperty::CUM_REGEN:            return "Cum Regen";
        case AspectProperty::VIRILITY_POTENCY:     return "Virility";
        case AspectProperty::RACIAL_PHALLUS_MORPH: return "Phallus";
        case AspectProperty::RACIAL_SHEATH_MORPH:  return "Sheath";

        case AspectProperty::VAGINA_DEPTH:         return "Depth";
        case AspectProperty::VAGINA_TIGHTNESS:     return "Tightness";
        case AspectProperty::CLIT_SIZE:            return "Clit";
        case AspectProperty::LABIA_SIZE:           return "Labia";
        case AspectProperty::LUBRICATION_WETNESS:  return "Wetness";
        case AspectProperty::FERTILITY_RECEPTIVITY:return "Fertility";
        case AspectProperty::RACIAL_YONI_MORPH:    return "Yoni";

        case AspectProperty::BUTT_SIZE:            return "Butt";
        case AspectProperty::HIP_WIDTH:            return "Hips";
        case AspectProperty::ANUS_CAPACITY:        return "Anus Cap";
        case AspectProperty::ANUS_DEPTH:           return "Anus Depth";
        case AspectProperty::ANUS_ELASTICITY:      return "Elastic";
        case AspectProperty::ANUS_WETNESS:         return "Wetness";

        case AspectProperty::LEG_LENGTH:           return "Length";
        case AspectProperty::THIGH_FULLNESS:       return "Thighs";
        case AspectProperty::LEG_HAIR:             return "LegHair";
        case AspectProperty::RACIAL_LEGS_BIPED:    return "Biped";
        case AspectProperty::RACIAL_STANCE_MORPH:  return "Stance";
        case AspectProperty::RACIAL_BODY_CONFIG:   return "Taur";
        case AspectProperty::FOOT_MORPH:           return "Foot";
        case AspectProperty::SPRINT_AGILITY:       return "Sprint";

        case AspectProperty::FACE_SHAPE:           return "Face";
        case AspectProperty::NOSE_SIZE:            return "Nose";
        case AspectProperty::FACIAL_BEARD:         return "Beard";
        case AspectProperty::EYEBROW_SHAPE:        return "Eyebrow";
        case AspectProperty::RACIAL_FACIAL_STRUCTURE: return "Visage";
        case AspectProperty::RACIAL_MUZZLE_MORPH:  return "Muzzle";
        case AspectProperty::PREDATORY_PERCEPTION: return "Sense";

        case AspectProperty::LIP_FULLNESS:         return "Lips";
        case AspectProperty::RACIAL_DENTITION:     return "Fangs";
        case AspectProperty::RACIAL_TONGUE:        return "Tongue";
        case AspectProperty::THROAT_DEPTH:         return "Throat";
        case AspectProperty::SALIVA_PRODUCTION:    return "Saliva";

        case AspectProperty::HAIR_GROWTH_RATE:     return "Growth";
        case AspectProperty::HAIR_LENGTH:          return "Length";
        case AspectProperty::HAIR_VOLUME:          return "Volume";
        case AspectProperty::HAIR_STYLE:           return "Style";
        case AspectProperty::HAIR_COLOR:           return "Color";
        case AspectProperty::RACIAL_MANE_MORPH:    return "Mane";

        case AspectProperty::EYE_PUPIL_SHAPE:      return "Pupils";
        case AspectProperty::EYE_IRIS_COLOR:       return "Iris";
        case AspectProperty::DARKVISION_AURA:      return "Darkvis";
        case AspectProperty::ALLURING_GAZE:        return "Allure";

        case AspectProperty::EAR_SIZE:             return "Ear Size";
        case AspectProperty::RACIAL_EAR_MORPH:     return "Ears";
        case AspectProperty::KEEN_HEARING:         return "Hearing";

        case AspectProperty::STATURE_HEIGHT:       return "Height";
        case AspectProperty::MUSCLE_PHYSIQUE:      return "Muscle";
        case AspectProperty::WAIST_TAPER:          return "Waist";
        case AspectProperty::FEMININITY_MASCULINITY: return "Gender";
        case AspectProperty::BODY_HAIR:            return "BodyHair";
        case AspectProperty::STOMACH_FIRMNESS:     return "Tone";
        case AspectProperty::HEALTH_VITALITY:      return "Vitality";

        case AspectProperty::RACIAL_COVERING_TYPE: return "Covering";
        case AspectProperty::RACIAL_PATTERN_COLOR: return "Pattern";
        case AspectProperty::DERMIS_ELASTICITY:    return "Elastic";
        case AspectProperty::NATURAL_ARMOR:        return "Armor";

        case AspectProperty::ARM_MUSCLE:           return "Muscle";
        case AspectProperty::UNDERARM_HAIR:        return "ArmHair";
        case AspectProperty::GRIP_STRENGTH:        return "Grip";
        case AspectProperty::CLAWS_NAILS:          return "Claws";
        case AspectProperty::MANUAL_DEXTERITY:     return "Dexterity";

        case AspectProperty::HORN_SIZE:            return "Size";
        case AspectProperty::HORN_SHAPE:           return "Shape";
        case AspectProperty::HORN_TEXTURE:         return "Texture";
        case AspectProperty::RACIAL_HORN_PRIMARY:  return "Horns";
        case AspectProperty::RACIAL_HORN_VARIANT:  return "Horn Var";
        case AspectProperty::ANTENNAE_MORPH:       return "Antenna";
        case AspectProperty::ANTENNAE_LENGTH:      return "Ant Len";
        case AspectProperty::ANTENNAE_TYPE:        return "Ant Type";
        case AspectProperty::WING_SIZE:            return "Size";
        case AspectProperty::WING_TYPE:            return "Type";
        case AspectProperty::GLIDING_FLIGHT:       return "Flight";
        case AspectProperty::RACIAL_WING_PRIMARY:  return "Wings";
        case AspectProperty::RACIAL_WING_VARIANT:  return "Wing Var";
        case AspectProperty::TAIL_LENGTH:          return "Length";
        case AspectProperty::TAIL_GIRTH:           return "Girth";
        case AspectProperty::TAIL_TYPE:            return "Type";
        case AspectProperty::RACIAL_TAIL_PRIMARY:  return "Tail";
        case AspectProperty::RACIAL_TAIL_VARIANT:  return "Tail Var";
        case AspectProperty::PART_REMOVAL:         return "Shed";

        case AspectProperty::ATTACK_POWER:         return "Power";
        case AspectProperty::STRIKE_VELOCITY:      return "Velocity";
        case AspectProperty::DAMAGE_PHYSICAL:      return "Damage";
        case AspectProperty::DAMAGE_ELEMENTAL:     return "Elements";
        case AspectProperty::CRITICAL_POWER:       return "Critical";
        case AspectProperty::LIFE_LEECH:           return "Leech";
        case AspectProperty::ARMOR_PIERCING:       return "Pierce";
        case AspectProperty::WEAPON_BLEED:         return "Bleed";
        case AspectProperty::ARMOR_RATING:         return "Armor";
        case AspectProperty::FORTITUDE_STAT:       return "Fortitude";
        case AspectProperty::WARD_RESISTANCE:      return "Wards";
        case AspectProperty::MIND_WARD:            return "Mind Ward";
        case AspectProperty::ARCANE_STAT:          return "Arcane";
        case AspectProperty::MANA_CEILING:         return "Mana";
        case AspectProperty::MANA_REGENERATION:    return "Mana Regen";

        case AspectProperty::SOULBOUND_SEAL:       return "Soulbound";
        case AspectProperty::SERVITUDE_INHIBITION: return "Servitude";
        case AspectProperty::SENSORY_VIBRATION:    return "Sensory";

        case AspectProperty::SCALE_SIZE:           return "Scale";
        case AspectProperty::SECONDARY_SIZE:       return "Width";
        case AspectProperty::VOLUME_CAPACITY:      return "Volume";
        case AspectProperty::DEPTH:                return "Depth";
        case AspectProperty::ELASTICITY:           return "Elastic";
        case AspectProperty::FLUID_PRODUCTION:     return "Fluids";
        case AspectProperty::REGENERATION_RATE:    return "Regen";
        case AspectProperty::HAIR_GROWTH:          return "Growth";
        case AspectProperty::PHYSIQUE_STAT:        return "Physique";
        case AspectProperty::AGILITY_STAT:         return "Agility";
        case AspectProperty::HEALTH_CEILING:       return "Health";
        case AspectProperty::VIRILITY_FACTOR:      return "Virility";
        case AspectProperty::FERTILITY_FACTOR:     return "Fertility";
        case AspectProperty::CORRUPTION_AURA:      return "Corrupt";
        case AspectProperty::RACIAL_TRANSFORMATION:return "Awaken";
        default:                                   return "Prop";
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

bool propertySupportsLimits(AspectProperty prop, const item* baseItem)
{
    switch (prop)
    {
        case AspectProperty::BREAST_SIZE:
        case AspectProperty::NIPPLE_LENGTH:
        case AspectProperty::NIPPLE_GIRTH:
        case AspectProperty::AREOLA_SIZE:
        case AspectProperty::LACTATION_VOLUME:
        case AspectProperty::LACTATION_REGEN:

        case AspectProperty::UDDER_SIZE:
        case AspectProperty::TEAT_LENGTH:
        case AspectProperty::UDDER_CAPACITY:
        case AspectProperty::UDDER_REGEN:

        case AspectProperty::PENIS_LENGTH:
        case AspectProperty::PENIS_GIRTH:
        case AspectProperty::KNOT_SIZE:
        case AspectProperty::TESTES_SIZE:
        case AspectProperty::CUM_VOLUME:
        case AspectProperty::CUM_REGEN:

        case AspectProperty::CLIT_SIZE:
        case AspectProperty::LABIA_SIZE:
        case AspectProperty::VAGINA_DEPTH:
        case AspectProperty::VAGINA_TIGHTNESS:
        case AspectProperty::LUBRICATION_WETNESS:

        case AspectProperty::BUTT_SIZE:
        case AspectProperty::HIP_WIDTH:
        case AspectProperty::ANUS_CAPACITY:
        case AspectProperty::ANUS_DEPTH:
        case AspectProperty::ANUS_ELASTICITY:
        case AspectProperty::ANUS_WETNESS:

        case AspectProperty::LEG_LENGTH:
        case AspectProperty::THIGH_FULLNESS:
        case AspectProperty::LEG_HAIR:

        case AspectProperty::LIP_FULLNESS:
        case AspectProperty::NOSE_SIZE:
        case AspectProperty::FACIAL_BEARD:
        case AspectProperty::EYEBROW_SHAPE:

        case AspectProperty::HAIR_LENGTH:
        case AspectProperty::HAIR_VOLUME:

        case AspectProperty::STATURE_HEIGHT:
        case AspectProperty::MUSCLE_PHYSIQUE:
        case AspectProperty::WAIST_TAPER:
        case AspectProperty::FEMININITY_MASCULINITY:
        case AspectProperty::BODY_HAIR:

        case AspectProperty::UNDERARM_HAIR:
        case AspectProperty::ARM_MUSCLE:

        case AspectProperty::HORN_SIZE:
        case AspectProperty::WING_SIZE:
        case AspectProperty::TAIL_LENGTH:
        case AspectProperty::TAIL_GIRTH:
        case AspectProperty::EAR_SIZE:
        case AspectProperty::ANTENNAE_LENGTH:

        case AspectProperty::SCALE_SIZE:
        case AspectProperty::SECONDARY_SIZE:
        case AspectProperty::VOLUME_CAPACITY:
        case AspectProperty::DEPTH:
        case AspectProperty::ELASTICITY:
        case AspectProperty::FLUID_PRODUCTION:
        case AspectProperty::REGENERATION_RATE:
        case AspectProperty::HAIR_GROWTH:
            return true;

        default:
            return false;
    }
}

std::vector<std::string> getPropertyLimitSteps(AspectProperty prop)
{
    switch (prop)
    {
        case AspectProperty::BREAST_SIZE:
            return { "flat", "small", "perky", "voluptuous", "massive", "titanic" };
        case AspectProperty::NIPPLE_LENGTH:
            return { "flat", "tiny", "distinct", "protruding", "long", "hyper" };
        case AspectProperty::NIPPLE_GIRTH:
            return { "thin", "normal", "broad", "puffy", "wide", "massive" };
        case AspectProperty::AREOLA_SIZE:
            return { "tiny", "small", "average", "large", "plate", "hyper" };
        case AspectProperty::LACTATION_VOLUME:
        case AspectProperty::FLUID_PRODUCTION:
            return { "dry", "droplets", "trickle", "stream", "torrent", "deluge" };
        case AspectProperty::LACTATION_REGEN:
        case AspectProperty::REGENERATION_RATE:
        case AspectProperty::UDDER_REGEN:
            return { "none", "slow", "steady", "rapid", "surging", "fountain" };

        case AspectProperty::UDDER_SIZE:
            return { "flat", "budding", "small", "full", "heavy", "gargantuan" };
        case AspectProperty::TEAT_LENGTH:
            return { "tiny", "small", "standard", "long", "dangling", "hyper" };
        case AspectProperty::UDDER_CAPACITY:
            return { "dry", "small", "moderate", "bountiful", "deluging", "endless" };

        case AspectProperty::PENIS_LENGTH:
            return { "tiny", "modest", "average", "large", "horse", "monster" };
        case AspectProperty::PENIS_GIRTH:
            return { "thin", "slim", "thick", "girthy", "massive", "arm" };
        case AspectProperty::KNOT_SIZE:
            return { "none", "slight", "pronounced", "large", "massive", "locking" };
        case AspectProperty::TESTES_SIZE:
            return { "pea", "small", "apple", "grapefruit", "watermelon", "carriage" };
        case AspectProperty::CUM_VOLUME:
            return { "drops", "trickle", "splash", "squirt", "geyser", "tsunami" };
        case AspectProperty::CUM_REGEN:
            return { "sluggish", "daily", "hourly", "rapid", "nonstop", "surging" };

        case AspectProperty::CLIT_SIZE:
            return { "button", "small", "prominent", "engorged", "pseudo", "massive" };
        case AspectProperty::LABIA_SIZE:
            return { "tidy", "small", "petal", "fleshy", "pendulous", "drapery" };
        case AspectProperty::VAGINA_DEPTH:
        case AspectProperty::DEPTH:
            return { "shallow", "snug", "average", "deep", "cavernous", "abyssal" };
        case AspectProperty::VAGINA_TIGHTNESS:
            return { "gaping", "relaxed", "loose", "snug", "clenched", "impassable" };
        case AspectProperty::LUBRICATION_WETNESS:
            return { "dry", "moist", "slick", "dripping", "soaking", "torrential" };

        case AspectProperty::BUTT_SIZE:
            return { "flat", "small", "round", "plump", "huge", "colossal" };
        case AspectProperty::HIP_WIDTH:
        case AspectProperty::SECONDARY_SIZE:
            return { "narrow", "slender", "average", "wide", "flaring", "broad" };
        case AspectProperty::ANUS_CAPACITY:
        case AspectProperty::VOLUME_CAPACITY:
            return { "tight", "snug", "receptive", "cavernous", "gaping", "bottomless" };
        case AspectProperty::ANUS_DEPTH:
            return { "shallow", "short", "average", "deep", "abyssal", "endless" };
        case AspectProperty::ANUS_ELASTICITY:
        case AspectProperty::ELASTICITY:
            return { "rigid", "firm", "pliant", "elastic", "hyper-elastic", "liquid" };
        case AspectProperty::ANUS_WETNESS:
            return { "dry", "normal", "moist", "wet", "dripping", "soaking" };

        case AspectProperty::LEG_LENGTH:
            return { "short", "average", "long", "towering", "stilt", "sky" };
        case AspectProperty::THIGH_FULLNESS:
            return { "slim", "slender", "curvy", "thick", "muscular", "thunderous" };
        case AspectProperty::LEG_HAIR:
        case AspectProperty::BODY_HAIR:
            return { "smooth", "faint", "light", "moderate", "hairy", "pelt" };

        case AspectProperty::LIP_FULLNESS:
            return { "thin", "small", "normal", "plump", "full", "huge" };
        case AspectProperty::NOSE_SIZE:
            return { "tiny", "small", "normal", "large", "huge", "massive" };
        case AspectProperty::FACIAL_BEARD:
            return { "clean", "stubble", "trimmed", "full", "long", "wizard" };
        case AspectProperty::EYEBROW_SHAPE:
            return { "thin", "fine", "natural", "thick", "bushy", "wild" };

        case AspectProperty::HAIR_LENGTH:
        case AspectProperty::HAIR_GROWTH:
            return { "bald", "short", "shoulder", "waist", "floor", "infinite" };
        case AspectProperty::HAIR_VOLUME:
            return { "sparse", "thin", "normal", "full", "voluminous", "enormous" };

        case AspectProperty::STATURE_HEIGHT:
            return { "tiny", "short", "average", "tall", "giant", "titanic" };
        case AspectProperty::MUSCLE_PHYSIQUE:
            return { "soft", "light", "toned", "athletic", "ripped", "colossus" };
        case AspectProperty::WAIST_TAPER:
            return { "wasp", "slender", "average", "wide", "heavy", "stout" };
        case AspectProperty::FEMININITY_MASCULINITY:
            return { "masculine", "androgynous", "soft", "feminine", "doll", "hyper-fem" };

        case AspectProperty::UNDERARM_HAIR:
            return { "hairless", "faint", "trimmed", "bushy", "luxuriant", "wild" };
        case AspectProperty::ARM_MUSCLE:
            return { "soft", "toned", "athletic", "muscular", "massive", "colossus" };

        case AspectProperty::HORN_SIZE:
            return { "budding", "short", "medium", "long", "towering", "colossal" };
        case AspectProperty::WING_SIZE:
            return { "tiny", "small", "medium", "large", "expansive", "massive" };
        case AspectProperty::TAIL_LENGTH:
            return { "stub", "short", "medium", "long", "floor", "endless" };
        case AspectProperty::TAIL_GIRTH:
            return { "thin", "slender", "thick", "fluffy", "massive", "hyper" };
        case AspectProperty::EAR_SIZE:
            return { "tiny", "small", "normal", "large", "long", "huge" };
        case AspectProperty::ANTENNAE_LENGTH:
            return { "nubs", "short", "medium", "long", "arching", "giant" };

        case AspectProperty::SCALE_SIZE:
        default:
            return { "tiny", "small", "normal", "large", "huge", "maximum" };
    }
}

std::string getLimitStepLabel(AspectProperty prop, int stepIndex)
{
    auto steps = getPropertyLimitSteps(prop);
    if (steps.empty()) return "none";
    int clamped = std::clamp(stepIndex, 0, static_cast<int>(steps.size()) - 1);
    return steps[clamped];
}


