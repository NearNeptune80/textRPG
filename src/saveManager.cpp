#include "saveManager.h"
#include "game.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

bool saveManager::saveGame(game* g, const std::string& filePath)
{
    if (!g || !g->Player || !g->map) return false;

    nlohmann::json saveJson;

    nlohmann::json timeJson;
    timeJson["minute"] = g->gameTime.minute;
    timeJson["hour"] = g->gameTime.hour;
    timeJson["day"] = g->gameTime.day;
    timeJson["month"] = g->gameTime.month;
    timeJson["year"] = g->gameTime.year;
    timeJson["dayOfWeek"] = g->gameTime.dayOfWeek;
    saveJson["time"] = timeJson;

    saveJson["currentMap"] = g->map->getId();
    saveJson["playerX"] = g->gridX;
    saveJson["playerY"] = g->gridY;
    saveJson["player"] = g->Player->toJson();

    nlohmann::json mapsJson = json::object();
    for (const auto& [mId, gMap] : g->mapCache)
    {
        mapsJson[mId] = gMap.saveStateToJson();
    }
    saveJson["maps"] = mapsJson;

    fs::path p(filePath);
    if (p.has_parent_path() && !fs::exists(p.parent_path()))
    {
        fs::create_directories(p.parent_path());
    }

    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << saveJson.dump(4);
    return true;
}

bool saveManager::loadGame(game* g, const std::string& filePath)
{
    if (!g) return false;

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        nlohmann::json saveJson;
        file >> saveJson;

        if (saveJson.contains("time"))
        {
            const auto& t = saveJson["time"];
            g->gameTime.minute = t.value("minute", 0);
            g->gameTime.hour = t.value("hour", 8);
            g->gameTime.day = t.value("day", 1);
            g->gameTime.month = t.value("month", 1);
            g->gameTime.year = t.value("year", 1);
            g->gameTime.dayOfWeek = t.value("dayOfWeek", 0);
        }

        if (!g->Player) g->Player = new entity("player_1", "Oellanix");
        if (saveJson.contains("player"))
        {
            g->Player->fromJson(saveJson["player"]);
        }

        std::string activeMap = saveJson.value("currentMap", "overworld");
        int targetX = saveJson.value("playerX", 1);
        int targetY = saveJson.value("playerY", 1);

        g->loadMap(activeMap, targetX, targetY);

        if (saveJson.contains("maps"))
        {
            for (auto& [mId, mJson] : saveJson["maps"].items())
            {
                if (g->mapCache.find(mId) == g->mapCache.end())
                {
                    gameMap newMap;
                    newMap.loadFromFile("data/maps/" + mId + ".json");
                    g->mapCache[mId] = newMap;
                }
                g->mapCache[mId].loadStateFromJson(mJson);
            }
        }

        g->refreshActionGrid();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Save System] Corrupted save file or invalid format: " << e.what() << "\n";
        return false;
    }
}

void saveManager::createInitialSave(game* g, const std::string& filePath)
{
    if (!g) return;

    g->Player = new entity("player_1", "Oellanix");

    g->Player->stats.setBaseStat("level", 1.0f);
    g->Player->stats.setBaseStat("xp", 0.0f);
    g->Player->stats.setBaseStat("physique", 12.0f);
    g->Player->stats.setBaseStat("arcane", 15.0f);
    g->Player->stats.setBaseStat("corruption", 0.0f);

    g->Player->stats.setBaseStat("health", 68.0f);
    g->Player->stats.setBaseStat("mana", 91.0f);
    g->Player->stats.setBaseStat("lust", 100.0f);
    g->Player->stats.setBaseStat("currency", 150.0f);
    g->Player->stats.setBaseStat("gems", 10.0f);

    auto createHumanPart = [](const std::string& id, const std::string& name, CoveringType covering, const std::string& color, int count = 1, const std::string& style = "", const std::vector<std::string>& tags = {})
        {
            bodyPart part;
            part.id = id;
            part.name = name;
            part.race = "Human";
            part.count = count;
            part.covering = covering;
            part.primaryColor = color;
            part.style = style;
            part.tags = tags;
            return part;
        };

    g->Player->anatomy.setPart(bodySlot::HAIR, createHumanPart("part_hair_human", "Hair", CoveringType::HAIR_COVERING, "Brown", 1, "Short loose"));
    g->Player->anatomy.setPart(bodySlot::HEAD, createHumanPart("part_head_human", "Face", CoveringType::SKIN, "Fair"));
    g->Player->anatomy.setPart(bodySlot::EYES, createHumanPart("part_eyes_human", "Eyes", CoveringType::IRIS, "Blue", 2));
    g->Player->anatomy.setPart(bodySlot::EARS, createHumanPart("part_ears_human", "Ears", CoveringType::SKIN, "Fair", 2));
    g->Player->anatomy.setPart(bodySlot::MOUTH, createHumanPart("part_mouth_human", "Tongue", CoveringType::FLESH, "Pink"));
    g->Player->anatomy.setPart(bodySlot::NECK, createHumanPart("part_neck_human", "Neck", CoveringType::SKIN, "Fair"));
    g->Player->anatomy.setPart(bodySlot::TORSO, createHumanPart("part_torso_human", "Torso", CoveringType::SKIN, "Fair"));

    bodyPart breasts = createHumanPart("part_breasts_human", "Nipples", CoveringType::SKIN, "Fair");
    breasts.cupSize = 0;
    g->Player->anatomy.setPart(bodySlot::BREASTS, breasts);

    g->Player->anatomy.setPart(bodySlot::STOMACH, createHumanPart("part_stomach_human", "Stomach", CoveringType::SKIN, "Fair"));
    g->Player->anatomy.setPart(bodySlot::BACK, createHumanPart("part_back_human", "Back", CoveringType::SKIN, "Fair"));
    g->Player->anatomy.setPart(bodySlot::ARMS, createHumanPart("part_arms_human", "Arms", CoveringType::SKIN, "Fair", 2));
    g->Player->anatomy.setPart(bodySlot::HANDS, createHumanPart("part_hands_human", "Hands", CoveringType::SKIN, "Fair", 2));
    g->Player->anatomy.setPart(bodySlot::FINGERS, createHumanPart("part_fingers_human", "Fingers", CoveringType::SKIN, "Fair", 10));
    g->Player->anatomy.setPart(bodySlot::HIPS, createHumanPart("part_hips_human", "Hips", CoveringType::SKIN, "Fair"));

    bodyPart penis = createHumanPart("part_penis_human", "Penis", CoveringType::SKIN, "Fair");
    penis.length = 14.0f;
    penis.diameter = 3.5f;
    g->Player->anatomy.setPart(bodySlot::GROIN, penis);

    bodyPart anus = createHumanPart("part_anus_human", "Anus", CoveringType::SKIN, "Fair");
    anus.secondaryColor = "Pink";
    g->Player->anatomy.setPart(bodySlot::ASS, anus);

    g->Player->anatomy.setPart(bodySlot::LEGS, createHumanPart("part_legs_human", "Legs", CoveringType::SKIN, "Fair", 2, "plantigrade", { "plantigrade" }));
    g->Player->anatomy.setPart(bodySlot::FEET, createHumanPart("part_feet_human", "Feet", CoveringType::SKIN, "Fair", 2, "plantigrade", { "plantigrade" }));

    // Pre-populate multi-page test items
    std::vector<std::string> initialItems = {
        "item_linen_shirt", "item_leather_trousers", "item_leather_boots", "item_leather_choker",
        "item_cloth_gloves", "item_silk_panties", "item_silk_bra", "item_ancient_tome",
        "item_canis_root", "item_golden_pendant", "item_canis_root", "item_canis_root",
        "item_linen_shirt", "item_leather_trousers", "item_leather_boots", "item_cloth_gloves",
        "item_canis_root", "item_canis_root", "item_canis_root", "item_ancient_tome",
        "item_golden_pendant", "item_silk_panties", "item_silk_bra", "item_leather_choker",
        "item_canis_root", "item_canis_root", "item_linen_shirt", "item_leather_trousers",
        "item_leather_boots", "item_cloth_gloves", "item_ancient_tome", "item_golden_pendant",
        "item_canis_root", "item_canis_root", "item_silk_panties", "item_silk_bra"
    };

    for (const auto& itemId : initialItems)
    {
        auto itemPtr = itemDatabase::getItem(itemId);
        if (itemPtr) g->Player->inventory.addItem(itemPtr);
    }

    g->loadMap("overworld", 1, 1);
    saveGame(g, filePath);
}