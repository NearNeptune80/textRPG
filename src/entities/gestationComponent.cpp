#include "entities/gestationComponent.h"
#include "entities/anatomyComponent.h"

#include <algorithm>

#include "common/randomEngine.h"

/**
 * Checks whether the host anatomy is physically capable of conception.
 */
bool gestationComponent::canConceive(const anatomyComponent& anatomy) const
{
    if (isPregnant) return false;
    return anatomy.hasVagina();
}

/**
 * Impregnates the host entity, rolling litter size and genetic inheritance per offspring.
 */
bool gestationComponent::impregnate(const std::string& fId, const std::string& fName, const std::string& fRace, const std::string& mRace, int customLitterSize)
{
    if (isPregnant) return false;

    isPregnant = true;
    fatherId = fId;
    fatherName = fName;
    fatherRace = fRace.empty() ? "Human" : fRace;
    motherRace = mRace.empty() ? "Human" : mRace;

    totalGestationDays = 30;
    gestationDaysRemaining = totalGestationDays;
    accumulatedMinutes = 0;

    if (customLitterSize > 0)
    {
        litterSize = customLitterSize;
    }
    else
    {
        // Demographic litter probability: 70% single, 20% twins, 10% triplets
        int roll = dice::rollInt(1, 100);
        if (roll <= 70) litterSize = 1;
        else if (roll <= 90) litterSize = 2;
        else litterSize = 3;
    }

    incubatedOffspringRaces.clear();

    for (int i = 0; i < litterSize; ++i)
    {
        // 45% paternal race, 45% maternal race, 10% hybrid chimera
        int rRoll = dice::rollInt(1, 100);
        if (fatherRace == motherRace || rRoll <= 45)
        {
            incubatedOffspringRaces.push_back(fatherRace);
        }
        else if (rRoll <= 90)
        {
            incubatedOffspringRaces.push_back(motherRace);
        }
        else
        {
            incubatedOffspringRaces.push_back(fatherRace + "-" + motherRace + " Hybrid");
        }
    }

    return true;
}

/**
 * Advances gestation days and returns true if labor is due.
 */
bool gestationComponent::processGestation(int daysPassed)
{
    if (!isPregnant || daysPassed <= 0) return false;

    gestationDaysRemaining = std::max(0, gestationDaysRemaining - daysPassed);
    return (gestationDaysRemaining <= 0);
}

bool gestationComponent::processGestationMinutes(int minutesPassed)
{
    if (!isPregnant || minutesPassed <= 0) return false;

    accumulatedMinutes += minutesPassed;
    if (accumulatedMinutes >= 1440)
    {
        int days = accumulatedMinutes / 1440;
        accumulatedMinutes %= 1440;
        return processGestation(days);
    }
    return (gestationDaysRemaining <= 0);
}

std::vector<OffspringInfo> gestationComponent::giveBirth(const std::string& motherId)
{
    if (!isPregnant) return {};

    std::vector<OffspringInfo> births;
    for (const auto& race : incubatedOffspringRaces)
    {
        OffspringInfo info;
        info.motherId = motherId;
        info.fatherId = fatherId;
        info.fatherName = fatherName;
        info.fatherRace = fatherRace;
        info.motherRace = motherRace;
        info.rolledRace = race;
        births.push_back(info);
    }

    // Reset gestation state
    isPregnant = false;
    fatherId = "";
    fatherName = "";
    gestationDaysRemaining = 0;
    accumulatedMinutes = 0;
    litterSize = 0;
    incubatedOffspringRaces.clear();

    return births;
}

nlohmann::json gestationComponent::toJson() const
{
    return nlohmann::json{
        {"isPregnant", isPregnant},
        {"conceptionDay", conceptionDay},
        {"fatherId", fatherId},
        {"fatherName", fatherName},
        {"fatherRace", fatherRace},
        {"motherRace", motherRace},
        {"gestationDaysRemaining", gestationDaysRemaining},
        {"totalGestationDays", totalGestationDays},
        {"litterSize", litterSize},
        {"accumulatedMinutes", accumulatedMinutes},
        {"incubatedOffspringRaces", incubatedOffspringRaces}
    };
}

void gestationComponent::fromJson(const nlohmann::json& j)
{
    if (j.contains("isPregnant")) isPregnant = j["isPregnant"].get<bool>();
    if (j.contains("conceptionDay")) conceptionDay = j["conceptionDay"].get<int>();
    if (j.contains("fatherId")) fatherId = j["fatherId"].get<std::string>();
    if (j.contains("fatherName")) fatherName = j["fatherName"].get<std::string>();
    if (j.contains("fatherRace")) fatherRace = j["fatherRace"].get<std::string>();
    if (j.contains("motherRace")) motherRace = j["motherRace"].get<std::string>();
    if (j.contains("gestationDaysRemaining")) gestationDaysRemaining = j["gestationDaysRemaining"].get<int>();
    if (j.contains("totalGestationDays")) totalGestationDays = j["totalGestationDays"].get<int>();
    if (j.contains("litterSize")) litterSize = j["litterSize"].get<int>();
    if (j.contains("accumulatedMinutes")) accumulatedMinutes = j["accumulatedMinutes"].get<int>();
    if (j.contains("incubatedOffspringRaces")) incubatedOffspringRaces = j["incubatedOffspringRaces"].get<std::vector<std::string>>();
}