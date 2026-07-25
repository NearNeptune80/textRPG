#pragma once
#include <string>
#include <nlohmann/json.hpp>

class game;

class saveManager
{
public:
    static bool saveGame(game* gameInstance, const std::string& filePath = "saves/save_01.json");
    static bool loadGame(game* gameInstance, const std::string& filePath = "saves/save_01.json");
    static void createInitialSave(game* gameInstance, const std::string& filePath = "saves/save_01.json");
};