#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

struct RoomDefinition
{
    std::string id;
    std::string name;
    int tier = 1;
    std::string category = "General";
    std::string description;
    float costGold = 0.0f;
    bool isStorageSafe = true;
    std::string stationType;
    std::vector<std::string> tags;
    std::vector<std::string> availableUpgrades;
};

class roomDatabase
{
public:
    static bool loadRoomsFromFile(const std::string& path = "data/config/room_types.json");
    static const RoomDefinition* getRoom(const std::string& roomId);
    static std::vector<RoomDefinition> getRootRooms();
    static std::vector<RoomDefinition> getAvailableUpgrades(const std::string& roomId);
    static const std::unordered_map<std::string, RoomDefinition>& getAllRooms();
    static void clear();

private:
    static std::unordered_map<std::string, RoomDefinition> s_rooms;
};

