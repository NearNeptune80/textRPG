#include "map/gameMap.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "common/randomEngine.h"
#include "entities/entity.h"
#include "entities/npcGenerator.h"
#include "items/itemDatabase.h"

using json = nlohmann::json;

gameMap::gameMap() = default;
gameMap::~gameMap() = default;

TileRuntimeData& gameMap::getRuntimeData(int x, int y)
{
    return runtimeData[makeTileKey(x, y)];
}

bool gameMap::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json data;
        file >> data;

        mapId = data.value("id", "");
        mapName = data.value("name", "Unknown Map");
        width = data.at("width").get<int>();
        height = data.at("height").get<int>();

        grid.clear();
        grid.resize(height, std::vector<Tile>(width, { TILE_VOID, STATE_HIDDEN }));
        runtimeData.clear();

        auto tilesJson = data.at("tiles");
        for (size_t y = 0; y < static_cast<size_t>(height) && y < tilesJson.size(); ++y)
        {
            auto rowJson = tilesJson[y];
            for (size_t x = 0; x < static_cast<size_t>(width) && x < rowJson.size(); ++x)
            {
                int typeInt = rowJson[x].get<int>();
                grid[y][x] = { static_cast<TileType>(typeInt), STATE_HIDDEN };
            }
        }

        if (data.contains("dangerLevels"))
        {
            auto dangerJson = data.at("dangerLevels");
            for (size_t y = 0; y < static_cast<size_t>(height) && y < dangerJson.size(); ++y)
            {
                auto rowJson = dangerJson[y];
                for (size_t x = 0; x < static_cast<size_t>(width) && x < rowJson.size(); ++x)
                {
                    getRuntimeData(static_cast<int>(x), static_cast<int>(y)).baseDangerLevel = rowJson[x].get<int>();
                }
            }
        }

        warps.clear();
        if (data.contains("warps"))
        {
            for (const auto& wJson : data.at("warps"))
            {
                MapWarp w;
                w.x = wJson.at("x").get<int>();
                w.y = wJson.at("y").get<int>();
                w.targetMap = wJson.at("targetMap").get<std::string>();
                w.targetX = wJson.at("targetX").get<int>();
                w.targetY = wJson.at("targetY").get<int>();
                warps.push_back(w);

                getRuntimeData(w.x, w.y).iconId = "icon_door";
            }
        }

        triggers.clear();
        if (data.contains("triggers"))
        {
            for (const auto& tJson : data.at("triggers"))
            {
                MapTrigger trig;
                trig.id = tJson.value("id", "");
                trig.label = tJson.value("label", "Interact");
                trig.tooltip = tJson.value("tooltip", "");
                trig.description = tJson.value("description", "");
                trig.x = tJson.at("x").get<int>();
                trig.y = tJson.at("y").get<int>();
                trig.sceneId = tJson.at("sceneId").get<std::string>();

                if (tJson.contains("conditions"))
                {
                    for (const auto& cJson : tJson.at("conditions"))
                    {
                        gameCondition cond;
                        cond.type = cJson.at("type").get<std::string>();
                        cond.target = cJson.at("target").get<std::string>();
                        cond.requiredValue = cJson.value("requiredValue", 0);
                        trig.conditions.push_back(cond);
                    }
                }
                triggers.push_back(trig);
            }
        }

        // Tile-specific or Zone Persistent Encounters
        if (data.contains("tileEncounters") && data["tileEncounters"].is_object())
        {
            for (const auto& [coordStr, encJson] : data["tileEncounters"].items())
            {
                size_t comma = coordStr.find(',');
                if (comma != std::string::npos)
                {
                    int x = std::stoi(coordStr.substr(0, comma));
                    int y = std::stoi(coordStr.substr(comma + 1));
                    auto& tData = getRuntimeData(x, y);
                    tData.ambushState.templatePool.clear();
                    if (encJson.contains("pool") && encJson["pool"].is_array())
                    {
                        for (const auto& pVal : encJson["pool"])
                        {
                            if (pVal.is_string()) tData.ambushState.templatePool.push_back(pVal.get<std::string>());
                        }
                    }
                    if (encJson.contains("template"))
                    {
                        tData.ambushState.templateId = encJson.value("template", "tpl_alley_bandit");
                        if (tData.ambushState.templatePool.empty())
                        {
                            tData.ambushState.templatePool.push_back(tData.ambushState.templateId);
                        }
                    }
                    else if (!tData.ambushState.templatePool.empty())
                    {
                        tData.ambushState.templateId = tData.ambushState.templatePool.front();
                    }
                    else
                    {
                        tData.ambushState.templateId = "tpl_alley_bandit";
                        tData.ambushState.templatePool.push_back("tpl_alley_bandit");
                    }
                    tData.ambushState.ambushChance = encJson.value("ambushChance", 50);
                    restockAmbushNPC(tData.ambushState);
                }
            }
        }

        // For any dangerous tile without explicit encounter template, assign default template
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                auto& tData = getRuntimeData(x, y);
                if (tData.baseDangerLevel > 0 && tData.ambushState.templateId.empty())
                {
                    std::string defTpl = (tData.baseDangerLevel >= 2) ? "tpl_rogue_mage" : "tpl_alley_bandit";
                    tData.ambushState.templateId = defTpl;
                    tData.ambushState.templatePool = { defTpl };
                    tData.ambushState.ambushChance = std::clamp(tData.baseDangerLevel * 25, 20, 75);
                    restockAmbushNPC(tData.ambushState);
                }
            }
        }

        // Descriptions & Tags
        defaultDescriptions.clear();
        if (data.contains("defaultDescriptions") && data["defaultDescriptions"].is_array())
        {
            for (const auto& d : data["defaultDescriptions"])
            {
                if (d.is_string()) defaultDescriptions.push_back(d.get<std::string>());
            }
        }
        else if (data.contains("description") && data["description"].is_string())
        {
            defaultDescriptions.push_back(data["description"].get<std::string>());
        }

        tileDescriptions.clear();
        if (data.contains("tileDescriptions") && data["tileDescriptions"].is_object())
        {
            for (const auto& [coordKey, val] : data["tileDescriptions"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tileDescriptions[coordKey] = list;
                }
                else if (val.is_string())
                {
                    tileDescriptions[coordKey] = { val.get<std::string>() };
                }
            }
        }

        tileTags.clear();
        if (data.contains("tileTags") && data["tileTags"].is_object())
        {
            for (const auto& [coordKey, val] : data["tileTags"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tileTags[coordKey] = list;
                }
                else if (val.is_string())
                {
                    tileTags[coordKey] = { val.get<std::string>() };
                }
            }
        }

        tagDescriptions.clear();
        if (data.contains("tagDescriptions") && data["tagDescriptions"].is_object())
        {
            for (const auto& [tagKey, val] : data["tagDescriptions"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tagDescriptions[tagKey] = list;
                }
                else if (val.is_string())
                {
                    tagDescriptions[tagKey] = { val.get<std::string>() };
                }
            }
        }

        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Map JSON Error (" << filePath << "): " << e.what() << "\n";
        return false;
    }
}

const std::vector<std::string>& gameMap::getTileTags(int x, int y) const
{
    static const std::vector<std::string> emptyTags;
    std::string key = std::to_string(x) + "," + std::to_string(y);
    auto it = tileTags.find(key);
    if (it != tileTags.end()) return it->second;
    return emptyTags;
}

std::string gameMap::getTileDescription(int x, int y) const
{
    std::string key = std::to_string(x) + "," + std::to_string(y);
    auto pickStable = [x, y](const std::vector<std::string>& list) -> std::string {
        if (list.empty()) return "";
        uint32_t h = (static_cast<uint32_t>(x) * 73856093u) ^ (static_cast<uint32_t>(y) * 19349663u);
        return list[h % list.size()];
    };

    // 1. Specific coordinate description
    auto itDesc = tileDescriptions.find(key);
    if (itDesc != tileDescriptions.end() && !itDesc->second.empty())
    {
        return pickStable(itDesc->second);
    }

    // 2. Tag-based descriptions
    auto itTags = tileTags.find(key);
    if (itTags != tileTags.end())
    {
        for (const auto& tag : itTags->second)
        {
            auto itTagDesc = tagDescriptions.find(tag);
            if (itTagDesc != tagDescriptions.end() && !itTagDesc->second.empty())
            {
                return pickStable(itTagDesc->second);
            }
        }
    }

    // 3. Default map descriptions
    if (!defaultDescriptions.empty())
    {
        return pickStable(defaultDescriptions);
    }

    return "You are in " + mapName + ".";
}

bool gameMap::isWalkable(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    TileType t = grid[y][x].type;
    return (t == TILE_FLOOR || t == TILE_DOOR);
}

bool gameMap::isOpaque(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return true;
    TileType t = grid[y][x].type;
    return (t == TILE_WALL || t == TILE_VOID);
}

void gameMap::updateDiscovery(int playerX, int playerY, int visionRadius)
{
    if (playerX < 0 || playerX >= width || playerY < 0 || playerY >= height) return;

    // 1. The tile the player has physically walked on is marked as visited & fully revealed
    if (grid[playerY][playerX].type != TILE_VOID)
    {
        grid[playerY][playerX].visited = true;
        grid[playerY][playerX].discovery = STATE_REVEALED;
    }

    // 2. Tiles the player has stood next to (cardinal adjacency: up, down, left, right) become partially discovered
    static const int cardinals[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    for (const auto& [dx, dy] : cardinals)
    {
        int tx = playerX + dx;
        int ty = playerY + dy;
        if (tx >= 0 && tx < width && ty >= 0 && ty < height)
        {
            if (grid[ty][tx].type != TILE_VOID && !grid[ty][tx].visited)
            {
                grid[ty][tx].discovery = STATE_PARTIAL;
            }
        }
    }
}

void gameMap::restockAmbushNPC(PersistentAmbushState& ambush)
{
    ambush.isDefeated = false;
    ambush.restockMinutesRemaining = 0;

    if (!ambush.npc)
    {
        if (!ambush.templatePool.empty())
        {
            int rIdx = dice::rollInt(0, static_cast<int>(ambush.templatePool.size()) - 1);
            ambush.templateId = ambush.templatePool[rIdx];
        }
        if (!ambush.templateId.empty())
        {
            ambush.npc = npcGenerator::generateFromTemplate(ambush.templateId);
        }
    }

    if (ambush.npc)
    {
        float maxHp = ambush.npc->getStat("max_health");
        if (maxHp <= 0.0f) maxHp = 50.0f;
        float maxMana = ambush.npc->getStat("max_mana");
        if (maxMana <= 0.0f) maxMana = 30.0f;

        ambush.npc->stats.setBaseStat("health", maxHp);
        ambush.npc->stats.setBaseStat("mana", maxMana);

        // Restore / top up random currency (e.g. 20-50 gold)
        float freshGold = static_cast<float>(dice::rollInt(20, 50));
        ambush.npc->stats.setBaseStat("currency", freshGold);

        // Restock items from template
        if (!ambush.templateId.empty())
        {
            auto fresh = npcGenerator::generateFromTemplate(ambush.templateId);
            if (fresh)
            {
                ambush.npc->inventory = fresh->inventory;
            }
        }
    }
}

void gameMap::processTimePassage(int minutesPassed)
{
    if (minutesPassed <= 0) return;

    for (auto& [key, runtime] : runtimeData)
    {
        runtime.processItemDecay(minutesPassed);

        // 24-hour restock countdown for defeated persistent ambushers
        if (runtime.ambushState.isDefeated)
        {
            runtime.ambushState.restockMinutesRemaining -= minutesPassed;
            if (runtime.ambushState.restockMinutesRemaining <= 0)
            {
                restockAmbushNPC(runtime.ambushState);
            }
        }
    }
}

Tile gameMap::getTile(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return { TILE_VOID, STATE_HIDDEN };
    return grid[y][x];
}

bool gameMap::checkWarp(int x, int y, MapWarp& outWarp) const
{
    for (const auto& w : warps)
    {
        if (w.x == x && w.y == y)
        {
            outWarp = w;
            return true;
        }
    }
    return false;
}

std::vector<MapTrigger> gameMap::getTriggersAt(int x, int y) const
{
    std::vector<MapTrigger> result;
    for (const auto& trig : triggers)
    {
        if (trig.x == x && trig.y == y)
        {
            result.push_back(trig);
        }
    }
    return result;
}

nlohmann::json gameMap::saveStateToJson() const
{
    json j;
    j["mapId"] = mapId;

    json discoveryGrid = json::array();
    json visitedGrid = json::array();
    for (int y = 0; y < height; ++y)
    {
        json row = json::array();
        json vRow = json::array();
        for (int x = 0; x < width; ++x)
        {
            row.push_back(static_cast<int>(grid[y][x].discovery));
            vRow.push_back(grid[y][x].visited);
        }
        discoveryGrid.push_back(row);
        visitedGrid.push_back(vRow);
    }
    j["discovery"] = discoveryGrid;
    j["visited"] = visitedGrid;

    json tileItemsMap = json::object();
    json tileNPCsMap = json::object();
    json tileAmbushesMap = json::object();

    for (const auto& [key, runtime] : runtimeData)
    {
        if (!runtime.droppedItems.empty())
        {
            json itemArray = json::array();
            for (const auto& entry : runtime.droppedItems)
            {
                if (entry.itemPtr)
                {
                    json itemEntry;
                    itemEntry["id"] = entry.itemPtr->id;
                    itemEntry["count"] = entry.itemPtr->count;
                    itemEntry["minutesRemaining"] = entry.minutesRemaining;
                    itemArray.push_back(itemEntry);
                }
            }
            tileItemsMap[std::to_string(key)] = itemArray;
        }

        if (runtime.persistentNPC)
        {
            tileNPCsMap[std::to_string(key)] = runtime.persistentNPC->toJson();
        }

        if (!runtime.ambushState.templateId.empty())
        {
            json aJson;
            aJson["templateId"] = runtime.ambushState.templateId;
            aJson["templatePool"] = runtime.ambushState.templatePool;
            aJson["isPermanentlyRemoved"] = runtime.ambushState.isPermanentlyRemoved;
            aJson["isDefeated"] = runtime.ambushState.isDefeated;
            aJson["restockMinutesRemaining"] = runtime.ambushState.restockMinutesRemaining;
            aJson["ambushChance"] = runtime.ambushState.ambushChance;
            if (runtime.ambushState.npc)
            {
                aJson["npc"] = runtime.ambushState.npc->toJson();
            }
            tileAmbushesMap[std::to_string(key)] = aJson;
        }
    }

    j["tileItems"] = tileItemsMap;
    j["tileNPCs"] = tileNPCsMap;
    j["tileAmbushes"] = tileAmbushesMap;

    return j;
}

void gameMap::loadStateFromJson(const json& j)
{
    if (j.contains("discovery"))
    {
        const auto& dGrid = j["discovery"];
        for (size_t y = 0; y < static_cast<size_t>(height) && y < dGrid.size(); ++y)
        {
            const auto& row = dGrid[y];
            for (size_t x = 0; x < static_cast<size_t>(width) && x < row.size(); ++x)
            {
                grid[y][x].discovery = static_cast<DiscoveryState>(row[x].get<int>());
            }
        }
    }

    if (j.contains("visited"))
    {
        const auto& vGrid = j["visited"];
        for (size_t y = 0; y < static_cast<size_t>(height) && y < vGrid.size(); ++y)
        {
            const auto& row = vGrid[y];
            for (size_t x = 0; x < static_cast<size_t>(width) && x < row.size(); ++x)
            {
                grid[y][x].visited = row[x].get<bool>();
            }
        }
    }

    if (j.contains("tileItems"))
    {
        for (auto& [keyStr, itemsJson] : j["tileItems"].items())
        {
            uint64_t key = std::stoull(keyStr);
            runtimeData[key].droppedItems.clear();
            for (const auto& itemEntry : itemsJson)
            {
                std::string itemId = itemEntry.is_string() ? itemEntry.get<std::string>() : itemEntry.value("id", "");
                int count = itemEntry.is_object() ? itemEntry.value("count", 1) : 1;
                int mins = itemEntry.is_object() ? itemEntry.value("minutesRemaining", 120) : 120;

                auto itemPtr = itemDatabase::getItem(itemId);
                if (itemPtr)
                {
                    itemPtr->count = count;
                    runtimeData[key].droppedItems.push_back({ itemPtr, mins });
                }
            }
        }
    }

    if (j.contains("tileNPCs"))
    {
        for (auto& [keyStr, npcJson] : j["tileNPCs"].items())
        {
            uint64_t key = std::stoull(keyStr);
            auto npc = std::make_shared<entity>("npc_temp", "Unknown");
            npc->fromJson(npcJson);
            runtimeData[key].persistentNPC = npc;
        }
    }

    if (j.contains("tileAmbushes"))
    {
        for (auto& [keyStr, aJson] : j["tileAmbushes"].items())
        {
            uint64_t key = std::stoull(keyStr);
            auto& aState = runtimeData[key].ambushState;
            aState.templateId = aJson.value("templateId", "");
            if (aJson.contains("templatePool") && aJson["templatePool"].is_array())
            {
                aState.templatePool = aJson["templatePool"].get<std::vector<std::string>>();
            }
            aState.isPermanentlyRemoved = aJson.value("isPermanentlyRemoved", false);
            aState.isDefeated = aJson.value("isDefeated", false);
            aState.restockMinutesRemaining = aJson.value("restockMinutesRemaining", 0);
            aState.ambushChance = aJson.value("ambushChance", 50);
            if (aJson.contains("npc"))
            {
                auto npc = std::make_shared<entity>("npc_ambush", "Ambush Enemy");
                npc->fromJson(aJson["npc"]);
                aState.npc = npc;
            }
        }
    }
}