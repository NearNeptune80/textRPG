#include "items/enchantmentAspects.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include "items/item.h"
#include "items/infusionTier.h"

static const AspectDefinition s_noneDef = { "none", "None", "No modifier selected.", "", ItemRarity::COMMON };

static const std::unordered_map<EnchantmentFocus, AspectDefinition> s_focusDefs = {
    { EnchantmentFocus::HEAD_FEATURE,        { "focus_head", "Head Feature", "Focuses infusion upon the head, facial contours, or predatory senses.", "Predator's", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::HORNS,               { "focus_horns", "Horns & Crown", "Focuses infusion upon cranial horn growths and demonic crests.", "Curved", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::HAIR,                { "focus_hair", "Mane & Follicles", "Focuses infusion upon hair volume, length, and coloration.", "Flowing", ItemRarity::COMMON } },
    { EnchantmentFocus::EYES,                { "focus_eyes", "Ocular Senses", "Focuses infusion upon eye hue, pupil configuration, and darkvision.", "Piercing", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::EARS,                { "focus_ears", "Auditory Organ", "Focuses infusion upon ear morphology and heightened auditory perception.", "Keen", ItemRarity::COMMON } },
    { EnchantmentFocus::MOUTH,               { "focus_mouth", "Mouth & Throat", "Focuses infusion upon lip fullness, dentition, tongue agility, and throat capacity.", "Hungering", ItemRarity::UNCOMMON } },
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
    { EnchantmentFocus::LEGS_FEET,           { "focus_legs_feet", "Legs & Stance", "Focuses infusion upon stride length, digitigrade/unguligrade stance, or taur body.", "Fleet", ItemRarity::UNCOMMON } },

    { EnchantmentFocus::ARMOR_REINFORCEMENT, { "focus_armor", "Armor Reinforcement", "Focuses infusion upon garment resilience, weave density, and deflection.", "Warded", ItemRarity::RARE } },
    { EnchantmentFocus::WEAPON_LETHALITY,    { "focus_weapon", "Weapon Lethality", "Focuses infusion upon edge sharpness, impact velocity, and critical trauma.", "Striking", ItemRarity::RARE } },
    { EnchantmentFocus::ARCANE_AMPLIFICATION,{ "focus_arcane", "Arcane Amplification", "Focuses infusion upon magical resonance, spell potency, and aura capacity.", "Eldritch", ItemRarity::EPIC } },
    { EnchantmentFocus::RESISTANCE_WARDING,  { "focus_resistance", "Elemental Warding", "Focuses infusion upon shielding against elemental and physical harm.", "Impenetrable", ItemRarity::RARE } },
    { EnchantmentFocus::BINDING_SPECIAL,     { "focus_binding", "Soulbound Binding", "Focuses infusion upon inescapable binding, servitude seals, and arousal stimulation.", "Runic", ItemRarity::LEGENDARY } },

    { EnchantmentFocus::CORE_ATTRIBUTES,     { "focus_core_attr", "Core Attributes", "Focuses infusion upon baseline vitality, mana aura, stamina, and physical might.", "Primal", ItemRarity::COMMON } },
    { EnchantmentFocus::GENERAL_ATTRIBUTES,  { "focus_gen_attr", "Attributes", "Focuses infusion upon combat ratings, spell power, defenses, agility, and fertility.", "Ascendant", ItemRarity::COMMON } },
    { EnchantmentFocus::SPECIAL_EFFECTS,     { "focus_special_fx", "Special Arcana", "Focuses infusion upon soulbound bindings, identity concealment, and servitude runes.", "Enigmatic", ItemRarity::EPIC } },
    { EnchantmentFocus::RETENTION_FLUIDS,    { "focus_retention", "Fluid Retention", "Focuses infusion upon bodily fluid containment, pregnancy capacity, and fullness.", "Retentive", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::BODY_DESIRES,        { "focus_body_desires", "Body Desires", "Focuses infusion upon primal erotic attunement and bodily fixations.", "Yearning", ItemRarity::RARE } },
    { EnchantmentFocus::BEHAVIORAL_DESIRES,  { "focus_behav_desires", "Behavioral Desires", "Focuses infusion upon personality inclinations, dominance, and surrender.", "Compelling", ItemRarity::RARE } },
    { EnchantmentFocus::CROTCH_MAMMARIES,    { "focus_crotch_mammaries", "Crotch Mammaries", "Focuses infusion upon inguinal udders, teats, and pastoral milk generation.", "Pastoral", ItemRarity::EPIC } },
    { EnchantmentFocus::ANTENNAE,            { "focus_antennae", "Cranial Antennae", "Focuses infusion upon sensory antennae feelers and insectoid plumes.", "Sensitive", ItemRarity::UNCOMMON } },
    { EnchantmentFocus::FLUIDS_CUM,          { "focus_fluids_cum", "Ejaculate Fluids", "Focuses infusion upon virile seed volume, replenishment velocity, and potency.", "Virile", ItemRarity::RARE } },
    { EnchantmentFocus::FLUIDS_MILK,         { "focus_fluids_milk", "Mammary Milk", "Focuses infusion upon breast milk volume, replenishment rate, and sweetness.", "Lacteal", ItemRarity::RARE } },
    { EnchantmentFocus::FLUIDS_GIRLCUM,      { "focus_fluids_girlcum", "Yoni Nectar", "Focuses infusion upon vaginal lubrication, nectar flow, and arousal aroma.", "Nectareous", ItemRarity::RARE } },
    { EnchantmentFocus::RACE_AWAKENING,      { "focus_race_awaken", "Race Awakening", "Awakens the primal morphology and racial essence of the reagent.", "Awakened", ItemRarity::EPIC } }
};

static const std::unordered_map<AspectProperty, AspectDefinition> s_propDefs = {
    // Chest & Breasts
    { AspectProperty::BREAST_SIZE,          { "prop_breast_size", "Breast Size", "Increases or reduces mammary cup volume and breast stature.", "Voluptuous", ItemRarity::COMMON } },
    { AspectProperty::BREAST_SHAPE,         { "prop_breast_shape", "Breast Shape", "Shapes mammary profile (perky, rounded, pendulous, teardrop).", "Shapely", ItemRarity::COMMON } },
    { AspectProperty::NIPPLE_LENGTH,        { "prop_nipple_length", "Nipple Length", "Extends or shortens nipple length and protrusion.", "Protruding", ItemRarity::COMMON } },
    { AspectProperty::NIPPLE_GIRTH,         { "prop_nipple_girth", "Nipple Girth", "Broadens nipple circumference, prominence, and areola diameter.", "Areolar", ItemRarity::COMMON } },
    { AspectProperty::NIPPLE_TYPE,          { "prop_nipple_type", "Nipple Type", "Alters nipple morphology (normal, inverted, puffed, suckable).", "Puffed", ItemRarity::UNCOMMON } },
    { AspectProperty::NIPPLE_CAPACITY,      { "prop_nipple_capacity", "Nipple Capacity", "Expands milk storage ducts and internal mammary reservoir capacity.", "Capacious", ItemRarity::UNCOMMON } },
    { AspectProperty::LACTATION_VOLUME,     { "prop_lactation_volume", "Lactation Volume", "Enhances milk yield and surge volume per expression.", "Abundant", ItemRarity::RARE } },
    { AspectProperty::LACTATION_REGEN,      { "prop_lactation_regen", "Lactation Regen", "Accelerates natural replenishment velocity of breast milk.", "Quenched", ItemRarity::RARE } },
    { AspectProperty::FLUID_TYPE,           { "prop_fluid_type", "Milk & Fluid Type", "Alters the flavor, sweetness, and alchemical qualities of expressed fluids.", "Nectareous", ItemRarity::RARE } },
    { AspectProperty::CROTCH_MAMMARY_MORPH, { "prop_crotch_mammary", "Crotch Mammaries & Udders", "Manifests, transforms, or expands inguinal crotch udders and teats.", "Pastoral", ItemRarity::EPIC } },

    // Phallus & Virility
    { AspectProperty::PENIS_LENGTH,         { "prop_penis_length", "Phallus Length", "Extends shaft length and reach.", "Towering", ItemRarity::COMMON } },
    { AspectProperty::PENIS_GIRTH,          { "prop_penis_girth", "Phallus Girth", "Thickens shaft circumference and density.", "Girthy", ItemRarity::COMMON } },
    { AspectProperty::KNOT_SIZE,            { "prop_knot_size", "Knot Size", "Develops, enlarges, or forms a pronounced locking bulb knot.", "Bulbous", ItemRarity::UNCOMMON } },
    { AspectProperty::TESTES_SIZE,          { "prop_testes_size", "Testes Size", "Expands testicular volume, weight, and sac fullness.", "Heavy", ItemRarity::COMMON } },
    { AspectProperty::CUM_VOLUME,           { "prop_cum_volume", "Cum Volume", "Increases fluid ejaculation output volume.", "Deluging", ItemRarity::RARE } },
    { AspectProperty::CUM_REGEN,            { "prop_cum_regen", "Cum Regeneration", "Accelerates recovery rate and fluid replenishment.", "Virile", ItemRarity::RARE } },
    { AspectProperty::VIRILITY_POTENCY,     { "prop_virility_potency", "Virility Potency", "Boosts fertility, masculine vigor, and impregnation chance.", "Potent", ItemRarity::RARE } },
    { AspectProperty::RACIAL_PHALLUS_MORPH, { "prop_racial_phallus", "Racial Phallus Morph", "Transforms phallus into race-specific morphology (barbed, equine, knotted, tentacled).", "Awakened", ItemRarity::RARE } },
    { AspectProperty::RACIAL_SHEATH_MORPH,  { "prop_racial_sheath", "Internal Sheath Morph", "Forms or refines a protective racial foreskin sheath.", "Enveloping", ItemRarity::UNCOMMON } },

    // Yoni & Fertility
    { AspectProperty::VAGINA_DEPTH,         { "prop_vagina_depth", "Yoni Depth", "Deepens vaginal canal reach and internal accommodation.", "Deepened", ItemRarity::COMMON } },
    { AspectProperty::VAGINA_TIGHTNESS,     { "prop_vagina_tightness", "Yoni Tightness", "Contracts or relaxes muscular tone and internal snugness.", "Snug", ItemRarity::COMMON } },
    { AspectProperty::CLIT_SIZE,            { "prop_clit_size", "Clitoris Size", "Enlarges clitoral hooded size and sensitivity.", "Engorged", ItemRarity::COMMON } },
    { AspectProperty::LABIA_SIZE,           { "prop_labia_size", "Labia Fullness", "Shapes and softens outer and inner labial folds.", "Petaled", ItemRarity::COMMON } },
    { AspectProperty::LUBRICATION_WETNESS,  { "prop_lube_wetness", "Natural Lubrication", "Enhances arousal responsiveness and fluid secretion.", "Glistening", ItemRarity::UNCOMMON } },
    { AspectProperty::FERTILITY_RECEPTIVITY,{ "prop_fertility_receptivity", "Fertility Receptivity", "Heightens conception probability, womb receptivity, and heat cycles.", "Blooming", ItemRarity::RARE } },
    { AspectProperty::RACIAL_YONI_MORPH,    { "prop_racial_yoni", "Racial Yoni Morph", "Morphs vaginal contours, ridging, or internal structure into race morphology.", "Primal", ItemRarity::RARE } },

    // Hips & Derriere
    { AspectProperty::BUTT_SIZE,            { "prop_butt_size", "Buttock Volume", "Augments gluteal roundness, fullness, and jiggle.", "Plump", ItemRarity::COMMON } },
    { AspectProperty::HIP_WIDTH,            { "prop_hip_width", "Hip Flare", "Broadens pelvic width and hourglass flare.", "Flared", ItemRarity::COMMON } },
    { AspectProperty::ANUS_CAPACITY,        { "prop_anus_capacity", "Anus Capacity", "Expands receptive internal passage volume.", "Capacious", ItemRarity::UNCOMMON } },
    { AspectProperty::ANUS_DEPTH,           { "prop_anus_depth", "Anus Depth", "Deepens receptive anal depth.", "Abyssal", ItemRarity::UNCOMMON } },
    { AspectProperty::ANUS_ELASTICITY,      { "prop_anus_elasticity", "Anus Elasticity", "Imparts supple pliability and smooth restorative elasticity.", "Pliant", ItemRarity::UNCOMMON } },

    // Legs & Lower Body
    { AspectProperty::LEG_LENGTH,           { "prop_leg_length", "Leg Stature", "Adjusts leg length, step stride, and leg elegance.", "Long", ItemRarity::COMMON } },
    { AspectProperty::THIGH_FULLNESS,       { "prop_thigh_fullness", "Thigh Fullness", "Builds plush, soft, or powerful muscular thigh curves.", "Thick", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_LEGS_BIPED,    { "prop_racial_legs_biped", "Racial Biped Legs", "Transforms legs into racial biped limbs, paws, or cloven feet.", "Beast", ItemRarity::RARE } },
    { AspectProperty::RACIAL_STANCE_MORPH,  { "prop_racial_stance", "Racial Stance Morph", "Morphs leg stance (plantigrade, digitigrade paws, unguligrade hooves).", "Feral", ItemRarity::RARE } },
    { AspectProperty::RACIAL_BODY_CONFIG,   { "prop_racial_body_config", "Racial Body Config", "Transforms lower anatomy into taur body, drider spider chassis, or naga serpent coil.", "Mythic", ItemRarity::EPIC } },
    { AspectProperty::SPRINT_AGILITY,       { "prop_sprint_agility", "Sprint Agility", "Imbues sudden sprinting acceleration, leap power, and swiftness.", "Fleet", ItemRarity::UNCOMMON } },

    // Head & Visage
    { AspectProperty::FACE_SHAPE,           { "prop_face_shape", "Visage Contours", "Refines jawline softness, cheekbones, and facial proportions.", "Chiseled", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_FACIAL_STRUCTURE,{ "prop_racial_face", "Racial Visage", "Attunes facial bone structure to racial aesthetics.", "Savage", ItemRarity::RARE } },
    { AspectProperty::RACIAL_MUZZLE_MORPH,  { "prop_racial_muzzle", "Racial Muzzle & Snout", "Shapes an animalistic snout, muzzle, beak, or feral nose.", "Feral", ItemRarity::RARE } },
    { AspectProperty::PREDATORY_PERCEPTION, { "prop_predatory_perception", "Predatory Instinct", "Sharpens sensory tracking and awareness of surroundings.", "Keen", ItemRarity::UNCOMMON } },

    // Mouth & Throat
    { AspectProperty::LIP_FULLNESS,         { "prop_lip_fullness", "Lip Fullness", "Plumps lips into lush, pillowy, inviting curves.", "Pouty", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_DENTITION,     { "prop_racial_dentition", "Racial Fangs & Teeth", "Grows sharp canine fangs, venomous hollow vipers, or predatory dentition.", "Fanged", ItemRarity::RARE } },
    { AspectProperty::RACIAL_TONGUE,        { "prop_racial_tongue", "Racial Tongue Morph", "Extends prehensile reach, fork, barbs, or serpentine agility to the tongue.", "Forked", ItemRarity::RARE } },
    { AspectProperty::THROAT_DEPTH,         { "prop_throat_depth", "Throat Depth", "Suppresses gag reflex and stretches esophageal accommodation.", "Swallowing", ItemRarity::UNCOMMON } },
    { AspectProperty::SALIVA_PRODUCTION,    { "prop_saliva_prod", "Saliva Production", "Surges natural saliva generation, sweetness, or aphrodisiac viscosity.", "Slavering", ItemRarity::UNCOMMON } },

    // Hair & Follicles
    { AspectProperty::HAIR_GROWTH_RATE,     { "prop_hair_growth_rate", "Growth Rate", "Accelerates hair follicle growth speed over time.", "Sprouting", ItemRarity::COMMON } },
    { AspectProperty::HAIR_LENGTH,          { "prop_hair_length", "Hair Length", "Extends or trims mane/hair length.", "Flowing", ItemRarity::COMMON } },
    { AspectProperty::HAIR_VOLUME,          { "prop_hair_volume", "Hair Volume & Thickness", "Expands hair body, lush density, and silky volume.", "Lush", ItemRarity::COMMON } },
    { AspectProperty::HAIR_STYLE,           { "prop_hair_style", "Hair Style / Weave", "Restyles follicles into braided, wavy, cropped, or wild manes.", "Braided", ItemRarity::COMMON } },
    { AspectProperty::HAIR_COLOR,           { "prop_hair_color", "Hair Hue & Sheen", "Shifts hair pigmentation, highlights, and radiant luster.", "Vibrant", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_MANE_MORPH,    { "prop_racial_mane", "Racial Mane Morph", "Grows a sweeping leonine mane, horse crest, or racial hair plumage.", "Regal", ItemRarity::RARE } },

    // Eyes & Vision
    { AspectProperty::EYE_PUPIL_SHAPE,      { "prop_eye_pupils", "Pupil Shape", "Morphs pupils into slit, horizontal goat, heart, or demonic shapes.", "Slit", ItemRarity::COMMON } },
    { AspectProperty::EYE_IRIS_COLOR,       { "prop_eye_color", "Iris Coloration", "Infuses luminous hues, two-tone heterochromia, or glowing irises.", "Luminous", ItemRarity::COMMON } },
    { AspectProperty::DARKVISION_AURA,      { "prop_darkvision", "Darkvision", "Grants clear sight piercing darkness and illusions.", "Nocturnal", ItemRarity::RARE } },
    { AspectProperty::ALLURING_GAZE,        { "prop_alluring_gaze", "Hypnotic Allure", "Radiates seductive charm captivating onlookers.", "Hypnotic", ItemRarity::RARE } },

    // Ears & Auditory
    { AspectProperty::EAR_SIZE,             { "prop_ear_size", "Ear Size & Flare", "Extends or shapes ear tips and perimeter.", "Pointed", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_EAR_MORPH,     { "prop_racial_ears", "Racial Ear Morph", "Morphs ears into floppy hound ears, lupine points, bovine ears, or elven fans.", "Wild", ItemRarity::RARE } },
    { AspectProperty::KEEN_HEARING,         { "prop_keen_hearing", "Auditory Acuity", "Heightens hearing range and detects stealthy movements.", "Attentive", ItemRarity::UNCOMMON } },

    // Torso & Stature
    { AspectProperty::STATURE_HEIGHT,       { "prop_height", "Stature & Height", "Increases or decreases overall physical body height.", "Towering", ItemRarity::COMMON } },
    { AspectProperty::MUSCLE_PHYSIQUE,      { "prop_muscle_physique", "Muscular Build", "Sculpts ripped abdominal definition and core muscle.", "Mighty", ItemRarity::COMMON } },
    { AspectProperty::WAIST_TAPER,          { "prop_waist_taper", "Waist Taper", "Narrows waistline into a snatched, slender taper.", "Slender", ItemRarity::COMMON } },
    { AspectProperty::STOMACH_FIRMNESS,     { "prop_stomach_tone", "Stomach Firmness", "Firms stomach into toned abs or soft plush curvature.", "Toned", ItemRarity::COMMON } },
    { AspectProperty::HEALTH_VITALITY,      { "prop_health_vitality", "Vitality Pool", "Enhances core constitution and max health.", "Vital", ItemRarity::RARE } },

    // Skin & Dermis
    { AspectProperty::RACIAL_COVERING_TYPE, { "prop_racial_covering", "Racial Covering Morph", "Covers dermis in plush fur, iridescent scales, feathers, or chiseled carapace.", "Pelt", ItemRarity::RARE } },
    { AspectProperty::RACIAL_PATTERN_COLOR, { "prop_racial_pattern", "Pattern & Coat Color", "Shifts coat markings (stripes, spots, rosettes, gradients) and base tone.", "Spotted", ItemRarity::RARE } },
    { AspectProperty::DERMIS_ELASTICITY,    { "prop_dermis_elasticity", "Dermis Elasticity", "Infuses skin with soft, supple velvety smoothness and stretch.", "Velvet", ItemRarity::UNCOMMON } },
    { AspectProperty::NATURAL_ARMOR,        { "prop_natural_armor", "Dermal Hardening", "Toughens skin into natural protective deflection.", "Hardened", ItemRarity::UNCOMMON } },

    // Arms & Hands
    { AspectProperty::ARM_MUSCLE,           { "prop_arm_muscle", "Arm Strength", "Enhances bicep and forearm power.", "Iron", ItemRarity::COMMON } },
    { AspectProperty::CLAWS_NAILS,          { "prop_claws_nails", "Claws & Nails", "Sharpens fingernails into predatory claws, talons, or soft pads.", "Lethal", ItemRarity::UNCOMMON } },
    { AspectProperty::MANUAL_DEXTERITY,     { "prop_manual_dexterity", "Manual Dexterity", "Sharpens hand-eye coordination and fine motor precision.", "Nimble", ItemRarity::COMMON } },

    // Horns, Wings & Tail
    { AspectProperty::HORN_SIZE,            { "prop_horn_size", "Horn Size", "Lengthens and thickens cranial horns.", "Majestic", ItemRarity::COMMON } },
    { AspectProperty::HORN_SHAPE,           { "prop_horn_shape", "Horn Shape & Arch", "Curves horns into ram curls, sweeping gazelle spikes, or demonic spirals.", "Curved", ItemRarity::COMMON } },
    { AspectProperty::HORN_TEXTURE,         { "prop_horn_texture", "Horn Texture", "Polishes horn surface into smooth obsidian, ribbed bone, or crystalline ridges.", "Ribbed", ItemRarity::COMMON } },
    { AspectProperty::RACIAL_HORN_PRIMARY,  { "prop_racial_horn_primary", "Racial Horns", "Sprouts or transforms horns matching reagent race.", "Crowned", ItemRarity::RARE } },
    { AspectProperty::RACIAL_HORN_VARIANT,  { "prop_racial_horn_variant", "Horn Morph Variant", "Selects alternative horn configuration or extra brow spikes.", "Spiked", ItemRarity::RARE } },
    { AspectProperty::WING_SIZE,            { "prop_wing_size", "Wing Span", "Expands wingspan and aerodynamic surface area.", "Soaring", ItemRarity::COMMON } },
    { AspectProperty::WING_TYPE,            { "prop_wing_type", "Wing Membrane Type", "Alters wing structure (leathery bat, feathered angelic, insectoid chitin).", "Feathered", ItemRarity::UNCOMMON } },
    { AspectProperty::GLIDING_FLIGHT,       { "prop_gliding_flight", "Gliding & Flight", "Imbues lift, sustained glide, or airborne maneuverability.", "Ascendant", ItemRarity::RARE } },
    { AspectProperty::RACIAL_WING_PRIMARY,  { "prop_racial_wing_primary", "Racial Wings", "Sprouts or transforms wings matching reagent race.", "Ascendant", ItemRarity::RARE } },
    { AspectProperty::RACIAL_WING_VARIANT,  { "prop_racial_wing_variant", "Wing Morph Variant", "Selects secondary wing pair or crest plume configuration.", "Celestial", ItemRarity::RARE } },
    { AspectProperty::TAIL_LENGTH,          { "prop_tail_length", "Tail Length", "Extends caudal length and reach.", "Long", ItemRarity::COMMON } },
    { AspectProperty::TAIL_GIRTH,           { "prop_tail_girth", "Tail Girth & Fluff", "Expands tail thickness, fur volume, or muscular base.", "Fluffy", ItemRarity::COMMON } },
    { AspectProperty::TAIL_TYPE,            { "prop_tail_type", "Tail Configuration", "Alters tail structure (prehensile, spade-tipped, split, clubbed).", "Prehensile", ItemRarity::UNCOMMON } },
    { AspectProperty::RACIAL_TAIL_PRIMARY,  { "prop_racial_tail_primary", "Racial Tail", "Sprouts or transforms tail matching reagent race.", "Primal", ItemRarity::RARE } },
    { AspectProperty::RACIAL_TAIL_VARIANT,  { "prop_racial_tail_variant", "Tail Morph Variant", "Selects multi-tail, tufted tip, or armored variant.", "Mythic", ItemRarity::RARE } },
    { AspectProperty::PART_REMOVAL,         { "prop_part_removal", "Anatomical Cleansing", "Retracts, dissolves, or sheds the targeted anatomical structure.", "Vanishing", ItemRarity::UNCOMMON } },

    // Combat & Arcana
    { AspectProperty::DAMAGE_PHYSICAL,      { "prop_damage_physical", "Physical Lethality", "Infuses striking edge and impact velocity to deal bonus physical trauma.", "Striking", ItemRarity::UNCOMMON } },
    { AspectProperty::ATTACK_POWER,         { "prop_attack_power", "Attack Power", "Bolsters baseline offensive damage output.", "Brutal", ItemRarity::COMMON } },
    { AspectProperty::STRIKE_VELOCITY,      { "prop_strike_velocity", "Strike Velocity", "Accelerates weapon swing speed and attack tempo.", "Swift", ItemRarity::UNCOMMON } },
    { AspectProperty::CRITICAL_POWER,       { "prop_critical_power", "Critical Precision", "Sharpens strike precision to inflict devastating critical wounds.", "Keen", ItemRarity::RARE } },
    { AspectProperty::LIFE_LEECH,           { "prop_life_leech", "Vampiric Leech", "Siphons the vital essence of targets on strike to restore health.", "Vampiric", ItemRarity::EPIC } },
    { AspectProperty::DAMAGE_ELEMENTAL,     { "prop_damage_elemental", "Elemental Surge", "Surges with fiery, icy, or toxic energies inflicting elemental damage.", "Elemental", ItemRarity::RARE } },
    { AspectProperty::ARCANE_STAT,          { "prop_arcane", "Arcane Resonance", "Attunes the essence to arcane channels and magical spellcasting.", "Clarity", ItemRarity::RARE } },
    { AspectProperty::MANA_CEILING,         { "prop_mana_ceiling", "Maximum Aura", "Expands maximum mana pool and magical reserves.", "Aura", ItemRarity::EPIC } },
    { AspectProperty::MANA_REGENERATION,    { "prop_mana_regen", "Mana Regeneration", "Accelerates passive replenishment of magical reserves.", "Focus", ItemRarity::RARE } },

    // Armor & Defense
    { AspectProperty::ARMOR_RATING,         { "prop_armor_rating", "Armor Protection", "Bolsters physical deflection and reduces incoming kinetic trauma.", "Reinforced", ItemRarity::UNCOMMON } },
    { AspectProperty::FORTITUDE_STAT,       { "prop_fortitude", "Fortitude", "Grants enduring bodily resilience against stuns and physical impact.", "Fortified", ItemRarity::COMMON } },
    { AspectProperty::WARD_RESISTANCE,      { "prop_ward_resistance", "Elemental Warding", "Weaves a shimmering barrier deflecting magical and elemental damage.", "Warded", ItemRarity::RARE } },
    { AspectProperty::MIND_WARD,            { "prop_mind_ward", "Psychic Ward", "Protects against mental charms, hypnosis, and aphrodisiac corruption.", "Sanctuary", ItemRarity::RARE } },

    // Binding, Seals & Sensory
    { AspectProperty::SOULBOUND_SEAL,       { "prop_soulbound", "Soulbound Seal", "Affixes clothing permanently onto wearer until purged with essence.", "Soulbound", ItemRarity::EPIC } },
    { AspectProperty::SERVITUDE_INHIBITION,  { "prop_servitude", "Servitude Binding", "Suppresses uninhibited transformations and seals willpower.", "Servitude", ItemRarity::EPIC } },
    { AspectProperty::SENSORY_VIBRATION,    { "prop_vibration", "Sensory Vibration", "Generates continuous rhythmic sensory stimulation.", "Buzzing", ItemRarity::RARE } },

    // Backwards compatibility mappings
    { AspectProperty::SCALE_SIZE,           { "prop_scale_size", "Scale & Dimensions", "Magnifies or shrinks the primary dimension.", "Greater", ItemRarity::COMMON } },
    { AspectProperty::SECONDARY_SIZE,       { "prop_secondary_size", "Secondary Dimensions", "Modifies secondary dimensions such as width or thickness.", "Broadened", ItemRarity::COMMON } },
    { AspectProperty::VOLUME_CAPACITY,      { "prop_volume_capacity", "Volume Capacity", "Expands fluid holding capacity or internal elasticity.", "Expansive", ItemRarity::UNCOMMON } },
    { AspectProperty::DEPTH,                { "prop_depth", "Depth & Accommodative Reach", "Increases depth of orifices or structural cavity reach.", "Deepened", ItemRarity::UNCOMMON } },
    { AspectProperty::ELASTICITY,           { "prop_elasticity", "Tissue Elasticity", "Enhances pliability and elastic rebound of bodily tissue.", "Pliant", ItemRarity::UNCOMMON } },
    { AspectProperty::FLUID_PRODUCTION,     { "prop_fluid_production", "Fluid Production", "Increases volume of lactation, essence fluids, or natural lubrication.", "Overflowing", ItemRarity::RARE } },
    { AspectProperty::REGENERATION_RATE,    { "prop_regeneration", "Fluid Regeneration", "Accelerates the biological replenishment rate of fluids.", "Quenched", ItemRarity::RARE } },
    { AspectProperty::HAIR_GROWTH,          { "prop_hair_growth", "Follicle Growth", "Accelerates follicle growth or shifts hair density.", "Luxuriant", ItemRarity::COMMON } },
    { AspectProperty::PHYSIQUE_STAT,        { "prop_physique", "Physique & Might", "Imbues brute muscle density, fortitude, and physical power.", "Might", ItemRarity::RARE } },
    { AspectProperty::AGILITY_STAT,         { "prop_agility", "Agility & Reflexes", "Enhances movement speed, balance, and critical evasion.", "Instinct", ItemRarity::RARE } },
    { AspectProperty::HEALTH_CEILING,       { "prop_health_ceiling", "Maximum Vitality", "Bolsters maximum health reserves.", "Vitality", ItemRarity::EPIC } },
    { AspectProperty::VIRILITY_FACTOR,      { "prop_virility", "Virile Potency", "Dramatically surges virility, masculine arousal, and potency.", "Virility", ItemRarity::RARE } },
    { AspectProperty::FERTILITY_FACTOR,     { "prop_fertility", "Fertile Receptivity", "Elevates fertile receptivity, feminine hormonal response, and bloom.", "Fertility", ItemRarity::RARE } },
    { AspectProperty::CORRUPTION_AURA,      { "prop_corruption", "Demonic Corruption", "Induces seductive corruption and unrestrained libido.", "Sin", ItemRarity::EPIC } },
    { AspectProperty::RACIAL_TRANSFORMATION,{ "prop_racial_tf", "Racial Awakening", "Manifests or awakens the full racial morphology of the reagent.", "Awakened", ItemRarity::RARE } },

    // Core Attributes
    { AspectProperty::CORE_HEALTH,          { "prop_core_health", "Health Vitality", "Bolsters maximum health reserves and physical endurance.", "Vital", ItemRarity::COMMON } },
    { AspectProperty::CORE_MANA,            { "prop_core_mana", "Mana Aura", "Expands maximum mana pool and magical reserves.", "Aura", ItemRarity::COMMON } },
    { AspectProperty::CORE_STAMINA,         { "prop_core_stamina", "Stamina", "Enhances stamina threshold and action endurance.", "Enduring", ItemRarity::COMMON } },
    { AspectProperty::CORE_PHYSIQUE,        { "prop_core_physique", "Physique Might", "Augments core muscular density, raw power, and physical fortitude.", "Might", ItemRarity::COMMON } },
    { AspectProperty::CORE_ARCANE,          { "prop_core_arcane", "Arcane Resonance", "Deepens attunement to arcane currents and mystic energy.", "Resonant", ItemRarity::COMMON } },
    { AspectProperty::CORE_CORRUPTION,      { "prop_core_corruption", "Demonic Corruption", "Induces seductive corruption and unrestrained libido.", "Corrupt", ItemRarity::UNCOMMON } },

    // General Attributes
    { AspectProperty::ATTR_PHYSICAL_DAMAGE, { "prop_attr_phys_dmg", "Physical Damage", "Increases melee strike force and kinetic weapon impact.", "Striking", ItemRarity::COMMON } },
    { AspectProperty::ATTR_SPELL_POWER,     { "prop_attr_spell_pwr", "Spell Power", "Amplifies destructive and beneficial spell potency.", "Potent", ItemRarity::COMMON } },
    { AspectProperty::ATTR_ARMOR,           { "prop_attr_armor", "Defense / Armor", "Bolsters kinetic deflection and physical armor protection.", "Armored", ItemRarity::COMMON } },
    { AspectProperty::ATTR_WARDING,         { "prop_attr_warding", "Wards / Resistance", "Weaves defensive barriers against elemental and magical harm.", "Warded", ItemRarity::COMMON } },
    { AspectProperty::ATTR_FERTILITY,       { "prop_attr_fertility", "Fertility", "Elevates conception probability and womb bloom.", "Fertile", ItemRarity::COMMON } },
    { AspectProperty::ATTR_VIRILITY,        { "prop_attr_virility", "Virility", "Heightens reproductive virility and masculine vigor.", "Virile", ItemRarity::COMMON } },
    { AspectProperty::ATTR_CRITICAL,        { "prop_attr_critical", "Crit Precision", "Sharpens accuracy to exploit vulnerabilities and strike critically.", "Precise", ItemRarity::COMMON } },
    { AspectProperty::ATTR_AGILITY,         { "prop_attr_agility", "Agility / Speed", "Enhances movement speed, balance, and evasion reflexes.", "Swift", ItemRarity::COMMON } },

    // Special Effects
    { AspectProperty::CONCEAL_IDENTITY,     { "prop_conceal_identity", "Conceal Identity", "Obscures facial features under an arcane shroud, hiding true identity.", "Shrouded", ItemRarity::EPIC } },

    // Fluid Retention
    { AspectProperty::RETENTION_STOMACH,    { "prop_retention_stomach", "Stomach Retention", "Increases capacity to retain fluids and food in the stomach without digestion discomfort.", "Sated", ItemRarity::UNCOMMON } },
    { AspectProperty::RETENTION_MAMMARY,    { "prop_retention_mammary", "Mammary Retention", "Suppresses spontaneous milk leakage, allowing extreme engorgement.", "Engorged", ItemRarity::UNCOMMON } },
    { AspectProperty::RETENTION_YONI,       { "prop_retention_yoni", "Yoni Retention", "Tightly seals semen and fluids within the vaginal canal.", "Sealed", ItemRarity::UNCOMMON } },
    { AspectProperty::RETENTION_ANAL,       { "prop_retention_anal", "Anal Retention", "Tightly locks enema fluids and seed within the anal passage.", "Locked", ItemRarity::UNCOMMON } },

    // Body Desires / Fetishes
    { AspectProperty::DESIRE_ANAL,          { "prop_desire_anal", "Anal Attunement", "Heightens psychological craving and physical sensitivity for anal play.", "Rearward", ItemRarity::RARE } },
    { AspectProperty::DESIRE_MAMMARY,       { "prop_desire_mammary", "Mammary Attunement", "Deepens fixations with breast worship, suckling, and chest contact.", "Maternal", ItemRarity::RARE } },
    { AspectProperty::DESIRE_PHALLIC,       { "prop_desire_phallic", "Phallic Attunement", "Intensifies desires for phallic pleasure, stroking, and penetrating.", "Phallic", ItemRarity::RARE } },
    { AspectProperty::DESIRE_YONI,          { "prop_desire_yoni", "Yoni Attunement", "Heightens cravings for vaginal intimacy, fingering, and mating.", "Receptive", ItemRarity::RARE } },
    { AspectProperty::DESIRE_FEET,          { "prop_desire_feet", "Foot Attunement", "Imbues an erotic fascination with soles, paws, hooves, and trampling.", "Pedal", ItemRarity::RARE } },

    // Behavioral Desires / Fetishes
    { AspectProperty::DESIRE_DOMINANT,      { "prop_desire_dominant", "Dominant Instinct", "Inspires command, authority, and assertiveness over partners.", "Imperious", ItemRarity::RARE } },
    { AspectProperty::DESIRE_SUBMISSIVE,    { "prop_desire_submissive", "Submissive Instinct", "Encourages yielding, obedience, and blissful surrender.", "Obedient", ItemRarity::RARE } },
    { AspectProperty::DESIRE_BONDAGE,       { "prop_desire_bondage", "Bondage Desire", "Creates an erotic longing to be tightly bound and restrained.", "Bound", ItemRarity::RARE } },
    { AspectProperty::DESIRE_EXHIBITIONISM, { "prop_desire_exhibitionism", "Exhibitionism", "Stirs thrilling arousal from displaying one's body openly.", "Exposed", ItemRarity::RARE } },
    { AspectProperty::DESIRE_CHASTITY,      { "prop_desire_chastity", "Chastity Desire", "Imparts deep satisfaction from sexual denial and confinement.", "Chaste", ItemRarity::RARE } },
    { AspectProperty::DESIRE_MASOCHISM,     { "prop_desire_masochism", "Masochism", "Transmutes physical discipline and impact into searing pleasure.", "Suffering", ItemRarity::RARE } },
    { AspectProperty::DESIRE_SADISM,        { "prop_desire_sadism", "Sadism", "Draws erotic excitement from dominating, marking, and teasing others.", "Cruel", ItemRarity::RARE } },

    // Additional Facial, Torso, Arm, Chest & Udder options
    { AspectProperty::NOSE_SIZE,            { "prop_nose_size", "Nose Size", "Adjusts nasal bridge, button tip, and contour prominence.", "Sculpted", ItemRarity::COMMON } },
    { AspectProperty::FACIAL_BEARD,         { "prop_facial_beard", "Facial Beard", "Styles and grows rugged facial hair, stubble, or long beards.", "Bearded", ItemRarity::COMMON } },
    { AspectProperty::EYEBROW_SHAPE,        { "prop_eyebrow_shape", "Eyebrow Contours", "Shapes eyebrow arch, thickness, and expression.", "Arched", ItemRarity::COMMON } },
    { AspectProperty::FEMININITY_MASCULINITY,{ "prop_fem_masc", "Femininity / Masculinity", "Shifts overall body and facial aura between masculine and feminine.", "Alluring", ItemRarity::COMMON } },
    { AspectProperty::BODY_HAIR,            { "prop_body_hair", "Body Hairiness", "Increases or removes chest, stomach, and leg body hair.", "Furred", ItemRarity::COMMON } },
    { AspectProperty::UNDERARM_HAIR,        { "prop_underarm_hair", "Underarm Hair", "Trims, grooms, or grows underarm hair.", "Groomed", ItemRarity::COMMON } },
    { AspectProperty::GRIP_STRENGTH,        { "prop_grip_strength", "Grip Strength", "Strengthens manual grip and grappling leverage.", "Gripping", ItemRarity::COMMON } },
    { AspectProperty::ANUS_WETNESS,         { "prop_anus_wetness", "Anus Wetness", "Increases natural internal lubrication of the anal canal.", "Moist", ItemRarity::UNCOMMON } },
    { AspectProperty::AREOLA_SIZE,          { "prop_areola_size", "Areola Diameter", "Expands or contracts the pigmented circle surrounding the nipple.", "Rosy", ItemRarity::COMMON } },
    { AspectProperty::MILK_FLAVOR,          { "prop_milk_flavor", "Milk Flavor & Nectar", "Imparts sweetened alchemical flavoring to expressed fluids.", "Nectar", ItemRarity::RARE } },
    { AspectProperty::UDDER_SIZE,           { "prop_udder_size", "Udder Volume", "Expands inguinal udder swell and milk storage capacity.", "Bountiful", ItemRarity::RARE } },
    { AspectProperty::TEAT_LENGTH,          { "prop_teat_length", "Teat Length", "Extends or trims udder teat length for milking.", "Milked", ItemRarity::COMMON } },
    { AspectProperty::TEAT_COUNT,           { "prop_teat_count", "Teat Count", "Increases number of teats (two, four, or six).", "Prolific", ItemRarity::UNCOMMON } },
    { AspectProperty::UDDER_CAPACITY,       { "prop_udder_capacity", "Udder Capacity", "Expands internal udder milk holding reservoir.", "Capacious", ItemRarity::RARE } },
    { AspectProperty::UDDER_REGEN,          { "prop_udder_regen", "Udder Lactation Regen", "Accelerates replenishment rate of udder milk.", "Quenched", ItemRarity::RARE } },
    { AspectProperty::ANTENNAE_MORPH,       { "prop_antennae_morph", "Racial Antennae", "Sprouts or transforms cranial sensory antennae.", "Attuned", ItemRarity::RARE } },
    { AspectProperty::ANTENNAE_LENGTH,      { "prop_antennae_length", "Antennae Length", "Extends antennae reach and sensory sensitivity.", "Feathered", ItemRarity::COMMON } },
    { AspectProperty::ANTENNAE_TYPE,        { "prop_antennae_type", "Antennae Type", "Alters antennae structure (feathered moth, slender thread, segmented).", "Silken", ItemRarity::UNCOMMON } },
    { AspectProperty::FOOT_MORPH,           { "prop_foot_morph", "Foot Morphology", "Morphs feet into hooves, padded paws, talons, or webbed digits.", "Paw", ItemRarity::RARE } },
    { AspectProperty::LEG_HAIR,             { "prop_leg_hair", "Leg Hair / Fur", "Adjusts follicle covering, shaves, or grows plush leg fur.", "Pelt", ItemRarity::COMMON } },
    { AspectProperty::ARMOR_PIERCING,       { "prop_armor_piercing", "Armor Piercing", "Strikes bypass natural armor deflection and hardened scales.", "Sundering", ItemRarity::RARE } },
    { AspectProperty::WEAPON_BLEED,         { "prop_weapon_bleed", "Lacerating Bleed", "Inflicts deep hemorrhaging wounds causing damage over time.", "Lacerating", ItemRarity::UNCOMMON } }
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
    if (!baseItem)
    {
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

    bool isWeapon = (baseItem->category == ItemCategory::WEAPON ||
                     baseItem->targetSlot == equipSlot::WEAPON_MAIN ||
                     baseItem->targetSlot == equipSlot::WEAPON_OFF);

    if (isWeapon)
    {
        return {
            EnchantmentFocus::WEAPON_LETHALITY,
            EnchantmentFocus::ARCANE_AMPLIFICATION,
            EnchantmentFocus::RESISTANCE_WARDING,
            EnchantmentFocus::BINDING_SPECIAL
        };
    }

    bool isApparel = (baseItem->category == ItemCategory::CLOTHING ||
                      baseItem->category == ItemCategory::UNDERWEAR ||
                      baseItem->category == ItemCategory::ACCESSORY ||
                      (baseItem->isEquippable && !isWeapon));

    if (isApparel)
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
            EnchantmentFocus::ARMOR_REINFORCEMENT,
            EnchantmentFocus::RESISTANCE_WARDING,
            EnchantmentFocus::ARCANE_AMPLIFICATION,
            EnchantmentFocus::BINDING_SPECIAL
        };
    }

    // Consumables / Food / Potions with racial affinity
    if (baseItem->isRacialReagent())
    {
        return {
            EnchantmentFocus::RACE_AWAKENING,
            EnchantmentFocus::HEAD_FEATURE,
            EnchantmentFocus::HORNS,
            EnchantmentFocus::ANTENNAE,
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
            EnchantmentFocus::WINGS,
            EnchantmentFocus::TAIL,
            EnchantmentFocus::GENITALIA_PRIMARY,
            EnchantmentFocus::GENITALIA_SECONDARY,
            EnchantmentFocus::HIPS_ASS,
            EnchantmentFocus::LEGS_FEET,
            EnchantmentFocus::FLUIDS_CUM,
            EnchantmentFocus::FLUIDS_MILK,
            EnchantmentFocus::FLUIDS_GIRLCUM,
            EnchantmentFocus::CORE_ATTRIBUTES,
            EnchantmentFocus::SPECIAL_EFFECTS,
            EnchantmentFocus::ARCANE_AMPLIFICATION,
            EnchantmentFocus::BINDING_SPECIAL
        };
    }

    // Generic consumable / food without race affinity
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
        EnchantmentFocus::FLUIDS_CUM,
        EnchantmentFocus::FLUIDS_MILK,
        EnchantmentFocus::FLUIDS_GIRLCUM,
        EnchantmentFocus::ARCANE_AMPLIFICATION,
        EnchantmentFocus::BINDING_SPECIAL
    };
}

std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus, const item* baseItem)
{
    bool isRacial = baseItem && baseItem->isRacialReagent();

    switch (focus)
    {
        case EnchantmentFocus::CORE_ATTRIBUTES:
            return {
                AspectProperty::CORE_HEALTH,
                AspectProperty::CORE_MANA,
                AspectProperty::CORE_STAMINA,
                AspectProperty::CORE_PHYSIQUE,
                AspectProperty::CORE_ARCANE,
                AspectProperty::CORE_CORRUPTION
            };

        case EnchantmentFocus::GENERAL_ATTRIBUTES:
            return {
                AspectProperty::ATTR_PHYSICAL_DAMAGE,
                AspectProperty::ATTR_SPELL_POWER,
                AspectProperty::ATTR_ARMOR,
                AspectProperty::ATTR_WARDING,
                AspectProperty::ATTR_FERTILITY,
                AspectProperty::ATTR_VIRILITY,
                AspectProperty::ATTR_CRITICAL,
                AspectProperty::ATTR_AGILITY
            };

        case EnchantmentFocus::SPECIAL_EFFECTS:
            return {
                AspectProperty::SOULBOUND_SEAL,
                AspectProperty::CONCEAL_IDENTITY,
                AspectProperty::SERVITUDE_INHIBITION,
                AspectProperty::SENSORY_VIBRATION
            };

        case EnchantmentFocus::RETENTION_FLUIDS:
            return {
                AspectProperty::RETENTION_STOMACH,
                AspectProperty::RETENTION_MAMMARY,
                AspectProperty::RETENTION_YONI,
                AspectProperty::RETENTION_ANAL
            };

        case EnchantmentFocus::BODY_DESIRES:
            return {
                AspectProperty::DESIRE_ANAL,
                AspectProperty::DESIRE_MAMMARY,
                AspectProperty::DESIRE_PHALLIC,
                AspectProperty::DESIRE_YONI,
                AspectProperty::DESIRE_FEET
            };

        case EnchantmentFocus::BEHAVIORAL_DESIRES:
            return {
                AspectProperty::DESIRE_DOMINANT,
                AspectProperty::DESIRE_SUBMISSIVE,
                AspectProperty::DESIRE_BONDAGE,
                AspectProperty::DESIRE_EXHIBITIONISM,
                AspectProperty::DESIRE_CHASTITY,
                AspectProperty::DESIRE_MASOCHISM,
                AspectProperty::DESIRE_SADISM
            };

        case EnchantmentFocus::BREASTS:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::BREAST_SIZE,
                AspectProperty::BREAST_SHAPE,
                AspectProperty::NIPPLE_LENGTH,
                AspectProperty::NIPPLE_GIRTH,
                AspectProperty::NIPPLE_TYPE,
                AspectProperty::NIPPLE_CAPACITY,
                AspectProperty::AREOLA_SIZE,
                AspectProperty::LACTATION_VOLUME,
                AspectProperty::LACTATION_REGEN,
                AspectProperty::FLUID_TYPE,
                AspectProperty::MILK_FLAVOR
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::CROTCH_MAMMARY_MORPH);
            }
            return props;
        }

        case EnchantmentFocus::CROTCH_MAMMARIES:
            return {
                AspectProperty::UDDER_SIZE,
                AspectProperty::TEAT_LENGTH,
                AspectProperty::TEAT_COUNT,
                AspectProperty::UDDER_CAPACITY,
                AspectProperty::UDDER_REGEN
            };

        case EnchantmentFocus::GENITALIA_PRIMARY:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::PENIS_LENGTH,
                AspectProperty::PENIS_GIRTH,
                AspectProperty::KNOT_SIZE,
                AspectProperty::TESTES_SIZE,
                AspectProperty::CUM_VOLUME,
                AspectProperty::CUM_REGEN,
                AspectProperty::VIRILITY_POTENCY
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_PHALLUS_MORPH);
                props.push_back(AspectProperty::RACIAL_SHEATH_MORPH);
            }
            return props;
        }

        case EnchantmentFocus::GENITALIA_SECONDARY:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::VAGINA_DEPTH,
                AspectProperty::VAGINA_TIGHTNESS,
                AspectProperty::CLIT_SIZE,
                AspectProperty::LABIA_SIZE,
                AspectProperty::LUBRICATION_WETNESS,
                AspectProperty::FERTILITY_RECEPTIVITY
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_YONI_MORPH);
            }
            return props;
        }

        case EnchantmentFocus::HIPS_ASS:
            return {
                AspectProperty::BUTT_SIZE,
                AspectProperty::HIP_WIDTH,
                AspectProperty::ANUS_CAPACITY,
                AspectProperty::ANUS_DEPTH,
                AspectProperty::ANUS_ELASTICITY,
                AspectProperty::ANUS_WETNESS
            };

        case EnchantmentFocus::LEGS_FEET:
        {
            std::vector<AspectProperty> props;
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_LEGS_BIPED);
                props.push_back(AspectProperty::RACIAL_STANCE_MORPH);
                props.push_back(AspectProperty::RACIAL_BODY_CONFIG);
                props.push_back(AspectProperty::FOOT_MORPH);
            }
            props.push_back(AspectProperty::LEG_LENGTH);
            props.push_back(AspectProperty::THIGH_FULLNESS);
            props.push_back(AspectProperty::LEG_HAIR);
            props.push_back(AspectProperty::SPRINT_AGILITY);
            return props;
        }

        case EnchantmentFocus::HEAD_FEATURE:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::FACE_SHAPE
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_FACIAL_STRUCTURE);
                props.push_back(AspectProperty::RACIAL_MUZZLE_MORPH);
            }
            props.push_back(AspectProperty::PREDATORY_PERCEPTION);
            return props;
        }

        case EnchantmentFocus::FACE:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::LIP_FULLNESS,
                AspectProperty::NOSE_SIZE,
                AspectProperty::FACIAL_BEARD,
                AspectProperty::EYEBROW_SHAPE,
                AspectProperty::FACE_SHAPE
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_MUZZLE_MORPH);
            }
            props.push_back(AspectProperty::ALLURING_GAZE);
            return props;
        }

        case EnchantmentFocus::MOUTH:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::LIP_FULLNESS
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_DENTITION);
                props.push_back(AspectProperty::RACIAL_TONGUE);
            }
            props.push_back(AspectProperty::THROAT_DEPTH);
            props.push_back(AspectProperty::SALIVA_PRODUCTION);
            return props;
        }

        case EnchantmentFocus::HAIR:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::HAIR_GROWTH_RATE,
                AspectProperty::HAIR_LENGTH,
                AspectProperty::HAIR_VOLUME,
                AspectProperty::HAIR_STYLE,
                AspectProperty::HAIR_COLOR
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_MANE_MORPH);
            }
            return props;
        }

        case EnchantmentFocus::EYES:
            return {
                AspectProperty::EYE_PUPIL_SHAPE,
                AspectProperty::EYE_IRIS_COLOR,
                AspectProperty::DARKVISION_AURA,
                AspectProperty::ALLURING_GAZE
            };

        case EnchantmentFocus::EARS:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::EAR_SIZE
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_EAR_MORPH);
            }
            props.push_back(AspectProperty::KEEN_HEARING);
            return props;
        }

        case EnchantmentFocus::TORSO:
            return {
                AspectProperty::STATURE_HEIGHT,
                AspectProperty::MUSCLE_PHYSIQUE,
                AspectProperty::WAIST_TAPER,
                AspectProperty::FEMININITY_MASCULINITY,
                AspectProperty::BODY_HAIR,
                AspectProperty::STOMACH_FIRMNESS,
                AspectProperty::HEALTH_VITALITY
            };

        case EnchantmentFocus::SKIN:
        {
            std::vector<AspectProperty> props;
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_COVERING_TYPE);
                props.push_back(AspectProperty::RACIAL_PATTERN_COLOR);
            }
            props.push_back(AspectProperty::DERMIS_ELASTICITY);
            props.push_back(AspectProperty::NATURAL_ARMOR);
            return props;
        }

        case EnchantmentFocus::ARMS:
            return {
                AspectProperty::ARM_MUSCLE,
                AspectProperty::UNDERARM_HAIR,
                AspectProperty::GRIP_STRENGTH,
                AspectProperty::CLAWS_NAILS,
                AspectProperty::MANUAL_DEXTERITY
            };

        case EnchantmentFocus::HORNS:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::HORN_SIZE,
                AspectProperty::HORN_SHAPE,
                AspectProperty::HORN_TEXTURE
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_HORN_PRIMARY);
                props.push_back(AspectProperty::RACIAL_HORN_VARIANT);
            }
            props.push_back(AspectProperty::PART_REMOVAL);
            return props;
        }

        case EnchantmentFocus::ANTENNAE:
        {
            std::vector<AspectProperty> props;
            if (isRacial)
            {
                props.push_back(AspectProperty::ANTENNAE_MORPH);
            }
            props.push_back(AspectProperty::ANTENNAE_LENGTH);
            props.push_back(AspectProperty::ANTENNAE_TYPE);
            props.push_back(AspectProperty::PART_REMOVAL);
            return props;
        }

        case EnchantmentFocus::WINGS:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::WING_SIZE,
                AspectProperty::WING_TYPE,
                AspectProperty::GLIDING_FLIGHT
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_WING_PRIMARY);
                props.push_back(AspectProperty::RACIAL_WING_VARIANT);
            }
            props.push_back(AspectProperty::PART_REMOVAL);
            return props;
        }

        case EnchantmentFocus::TAIL:
        {
            std::vector<AspectProperty> props = {
                AspectProperty::TAIL_LENGTH,
                AspectProperty::TAIL_GIRTH,
                AspectProperty::TAIL_TYPE
            };
            if (isRacial)
            {
                props.push_back(AspectProperty::RACIAL_TAIL_PRIMARY);
                props.push_back(AspectProperty::RACIAL_TAIL_VARIANT);
            }
            props.push_back(AspectProperty::PART_REMOVAL);
            return props;
        }

        case EnchantmentFocus::FLUIDS_CUM:
            return {
                AspectProperty::CUM_VOLUME,
                AspectProperty::CUM_REGEN,
                AspectProperty::VIRILITY_POTENCY
            };

        case EnchantmentFocus::FLUIDS_MILK:
            return {
                AspectProperty::LACTATION_VOLUME,
                AspectProperty::LACTATION_REGEN,
                AspectProperty::FLUID_TYPE,
                AspectProperty::MILK_FLAVOR
            };

        case EnchantmentFocus::FLUIDS_GIRLCUM:
            return {
                AspectProperty::LUBRICATION_WETNESS,
                AspectProperty::FERTILITY_RECEPTIVITY
            };

        case EnchantmentFocus::RACE_AWAKENING:
            return {
                AspectProperty::RACIAL_TRANSFORMATION
            };

        case EnchantmentFocus::WEAPON_LETHALITY:
            return {
                AspectProperty::ATTACK_POWER,
                AspectProperty::STRIKE_VELOCITY,
                AspectProperty::DAMAGE_PHYSICAL,
                AspectProperty::DAMAGE_ELEMENTAL,
                AspectProperty::CRITICAL_POWER,
                AspectProperty::LIFE_LEECH,
                AspectProperty::ARMOR_PIERCING,
                AspectProperty::WEAPON_BLEED
            };

        case EnchantmentFocus::ARMOR_REINFORCEMENT:
            return {
                AspectProperty::ARMOR_RATING,
                AspectProperty::FORTITUDE_STAT,
                AspectProperty::HEALTH_VITALITY
            };

        case EnchantmentFocus::ARCANE_AMPLIFICATION:
            return {
                AspectProperty::ARCANE_STAT,
                AspectProperty::MANA_CEILING,
                AspectProperty::MANA_REGENERATION,
                AspectProperty::DAMAGE_ELEMENTAL
            };

        case EnchantmentFocus::RESISTANCE_WARDING:
            return {
                AspectProperty::WARD_RESISTANCE,
                AspectProperty::MIND_WARD,
                AspectProperty::FORTITUDE_STAT
            };

        case EnchantmentFocus::BINDING_SPECIAL:
            return {
                AspectProperty::SOULBOUND_SEAL,
                AspectProperty::SERVITUDE_INHIBITION,
                AspectProperty::SENSORY_VIBRATION
            };

        default:
            return {
                AspectProperty::ARMOR_RATING,
                AspectProperty::ARCANE_STAT,
                AspectProperty::HEALTH_VITALITY
            };
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
        // Core Attributes
        case AspectProperty::CORE_HEALTH:          return "Health";
        case AspectProperty::CORE_MANA:            return "Mana";
        case AspectProperty::CORE_STAMINA:         return "Stamina";
        case AspectProperty::CORE_PHYSIQUE:        return "Physiq";
        case AspectProperty::CORE_ARCANE:          return "Arcane";
        case AspectProperty::CORE_CORRUPTION:      return "Corrupt";

        // General Attributes
        case AspectProperty::ATTR_PHYSICAL_DAMAGE: return "Damage";
        case AspectProperty::ATTR_SPELL_POWER:     return "Sp Power";
        case AspectProperty::ATTR_ARMOR:           return "Armor";
        case AspectProperty::ATTR_WARDING:         return "Wards";
        case AspectProperty::ATTR_FERTILITY:       return "Fertil";
        case AspectProperty::ATTR_VIRILITY:        return "Viril";
        case AspectProperty::ATTR_CRITICAL:        return "Crit";
        case AspectProperty::ATTR_AGILITY:         return "Agility";

        // Special Effects
        case AspectProperty::CONCEAL_IDENTITY:     return "Shroud";

        // Retention Fluids
        case AspectProperty::RETENTION_STOMACH:    return "Stomach";
        case AspectProperty::RETENTION_MAMMARY:    return "Mammary";
        case AspectProperty::RETENTION_YONI:       return "Yoni";
        case AspectProperty::RETENTION_ANAL:       return "Anal";

        // Desires
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

        // Chest & Breasts
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

        // Crotch Udders
        case AspectProperty::UDDER_SIZE:           return "Udder";
        case AspectProperty::TEAT_LENGTH:          return "TeatLen";
        case AspectProperty::TEAT_COUNT:           return "Teats";
        case AspectProperty::UDDER_CAPACITY:       return "UddrCap";
        case AspectProperty::UDDER_REGEN:          return "UddrReg";

        // Phallus & Virility
        case AspectProperty::PENIS_LENGTH:         return "Length";
        case AspectProperty::PENIS_GIRTH:          return "Girth";
        case AspectProperty::KNOT_SIZE:            return "Knot";
        case AspectProperty::TESTES_SIZE:          return "Testes";
        case AspectProperty::CUM_VOLUME:           return "Cum Vol";
        case AspectProperty::CUM_REGEN:            return "Cum Regen";
        case AspectProperty::VIRILITY_POTENCY:     return "Virility";
        case AspectProperty::RACIAL_PHALLUS_MORPH: return "Phallus";
        case AspectProperty::RACIAL_SHEATH_MORPH:  return "Sheath";

        // Yoni & Fertility
        case AspectProperty::VAGINA_DEPTH:         return "Depth";
        case AspectProperty::VAGINA_TIGHTNESS:     return "Tightness";
        case AspectProperty::CLIT_SIZE:            return "Clit";
        case AspectProperty::LABIA_SIZE:           return "Labia";
        case AspectProperty::LUBRICATION_WETNESS:  return "Wetness";
        case AspectProperty::FERTILITY_RECEPTIVITY:return "Fertility";
        case AspectProperty::RACIAL_YONI_MORPH:    return "Yoni";

        // Hips & Derriere
        case AspectProperty::BUTT_SIZE:            return "Butt";
        case AspectProperty::HIP_WIDTH:            return "Hips";
        case AspectProperty::ANUS_CAPACITY:        return "Anus Cap";
        case AspectProperty::ANUS_DEPTH:           return "Anus Depth";
        case AspectProperty::ANUS_ELASTICITY:      return "Elastic";
        case AspectProperty::ANUS_WETNESS:         return "Wetness";

        // Legs & Lower Body
        case AspectProperty::LEG_LENGTH:           return "Length";
        case AspectProperty::THIGH_FULLNESS:       return "Thighs";
        case AspectProperty::LEG_HAIR:             return "LegHair";
        case AspectProperty::RACIAL_LEGS_BIPED:    return "Biped";
        case AspectProperty::RACIAL_STANCE_MORPH:  return "Stance";
        case AspectProperty::RACIAL_BODY_CONFIG:   return "Taur";
        case AspectProperty::FOOT_MORPH:           return "Foot";
        case AspectProperty::SPRINT_AGILITY:       return "Sprint";

        // Head & Visage
        case AspectProperty::FACE_SHAPE:           return "Face";
        case AspectProperty::NOSE_SIZE:            return "Nose";
        case AspectProperty::FACIAL_BEARD:         return "Beard";
        case AspectProperty::EYEBROW_SHAPE:        return "Eyebrow";
        case AspectProperty::RACIAL_FACIAL_STRUCTURE: return "Visage";
        case AspectProperty::RACIAL_MUZZLE_MORPH:  return "Muzzle";
        case AspectProperty::PREDATORY_PERCEPTION: return "Sense";

        // Mouth & Throat
        case AspectProperty::LIP_FULLNESS:         return "Lips";
        case AspectProperty::RACIAL_DENTITION:     return "Fangs";
        case AspectProperty::RACIAL_TONGUE:        return "Tongue";
        case AspectProperty::THROAT_DEPTH:         return "Throat";
        case AspectProperty::SALIVA_PRODUCTION:    return "Saliva";

        // Hair & Follicles
        case AspectProperty::HAIR_GROWTH_RATE:     return "Growth";
        case AspectProperty::HAIR_LENGTH:          return "Length";
        case AspectProperty::HAIR_VOLUME:          return "Volume";
        case AspectProperty::HAIR_STYLE:           return "Style";
        case AspectProperty::HAIR_COLOR:           return "Color";
        case AspectProperty::RACIAL_MANE_MORPH:    return "Mane";

        // Eyes & Vision
        case AspectProperty::EYE_PUPIL_SHAPE:      return "Pupils";
        case AspectProperty::EYE_IRIS_COLOR:       return "Iris";
        case AspectProperty::DARKVISION_AURA:      return "Darkvis";
        case AspectProperty::ALLURING_GAZE:        return "Allure";

        // Ears & Auditory
        case AspectProperty::EAR_SIZE:             return "Ear Size";
        case AspectProperty::RACIAL_EAR_MORPH:     return "Ears";
        case AspectProperty::KEEN_HEARING:         return "Hearing";

        // Torso & Stature
        case AspectProperty::STATURE_HEIGHT:       return "Height";
        case AspectProperty::MUSCLE_PHYSIQUE:      return "Muscle";
        case AspectProperty::WAIST_TAPER:          return "Waist";
        case AspectProperty::FEMININITY_MASCULINITY: return "Gender";
        case AspectProperty::BODY_HAIR:            return "BodyHair";
        case AspectProperty::STOMACH_FIRMNESS:     return "Tone";
        case AspectProperty::HEALTH_VITALITY:      return "Vitality";

        // Skin & Dermis
        case AspectProperty::RACIAL_COVERING_TYPE: return "Covering";
        case AspectProperty::RACIAL_PATTERN_COLOR: return "Pattern";
        case AspectProperty::DERMIS_ELASTICITY:    return "Elastic";
        case AspectProperty::NATURAL_ARMOR:        return "Armor";

        // Arms & Hands
        case AspectProperty::ARM_MUSCLE:           return "Muscle";
        case AspectProperty::UNDERARM_HAIR:        return "ArmHair";
        case AspectProperty::GRIP_STRENGTH:        return "Grip";
        case AspectProperty::CLAWS_NAILS:          return "Claws";
        case AspectProperty::MANUAL_DEXTERITY:     return "Dexterity";

        // Horns, Wings, Tail & Antennae
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

        // Combat & Arcana
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

        // Binding, Seals & Sensory
        case AspectProperty::SOULBOUND_SEAL:       return "Soulbound";
        case AspectProperty::SERVITUDE_INHIBITION: return "Servitude";
        case AspectProperty::SENSORY_VIBRATION:    return "Sensory";

        // Legacy aliases
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
        // Chest & Breasts
        case AspectProperty::BREAST_SIZE:
        case AspectProperty::NIPPLE_LENGTH:
        case AspectProperty::NIPPLE_GIRTH:
        case AspectProperty::AREOLA_SIZE:
        case AspectProperty::LACTATION_VOLUME:
        case AspectProperty::LACTATION_REGEN:

        // Crotch Udders
        case AspectProperty::UDDER_SIZE:
        case AspectProperty::TEAT_LENGTH:
        case AspectProperty::UDDER_CAPACITY:
        case AspectProperty::UDDER_REGEN:

        // Phallus
        case AspectProperty::PENIS_LENGTH:
        case AspectProperty::PENIS_GIRTH:
        case AspectProperty::KNOT_SIZE:
        case AspectProperty::TESTES_SIZE:
        case AspectProperty::CUM_VOLUME:
        case AspectProperty::CUM_REGEN:

        // Yoni
        case AspectProperty::CLIT_SIZE:
        case AspectProperty::LABIA_SIZE:
        case AspectProperty::VAGINA_DEPTH:
        case AspectProperty::VAGINA_TIGHTNESS:
        case AspectProperty::LUBRICATION_WETNESS:

        // Hips & Ass
        case AspectProperty::BUTT_SIZE:
        case AspectProperty::HIP_WIDTH:
        case AspectProperty::ANUS_CAPACITY:
        case AspectProperty::ANUS_DEPTH:
        case AspectProperty::ANUS_ELASTICITY:
        case AspectProperty::ANUS_WETNESS:

        // Legs
        case AspectProperty::LEG_LENGTH:
        case AspectProperty::THIGH_FULLNESS:
        case AspectProperty::LEG_HAIR:

        // Face
        case AspectProperty::LIP_FULLNESS:
        case AspectProperty::NOSE_SIZE:
        case AspectProperty::FACIAL_BEARD:
        case AspectProperty::EYEBROW_SHAPE:

        // Hair
        case AspectProperty::HAIR_LENGTH:
        case AspectProperty::HAIR_VOLUME:

        // Torso / Core
        case AspectProperty::STATURE_HEIGHT:
        case AspectProperty::MUSCLE_PHYSIQUE:
        case AspectProperty::WAIST_TAPER:
        case AspectProperty::FEMININITY_MASCULINITY:
        case AspectProperty::BODY_HAIR:

        // Arms
        case AspectProperty::UNDERARM_HAIR:
        case AspectProperty::ARM_MUSCLE:

        // Horns, Wings, Tail, Ears, Antennae
        case AspectProperty::HORN_SIZE:
        case AspectProperty::WING_SIZE:
        case AspectProperty::TAIL_LENGTH:
        case AspectProperty::TAIL_GIRTH:
        case AspectProperty::EAR_SIZE:
        case AspectProperty::ANTENNAE_LENGTH:

        // Legacy continuous
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


