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

// Orifices and Bodily Fluids
bool anatomyComponent::hasOrifice(bodySlot slot) const
{
    const bodyPart* part = getPart(slot);
    return part && part->orifice.exists;
}

OrificeData* anatomyComponent::getOrifice(bodySlot slot)
{
    bodyPart* part = getPart(slot);
    if (part && part->orifice.exists) return &part->orifice;
    return nullptr;
}

const OrificeData* anatomyComponent::getOrifice(bodySlot slot) const
{
    const bodyPart* part = getPart(slot);
    if (part && part->orifice.exists) return &part->orifice;
    return nullptr;
}

void anatomyComponent::transferFluidToOrifice(bodySlot slot, const std::string& fluidType, float amount)
{
    OrificeData* orf = getOrifice(slot);
    if (!orf || amount <= 0.0f) return;

    float currentTotal = 0.0f;
    for (const auto& [f, amt] : orf->storedFluids) currentTotal += amt;

    float spaceAvailable = std::max(0.0f, orf->maxCapacityMl - currentTotal);
    float actualTransfer = std::min(amount, spaceAvailable);
    orf->storedFluids[fluidType] += actualTransfer;
}

void anatomyComponent::stretchOrifice(bodySlot slot, float diameter)
{
    OrificeData* orf = getOrifice(slot);
    if (!orf || diameter <= 0.0f) return;

    if (diameter > orf->currentStretch)
    {
        float excess = diameter - orf->currentStretch;
        float stretchGain = excess * (1.0f - (orf->elasticity / 150.0f));
        orf->currentStretch = std::clamp(orf->currentStretch + stretchGain, 0.0f, 100.0f);
    }
}

void anatomyComponent::processBiologicalRecovery(int minutesPassed)
{
    if (minutesPassed <= 0) return;
    float hours = static_cast<float>(minutesPassed) / 60.0f;

    // Orifice Stretch Recovery & Fluid Production
    for (auto& optPart : parts)
    {
        if (!optPart.has_value()) continue;
        bodyPart& part = optPart.value();

        // 1. Orifice stretch recovery
        if (part.orifice.exists && part.orifice.currentStretch > 0.0f)
        {
            float recoveryRate = (part.orifice.elasticity / 20.0f) * hours;
            part.orifice.currentStretch = std::max(0.0f, part.orifice.currentStretch - recoveryRate);
        }

        // 2. Fluid regeneration (Milk, Cum, Girlcum)
        if (part.fluidRegenPerHour > 0.0f && part.maxFluidMl > 0.0f)
        {
            part.currentFluidMl = std::min(part.maxFluidMl, part.currentFluidMl + (part.fluidRegenPerHour * hours));
        }
    }
}

// 3-Tier Weighted Racial Classification
std::unordered_map<std::string, float> anatomyComponent::calculateRacePercentages() const
{
    return calculateWeightedRacePercentages();
}

std::unordered_map<std::string, float> anatomyComponent::calculateWeightedRacePercentages() const
{
    std::unordered_map<std::string, float> raceScores;
    float totalWeight = 0.0f;

    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        if (parts[i].has_value())
        {
            const std::string& race = parts[i]->race;
            if (race.empty()) continue;

            bodySlot slot = static_cast<bodySlot>(i);
            float weight = 1.0f;
            if (slot == bodySlot::HEAD || slot == bodySlot::TORSO || slot == bodySlot::GROIN) weight = 3.0f;
            else if (slot == bodySlot::TAIL || slot == bodySlot::EARS || slot == bodySlot::WINGS || slot == bodySlot::HORNS) weight = 2.0f;

            raceScores[race] += weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0f) return { {"Human", 100.0f} };

    std::unordered_map<std::string, float> percentages;
    for (const auto& [race, score] : raceScores)
    {
        percentages[race] = (score / totalWeight) * 100.0f;
    }
    return percentages;
}

RacialClassification anatomyComponent::getRacialClassification() const
{
    auto percentages = calculateWeightedRacePercentages();
    std::vector<std::pair<std::string, float>> sorted(percentages.begin(), percentages.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    RacialClassification result;
    if (sorted.empty()) return result;

    result.primaryRace = sorted[0].first;
    result.primaryPercentage = sorted[0].second;

    if (sorted.size() > 1)
    {
        result.secondaryRace = sorted[1].first;
        result.secondaryPercentage = sorted[1].second;
    }

    // Tier 1: Dominant Morph (>= 50%)
    if (result.primaryPercentage >= 50.0f)
    {
        result.tier = RacialTier::DOMINANT_MORPH;
        if (result.primaryPercentage >= 99.0f)
        {
            result.title = (result.primaryRace == "Human") ? "Human" : "Pure " + result.primaryRace;
        }
        else
        {
            result.title = result.primaryRace + "-Morph";
        }
    }
    // Tier 2: Dual-Hybrid (Top 2 both >= 40%)
    else if (sorted.size() >= 2 && sorted[0].second >= 40.0f && sorted[1].second >= 40.0f)
    {
        result.tier = RacialTier::DUAL_HYBRID;
        result.title = sorted[0].first + "-" + sorted[1].first + " Hybrid";
    }
    // Tier 3: Chaotic Chimera (< 40% across 3+ species)
    else
    {
        result.tier = RacialTier::CHAOTIC_CHIMERA;
        result.title = "Chaotic Chimera";
    }

    return result;
}

std::string anatomyComponent::getDominantRace() const
{
    return getRacialClassification().primaryRace;
}

std::string anatomyComponent::getRacialTitle() const
{
    return getRacialClassification().title;
}

bool anatomyComponent::isDualHybrid() const
{
    return getRacialClassification().tier == RacialTier::DUAL_HYBRID;
}

bool anatomyComponent::isChaoticChimera() const
{
    return getRacialClassification().tier == RacialTier::CHAOTIC_CHIMERA;
}

// Gender Archetypes
bool anatomyComponent::hasPenis() const
{
    const bodyPart* groin = getPart(bodySlot::GROIN);
    if (!groin) return false;
    return groin->name == "Penis" || groin->length > 0.0f || hasTag(bodySlot::GROIN, "penis") || hasTag(bodySlot::GROIN, "has_penis");
}

bool anatomyComponent::hasVagina() const
{
    const bodyPart* groin = getPart(bodySlot::GROIN);
    if (!groin) return false;
    return groin->name == "Vagina" || groin->orifice.exists || hasTag(bodySlot::GROIN, "vagina") || hasTag(bodySlot::GROIN, "has_vagina") || hasTag(bodySlot::GROIN, "female_genitalia");
}

bool anatomyComponent::hasBreasts() const
{
    const bodyPart* breasts = getPart(bodySlot::BREASTS);
    return breasts && breasts->cupSize > 0;
}

GenderArchetype anatomyComponent::getGenderArchetype() const
{
    bool penis = hasPenis();
    bool vagina = hasVagina();
    bool breasts = hasBreasts();
    BodyPresentation pres = getVisualPresentation();

    if (penis && vagina)
    {
        return GenderArchetype::HERMAPHRODITE;
    }
    if (penis && !vagina)
    {
        if (breasts || pres == BodyPresentation::FEMININE) return GenderArchetype::GYNOMORPH;
        return GenderArchetype::MALE;
    }
    if (vagina && !penis)
    {
        if (!breasts && pres == BodyPresentation::MASCULINE) return GenderArchetype::ANDROMORPH;
        return GenderArchetype::FEMALE;
    }
    return GenderArchetype::ASEXUAL_NULL;
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

    if (hasVagina()) femininePoints += 2;
    if (hasPenis()) masculinePoints += 2;

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
    std::cout << " Archetype: " << genderArchetypeToString(getGenderArchetype()) << " | Title: " << getRacialTitle() << "\n";
    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        if (parts[i].has_value())
        {
            const auto& p = parts[i].value();
            std::cout << "[" << getSlotName(static_cast<bodySlot>(i)) << "] " << p.name << " (" << p.race << ")\n";
        }
    }
    std::cout << "=====================\n\n";
}