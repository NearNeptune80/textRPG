#include "quest/questDatabase.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::unordered_map<std::string, questScene> questDatabase::registry;
std::vector<MapTrigger> questDatabase::globalTriggers;
std::unordered_map<std::string, QuestDefinition> questDatabase::quests;

static conditionNode parseConditionNode(const json& j)
{
    conditionNode node;
    if (j.contains("op"))
    {
        std::string opStr = j.at("op").get<std::string>();
        if (opStr == "AND") node.op = conditionOperator::AND;
        else if (opStr == "OR") node.op = conditionOperator::OR;
        else if (opStr == "NOT") node.op = conditionOperator::NOT;
        else node.op = conditionOperator::LEAF;

        if (j.contains("children"))
        {
            for (const auto& childJson : j.at("children"))
            {
                node.children.push_back(parseConditionNode(childJson));
            }
        }
    }
    else
    {
        node.op = conditionOperator::LEAF;
        node.condition.type = j.value("type", "");
        node.condition.target = j.value("target", "");
        node.condition.requiredValue = j.value("requiredValue", 0);
        node.condition.floatValue = j.value("floatValue", 0.0f);
        node.condition.stringValue = j.value("stringValue", "");
        node.condition.minValue = j.value("minValue", 0);
        node.condition.maxValue = j.value("maxValue", 0);
    }
    return node;
}

static gameEffect parseGameEffect(const json& res)
{
    gameEffect eff;
    eff.action = res.at("action").get<std::string>();
    eff.target = res.value("target", "");
    eff.amount = res.value("amount", 0);
    eff.x = res.value("x", 0);
    eff.y = res.value("y", 0);
    eff.floatAmount = res.value("floatAmount", 0.0f);
    eff.secondaryTarget = res.value("secondaryTarget", "");
    eff.stringVal = res.value("stringVal", "");
    eff.extraString = res.value("extraString", "");
    if (res.contains("weights")) eff.weights = res["weights"].get<std::vector<int>>();
    if (res.contains("branches")) eff.branches = res["branches"].get<std::vector<std::string>>();
    return eff;
}

bool questDatabase::loadDatabase(const std::string& pathStr)
{
    registry.clear();
    globalTriggers.clear();
    quests.clear();
    fs::path p(pathStr);

    auto loadSingleFile = [](const fs::path& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        try
        {
            json data;
            file >> data;

            if (data.contains("scenes"))
            {
                for (const auto& sJson : data.at("scenes"))
                {
                    questScene scene;
                    scene.id = sJson.at("id").get<std::string>();
                    scene.speakerName = sJson.value("speakerName", "Unknown");
                    scene.bodyText = sJson.value("bodyText", "");

                    if (sJson.contains("choices"))
                    {
                        for (const auto& cJson : sJson.at("choices"))
                        {
                            dialogueChoice choice;
                            choice.label = cJson.value("label", "Continue");
                            choice.tooltip = cJson.value("tooltip", "");
                            choice.nextSceneId = cJson.value("nextSceneId", "EXIT");

                            if (cJson.contains("requirements"))
                            {
                                for (const auto& req : cJson.at("requirements"))
                                {
                                    choice.requirements.push_back(parseConditionNode(req));
                                }
                            }

                            if (cJson.contains("results"))
                            {
                                for (const auto& res : cJson.at("results"))
                                {
                                    choice.results.push_back(parseGameEffect(res));
                                }
                            }
                            scene.choices.push_back(choice);
                        }
                    }
                    registry[scene.id] = scene;
                }
            }

            if (data.contains("triggers"))
            {
                for (const auto& tJson : data.at("triggers"))
                {
                    MapTrigger trig;
                    trig.id = tJson.value("id", "");
                    trig.mapId = tJson.value("mapId", "");
                    trig.x = tJson.at("x").get<int>();
                    trig.y = tJson.at("y").get<int>();
                    trig.label = tJson.value("label", "Interact");
                    trig.tooltip = tJson.value("tooltip", "");
                    trig.sceneId = tJson.at("sceneId").get<std::string>();

                    if (tJson.contains("conditions"))
                    {
                        for (const auto& cJson : tJson.at("conditions"))
                        {
                            trig.conditions.push_back(parseConditionNode(cJson));
                        }
                    }
                    questDatabase::globalTriggers.push_back(trig);
                }
            }

            // Parse Quest Definition if present
            if (data.contains("id") && data.contains("name"))
            {
                QuestDefinition q;
                q.id = data["id"].get<std::string>();
                q.name = data.value("name", q.id);
                q.description = data.value("description", "");
                q.category = data.value("category", "Main Quest");
                q.giver = data.value("giver", "Unknown");
                q.location = data.value("location", "The Realm");
                q.rewardsDescription = data.value("rewards", "");

                int maxStage = 0;
                if (data.contains("stages") && data["stages"].is_object())
                {
                    for (auto& [stKey, stVal] : data["stages"].items())
                    {
                        try {
                            int stIdx = std::stoi(stKey);
                            q.stages[stIdx] = stVal.get<std::string>();
                            if (stIdx > maxStage) maxStage = stIdx;
                        } catch (...) {}
                    }
                }
                q.completionStage = data.value("completionStage", maxStage);
                quests[q.id] = q;
            }
        }
        catch (const json::exception& e)
        {
            std::cerr << "Quest JSON Parsing Error (" << filePath.string() << "): " << e.what() << "\n";
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
    else if (fs::exists(p))
    {
        loadSingleFile(p);
    }
    return true;
}

bool questDatabase::exists(const std::string& id)
{
    return registry.find(id) != registry.end();
}

/**
 * Retrieves a quest scene by ID (single hash lookup). Returns fallback error scene if missing.
 */
questScene questDatabase::getScene(const std::string& id)
{
    auto it = registry.find(id);
    if (it != registry.end()) return it->second;

    questScene fallback;
    fallback.id = "error";
    fallback.speakerName = "System";
    fallback.bodyText = "Error: Scene " + id + " not found.";
    return fallback;
}

/**
 * Queries all global quest triggers matching a specific map location.
 */
std::vector<MapTrigger> questDatabase::getTriggersForLocation(const std::string& mapId, int x, int y)
{
    std::vector<MapTrigger> matches;
    for (const auto& trig : globalTriggers)
    {
        if (trig.mapId == mapId && trig.x == x && trig.y == y) matches.push_back(trig);
    }
    return matches;
}

const QuestDefinition* questDatabase::getQuest(const std::string& id)
{
    auto it = quests.find(id);
    if (it != quests.end()) return &it->second;
    return nullptr;
}

std::vector<QuestDefinition> questDatabase::getAllQuests()
{
    std::vector<QuestDefinition> list;
    list.reserve(quests.size());
    for (const auto& [_, q] : quests)
    {
        list.push_back(q);
    }
    return list;
}