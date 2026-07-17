#include "entity.h"
#include <iostream>

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
    if (hasPart(slot))
    {
        return &parts[slot];
    }
    return nullptr;
}

bool anatomyComponent::hasTag(bodySlot slot, const std::string& tag) const
{
    if (!hasPart(slot)) return false;

    const auto& tags = parts.at(slot).tags;
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

std::string getSlotName(bodySlot slot)
{
    switch (slot)
    {
        case bodySlot::HAIR: return "Hair";
        case bodySlot::HEAD: return "Head";
        case bodySlot::EYES: return "Eyes";
        case bodySlot::EARS: return "Ears";
        case bodySlot::MOUTH: return "Mouth";
        case bodySlot::NECK: return "Neck";
        case bodySlot::HORNS: return "Horns";
        case bodySlot::ANTENNAE: return "Antennae";
        case bodySlot::TORSO: return "Torso";
        case bodySlot::BREASTS: return "Breasts";
        case bodySlot::NIPPLES: return "Nipples";
        case bodySlot::STOMACH: return "Stomach";
        case bodySlot::BACK: return "Back";
        case bodySlot::ARMS: return "Arms";
        case bodySlot::HANDS: return "Hands";
        case bodySlot::FINGERS: return "Fingers";
        case bodySlot::HIPS: return "Hips";
        case bodySlot::GROIN: return "Groin";
        case bodySlot::ASS: return "Ass";
        case bodySlot::TAIL: return "Tail";
        case bodySlot::LEGS: return "Legs";
        case bodySlot::FEET: return "Feet";
        case bodySlot::WINGS: return "Wings";
        case bodySlot::TENTACLES: return "Tentacles";
        default: return "Unknown Slot";
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
                << " | Covering: " << pair.second.covering
                << " | Color: " << pair.second.color << "\n";

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

// --- entity Constructor ---

entity::entity(std::string entityId, std::string entityName)
    : id(entityId), name(entityName)
{
}