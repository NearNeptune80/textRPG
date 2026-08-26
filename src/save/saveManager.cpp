#include "save/saveManager.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "core/game.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string saveManager::sanitizeFilename(const std::string& input)
{
    std::string clean = input;
    for (char& c : clean)
    {
        if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
            c = '_';
        }
    }
    return clean;
}

SaveMetaData saveManager::readMetadata(const std::string& filePath)
{
    SaveMetaData meta;
    meta.fileName = fs::path(filePath).filename().string();
    std::ifstream file(filePath);
    if (!file.is_open()) return meta;

    try
    {
        json j;
        file >> j;
        if (j.contains("metadata"))
        {
            const auto& m = j["metadata"];
            meta.saveName = m.value("saveName", "Unnamed Save");
            meta.characterName = m.value("characterName", "Unknown");
            meta.characterLevel = m.value("characterLevel", 1);
            meta.activeQuest = m.value("activeQuest", "None");
            meta.mapLocation = m.value("mapLocation", "Unknown");
            meta.timestamp = m.value("timestamp", "");
            meta.isAutosave = m.value("isAutosave", false);
        }
    }
    catch (...) {}

    return meta;
}

std::vector<CharacterSaveGroup> saveManager::getSavesGroupedByCharacter()
{
    std::unordered_map<std::string, std::vector<SaveMetaData>> grouped;

    if (!fs::exists("saves"))
    {
        fs::create_directory("saves");
    }

    for (const auto& entry : fs::directory_iterator("saves"))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            SaveMetaData meta = readMetadata(entry.path().string());
            if (!meta.characterName.empty())
            {
                grouped[meta.characterName].push_back(meta);
            }
        }
    }

    std::vector<CharacterSaveGroup> result;
    for (auto& [charName, saveList] : grouped)
    {
        std::sort(saveList.begin(), saveList.end(), [](const SaveMetaData& a, const SaveMetaData& b) {
            return a.timestamp > b.timestamp;
        });

        result.push_back({charName, saveList});
    }

    return result;
}

json saveManager::buildPayload(game* g, const std::string& customSaveName)
{
    json j;

    std::time_t now = std::time(nullptr);
    char timeBuffer[30];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", std::localtime(&now));

    std::string charName = (g->Player && !g->Player->name.empty()) ? g->Player->name : "Hero";
    std::string currentQuest = "None";

    j["metadata"] = {
        {"saveName", customSaveName},
        {"characterName", charName},
        {"characterLevel", g->Player ? g->Player->stats.level : 1},
        {"activeQuest", currentQuest},
        {"mapLocation", g->map ? g->map->getId() : "Unknown"},
        {"timestamp", std::string(timeBuffer)},
        {"isAutosave", customSaveName.rfind("Autosave", 0) == 0}
    };

    j["time"] = {
        {"minute", g->gameTime.minute},
        {"hour", g->gameTime.hour},
        {"day", g->gameTime.day},
        {"month", g->gameTime.month},
        {"year", g->gameTime.year},
        {"dayOfWeek", g->gameTime.dayOfWeek}
    };

    if (g->Player) j["player"] = g->Player->toJson();
    if (g->map) j["map"] = g->map->saveStateToJson();

    return j;
}

bool saveManager::saveNamedGame(game* g, const std::string& customSaveName)
{
    if (!fs::exists("saves")) fs::create_directory("saves");

    std::string charName = (g->Player && !g->Player->name.empty()) ? g->Player->name : "Hero";
    std::string fileName = "saves/" + sanitizeFilename(charName) + "_" + sanitizeFilename(customSaveName) + ".json";

    std::ofstream file(fileName);
    if (!file.is_open()) return false;

    json payload = buildPayload(g, customSaveName);
    file << payload.dump(4);
    return true;
}

bool saveManager::saveAutosave(game* g, int maxAutosaves)
{
    if (!g->Player) return false;
    if (!fs::exists("saves")) fs::create_directory("saves");

    std::string charName = sanitizeFilename(!g->Player->name.empty() ? g->Player->name : "Hero");

    for (int i = maxAutosaves; i >= 1; --i)
    {
        std::string currentPath = "saves/" + charName + "_Autosave_" + std::to_string(i) + ".json";

        if (i == maxAutosaves)
        {
            if (fs::exists(currentPath)) fs::remove(currentPath);
        }
        else
        {
            std::string nextPath = "saves/" + charName + "_Autosave_" + std::to_string(i + 1) + ".json";
            if (fs::exists(currentPath))
            {
                fs::rename(currentPath, nextPath);
            }
        }
    }

    std::string newestPath = "saves/" + charName + "_Autosave_1.json";
    std::ofstream file(newestPath);
    if (!file.is_open()) return false;

    json payload = buildPayload(g, "Autosave 1");
    file << payload.dump(4);
    return true;
}

bool saveManager::exists(game* g, const std::string& customSaveName)
{
    std::string charName = (g->Player && !g->Player->name.empty()) ? g->Player->name : "Hero";
    std::string fileName = "saves/" + sanitizeFilename(charName) + "_" + sanitizeFilename(customSaveName) + ".json";
    return fs::exists(fileName);
}

bool saveManager::loadFromFile(game* g, const std::string& fileName)
{
    std::string path = "saves/" + fileName;
    std::ifstream file(path);
    if (!file.is_open()) return false;

    try
    {
        json j;
        file >> j;

        if (j.contains("time"))
        {
            const auto& t = j["time"];
            g->gameTime.minute = t.value("minute", 0);
            g->gameTime.hour = t.value("hour", 8);
            g->gameTime.day = t.value("day", 1);
            g->gameTime.month = t.value("month", 1);
            g->gameTime.year = t.value("year", 1);
            g->gameTime.dayOfWeek = t.value("dayOfWeek", 1);
        }

        if (j.contains("player"))
        {
            if (!g->Player)
            {
                g->Player = new entity("player_main", "Hero");
            }
            g->Player->fromJson(j["player"]);
        }

        if (j.contains("map") && g->map)
        {
            g->map->loadStateFromJson(j["map"]);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}