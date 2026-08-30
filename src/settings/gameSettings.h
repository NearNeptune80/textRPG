#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "common/enums.h"

struct DemographicSettings
{
    // Sexuality Distribution (%)
    float percentHetero = 40.0f;
    float percentBi = 30.0f;
    float percentHomo = 20.0f;
    float percentAsexual = 10.0f;

    // Gender Archetype Distribution (%)
    float percentMale = 30.0f;
    float percentFemale = 40.0f;
    float percentHermaphrodite = 15.0f;
    float percentGynomorph = 7.0f;
    float percentAndromorph = 5.0f;
    float percentNull = 3.0f;

    // Age Distribution (%)
    float percentYoungAdult = 40.0f;
    float percentAdult = 35.0f;
    float percentMature = 20.0f;
    float percentElder = 5.0f;

    // Furry Distribution (%)
    float percentHuman = 50.0f;
    float percentPartial = 30.0f;
    float percentAnthro = 15.0f;
    float percentFeral = 5.0f;

    SexualOrientation rollSexuality(float roll01) const
    {
        float total = percentHetero + percentBi + percentHomo + percentAsexual;
        if (total <= 0.0f) total = 100.0f;

        float val = roll01 * total;
        if (val < percentHetero) return SexualOrientation::HETEROSEXUAL;
        val -= percentHetero;
        if (val < percentBi) return SexualOrientation::BISEXUAL;
        val -= percentBi;
        if (val < percentHomo) return SexualOrientation::HOMOSEXUAL;
        return SexualOrientation::ASEXUAL;
    }

    GenderArchetype rollGenderArchetype(float roll01) const
    {
        float total = percentMale + percentFemale + percentHermaphrodite + percentGynomorph + percentAndromorph + percentNull;
        if (total <= 0.0f) total = 100.0f;

        float val = roll01 * total;
        if (val < percentMale) return GenderArchetype::MALE;
        val -= percentMale;
        if (val < percentFemale) return GenderArchetype::FEMALE;
        val -= percentFemale;
        if (val < percentHermaphrodite) return GenderArchetype::HERMAPHRODITE;
        val -= percentHermaphrodite;
        if (val < percentGynomorph) return GenderArchetype::GYNOMORPH;
        val -= percentGynomorph;
        if (val < percentAndromorph) return GenderArchetype::ANDROMORPH;
        return GenderArchetype::ASEXUAL_NULL;
    }
};

struct ContentSettings
{
    bool pregnancyEnabled = true;
    bool lactationEnabled = true;
    bool nonConEnabled = false;
    bool publicSexEnabled = true;
    bool extremeContentEnabled = false;
    float fluidMultiplier = 1.0f;
    float transformationSpeedMultiplier = 1.0f;

    // 10 Fetishes: 0=Disabled, 1=Hate, 2=Dislike, 3=Neutral, 4=Like, 5=Love, 6=Always
    std::unordered_map<std::string, int> fetishPreferences = {
        { "Anal", 3 }, { "Buttslut", 3 }, { "Vaginal", 3 }, { "Pussy slut", 3 },
        { "Oral", 3 }, { "Oral performer", 3 }, { "Breasts lover", 3 }, { "Breasts", 3 },
        { "Milk lover", 3 }, { "Lactation", 3 }
    };
};

struct GameplaySettings
{
    float difficultyMultiplier = 1.0f;
    int difficultyLevel = 0; // 0=Human, 1=Morph, 2=Demon, 3=Lilin, 4=Lilith
    float currencyLossOnDefeatPercent = 0.15f;
    bool autoSaveOnMapChange = true;
    bool autoSaveOnSceneExit = true;
    int autoSaveFrequency = 0; // 0=Always, 1=Daily, 2=Weekly, 3=Off
    int maxAutoSaves = 3;
    std::string unitPreference = "Metric"; // Metric vs Imperial
    std::string genderPronounMode = "Normal"; // Normal vs Custom
    bool enchantmentInstability = true;
    bool badEndsEnabled = true;
    bool levelDrainEnabled = true;
    bool opportunisticAttackers = true;
    bool autoLoot = true;
    bool sharedEncyclopedia = false;
    bool stormInterruptions = true;
};

struct DisplaySettings
{
    int fontSize = 18; // 12..36
    bool fadeInEnabled = false;
    bool showArtwork = true;
    bool showThumbnails = true;
    int descriptionVerbosity = 0; // 0 = Full, 1 = Condensed, 2 = Minimal
    std::string activeTheme = "default";
    std::string activeLayout = "data/layouts/default_layout.json";
};

struct GameSettings
{
    DemographicSettings demographics;
    ContentSettings content;
    GameplaySettings gameplay;
    DisplaySettings display;

    nlohmann::json toJson() const
    {
        return nlohmann::json{
            {"demographics", {
                {"percentHetero", demographics.percentHetero},
                {"percentBi", demographics.percentBi},
                {"percentHomo", demographics.percentHomo},
                {"percentAsexual", demographics.percentAsexual},
                {"percentMale", demographics.percentMale},
                {"percentFemale", demographics.percentFemale},
                {"percentHermaphrodite", demographics.percentHermaphrodite},
                {"percentGynomorph", demographics.percentGynomorph},
                {"percentAndromorph", demographics.percentAndromorph},
                {"percentNull", demographics.percentNull},
                {"percentYoungAdult", demographics.percentYoungAdult},
                {"percentAdult", demographics.percentAdult},
                {"percentMature", demographics.percentMature},
                {"percentElder", demographics.percentElder},
                {"percentHuman", demographics.percentHuman},
                {"percentPartial", demographics.percentPartial},
                {"percentAnthro", demographics.percentAnthro},
                {"percentFeral", demographics.percentFeral}
            }},
            {"content", {
                {"pregnancyEnabled", content.pregnancyEnabled},
                {"lactationEnabled", content.lactationEnabled},
                {"nonConEnabled", content.nonConEnabled},
                {"publicSexEnabled", content.publicSexEnabled},
                {"extremeContentEnabled", content.extremeContentEnabled},
                {"fluidMultiplier", content.fluidMultiplier},
                {"transformationSpeedMultiplier", content.transformationSpeedMultiplier},
                {"fetishPreferences", content.fetishPreferences}
            }},
            {"gameplay", {
                {"difficultyMultiplier", gameplay.difficultyMultiplier},
                {"difficultyLevel", gameplay.difficultyLevel},
                {"currencyLossOnDefeatPercent", gameplay.currencyLossOnDefeatPercent},
                {"autoSaveOnMapChange", gameplay.autoSaveOnMapChange},
                {"autoSaveOnSceneExit", gameplay.autoSaveOnSceneExit},
                {"autoSaveFrequency", gameplay.autoSaveFrequency},
                {"maxAutoSaves", gameplay.maxAutoSaves},
                {"unitPreference", gameplay.unitPreference},
                {"genderPronounMode", gameplay.genderPronounMode},
                {"enchantmentInstability", gameplay.enchantmentInstability},
                {"badEndsEnabled", gameplay.badEndsEnabled},
                {"levelDrainEnabled", gameplay.levelDrainEnabled},
                {"opportunisticAttackers", gameplay.opportunisticAttackers},
                {"autoLoot", gameplay.autoLoot},
                {"sharedEncyclopedia", gameplay.sharedEncyclopedia},
                {"stormInterruptions", gameplay.stormInterruptions}
            }},
            {"display", {
                {"fontSize", display.fontSize},
                {"fadeInEnabled", display.fadeInEnabled},
                {"showArtwork", display.showArtwork},
                {"showThumbnails", display.showThumbnails},
                {"descriptionVerbosity", display.descriptionVerbosity},
                {"activeTheme", display.activeTheme},
                {"activeLayout", display.activeLayout}
            }}
        };
    }

    void fromJson(const nlohmann::json& j)
    {
        if (j.contains("demographics"))
        {
            const auto& d = j["demographics"];
            if (d.contains("percentHetero")) demographics.percentHetero = d["percentHetero"].get<float>();
            if (d.contains("percentBi")) demographics.percentBi = d["percentBi"].get<float>();
            if (d.contains("percentHomo")) demographics.percentHomo = d["percentHomo"].get<float>();
            if (d.contains("percentAsexual")) demographics.percentAsexual = d["percentAsexual"].get<float>();
            if (d.contains("percentMale")) demographics.percentMale = d["percentMale"].get<float>();
            if (d.contains("percentFemale")) demographics.percentFemale = d["percentFemale"].get<float>();
            if (d.contains("percentHermaphrodite")) demographics.percentHermaphrodite = d["percentHermaphrodite"].get<float>();
            if (d.contains("percentGynomorph")) demographics.percentGynomorph = d["percentGynomorph"].get<float>();
            if (d.contains("percentAndromorph")) demographics.percentAndromorph = d["percentAndromorph"].get<float>();
            if (d.contains("percentNull")) demographics.percentNull = d["percentNull"].get<float>();
            if (d.contains("percentYoungAdult")) demographics.percentYoungAdult = d["percentYoungAdult"].get<float>();
            if (d.contains("percentAdult")) demographics.percentAdult = d["percentAdult"].get<float>();
            if (d.contains("percentMature")) demographics.percentMature = d["percentMature"].get<float>();
            if (d.contains("percentElder")) demographics.percentElder = d["percentElder"].get<float>();
            if (d.contains("percentHuman")) demographics.percentHuman = d["percentHuman"].get<float>();
            if (d.contains("percentPartial")) demographics.percentPartial = d["percentPartial"].get<float>();
            if (d.contains("percentAnthro")) demographics.percentAnthro = d["percentAnthro"].get<float>();
            if (d.contains("percentFeral")) demographics.percentFeral = d["percentFeral"].get<float>();
        }

        if (j.contains("content"))
        {
            const auto& c = j["content"];
            if (c.contains("pregnancyEnabled")) content.pregnancyEnabled = c["pregnancyEnabled"].get<bool>();
            if (c.contains("lactationEnabled")) content.lactationEnabled = c["lactationEnabled"].get<bool>();
            if (c.contains("nonConEnabled")) content.nonConEnabled = c["nonConEnabled"].get<bool>();
            if (c.contains("publicSexEnabled")) content.publicSexEnabled = c["publicSexEnabled"].get<bool>();
            if (c.contains("extremeContentEnabled")) content.extremeContentEnabled = c["extremeContentEnabled"].get<bool>();
            if (c.contains("fluidMultiplier")) content.fluidMultiplier = c["fluidMultiplier"].get<float>();
            if (c.contains("transformationSpeedMultiplier")) content.transformationSpeedMultiplier = c["transformationSpeedMultiplier"].get<float>();
            if (c.contains("fetishPreferences")) content.fetishPreferences = c["fetishPreferences"].get<std::unordered_map<std::string, int>>();
        }

        if (j.contains("gameplay"))
        {
            const auto& g = j["gameplay"];
            if (g.contains("difficultyMultiplier")) gameplay.difficultyMultiplier = g["difficultyMultiplier"].get<float>();
            if (g.contains("difficultyLevel")) gameplay.difficultyLevel = g["difficultyLevel"].get<int>();
            if (g.contains("currencyLossOnDefeatPercent")) gameplay.currencyLossOnDefeatPercent = g["currencyLossOnDefeatPercent"].get<float>();
            if (g.contains("autoSaveOnMapChange")) gameplay.autoSaveOnMapChange = g["autoSaveOnMapChange"].get<bool>();
            if (g.contains("autoSaveOnSceneExit")) gameplay.autoSaveOnSceneExit = g["autoSaveOnSceneExit"].get<bool>();
            if (g.contains("autoSaveFrequency")) gameplay.autoSaveFrequency = g["autoSaveFrequency"].get<int>();
            if (g.contains("maxAutoSaves")) gameplay.maxAutoSaves = g["maxAutoSaves"].get<int>();
            if (g.contains("unitPreference")) gameplay.unitPreference = g["unitPreference"].get<std::string>();
            if (g.contains("genderPronounMode")) gameplay.genderPronounMode = g["genderPronounMode"].get<std::string>();
            if (g.contains("enchantmentInstability")) gameplay.enchantmentInstability = g["enchantmentInstability"].get<bool>();
            if (g.contains("badEndsEnabled")) gameplay.badEndsEnabled = g["badEndsEnabled"].get<bool>();
            if (g.contains("levelDrainEnabled")) gameplay.levelDrainEnabled = g["levelDrainEnabled"].get<bool>();
            if (g.contains("opportunisticAttackers")) gameplay.opportunisticAttackers = g["opportunisticAttackers"].get<bool>();
            if (g.contains("autoLoot")) gameplay.autoLoot = g["autoLoot"].get<bool>();
            if (g.contains("sharedEncyclopedia")) gameplay.sharedEncyclopedia = g["sharedEncyclopedia"].get<bool>();
            if (g.contains("stormInterruptions")) gameplay.stormInterruptions = g["stormInterruptions"].get<bool>();
        }

        if (j.contains("display"))
        {
            const auto& disp = j["display"];
            if (disp.contains("fontSize")) display.fontSize = disp["fontSize"].get<int>();
            if (disp.contains("fadeInEnabled")) display.fadeInEnabled = disp["fadeInEnabled"].get<bool>();
            if (disp.contains("showArtwork")) display.showArtwork = disp["showArtwork"].get<bool>();
            if (disp.contains("showThumbnails")) display.showThumbnails = disp["showThumbnails"].get<bool>();
            if (disp.contains("descriptionVerbosity")) display.descriptionVerbosity = disp["descriptionVerbosity"].get<int>();
            if (disp.contains("activeTheme")) display.activeTheme = disp["activeTheme"].get<std::string>();
            if (disp.contains("activeLayout")) display.activeLayout = disp["activeLayout"].get<std::string>();
        }
    }
};
