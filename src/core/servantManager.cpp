#include "core/servantManager.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "common/randomEngine.h"
#include "core/game.h"
#include "entities/entity.h"
#include "items/itemDatabase.h"

using json = nlohmann::json;

std::unordered_map<std::string, ServantJobDefinition> servantManager::s_jobs;
std::vector<std::shared_ptr<entity>> servantManager::s_servants;

float servantManager::baseStaminaRegenPerHour = 15.0f;
float servantManager::exhaustionMoralePenalty = -5.0f;
float servantManager::exhaustionAffectionPenalty = -3.0f;
float servantManager::exhaustionObediencePenalty = -2.0f;

bool servantManager::loadJobsFromFile(const std::string& path)
{
    s_jobs.clear();
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[servantManager] Could not open jobs config: " << path << "\n";
        return false;
    }

    try
    {
        json j;
        file >> j;

        if (j.contains("restConfig") && j["restConfig"].is_object())
        {
            const auto& rCfg = j["restConfig"];
            baseStaminaRegenPerHour = rCfg.value("baseStaminaRegenPerHour", 15.0f);
            exhaustionMoralePenalty = rCfg.value("exhaustionMoralePenalty", -5.0f);
            exhaustionAffectionPenalty = rCfg.value("exhaustionAffectionPenalty", -3.0f);
            exhaustionObediencePenalty = rCfg.value("exhaustionObediencePenalty", -2.0f);
        }

        if (j.contains("jobs") && j["jobs"].is_array())
        {
            for (const auto& jobJson : j["jobs"])
            {
                ServantJobDefinition def;
                def.id = jobJson.value("id", "");
                def.name = jobJson.value("name", def.id);
                def.description = jobJson.value("description", "");
                def.staminaCostPerHour = jobJson.value("staminaCostPerHour", 10.0f);
                def.staminaRegenPerHour = jobJson.value("staminaRegenPerHour", 0.0f);
                def.obedienceDelta = jobJson.value("obedienceDelta", 0.0f);
                def.moraleDelta = jobJson.value("moraleDelta", 0.0f);
                def.affectionDelta = jobJson.value("affectionDelta", 0.0f);
                def.goldYieldPerHour = jobJson.value("goldYieldPerHour", 0.0f);
                def.itemYieldId = jobJson.value("itemYieldId", "");
                def.itemYieldChance = jobJson.value("itemYieldChance", 0.0f);
                def.requiredRoomType = jobJson.value("requiredRoomType", "");
                def.outOfWorkFlag = jobJson.value("outOfWorkFlag", "");

                if (jobJson.contains("requiredTags") && jobJson["requiredTags"].is_array())
                {
                    def.requiredTags = jobJson["requiredTags"].get<std::vector<std::string>>();
                }

                if (!def.id.empty())
                {
                    s_jobs[def.id] = def;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[servantManager] Parsing error in " << path << ": " << e.what() << "\n";
        return false;
    }
}

const ServantJobDefinition* servantManager::getJobDefinition(const std::string& jobId)
{
    auto it = s_jobs.find(jobId);
    if (it != s_jobs.end()) return &it->second;
    return nullptr;
}

const std::unordered_map<std::string, ServantJobDefinition>& servantManager::getAllJobs()
{
    return s_jobs;
}

void servantManager::registerServant(std::shared_ptr<entity> ent)
{
    if (!ent) return;
    ent->isServant = true;
    if (ent->servant.servantId.empty())
    {
        ent->servant.servantId = ent->id;
    }

    for (const auto& existing : s_servants)
    {
        if (existing && existing->id == ent->id) return; // already registered
    }
    s_servants.push_back(ent);
}

void servantManager::removeServant(const std::string& id)
{
    for (auto it = s_servants.begin(); it != s_servants.end(); ++it)
    {
        if (*it && (*it)->id == id)
        {
            (*it)->isServant = false;
            s_servants.erase(it);
            return;
        }
    }
}

std::vector<std::shared_ptr<entity>> servantManager::getAllServants()
{
    return s_servants;
}

std::shared_ptr<entity> servantManager::getServant(const std::string& id)
{
    for (const auto& ent : s_servants)
    {
        if (ent && ent->id == id) return ent;
    }
    return nullptr;
}

bool servantManager::hasServant(const std::string& id)
{
    return getServant(id) != nullptr;
}

size_t servantManager::getServantCount()
{
    return s_servants.size();
}

void servantManager::clear()
{
    s_servants.clear();
}

void servantManager::tickHourlyServants(game* g, int currentHour)
{
    if (s_jobs.empty())
    {
        loadJobsFromFile();
    }

    for (auto& s : s_servants)
    {
        if (!s) continue;

        std::string jobId = s->servant.getJobForHour(currentHour);
        const ServantJobDefinition* jobDef = getJobDefinition(jobId);

        if (jobId == "rest" || jobId.empty())
        {
            float regen = (jobDef && jobDef->staminaRegenPerHour > 0.0f) ? jobDef->staminaRegenPerHour : baseStaminaRegenPerHour;
            s->servant.stats.stamina += regen;
            s->servant.stats.morale += (jobDef ? jobDef->moraleDelta : 0.5f);
            s->servant.stats.affection += (jobDef ? jobDef->affectionDelta : 0.2f);
            s->servant.stats.clamp();
        }
        else
        {
            if (s->servant.stats.stamina <= 0.0f)
            {
                s->servant.stats.isExhausted = true;
                s->servant.stats.morale += exhaustionMoralePenalty;
                s->servant.stats.affection += exhaustionAffectionPenalty;
                s->servant.stats.obedience += exhaustionObediencePenalty;
                s->servant.stats.clamp();

                if (g)
                {
                    g->addLogEntry("[EXHAUSTION]", std::format("{} collapsed from exhaustion while assigned to {}! Yields halted.", s->name, jobDef ? jobDef->name : jobId), { 240, 80, 80, 255 });
                }
            }
            else
            {
                float cost = jobDef ? jobDef->staminaCostPerHour : 10.0f;
                s->servant.stats.stamina -= cost;

                if (jobDef)
                {
                    s->servant.stats.morale += jobDef->moraleDelta;
                    s->servant.stats.obedience += jobDef->obedienceDelta;
                    s->servant.stats.affection += jobDef->affectionDelta;
                }
                s->servant.stats.clamp();

                if (g && jobDef)
                {
                    if (jobDef->goldYieldPerHour > 0.0f && g->getPlayer())
                    {
                        g->getPlayer()->stats.modifyBaseStat("currency", jobDef->goldYieldPerHour);
                        g->addLogEntry("[SERVANT]", std::format("{} produced {:.0f}¤ from {}.", s->name, jobDef->goldYieldPerHour, jobDef->name), { 220, 200, 100, 255 });
                    }

                    if (!jobDef->itemYieldId.empty() && jobDef->itemYieldChance > 0.0f)
                    {
                        int roll = dice::rollInt(1, 100);
                        if (roll <= static_cast<int>(jobDef->itemYieldChance * 100.0f))
                        {
                            auto itm = itemDatabase::getItem(jobDef->itemYieldId);
                            if (itm && g->getPlayer())
                            {
                                g->getPlayer()->inventory.addItem(itm);
                                g->addLogEntry("[SERVANT]", std::format("{} gathered 1x {} from {}.", s->name, itm->name, jobDef->name), { 140, 220, 140, 255 });
                            }
                        }
                    }

                    if (!jobDef->outOfWorkFlag.empty())
                    {
                        s->quests.setQuestStage(jobDef->outOfWorkFlag, 1);
                    }
                }
            }
        }
    }
}

nlohmann::json servantManager::saveStateToJson()
{
    json j = json::array();
    for (const auto& s : s_servants)
    {
        if (s)
        {
            j.push_back(s->toJson());
        }
    }
    return j;
}

void servantManager::loadStateFromJson(const nlohmann::json& j)
{
    clear();
    if (!j.is_array()) return;

    for (const auto& sJson : j)
    {
        auto ent = std::make_shared<entity>("servant_loaded", "Servant");
        ent->fromJson(sJson);
        registerServant(ent);
    }
}
