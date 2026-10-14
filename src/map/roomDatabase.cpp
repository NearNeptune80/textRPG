#include "map/roomDatabase.h"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::unordered_map<std::string, RoomDefinition> roomDatabase::s_rooms;

bool roomDatabase::loadRoomsFromFile(const std::string& path)
{
    s_rooms.clear();
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[roomDatabase] Failed to open room config: " << path << "\n";
        return false;
    }

    try
    {
        json j;
        file >> j;

        if (j.contains("rooms") && j["rooms"].is_array())
        {
            for (const auto& rJson : j["rooms"])
            {
                RoomDefinition def;
                def.id = rJson.value("id", "");
                def.name = rJson.value("name", def.id);
                def.tier = rJson.value("tier", 1);
                def.category = rJson.value("category", "General");
                def.description = rJson.value("description", "");
                def.costGold = rJson.value("costGold", 0.0f);
                def.isStorageSafe = rJson.value("isStorageSafe", true);
                def.stationType = rJson.value("stationType", "");

                if (rJson.contains("tags") && rJson["tags"].is_array())
                {
                    def.tags = rJson["tags"].get<std::vector<std::string>>();
                }

                if (rJson.contains("availableUpgrades") && rJson["availableUpgrades"].is_array())
                {
                    def.availableUpgrades = rJson["availableUpgrades"].get<std::vector<std::string>>();
                }

                if (!def.id.empty())
                {
                    s_rooms[def.id] = def;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[roomDatabase] Error parsing " << path << ": " << e.what() << "\n";
        return false;
    }
}

const RoomDefinition* roomDatabase::getRoom(const std::string& roomId)
{
    if (s_rooms.empty())
    {
        loadRoomsFromFile();
    }
    auto it = s_rooms.find(roomId);
    if (it != s_rooms.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::vector<RoomDefinition> roomDatabase::getRootRooms()
{
    if (s_rooms.empty())
    {
        loadRoomsFromFile();
    }
    std::vector<RoomDefinition> rootRooms;
    for (const auto& [id, def] : s_rooms)
    {
        if (def.tier == 1)
        {
            rootRooms.push_back(def);
        }
    }
    return rootRooms;
}

std::vector<RoomDefinition> roomDatabase::getAvailableUpgrades(const std::string& roomId)
{
    const RoomDefinition* cur = getRoom(roomId);
    std::vector<RoomDefinition> upgrades;
    if (!cur) return upgrades;

    for (const auto& upId : cur->availableUpgrades)
    {
        const RoomDefinition* upDef = getRoom(upId);
        if (upDef)
        {
            upgrades.push_back(*upDef);
        }
    }
    return upgrades;
}

const std::unordered_map<std::string, RoomDefinition>& roomDatabase::getAllRooms()
{
    if (s_rooms.empty())
    {
        loadRoomsFromFile();
    }
    return s_rooms;
}

void roomDatabase::clear()
{
    s_rooms.clear();
}

