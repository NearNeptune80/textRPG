#include "questDatabase.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::unordered_map<std::string, questScene> questDatabase::registry;
std::vector<MapTrigger> questDatabase::globalTriggers;

bool questDatabase::loadDatabase(const std::string& pathStr)
{
    registry.clear();
    globalTriggers.clear();
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
                                choice.nextSceneId = cJson.value("nextSceneId", "EXIT");

                                if (cJson.contains("requirements"))
                                {
                                    for (const auto& req : cJson.at("requirements"))
                                    {
                                        gameCondition cond;
                                        cond.type = req.at("type").get<std::string>();
                                        cond.target = req.at("target").get<std::string>();
                                        cond.requiredValue = req.value("requiredValue", 0);
                                        choice.requirements.push_back(conditionNode{cond});
                                    }
                                }

                                if (cJson.contains("results"))
                                {
                                    for (const auto& res : cJson.at("results"))
                                    {
                                        gameEffect eff;
                                        eff.action = res.at("action").get<std::string>();
                                        eff.target = res.at("target").get<std::string>();
                                        eff.amount = res.value("amount", 0);
                                        choice.results.push_back(eff);
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
                        trig.sceneId = tJson.at("sceneId").get<std::string>();

                        if (tJson.contains("conditions"))
                        {
                            for (const auto& cJson : tJson.at("conditions"))
                            {
                                gameCondition cond;
                                cond.type = cJson.at("type").get<std::string>();
                                cond.target = cJson.at("target").get<std::string>();
                                cond.requiredValue = cJson.value("requiredValue", 0);
                                trig.conditions.push_back(conditionNode{cond});
                            }
                        }
                        questDatabase::globalTriggers.push_back(trig);
                    }
                }
            }
            catch (const json::exception& e)
            {
                std::cerr << "Quest JSON Parsing Error (" << filePath.string() << "): " << e.what() << "\n";
            }
        };

    if (fs::is_directory(p))
    {
        for (const auto& entry : fs::directory_iterator(p))
        {
            if (entry.path().extension() == ".json") loadSingleFile(entry.path());
        }
    }
    else if (fs::exists(p))
    {
        loadSingleFile(p);
    }
    return true;
}

bool questDatabase::exists(const std::string& id) { return registry.find(id) != registry.end(); }

questScene questDatabase::getScene(const std::string& id)
{
    if (exists(id)) return registry[id];

    questScene fallback;
    fallback.id = "error";
    fallback.speakerName = "System";
    fallback.bodyText = "Error: Scene " + id + " not found.";
    return fallback;
}

std::vector<MapTrigger> questDatabase::getTriggersForLocation(const std::string& mapId, int x, int y)
{
    std::vector<MapTrigger> matches;
    for (const auto& trig : globalTriggers)
    {
        if (trig.mapId == mapId && trig.x == x && trig.y == y) matches.push_back(trig);
    }
    return matches;
}