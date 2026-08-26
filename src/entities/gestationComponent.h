#pragma once

#include <string>
#include <vector>
#include <random>
#include <nlohmann/json.hpp>

class anatomyComponent;

struct OffspringInfo
{
    std::string motherId;
    std::string fatherId;
    std::string fatherName;
    std::string fatherRace;
    std::string motherRace;
    std::string rolledRace;
};

class gestationComponent
{
public:
    bool isPregnant = false;
    int conceptionDay = 0;
    std::string fatherId = "";
    std::string fatherName = "Unknown";
    std::string fatherRace = "Human";
    std::string motherRace = "Human";
    int gestationDaysRemaining = 0;
    int totalGestationDays = 30;
    int litterSize = 1;
    int accumulatedMinutes = 0;
    std::vector<std::string> incubatedOffspringRaces;

    bool canConceive(const anatomyComponent& anatomy) const;
    bool impregnate(const std::string& fId, const std::string& fName, const std::string& fRace, const std::string& mRace, int customLitterSize = 0);
    bool processGestation(int daysPassed);
    bool processGestationMinutes(int minutesPassed);
    std::vector<OffspringInfo> giveBirth(const std::string& motherId);

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};