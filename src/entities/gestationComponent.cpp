#include "entities/gestationComponent.h"
#include "entities/anatomyComponent.h"

#include <algorithm>
#include <random>

bool gestationComponent::canConceive(const anatomyComponent& anatomy) const
{
    if (isPregnant) return false;
    return anatomy.hasVagina();
}

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

    if (customLitterSize > 0)
    {
        litterSize = customLitterSize;
    }
    else
    {
        // 70% chance 1, 20% chance 2, 10% chance 3
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 100);
        int roll = dist(gen);
        if (roll <= 70) litterSize = 1;
        else if (roll <= 90) litterSize = 2;
        else litterSize = 3;
    }

    incubatedOffspringRaces.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> raceDist(1, 100);

    for (int i = 0; i < litterSize; ++i)
    {
        // 50/50 inheritance or hybrid
        int rRoll = raceDist(gen);
        if (fatherRace == motherRace)
        {
            incubatedOffspringRaces.push_back(fatherRace);
        }
        else if (rRoll <= 45)
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

bool gestationComponent::processGestation(int daysPassed)
{
    if (!isPregnant || daysPassed <= 0) return false;

    gestationDaysRemaining = std::max(0, gestationDaysRemaining - daysPassed);
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
    if (j.contains("incubatedOffspringRaces")) incubatedOffspringRaces = j["incubatedOffspringRaces"].get<std::vector<std::string>>();
}
