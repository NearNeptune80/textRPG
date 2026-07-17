#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

// 1. Anatomical Sockets
enum class bodySlot
{
    HAIR, HEAD, EYES, EARS, MOUTH, NECK,
    HORNS, ANTENNAE,
    TORSO, BREASTS, NIPPLES, STOMACH, BACK,
    ARMS, HANDS, FINGERS,
    HIPS, GROIN, ASS, TAIL,
    LEGS, FEET,
    WINGS, TENTACLES
};

// 2. Main UI Equipment Grid (Now with Piercings)
enum class equipSlot
{
    HEADWEAR, EYEWEAR, MOUTHWEAR, NECKWEAR,
    SHOULDERS, TORSO_UNDER, TORSO_OVER,
    CHEST_WEAR, STOMACH_WEAR,
    WRISTS, HANDS, FINGER_PRIMARY, FINGER_SECONDARY,
    HIPS_WEAR, GROIN_UNDER, GROIN_OVER,
    LEGS_INNER, LEGS_OUTER, FEET,
    HORNS_SLOT, WINGS_SLOT, TAIL_SLOT,
    WEAPON_MAIN, WEAPON_OFF,
    // --- PIERCINGS ---
    PIERCING_EAR, PIERCING_NOSE, PIERCING_LIP,
    PIERCING_NIPPLE, PIERCING_NAVEL, PIERCING_GENITAL,
    NONE
};

// 3. Alternate UI Grid (Tattoos Page)
enum class tattooSlot
{
    FACE, NECK,
    CHEST, BREASTS, STOMACH, BACK,
    SHOULDERS, ARM_LEFT, ARM_RIGHT, HANDS,
    HIPS, GROIN, ASS,
    LEG_LEFT, LEG_RIGHT, FEET,
    NONE
};

// 4. Data Structs
struct bodyPart
{
    std::string id;
    std::string name;
    std::string race;
    std::string covering;
    std::string color;
    std::vector<std::string> tags;
};

struct tattoo
{
    std::string id;
    std::string name;
    std::string color;
    bool glowing; // Because magical glowing tattoos are a staple!
    std::vector<std::string> tags;
};

// 5. The Anatomy Component
class anatomyComponent
{
private:
    std::unordered_map<bodySlot, bodyPart> parts;
    std::unordered_map<tattooSlot, tattoo> tattoos;

public:
    // Body Part Methods
    void setPart(bodySlot slot, const bodyPart& part);
    void removePart(bodySlot slot);
    bool hasPart(bodySlot slot) const;
    bodyPart* getPart(bodySlot slot);
    bool hasTag(bodySlot slot, const std::string& tag) const;

    // Tattoo Methods
    void setTattoo(tattooSlot slot, const tattoo& tat);
    void removeTattoo(tattooSlot slot);
    bool hasTattoo(tattooSlot slot) const;
    tattoo* getTattoo(tattooSlot slot);

    void printDebug() const;
};

// 6. The Core Entity
class entity
{
public:
    std::string id;
    std::string name;

    anatomyComponent anatomy;

    entity(std::string entityId, std::string entityName);
};