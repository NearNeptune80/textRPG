#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class game;

struct SaveMetaData
{
    std::string fileName;       // e.g. "Hero_BeforeBoss.json"
    std::string saveName;       // Custom name: "Before Boss" or "Autosave 1"
    std::string characterName;  // e.g. "Hero"
    int characterLevel{1};
    std::string activeQuest;    // e.g. "Clear the Alleyways"
    std::string mapLocation;    // e.g. "Old Town"
    std::string timestamp;      // e.g. "2026-08-04 18:42"
    bool isAutosave{false};
};

struct CharacterSaveGroup
{
    std::string characterName;
    std::vector<SaveMetaData> saves;
};

class saveManager
{
public:
    // Manual named saves
    static bool saveNamedGame(game* g, const std::string& customSaveName);

    // Per-character rolling autosaves (default: 3)
    static bool saveAutosave(game* g, int maxAutosaves = 3);

    // Loading & checks
    static bool loadFromFile(game* g, const std::string& fileName);
    static bool exists(game* g, const std::string& customSaveName);

    // Grouped metadata parsing for UI dropdowns
    static std::vector<CharacterSaveGroup> getSavesGroupedByCharacter();
    static SaveMetaData readMetadata(const std::string& filePath);

private:
    static std::string sanitizeFilename(const std::string& input);
    static nlohmann::json buildPayload(game* g, const std::string& customSaveName);
};