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

std::string saveManager::getSavesDirectory()
{
    std::string dir = "data/saves";
    if (!fs::exists(dir))
    {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
        {
            dir = "saves";
            fs::create_directories(dir, ec);
        }
    }
    return dir;
}

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

        meta.saveVersion = j.value("saveVersion", 1);

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
    std::string savesDir = getSavesDirectory();

    for (const auto& entry : fs::directory_iterator(savesDir))
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
    j["saveVersion"] = CURRENT_SAVE_VERSION;

    std::time_t now = std::time(nullptr);
    char timeBuffer[30];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", std::localtime(&now));

    entity* p = g->getPlayer();
    std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
    std::string currentQuest = "None";

    j["metadata"] = {
        {"saveVersion", CURRENT_SAVE_VERSION},
        {"saveName", customSaveName},
        {"characterName", charName},
        {"characterLevel", p ? p->stats.level : 1},
        {"activeQuest", currentQuest},
        {"mapLocation", g->map ? g->map->getId() : "Unknown"},
        {"timestamp", std::string(timeBuffer)},
        {"isAutosave", customSaveName.rfind("Autosave", 0) == 0}
    };

    j["currentMap"] = g->map ? g->map->getId() : "overworld";
    j["playerX"] = g->gridX;
    j["playerY"] = g->gridY;

    j["time"] = {
        {"minute", g->gameTime.minute},
        {"hour", g->gameTime.hour},
        {"day", g->gameTime.day},
        {"month", g->gameTime.month},
        {"year", g->gameTime.year},
        {"dayOfWeek", g->gameTime.dayOfWeek}
    };

    if (p) j["player"] = p->toJson();
    if (g->map) j["map"] = g->map->saveStateToJson();
    j["settings"] = g->settings.toJson();

    return j;
}

/**
 * Atomically writes JSON content to disk using a temporary file and rename to prevent corruption on crash.
 */
bool saveManager::writeAtomicJson(const std::string& targetPath, const json& payload)
{
    std::string tmpPath = targetPath + ".tmp";
    std::error_code ec;

    {
        std::ofstream file(tmpPath);
        if (!file.is_open()) return false;
        file << payload.dump(4);
    }

    fs::rename(tmpPath, targetPath, ec);
    if (ec)
    {
        fs::copy_file(tmpPath, targetPath, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmpPath, ec);
    }

    return !ec;
}

/**
 * Writes a user-named manual save file.
 */
bool saveManager::saveNamedGame(game* g, const std::string& customSaveName)
{
    std::string savesDir = getSavesDirectory();
    entity* p = g ? g->getPlayer() : nullptr;
    std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
    std::string fileName = savesDir + "/" + sanitizeFilename(charName) + "_" + sanitizeFilename(customSaveName) + ".json";

    json payload = buildPayload(g, customSaveName);
    return writeAtomicJson(fileName, payload);
}

/**
 * Rolls rotating auto-save slots (Autosave_1 becomes newest, oldest evicted).
 */
bool saveManager::saveAutosave(game* g, int maxAutosaves)
{
    if (!g || !g->getPlayer()) return false;
    std::string savesDir = getSavesDirectory();
    std::string charName = sanitizeFilename(!g->getPlayer()->name.empty() ? g->getPlayer()->name : "Hero");

    for (int i = maxAutosaves; i >= 1; --i)
    {
        std::string currentPath = savesDir + "/" + charName + "_Autosave_" + std::to_string(i) + ".json";

        if (i == maxAutosaves)
        {
            if (fs::exists(currentPath)) fs::remove(currentPath);
        }
        else
        {
            std::string nextPath = savesDir + "/" + charName + "_Autosave_" + std::to_string(i + 1) + ".json";
            if (fs::exists(currentPath))
            {
                std::error_code ec;
                fs::rename(currentPath, nextPath, ec);
            }
        }
    }

    std::string newestPath = savesDir + "/" + charName + "_Autosave_1.json";
    json payload = buildPayload(g, "Autosave 1");
    return writeAtomicJson(newestPath, payload);
}

bool saveManager::exists(game* g, const std::string& customSaveName)
{
    std::string savesDir = getSavesDirectory();
    entity* p = g ? g->getPlayer() : nullptr;
    std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
    std::string fileName = savesDir + "/" + sanitizeFilename(charName) + "_" + sanitizeFilename(customSaveName) + ".json";
    return fs::exists(fileName);
}

/**
 * Loads world state, player attributes, quests, and discovery maps from a JSON save file.
 */
bool saveManager::loadFromFile(game* g, const std::string& fileName)
{
    if (!g) return false;

    std::string savesDir = getSavesDirectory();
    std::string path = fileName;
    if (!fs::exists(path))
    {
        path = savesDir + "/" + fileName;
    }

    std::ifstream file(path);
    if (!file.is_open()) return false;

    try
    {
        json j;
        file >> j;

        int version = j.value("saveVersion", 1);
        if (version > CURRENT_SAVE_VERSION)
        {
            std::cerr << "[SaveManager] Warning: Save file version (" << version
                      << ") is newer than engine version (" << CURRENT_SAVE_VERSION << ").\n";
        }

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
            if (!g->playerEntity)
            {
                g->playerEntity = std::make_shared<entity>("player_main", "Hero");
            }
            g->getPlayer()->fromJson(j["player"]);
        }

        if (j.contains("currentMap"))
        {
            std::string mapId = j.value("currentMap", "overworld");
            int pX = j.value("playerX", 1);
            int pY = j.value("playerY", 1);
            g->loadMap(mapId, pX, pY);
        }

        if (j.contains("map") && g->map)
        {
            g->map->loadStateFromJson(j["map"]);
        }

        if (j.contains("settings"))
        {
            g->settings.fromJson(j["settings"]);
        }

        g->refreshActionGrid();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SaveManager] Error loading save file " << path << ": " << e.what() << "\n";
        return false;
    }
}