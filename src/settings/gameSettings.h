#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <nlohmann/json.hpp>

#include "common/enums.h"

struct DemographicSettings
{
    float percentHetero = 40.0f;
    float percentBi = 30.0f;
    float percentHomo = 20.0f;
    float percentAsexual = 10.0f;

    float percentMale = 30.0f;
    float percentFemale = 40.0f;
    float percentHermaphrodite = 15.0f;
    float percentGynomorph = 7.0f;
    float percentAndromorph = 5.0f;
    float percentNull = 3.0f;

    float percentYoungAdult = 40.0f;
    float percentAdult = 35.0f;
    float percentMature = 20.0f;
    float percentElder = 5.0f;

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
    int rollAge(float roll01) const
    {
        float total = percentYoungAdult + percentAdult + percentMature + percentElder;
        if (total <= 0.0f) total = 100.0f;
        float val = roll01 * total;
        if (val < percentYoungAdult) {
            float sub = val / std::max(1.0f, percentYoungAdult);
            return 18 + static_cast<int>(sub * 7);;
        }
        val -= percentYoungAdult;
        if (val < percentAdult) {
            float sub = val / std::max(1.0f, percentAdult);
            return 26 + static_cast<int>(sub * 14);;
        }
        val -= percentAdult;
        if (val < percentMature) {
            float sub = val / std::max(1.0f, percentMature);
            return 41 + static_cast<int>(sub * 19);;
        }
        val -= percentMature;
        float sub = val / std::max(1.0f, percentElder);
        return 61 + static_cast<int>(sub * 25);;
    }

    std::string rollFurryStage(float roll01) const
    {
        float total = percentHuman + percentPartial + percentAnthro + percentFeral;
        if (total <= 0.0f) total = 100.0f;
        float val = roll01 * total;
        if (val < percentHuman) return "Human";
        val -= percentHuman;
        if (val < percentPartial) return "Partial";
        val -= percentPartial;
        if (val < percentAnthro) return "Anthro";
        return "Feral";
    }
};

enum class ContentFilterMode
{
    DROPDOWN = 0,     // Inline collapsible dropdown box with warning header
    WARN_CONFIRM = 1, // Interstitial confirmation warning prompt
    BLOCK_AND_SKIP = 2 // Completely hide, block, or bypass
};

inline std::string contentFilterModeToString(ContentFilterMode mode)
{
    switch (mode)
    {
        case ContentFilterMode::DROPDOWN: return "Dropdown";
        case ContentFilterMode::WARN_CONFIRM: return "Warn & Confirm";
        case ContentFilterMode::BLOCK_AND_SKIP: return "Block & Skip";
    }
    return "Dropdown";
}

inline ContentFilterMode stringToContentFilterMode(const std::string& str)
{
    if (str == "Warn & Confirm" || str == "WARN_CONFIRM") return ContentFilterMode::WARN_CONFIRM;
    if (str == "Block & Skip" || str == "BLOCK_AND_SKIP") return ContentFilterMode::BLOCK_AND_SKIP;
    return ContentFilterMode::DROPDOWN;
}

enum class ContentToggleState
{
    OFF = 0,
    WARN = 1,
    ON = 2
};

inline std::string contentToggleStateToString(ContentToggleState state)
{
    switch (state)
    {
        case ContentToggleState::OFF: return "OFF";
        case ContentToggleState::WARN: return "WARN";
        case ContentToggleState::ON: return "ON";
    }
    return "OFF";
}

inline ContentToggleState stringToContentToggleState(const std::string& str)
{
    if (str == "ON" || str == "on" || str == "true" || str == "ENABLED" || str == "enabled") return ContentToggleState::ON;
    if (str == "WARN" || str == "warn" || str == "WARNING" || str == "warning") return ContentToggleState::WARN;
    return ContentToggleState::OFF;
}

inline ContentToggleState intToContentToggleState(int val)
{
    if (val >= 2) return ContentToggleState::ON;
    if (val == 1) return ContentToggleState::WARN;
    return ContentToggleState::OFF;
}

struct ContentSettings
{
    ContentToggleState pregnancyState = ContentToggleState::ON;
    ContentToggleState lactationState = ContentToggleState::ON;
    ContentToggleState nonConState = ContentToggleState::OFF;
    ContentToggleState publicSexState = ContentToggleState::ON;
    ContentToggleState extremeContentState = ContentToggleState::OFF;
    ContentToggleState watersportsState = ContentToggleState::OFF;
    ContentToggleState spittingState = ContentToggleState::ON;
    ContentToggleState forcedTfState = ContentToggleState::OFF;
    ContentToggleState tentaclesState = ContentToggleState::ON;
    ContentToggleState bdsmState = ContentToggleState::ON;
    ContentToggleState incestState = ContentToggleState::OFF;
    ContentToggleState sizeDifferenceState = ContentToggleState::ON;
    ContentToggleState prolapseState = ContentToggleState::OFF;
    ContentToggleState aphrodisiacsState = ContentToggleState::ON;
    ContentToggleState hypnosisState = ContentToggleState::OFF;
    ContentToggleState cumInflationState = ContentToggleState::OFF;
    ContentToggleState feralState = ContentToggleState::OFF;

    bool pregnancyEnabled = true;
    bool lactationEnabled = true;
    bool nonConEnabled = false;
    bool publicSexEnabled = true;
    bool extremeContentEnabled = false;
    bool watersportsEnabled = false;
    bool spittingEnabled = true;
    bool forcedTfEnabled = false;
    bool tentaclesEnabled = true;
    bool bdsmEnabled = true;
    bool incestEnabled = false;
    bool sizeDifferenceEnabled = true;
    bool prolapseEnabled = false;
    bool aphrodisiacsEnabled = true;
    bool hypnosisEnabled = false;
    bool cumInflationEnabled = false;
    bool feralEnabled = false;
    std::string furryGenitalStyle = "Humanoid"; // "Humanoid" vs "Animalistic"

    int getFetishRating(const std::string& name) const
    {
        auto it = fetishPreferences.find(name);
        if (it != fetishPreferences.end()) return it->second;

        std::string norm = name;
        std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& [k, v] : fetishPreferences)
        {
            std::string kNorm = k;
            std::transform(kNorm.begin(), kNorm.end(), kNorm.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (kNorm == norm || kNorm.find(norm) != std::string::npos || norm.find(kNorm) != std::string::npos)
            {
                return v;
            }
        }
        return 3;
    }

    void setFetishPreference(const std::string& name, int rating)
    {
        fetishPreferences[name] = rating;

        if (name == "Tentacles & Monsters") fetishPreferences["Tentacles"] = rating;
        else if (name == "Tentacles") fetishPreferences["Tentacles & Monsters"] = rating;

        if (name == "Exhibitionism / Public Sex" || name == "Public Sex Exposure") fetishPreferences["Exhibitionism"] = rating;
        else if (name == "Exhibitionism") { fetishPreferences["Exhibitionism / Public Sex"] = rating; fetishPreferences["Public Sex Exposure"] = rating; }

        if (name == "Transformations") fetishPreferences["Forced Transformation"] = rating;
        else if (name == "Forced Transformation") fetishPreferences["Transformations"] = rating;

        if (name == "Insemination / Cum") fetishPreferences["Insemination"] = rating;
        else if (name == "Insemination") fetishPreferences["Insemination / Cum"] = rating;

        if (name == "BDSM & Restraints") { fetishPreferences["Bondage"] = rating; fetishPreferences["BDSM / Sadism"] = rating; }

        syncBools();
    }

    void syncBools()
    {
        pregnancyEnabled = (getFetishRating("Pregnancy") > 0);
        lactationEnabled = (getFetishRating("Lactation") > 0);
        nonConEnabled = (getFetishRating("Non-Consent") > 0);
        publicSexEnabled = (getFetishRating("Exhibitionism / Public Sex") > 0 || getFetishRating("Public Sex") > 0 || getFetishRating("Exhibitionism") > 0);
        extremeContentEnabled = (getFetishRating("Sadistic sex / Extreme") > 0 || getFetishRating("Sadism") > 0);
        watersportsEnabled = (getFetishRating("Watersports") > 0);
        spittingEnabled = (getFetishRating("Spitting") > 0);
        forcedTfEnabled = (getFetishRating("Transformations") > 0 || getFetishRating("Forced Transformation") > 0);
        tentaclesEnabled = (getFetishRating("Tentacles & Monsters") > 0 || getFetishRating("Tentacles") > 0);
        bdsmEnabled = (getFetishRating("BDSM & Restraints") > 0 || getFetishRating("BDSM") > 0 || getFetishRating("Bondage") > 0);
        incestEnabled = (getFetishRating("Incest") > 0);
        sizeDifferenceEnabled = (getFetishRating("Size Difference") > 0);
        prolapseEnabled = (getFetishRating("Prolapse") > 0);
        aphrodisiacsEnabled = (getFetishRating("Aphrodisiacs & Drugs") > 0);
        hypnosisEnabled = (getFetishRating("Hypnosis & Mind Control") > 0);
        cumInflationEnabled = (getFetishRating("Cum Inflation & Expansion") > 0);
        feralEnabled = (getFetishRating("Feral / Bestiality") > 0);

        pregnancyState = pregnancyEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        lactationState = lactationEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        nonConState = nonConEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        publicSexState = publicSexEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        extremeContentState = extremeContentEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        watersportsState = watersportsEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        spittingState = spittingEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        forcedTfState = forcedTfEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        tentaclesState = tentaclesEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        bdsmState = bdsmEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        incestState = incestEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        sizeDifferenceState = sizeDifferenceEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        prolapseState = prolapseEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        aphrodisiacsState = aphrodisiacsEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        hypnosisState = hypnosisEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        cumInflationState = cumInflationEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        feralState = feralEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
    }

    void syncStatesFromBools()
    {
        pregnancyState = pregnancyEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        lactationState = lactationEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        nonConState = nonConEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        publicSexState = publicSexEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        extremeContentState = extremeContentEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        watersportsState = watersportsEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        spittingState = spittingEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        forcedTfState = forcedTfEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        tentaclesState = tentaclesEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        bdsmState = bdsmEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        incestState = incestEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        sizeDifferenceState = sizeDifferenceEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        prolapseState = prolapseEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        aphrodisiacsState = aphrodisiacsEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        hypnosisState = hypnosisEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        cumInflationState = cumInflationEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
        feralState = feralEnabled ? ContentToggleState::ON : ContentToggleState::OFF;
    }

    void setToggle(ContentToggleState& stateVar, bool& boolVar, ContentToggleState newState)
    {
        stateVar = newState;
        boolVar = (newState != ContentToggleState::OFF);
    }

    float fluidMultiplier = 1.0f;
    float transformationSpeedMultiplier = 1.0f;
    ContentFilterMode contentFilterMode = ContentFilterMode::DROPDOWN;

    static std::unordered_map<std::string, int> loadDefaultFetishPreferences()
    {
        std::unordered_map<std::string, int> prefs;
        std::ifstream file("data/catalogs/fetishes.json");
        if (file.is_open())
        {
            try
            {
                nlohmann::json j;
                file >> j;
                if (j.contains("defaultPreferences") && j["defaultPreferences"].is_object())
                {
                    prefs = j["defaultPreferences"].get<std::unordered_map<std::string, int>>();
                }
                else if (j.contains("fetishes") && j["fetishes"].is_array())
                {
                    int defRating = j.value("defaultRating", 3);
                    for (const auto& item : j["fetishes"])
                    {
                        if (item.contains("name"))
                        {
                            prefs[item["name"].get<std::string>()] = defRating;
                        }
                    }
                }
            }
            catch (...) {}
        }
        return prefs;
    }

    struct FetishCatalogEntry {
        std::string id;
        std::string name;
        std::string category;
        std::string description;
    };

    static const std::vector<FetishCatalogEntry>& getFetishCatalog()
    {
        static const std::vector<FetishCatalogEntry> catalog = []() {
            std::vector<FetishCatalogEntry> list;
            std::ifstream file("data/catalogs/fetishes.json");
            if (file.is_open())
            {
                try
                {
                    nlohmann::json j;
                    file >> j;
                    if (j.contains("fetishes") && j["fetishes"].is_array())
                    {
                        for (const auto& f : j["fetishes"])
                        {
                            FetishCatalogEntry entry;
                            entry.id = f.value("id", "");
                            entry.name = f.value("name", "");
                            entry.category = f.value("category", "");
                            entry.description = f.value("description", "");
                            list.push_back(entry);
                        }
                    }
                }
                catch (...) {}
            }
            return list;
        }();
        return catalog;
    }

    static const std::vector<std::string>& getOptionsFetishList()
    {
        static const std::vector<std::string> list = []() {
            std::vector<std::string> names;
            std::ifstream file("data/catalogs/fetishes.json");
            if (file.is_open())
            {
                try
                {
                    nlohmann::json j;
                    file >> j;
                    if (j.contains("defaultPreferences") && j["defaultPreferences"].is_object())
                    {
                        for (const auto& [k, v] : j["defaultPreferences"].items())
                        {
                            names.push_back(k);
                        }
                        std::sort(names.begin(), names.end());
                    }
                }
                catch (...) {}
            }
            return names;
        }();
        return list;
    }

    std::unordered_map<std::string, int> fetishPreferences = loadDefaultFetishPreferences();
};

struct GameplaySettings
{
    float difficultyMultiplier = 1.0f;
    int difficultyLevel = 2; // 0=Very Easy (0.75x), 1=Easy (0.85x), 2=Normal (1.0x), 3=Hard (1.5x), 4=Nightmare (2.5x)
    float currencyLossOnDefeatPercent = 0.15f;
    bool autoSaveOnMapChange = true;
    bool autoSaveOnSceneExit = true;
    int maxAutoSaves = 3;
    std::string unitPreference = "Metric"; // Metric vs Imperial
    std::string genderPronounMode = "Normal"; // Normal vs Custom
    bool badEndsEnabled = true;
    bool levelDrainEnabled = true;
    bool opportunisticAttackers = true;
    bool autoLoot = true;
    bool sharedEncyclopedia = false;
};

struct DisplaySettings
{
    int fontSize = 18; // 12..36
    float uiScaleMultiplier = 1.0f; // 1.0f, 1.25f, 1.5f, 0.8f
    bool showArtwork = true;
    bool showThumbnails = true;
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
        ContentSettings cSync = content;
        auto syncToSerialize = [](ContentToggleState& st, bool en) {
            if (!en) st = ContentToggleState::OFF;
            else if (st == ContentToggleState::OFF) st = ContentToggleState::ON;
        };
        syncToSerialize(cSync.pregnancyState, cSync.pregnancyEnabled);
        syncToSerialize(cSync.lactationState, cSync.lactationEnabled);
        syncToSerialize(cSync.nonConState, cSync.nonConEnabled);
        syncToSerialize(cSync.publicSexState, cSync.publicSexEnabled);
        syncToSerialize(cSync.extremeContentState, cSync.extremeContentEnabled);
        syncToSerialize(cSync.watersportsState, cSync.watersportsEnabled);
        syncToSerialize(cSync.spittingState, cSync.spittingEnabled);
        syncToSerialize(cSync.forcedTfState, cSync.forcedTfEnabled);
        syncToSerialize(cSync.tentaclesState, cSync.tentaclesEnabled);
        syncToSerialize(cSync.bdsmState, cSync.bdsmEnabled);
        syncToSerialize(cSync.incestState, cSync.incestEnabled);
        syncToSerialize(cSync.sizeDifferenceState, cSync.sizeDifferenceEnabled);
        syncToSerialize(cSync.prolapseState, cSync.prolapseEnabled);
        syncToSerialize(cSync.aphrodisiacsState, cSync.aphrodisiacsEnabled);

        syncToSerialize(cSync.hypnosisState, cSync.hypnosisEnabled);
        syncToSerialize(cSync.cumInflationState, cSync.cumInflationEnabled);
        syncToSerialize(cSync.feralState, cSync.feralEnabled);

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
                {"pregnancyEnabled", cSync.pregnancyEnabled},
                {"pregnancyState", static_cast<int>(cSync.pregnancyState)},
                {"lactationEnabled", cSync.lactationEnabled},
                {"lactationState", static_cast<int>(cSync.lactationState)},
                {"nonConEnabled", cSync.nonConEnabled},
                {"nonConState", static_cast<int>(cSync.nonConState)},
                {"publicSexEnabled", cSync.publicSexEnabled},
                {"publicSexState", static_cast<int>(cSync.publicSexState)},
                {"extremeContentEnabled", cSync.extremeContentEnabled},
                {"extremeContentState", static_cast<int>(cSync.extremeContentState)},
                {"watersportsEnabled", cSync.watersportsEnabled},
                {"watersportsState", static_cast<int>(cSync.watersportsState)},
                {"spittingEnabled", cSync.spittingEnabled},
                {"spittingState", static_cast<int>(cSync.spittingState)},
                {"forcedTfEnabled", cSync.forcedTfEnabled},
                {"forcedTfState", static_cast<int>(cSync.forcedTfState)},
                {"tentaclesEnabled", cSync.tentaclesEnabled},
                {"tentaclesState", static_cast<int>(cSync.tentaclesState)},
                {"bdsmEnabled", cSync.bdsmEnabled},
                {"bdsmState", static_cast<int>(cSync.bdsmState)},
                {"incestEnabled", cSync.incestEnabled},
                {"incestState", static_cast<int>(cSync.incestState)},
                {"sizeDifferenceEnabled", cSync.sizeDifferenceEnabled},
                {"sizeDifferenceState", static_cast<int>(cSync.sizeDifferenceState)},
                {"prolapseEnabled", cSync.prolapseEnabled},
                {"prolapseState", static_cast<int>(cSync.prolapseState)},
                {"aphrodisiacsEnabled", cSync.aphrodisiacsEnabled},
                {"aphrodisiacsState", static_cast<int>(cSync.aphrodisiacsState)},
                {"hypnosisEnabled", cSync.hypnosisEnabled},
                {"hypnosisState", static_cast<int>(cSync.hypnosisState)},
                {"cumInflationEnabled", cSync.cumInflationEnabled},
                {"cumInflationState", static_cast<int>(cSync.cumInflationState)},
                {"feralEnabled", cSync.feralEnabled},
                {"feralState", static_cast<int>(cSync.feralState)},
                {"furryGenitalStyle", content.furryGenitalStyle},
                {"contentFilterMode", static_cast<int>(content.contentFilterMode)},
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
                {"maxAutoSaves", gameplay.maxAutoSaves},
                {"unitPreference", gameplay.unitPreference},
                {"genderPronounMode", gameplay.genderPronounMode},
                {"badEndsEnabled", gameplay.badEndsEnabled},
                {"levelDrainEnabled", gameplay.levelDrainEnabled},
                {"opportunisticAttackers", gameplay.opportunisticAttackers},
                {"autoLoot", gameplay.autoLoot},
                {"sharedEncyclopedia", gameplay.sharedEncyclopedia}
            }},
            {"display", {
                {"fontSize", display.fontSize},
                {"uiScaleMultiplier", display.uiScaleMultiplier},
                {"showArtwork", display.showArtwork},
                {"showThumbnails", display.showThumbnails},
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
            auto parseToggle = [&](const std::string& stateKey, const std::string& boolKey, ContentToggleState& stateVar, bool& boolVar) {
                if (c.contains(stateKey))
                {
                    if (c[stateKey].is_number()) stateVar = intToContentToggleState(c[stateKey].get<int>());
                    else if (c[stateKey].is_string()) stateVar = stringToContentToggleState(c[stateKey].get<std::string>());
                    boolVar = (stateVar != ContentToggleState::OFF);
                }
                if (c.contains(boolKey))
                {
                    bool b = c[boolKey].get<bool>();
                    boolVar = b;
                    if (!b) stateVar = ContentToggleState::OFF;
                    else if (stateVar == ContentToggleState::OFF) stateVar = ContentToggleState::ON;
                }
            };

            parseToggle("pregnancyState", "pregnancyEnabled", content.pregnancyState, content.pregnancyEnabled);
            parseToggle("lactationState", "lactationEnabled", content.lactationState, content.lactationEnabled);
            parseToggle("nonConState", "nonConEnabled", content.nonConState, content.nonConEnabled);
            parseToggle("publicSexState", "publicSexEnabled", content.publicSexState, content.publicSexEnabled);
            parseToggle("extremeContentState", "extremeContentEnabled", content.extremeContentState, content.extremeContentEnabled);
            parseToggle("watersportsState", "watersportsEnabled", content.watersportsState, content.watersportsEnabled);
            parseToggle("spittingState", "spittingEnabled", content.spittingState, content.spittingEnabled);
            parseToggle("forcedTfState", "forcedTfEnabled", content.forcedTfState, content.forcedTfEnabled);
            parseToggle("tentaclesState", "tentaclesEnabled", content.tentaclesState, content.tentaclesEnabled);
            parseToggle("bdsmState", "bdsmEnabled", content.bdsmState, content.bdsmEnabled);
            parseToggle("incestState", "incestEnabled", content.incestState, content.incestEnabled);
            parseToggle("sizeDifferenceState", "sizeDifferenceEnabled", content.sizeDifferenceState, content.sizeDifferenceEnabled);
            parseToggle("prolapseState", "prolapseEnabled", content.prolapseState, content.prolapseEnabled);
            parseToggle("aphrodisiacsState", "aphrodisiacsEnabled", content.aphrodisiacsState, content.aphrodisiacsEnabled);
            parseToggle("hypnosisState", "hypnosisEnabled", content.hypnosisState, content.hypnosisEnabled);
            parseToggle("cumInflationState", "cumInflationEnabled", content.cumInflationState, content.cumInflationEnabled);
            parseToggle("feralState", "feralEnabled", content.feralState, content.feralEnabled);
            if (c.contains("furryGenitalStyle")) content.furryGenitalStyle = c["furryGenitalStyle"].get<std::string>();
            if (c.contains("contentFilterMode")) content.contentFilterMode = static_cast<ContentFilterMode>(c["contentFilterMode"].get<int>());
            if (c.contains("fluidMultiplier")) content.fluidMultiplier = c["fluidMultiplier"].get<float>();
            if (c.contains("transformationSpeedMultiplier")) content.transformationSpeedMultiplier = c["transformationSpeedMultiplier"].get<float>();
            if (c.contains("fetishPreferences"))
            {
                auto loadedFetishes = c["fetishPreferences"].get<std::unordered_map<std::string, int>>();
                for (const auto& [k, v] : loadedFetishes)
                {
                    content.fetishPreferences[k] = v;
                }
            }
        }

        if (j.contains("gameplay"))
        {
            const auto& g = j["gameplay"];
            if (g.contains("difficultyMultiplier")) gameplay.difficultyMultiplier = g["difficultyMultiplier"].get<float>();
            if (g.contains("difficultyLevel")) gameplay.difficultyLevel = g["difficultyLevel"].get<int>();
            if (g.contains("currencyLossOnDefeatPercent")) gameplay.currencyLossOnDefeatPercent = g["currencyLossOnDefeatPercent"].get<float>();
            if (g.contains("autoSaveOnMapChange")) gameplay.autoSaveOnMapChange = g["autoSaveOnMapChange"].get<bool>();
            if (g.contains("autoSaveOnSceneExit")) gameplay.autoSaveOnSceneExit = g["autoSaveOnSceneExit"].get<bool>();
            if (g.contains("maxAutoSaves")) gameplay.maxAutoSaves = g["maxAutoSaves"].get<int>();
            if (g.contains("unitPreference")) gameplay.unitPreference = g["unitPreference"].get<std::string>();
            if (g.contains("genderPronounMode")) gameplay.genderPronounMode = g["genderPronounMode"].get<std::string>();
            if (g.contains("badEndsEnabled")) gameplay.badEndsEnabled = g["badEndsEnabled"].get<bool>();
            if (g.contains("levelDrainEnabled")) gameplay.levelDrainEnabled = g["levelDrainEnabled"].get<bool>();
            if (g.contains("opportunisticAttackers")) gameplay.opportunisticAttackers = g["opportunisticAttackers"].get<bool>();
            if (g.contains("autoLoot")) gameplay.autoLoot = g["autoLoot"].get<bool>();
            if (g.contains("sharedEncyclopedia")) gameplay.sharedEncyclopedia = g["sharedEncyclopedia"].get<bool>();
        }

        if (j.contains("display"))
        {
            const auto& disp = j["display"];
            if (disp.contains("fontSize")) display.fontSize = disp["fontSize"].get<int>();
            if (disp.contains("uiScaleMultiplier")) display.uiScaleMultiplier = disp["uiScaleMultiplier"].get<float>();
            if (disp.contains("showArtwork")) display.showArtwork = disp["showArtwork"].get<bool>();
            if (disp.contains("showThumbnails")) display.showThumbnails = disp["showThumbnails"].get<bool>();
            if (disp.contains("activeTheme")) display.activeTheme = disp["activeTheme"].get<std::string>();
            if (disp.contains("activeLayout")) display.activeLayout = disp["activeLayout"].get<std::string>();
        }
    }
};
