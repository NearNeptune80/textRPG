#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class game;

struct SaveMetaData
{
    std::string fileName;
    std::string saveName;
    std::string characterName;
    int characterLevel{1};
    std::string activeQuest;
    std::string mapLocation;
    std::string timestamp;
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
    static bool saveNamedGame(game* g, const std::string& customSaveName);
    static bool saveAutosave(game* g, int maxAutosaves = 3);
    static bool loadFromFile(game* g, const std::string& fileName);
    static bool exists(game* g, const std::string& customSaveName);

    static std::vector<CharacterSaveGroup> getSavesGroupedByCharacter();
    static SaveMetaData readMetadata(const std::string& filePath);

private:
    static std::string sanitizeFilename(const std::string& input);
    static nlohmann::json buildPayload(game* g, const std::string& customSaveName);
};