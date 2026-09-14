#include "save/saveManager.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "core/game.h"
#include "entities/namedCharacter.h"
#include "items/itemDatabase.h"
#include "items/infusionEffect.h"

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
    static std::unordered_map<std::string, std::pair<fs::file_time_type, SaveMetaData>> s_metadataCache;

    std::error_code ec;
    auto writeTime = fs::last_write_time(filePath, ec);
    if (!ec)
    {
        auto it = s_metadataCache.find(filePath);
        if (it != s_metadataCache.end() && it->second.first == writeTime)
        {
            return it->second.second;
        }
    }

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

    if (!ec)
    {
        s_metadataCache[filePath] = { writeTime, meta };
    }

    return meta;
}

std::vector<CharacterSaveGroup> saveManager::getSavesGroupedByCharacter()
{
    std::unordered_map<std::string, std::vector<SaveMetaData>> grouped;
    std::string savesDir = getSavesDirectory();

    std::vector<std::string> searchDirs = { savesDir, "saves", "data/saves" };
    std::unordered_set<std::string> seenFiles;

    for (const auto& dir : searchDirs)
    {
        if (!fs::exists(dir)) continue;
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::string filename = entry.path().filename().string();
                if (filename == "QuickSave.json" || filename == "settings.json" || filename == "theme.json")
                {
                    continue;
                }
                if (seenFiles.insert(filename).second)
                {
                    SaveMetaData meta = readMetadata(entry.path().string());
                    if (!meta.characterName.empty())
                    {
                        grouped[meta.characterName].push_back(meta);
                    }
                }
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

    // Sort character groups from most recently played to oldest
    std::sort(result.begin(), result.end(), [](const CharacterSaveGroup& a, const CharacterSaveGroup& b) {
        if (a.saves.empty()) return false;
        if (b.saves.empty()) return true;
        return a.saves.front().timestamp > b.saves.front().timestamp;
    });

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
    j["namedCharacters"] = NamedCharacterManager::saveStateToJson();
    j["settings"] = g->settings.toJson();

    json logArray = json::array();
    for (const auto& entry : g->getEventLog())
    {
        logArray.push_back({
            {"tag", entry.tag},
            {"text", entry.text},
            {"color", {entry.color.r, entry.color.g, entry.color.b, entry.color.a}},
            {"time", entry.timeStr}
        });
    }
    j["eventLog"] = logArray;

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

bool saveManager::deleteSave(const std::string& fileName)
{
    std::string savesDir = getSavesDirectory();
    std::string path = fileName;
    if (!fs::exists(path))
    {
        path = savesDir + "/" + fileName;
    }
    if (!fs::exists(path))
    {
        path = "saves/" + fileName;
    }

    if (fs::exists(path))
    {
        std::error_code ec;
        return fs::remove(path, ec);
    }
    return false;
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
            g->playerEntity->fromJson(j["player"]);
            g->Player = g->playerEntity.get();
        }

        bool prevAutoSave = g->settings.gameplay.autoSaveOnMapChange;
        g->settings.gameplay.autoSaveOnMapChange = false;

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

        if (j.contains("namedCharacters"))
        {
            NamedCharacterManager::loadStateFromJson(j["namedCharacters"]);
            if (g->map)
            {
                NamedCharacterManager::syncMapCharacters(g->map);
            }
        }

        if (j.contains("settings"))
        {
            g->settings.fromJson(j["settings"]);
        }
        else
        {
            g->settings.gameplay.autoSaveOnMapChange = prevAutoSave;
        }

        if (j.contains("eventLog") && j["eventLog"].is_array())
        {
            g->clearEventLog();
            for (const auto& item : j["eventLog"])
            {
                LogColor col{ 200, 200, 200, 255 };
                if (item.contains("color") && item["color"].is_array() && item["color"].size() >= 4)
                {
                    col = { item["color"][0].get<uint8_t>(), item["color"][1].get<uint8_t>(), item["color"][2].get<uint8_t>(), item["color"][3].get<uint8_t>() };
                }
                g->addLogEntry(item.value("tag", "[INFO]"), item.value("text", ""), col);
            }
        }

        g->Player = g->playerEntity.get();
        if (g->Player)
        {
            bool hasEnchantingSupplies = false;
            for (const auto& it : g->Player->inventory.backpack)
            {
                if (it && it->id == "item_plain_elixir") { hasEnchantingSupplies = true; break; }
            }
            if (!hasEnchantingSupplies || g->Player->inventory.backpack.size() <= 1)
            {
                grantStarterTestKit(g->Player);
            }
            else if (g->Player->getStat("arcaneEssence") < 40.0f)
            {
                g->Player->stats.setBaseStat("arcaneEssence", 50.0f);
            }
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

void saveManager::grantStarterTestKit(entity* player)
{
    if (!player) return;

    auto addIfMissing = [&](const std::string& itemId, int count = 1) {
        for (const auto& it : player->inventory.backpack)
        {
            if (it && it->id == itemId) return;
        }
        auto itemPtr = itemDatabase::getItem(itemId);
        if (itemPtr)
        {
            itemPtr->count = count;
            player->inventory.addItem(itemPtr);
        }
    };

    // 1. Uninfused Tonics for Enchanting at the Altar
    addIfMissing("item_plain_elixir", 3);

    // 2. Consumable Potions & Foods for Testing Vitals, Status Effects & Transmutation
    addIfMissing("item_canis_root", 2);
    addIfMissing("item_potion_health", 2);
    addIfMissing("item_potion_mana", 2);
    addIfMissing("item_apple_crisp", 2);
    addIfMissing("item_feline_mint", 2);

    // 3. Equippable Garments & Weapons across slots
    addIfMissing("item_golden_pendant", 1);
    addIfMissing("item_dagger_iron", 1);
    addIfMissing("item_cloth_gloves", 1);
    addIfMissing("item_leather_choker", 1);
    addIfMissing("item_leather_boots", 1);
    addIfMissing("item_leather_skirt", 1);
    addIfMissing("item_ancient_tome", 1);

    // 4. Pre-infused sample potions for live status effect testing
    bool hasPredator = false;
    bool hasPrimal = false;
    for (const auto& it : player->inventory.backpack)
    {
        if (it && it->id == "item_infused_predator") hasPredator = true;
        if (it && it->id == "item_infused_primal") hasPrimal = true;
    }

    if (!hasPredator)
    {
        auto infusedPredator = std::make_shared<item>();
        infusedPredator->id = "item_infused_predator";
        infusedPredator->name = "Elixir of Predator's Instinct";
        infusedPredator->description = "A shimmering amber tonic imbued with acute predatory senses. Drinking this elixir grants +3 Agility for 4 hours.";
        infusedPredator->tooltip = "Infused Elixir (+3 Agility for 4h).";
        infusedPredator->baseValue = 75;
        infusedPredator->isConsumable = true;
        infusedPredator->count = 1;
        InfusionEffect effPredator{ InfusionTargetType::CONSUMABLE_TONIC, EnchantmentFocus::HAIR, AspectProperty::AGILITY_STAT, InfusionTier::GREATER_BOON };
        infusedPredator->infusionEffects.push_back(effPredator);
        player->inventory.addItem(infusedPredator);
    }

    if (!hasPrimal)
    {
        auto infusedPrimal = std::make_shared<item>();
        infusedPrimal->id = "item_infused_primal";
        infusedPrimal->name = "Elixir of Primal Surge";
        infusedPrimal->description = "A deep crimson elixir radiating primordial warmth. Drinking this elixir grants +45 Vitality for 24 hours.";
        infusedPrimal->tooltip = "Infused Elixir (+45 Vitality for 24h).";
        infusedPrimal->baseValue = 90;
        infusedPrimal->isConsumable = true;
        infusedPrimal->count = 1;
        InfusionEffect effPrimal{ InfusionTargetType::CONSUMABLE_TONIC, EnchantmentFocus::TORSO, AspectProperty::HEALTH_CEILING, InfusionTier::BOON };
        infusedPrimal->infusionEffects.push_back(effPrimal);
        player->inventory.addItem(infusedPrimal);
    }

    // 5. Arcane Essence and Gold for Crafting
    if (player->getStat("arcaneEssence") < 50.0f)
    {
        player->stats.setBaseStat("arcaneEssence", 50.0f);
    }
    if (player->getStat("currency") < 250.0f)
    {
        player->stats.setBaseStat("currency", 500.0f);
    }
}