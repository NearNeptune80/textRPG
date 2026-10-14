#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

class entity;
class game;

struct ServantJobDefinition
{
    std::string id;
    std::string name;
    std::string description;
    float staminaCostPerHour = 10.0f;
    float staminaRegenPerHour = 0.0f;
    float obedienceDelta = 0.0f;
    float moraleDelta = 0.0f;
    float affectionDelta = 0.0f;
    float goldYieldPerHour = 0.0f;
    std::string itemYieldId = "";
    float itemYieldChance = 0.0f;
    std::string requiredRoomType = "";
    std::string outOfWorkFlag = "";
    std::vector<std::string> requiredTags;
};

class servantManager
{
public:
    static bool loadJobsFromFile(const std::string& path = "data/config/servant_jobs.json");
    static const ServantJobDefinition* getJobDefinition(const std::string& jobId);
    static const std::unordered_map<std::string, ServantJobDefinition>& getAllJobs();

    static void registerServant(std::shared_ptr<entity> ent);
    static void removeServant(const std::string& id);
    static std::vector<std::shared_ptr<entity>> getAllServants();
    static std::shared_ptr<entity> getServant(const std::string& id);
    static bool hasServant(const std::string& id);
    static size_t getServantCount();
    static void clear();

    static void tickHourlyServants(game* g, int currentHour);

    static nlohmann::json saveStateToJson();
    static void loadStateFromJson(const nlohmann::json& j);

    static float baseStaminaRegenPerHour;
    static float exhaustionMoralePenalty;
    static float exhaustionAffectionPenalty;
    static float exhaustionObediencePenalty;

private:
    static std::unordered_map<std::string, ServantJobDefinition> s_jobs;
    static std::vector<std::shared_ptr<entity>> s_servants;
};

