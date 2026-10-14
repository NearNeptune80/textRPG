#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "entities/gestationComponent.h"

class entity;

enum class OffspringGrowthStage
{
    CHILD = 0,
    ADULT_NPC = 1
};

struct OffspringRecord
{
    std::string id;
    std::string name;
    std::string motherId;
    std::string fatherId;
    std::string fatherName;
    std::string motherRace;
    std::string fatherRace;
    std::string rolledRace;
    int birthDay = 1;
    int birthMonth = 1;
    int birthYear = 1;
    int daysAlive = 0;
    OffspringGrowthStage stage = OffspringGrowthStage::CHILD;

    [[nodiscard]] bool isAdult() const { return stage == OffspringGrowthStage::ADULT_NPC; }

    nlohmann::json toJson() const;
    static OffspringRecord fromJson(const nlohmann::json& j);
};

class offspringManager
{
public:
    static void registerBirth(const OffspringInfo& info, const std::string& customName, int day, int month, int year);
    static void advanceDayAll();
    static std::vector<OffspringRecord>& getAllOffspring();
    static OffspringRecord* getOffspring(const std::string& id);
    static std::shared_ptr<entity> promoteToAdultNPC(const std::string& id);

    static nlohmann::json toJson();
    static void fromJson(const nlohmann::json& j);
    static void clear();

    static constexpr int ADULTHOOD_DAYS_THRESHOLD = 30;

private:
    static std::vector<OffspringRecord> s_records;
};

