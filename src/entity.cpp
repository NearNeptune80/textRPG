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