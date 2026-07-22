#include "questDatabase.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::unordered_map<std::string, questScene> questDatabase::registry;

void from_json(const json& j, gameEffect& e)
{
    j.at("action").get_to(e.action);
    j.at("target").get_to(e.target);
    j.at("amount").get_to(e.amount);
}

void from_json(const json& j, gameCondition& c)
{
    j.at("type").get_to(c.type);
    j.at("target").get_to(c.target);
    j.at("requiredValue").get_to(c.requiredValue);
}

void from_json(const json& j, dialogueChoice& d)
{
    j.at("label").get_to(d.label);
    if (j.contains("requirements"))
    {
        for (const auto& req : j.at("requirements")) d.requirements.push_back(req.get<gameCondition>());
    }
    if (j.contains("results"))
    {
        for (const auto& res : j.at("results")) d.results.push_back(res.get<gameEffect>());
    }
    j.at("nextSceneId").get_to(d.nextSceneId);
}

void from_json(const json& j, questScene& q)
{
    j.at("id").get_to(q.id);
    j.at("speakerName").get_to(q.speakerName);
    j.at("bodyText").get_to(q.bodyText);
    for (const auto& choice : j.at("choices"))
    {
        q.choices.push_back(choice.get<dialogueChoice>());
    }
}

bool questDatabase::loadDatabase(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json data;
        file >> data;
        registry.clear();
        for (const auto& sceneJson : data.at("scenes"))
        {
            questScene newScene = sceneJson.get<questScene>();
            registry[newScene.id] = newScene;
        }
        std::cout << "Loaded " << registry.size() << " scenes from database.\n";
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Quest JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool questDatabase::exists(const std::string& id)
{
    return registry.find(id) != registry.end();
}

questScene questDatabase::getScene(const std::string& id)
{
    if (exists(id)) return registry[id];

    questScene fallback;
    fallback.id = "error";
    fallback.speakerName = "System";
    fallback.bodyText = "Error: Scene " + id + " not found.";
    return fallback;
}