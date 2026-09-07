#include "entities/namedCharacter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "core/game.h"
#include "entities/questComponent.h"
#include "items/itemDatabase.h"
#include "map/gameMap.h"
#include "quest/questDatabase.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

std::unordered_map<std::string, std::shared_ptr<NamedCharacter>> NamedCharacterManager::registry;

bool NamedCharacter::resolveLocation(int hour, int dayOfWeek, const questComponent* quests, const game* gameCtx)
{
    // 1. Evaluate Quest-Driven NPC Relocations from questDatabase first
    if (quests)
    {
        auto questRelocs = questDatabase::getRelocationsForNPC(id);
        for (const auto& qReloc : questRelocs)
        {
            if (quests->hasQuest(qReloc.questId))
            {
                int currentStage = quests->getQuestStage(qReloc.questId);
                if (currentStage >= qReloc.minStage && currentStage <= qReloc.maxStage)
                {
                    bool conditionsMet = true;
                    if (gameCtx)
                    {
                        for (const auto& condNode : qReloc.conditions)
                        {
                            if (!gameCtx->checkSingleCondition(condNode.condition))
                            {
                                conditionsMet = false;
                                break;
                            }
                        }
                    }

                    if (conditionsMet)
                    {
                        currentMapId = qReloc.mapId;
                        currentX = qReloc.x;
                        currentY = qReloc.y;
                        currentActivity = qReloc.activity;
                        activeSceneId = !qReloc.overrideSceneId.empty() ? qReloc.overrideSceneId : defaultSceneId;
                        return true;
                    }
                }
            }
        }

        // 1b. Character-specific questOverrides (if any)
        for (const auto& qOverride : questOverrides)
        {
            if (quests->hasQuest(qOverride.questId))
            {
                int currentStage = quests->getQuestStage(qOverride.questId);
                if (currentStage >= qOverride.minStage && currentStage <= qOverride.maxStage)
                {
                    bool conditionsMet = true;
                    if (gameCtx)
                    {
                        for (const auto& cond : qOverride.conditions)
                        {
                            if (!gameCtx->checkSingleCondition(cond))
                            {
                                conditionsMet = false;
                                break;
                            }
                        }
                    }

                    if (conditionsMet)
                    {
                        currentMapId = qOverride.mapId;
                        currentX = qOverride.x;
                        currentY = qOverride.y;
                        currentActivity = qOverride.activity;
                        activeSceneId = !qOverride.overrideSceneId.empty() ? qOverride.overrideSceneId : defaultSceneId;
                        return true;
                    }
                }
            }
        }
    }

    // 2. Evaluate Hourly Daily Schedule
    for (const auto& sched : schedules)
    {
        if (!sched.daysOfWeek.empty())
        {
            if (std::find(sched.daysOfWeek.begin(), sched.daysOfWeek.end(), dayOfWeek) == sched.daysOfWeek.end())
            {
                continue;
            }
        }

        bool inWindow = false;
        if (sched.startHour <= sched.endHour)
        {
            inWindow = (hour >= sched.startHour && hour < sched.endHour);
        }
        else
        {
            // Overnight window (e.g. 22:00 to 08:00)
            inWindow = (hour >= sched.startHour || hour < sched.endHour);
        }

        if (inWindow)
        {
            currentMapId = sched.mapId;
            currentX = sched.x;
            currentY = sched.y;
            currentActivity = sched.activity;
            activeSceneId = defaultSceneId;
            return true;
        }
    }

    // Fallback to first schedule entry if defined
    if (!schedules.empty())
    {
        currentMapId = schedules.front().mapId;
        currentX = schedules.front().x;
        currentY = schedules.front().y;
        currentActivity = schedules.front().activity;
        activeSceneId = defaultSceneId;
        return true;
    }

    return false;
}

json NamedCharacter::toJson() const
{
    json j;
    j["id"] = id;
    j["currentMapId"] = currentMapId;
    j["currentX"] = currentX;
    j["currentY"] = currentY;
    j["currentActivity"] = currentActivity;
    j["activeSceneId"] = activeSceneId;
    if (characterEntity)
    {
        j["entity"] = characterEntity->toJson();
    }
    return j;
}

void NamedCharacter::fromJson(const json& j)
{
    if (j.contains("currentMapId")) currentMapId = j["currentMapId"].get<std::string>();
    if (j.contains("currentX")) currentX = j["currentX"].get<int>();
    if (j.contains("currentY")) currentY = j["currentY"].get<int>();
    if (j.contains("currentActivity")) currentActivity = j["currentActivity"].get<std::string>();
    if (j.contains("activeSceneId")) activeSceneId = j["activeSceneId"].get<std::string>();
    if (characterEntity && j.contains("entity"))
    {
        characterEntity->fromJson(j["entity"]);
    }
}

bool NamedCharacterManager::loadFromDirectory(const std::string& dirPath)
{
    registry.clear();
    fs::path p(dirPath);
    if (!fs::exists(p))
    {
        std::cerr << "[NamedCharacterManager] Path does not exist: " << dirPath << "\n";
        return false;
    }

    auto parseCharacter = [](const json& cJson) -> std::shared_ptr<NamedCharacter>
    {
        auto nc = std::make_shared<NamedCharacter>();
        nc->id = cJson.at("id").get<std::string>();
        nc->name = cJson.value("name", "Unknown");
        nc->title = cJson.value("title", "");
        nc->defaultSceneId = cJson.value("dialogueSceneId", "");
        nc->isMerchant = cJson.value("isMerchant", false);
        nc->buyMarkup = cJson.value("buyMarkup", 1.0f);
        nc->sellMarkdown = cJson.value("sellMarkdown", 0.5f);

        auto ent = std::make_shared<entity>(nc->id, nc->name);
        ent->stats.level = cJson.value("level", 1);
        ent->stats.setBaseStat("currency", static_cast<float>(cJson.value("currency", 0)));
        ent->buyMarkup = nc->buyMarkup;
        ent->sellMarkdown = nc->sellMarkdown;

        if (cJson.contains("baseStats") && cJson["baseStats"].is_object())
        {
            for (auto& [sKey, sVal] : cJson["baseStats"].items())
            {
                ent->stats.setBaseStat(sKey, sVal.get<float>());
            }
        }

        if (cJson.contains("inventory") && cJson["inventory"].is_array())
        {
            for (const auto& itemEntry : cJson["inventory"])
            {
                std::string itemId = itemEntry.value("id", "");
                int count = itemEntry.value("count", 1);
                auto itm = itemDatabase::getItem(itemId);
                if (itm)
                {
                    itm->count = count;
                    ent->inventory.addItem(itm);
                }
            }
        }

        std::string genderStr = cJson.value("gender", "");
        if (!genderStr.empty())
        {
            ent->genderArchetype = stringToGenderArchetype(genderStr);
        }

        std::string raceStr = cJson.value("race", "");
        if (!raceStr.empty())
        {
            bodyPart torso;
            torso.id = "part_torso_" + raceStr;
            torso.name = "Torso";
            torso.race = raceStr;
            ent->anatomy.setPart(bodySlot::TORSO, torso);
        }

        if (cJson.contains("unlockedPerks") && cJson["unlockedPerks"].is_array())
        {
            for (const auto& perkEntry : cJson["unlockedPerks"])
            {
                if (perkEntry.is_string()) ent->unlockPerk(perkEntry.get<std::string>());
            }
        }
        else if (cJson.contains("perks") && cJson["perks"].is_array())
        {
            for (const auto& perkEntry : cJson["perks"])
            {
                if (perkEntry.is_string()) ent->unlockPerk(perkEntry.get<std::string>());
            }
        }

        nc->characterEntity = ent;

        // Schedules
        if (cJson.contains("schedules") && cJson["schedules"].is_array())
        {
            for (const auto& sJson : cJson["schedules"])
            {
                ScheduleEntry s;
                s.startHour = sJson.value("startHour", 0);
                s.endHour = sJson.value("endHour", 24);
                s.mapId = sJson.value("mapId", "overworld");
                s.x = sJson.value("x", 0);
                s.y = sJson.value("y", 0);
                s.activity = sJson.value("activity", "");
                if (sJson.contains("daysOfWeek") && sJson["daysOfWeek"].is_array())
                {
                    s.daysOfWeek = sJson["daysOfWeek"].get<std::vector<int>>();
                }
                nc->schedules.push_back(s);
            }
        }

        // Quest Overrides
        if (cJson.contains("questOverrides") && cJson["questOverrides"].is_array())
        {
            for (const auto& qJson : cJson["questOverrides"])
            {
                QuestLocationOverride qo;
                qo.questId = qJson.value("questId", "");
                qo.minStage = qJson.value("minStage", 0);
                qo.maxStage = qJson.value("maxStage", 999);
                qo.mapId = qJson.value("mapId", "overworld");
                qo.x = qJson.value("x", 0);
                qo.y = qJson.value("y", 0);
                qo.activity = qJson.value("activity", "");
                qo.overrideSceneId = qJson.value("overrideSceneId", "");
                nc->questOverrides.push_back(qo);
            }
        }

        nc->resolveLocation(0, 0, nullptr, nullptr);
        return nc;
    };

    auto loadSingleFile = [&](const fs::path& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        try
        {
            json data;
            file >> data;

            if (data.is_object() && data.contains("id"))
            {
                auto nc = parseCharacter(data);
                registry[nc->id] = nc;
            }
            else if (data.contains("characters") && data["characters"].is_array())
            {
                for (const auto& cJson : data["characters"])
                {
                    auto nc = parseCharacter(cJson);
                    registry[nc->id] = nc;
                }
            }
        }
        catch (const json::exception& e)
        {
            std::cerr << "[NamedCharacterManager] JSON parsing error (" << filePath.string() << "): " << e.what() << "\n";
        }
    };

    if (fs::is_directory(p))
    {
        for (const auto& entry : fs::recursive_directory_iterator(p))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                loadSingleFile(entry.path());
            }
        }
    }
    else if (fs::is_regular_file(p))
    {
        loadSingleFile(p);
    }

    return true;
}

std::shared_ptr<NamedCharacter> NamedCharacterManager::getCharacter(const std::string& id)
{
    auto it = registry.find(id);
    if (it != registry.end()) return it->second;
    return nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<NamedCharacter>>& NamedCharacterManager::getAllCharacters()
{
    return registry;
}

void NamedCharacterManager::updateAllLocations(game* g)
{
    if (!g) return;

    int hour = g->gameTime.hour;
    int dayOfWeek = g->gameTime.dayOfWeek;
    const questComponent* quests = g->Player ? &g->Player->quests : nullptr;

    for (auto& [id, nc] : registry)
    {
        std::string oldMap = nc->currentMapId;
        int oldX = nc->currentX;
        int oldY = nc->currentY;

        nc->resolveLocation(hour, dayOfWeek, quests, g);

        if (oldMap != nc->currentMapId || oldX != nc->currentX || oldY != nc->currentY)
        {
            // Remove from old map if loaded
            if (g->map && g->map->getId() == oldMap)
            {
                auto& oldTile = g->map->getRuntimeData(oldX, oldY);
                std::erase(oldTile.namedNPCs, nc->characterEntity);
                if (oldTile.persistentNPC == nc->characterEntity)
                {
                    oldTile.persistentNPC = oldTile.namedNPCs.empty() ? nullptr : oldTile.namedNPCs.front();
                }
            }
            else if (g->mapCache.contains(oldMap))
            {
                auto& oldTile = g->mapCache[oldMap].getRuntimeData(oldX, oldY);
                std::erase(oldTile.namedNPCs, nc->characterEntity);
                if (oldTile.persistentNPC == nc->characterEntity)
                {
                    oldTile.persistentNPC = oldTile.namedNPCs.empty() ? nullptr : oldTile.namedNPCs.front();
                }
            }

            // Add to new map if loaded
            if (g->map && g->map->getId() == nc->currentMapId)
            {
                auto& newTile = g->map->getRuntimeData(nc->currentX, nc->currentY);
                if (std::find(newTile.namedNPCs.begin(), newTile.namedNPCs.end(), nc->characterEntity) == newTile.namedNPCs.end())
                {
                    newTile.namedNPCs.push_back(nc->characterEntity);
                }
                newTile.persistentNPC = nc->characterEntity;
            }
            else if (g->mapCache.contains(nc->currentMapId))
            {
                auto& newTile = g->mapCache[nc->currentMapId].getRuntimeData(nc->currentX, nc->currentY);
                if (std::find(newTile.namedNPCs.begin(), newTile.namedNPCs.end(), nc->characterEntity) == newTile.namedNPCs.end())
                {
                    newTile.namedNPCs.push_back(nc->characterEntity);
                }
                newTile.persistentNPC = nc->characterEntity;
            }
        }
    }
}

void NamedCharacterManager::syncMapCharacters(gameMap* targetMap)
{
    if (!targetMap) return;

    std::string mId = targetMap->getId();
    for (auto& [id, nc] : registry)
    {
        if (nc->currentMapId == mId)
        {
            auto& tile = targetMap->getRuntimeData(nc->currentX, nc->currentY);
            if (std::find(tile.namedNPCs.begin(), tile.namedNPCs.end(), nc->characterEntity) == tile.namedNPCs.end())
            {
                tile.namedNPCs.push_back(nc->characterEntity);
            }
            tile.persistentNPC = nc->characterEntity;
        }
    }
}

json NamedCharacterManager::saveStateToJson()
{
    json j = json::object();
    for (const auto& [id, nc] : registry)
    {
        j[id] = nc->toJson();
    }
    return j;
}

void NamedCharacterManager::loadStateFromJson(const json& j)
{
    if (!j.is_object()) return;
    for (auto& [id, cJson] : j.items())
    {
        auto it = registry.find(id);
        if (it != registry.end())
        {
            it->second->fromJson(cJson);
        }
    }
}

void NamedCharacterManager::clear()
{
    registry.clear();
}
