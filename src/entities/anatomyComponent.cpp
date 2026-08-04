#include "entities/anatomyComponent.h"

#include <algorithm>
#include <cmath>
#include <iostream>

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

void anatomyComponent::addMutation(const anatomyMutation& mut)
{
    activeMutations.push_back(mut);
}

void anatomyComponent::processMutations(int minutesPassed)
{
    if (minutesPassed <= 0 || activeMutations.empty()) return;

    for (auto it = activeMutations.begin(); it != activeMutations.end(); )
    {
        int tickMins = std::min(minutesPassed, it->minutesRemaining);
        it->minutesRemaining -= tickMins;

        bodyPart* part = getPart(it->targetSlot);

        if (part)
        {
            if (it->type == mutationType::GROWTH_LENGTH) part->length += (it->amountPerMinute * static_cast<float>(tickMins));
            else if (it->type == mutationType::GROWTH_DIAMETER) part->diameter += (it->amountPerMinute * static_cast<float>(tickMins));
        }

        if (it->minutesRemaining <= 0)
        {
            if (it->type == mutationType::REMOVE_PART)
            {
                removePart(it->targetSlot);
            }
            else if (part)
            {
                if (it->type == mutationType::CHANGE_RACE) part->race = it->targetValueString;
                else if (it->type == mutationType::CHANGE_COLOR) part->primaryColor = it->targetValueString;
                else if (it->type == mutationType::GROWTH_CUP) part->cupSize = static_cast<int>(it->targetValueFloat);
            }
            it = activeMutations.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::unordered_map<std::string, float> anatomyComponent::calculateRacePercentages() const
{
    std::unordered_map<std::string, float> raceCounts;
    int totalParts = 0;

    for (const auto& optPart : parts)
    {
        if (optPart.has_value())
        {
            const std::string& race = optPart->race;
            if (!race.empty())
            {
                raceCounts[race] += 1.0f;
                totalParts++;
            }
        }
    }

    if (totalParts == 0) return { {"Human", 100.0f} };

    std::unordered_map<std::string, float> racePercentages;
    for (const auto& [race, count] : raceCounts)
    {
        racePercentages[race] = (count / static_cast<float>(totalParts)) * 100.0f;
    }

    return racePercentages;
}

std::string anatomyComponent::getDominantRace() const
{
    auto percentages = calculateRacePercentages();
    std::string dominant = "Human";
    float maxPercent = -1.0f;

    for (const auto& [race, percent] : percentages)
    {
        if (percent > maxPercent)
        {
            maxPercent = percent;
            dominant = race;
        }
    }
    return dominant;
}

void anatomyComponent::applyTransformation(bodySlot slot, mutationType type, float amountOrVal,
                                           const std::string& strVal, int durationMinutes, const std::string& mutName)
{
    anatomyMutation mut;
    mut.id = mutName;
    mut.targetSlot = slot;
    mut.type = type;
    mut.targetValueFloat = amountOrVal;
    mut.targetValueString = strVal;
    mut.minutesRemaining = durationMinutes;

    if (durationMinutes > 0 && (type == mutationType::GROWTH_LENGTH || type == mutationType::GROWTH_DIAMETER))
    {
        mut.amountPerMinute = amountOrVal / static_cast<float>(durationMinutes);
    }
    else
    {
        mut.amountPerMinute = 0.0f;
    }

    addMutation(mut);
}

BodyPresentation anatomyComponent::getVisualPresentation() const
{
    int femininePoints = 0;
    int masculinePoints = 0;

    if (const bodyPart* breasts = getPart(bodySlot::BREASTS))
    {
        if (breasts->cupSize > 1) femininePoints += 2;
        else if (breasts->cupSize == 1) femininePoints += 1;
    }

    if (const bodyPart* groin = getPart(bodySlot::GROIN))
    {
        if (groin->name == "Vagina" || hasTag(bodySlot::GROIN, "female_genitalia"))
        {
            femininePoints += 2;
        }
        else if (groin->name == "Penis" || groin->length > 0.0f)
        {
            masculinePoints += 2;
        }
    }

    if (hasGlobalTag("feminine")) femininePoints += 2;
    if (hasGlobalTag("masculine")) masculinePoints += 2;

    if (femininePoints > masculinePoints) return BodyPresentation::FEMININE;
    if (masculinePoints > femininePoints) return BodyPresentation::MASCULINE;
    return BodyPresentation::ANDROGYNOUS;
}

float anatomyComponent::getAggregateStatBonus(const std::string& statName) const
{
    float totalBonus = 0.0f;
    for (const auto& optPart : parts)
    {
        if (optPart.has_value())
        {
            const auto& part = optPart.value();
            if (statName == "physique" && part.race == "Orc") totalBonus += 1.0f;
            if (statName == "arcane" && part.race == "Elf") totalBonus += 1.0f;
        }
    }
    return totalBonus;
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