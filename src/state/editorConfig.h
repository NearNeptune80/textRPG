#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

enum class EditorTabId {
    IDENTITY,
    BODY,
    FACE_HAIR,
    BREASTS,
    GENITALIA,
    APPENDAGES,
    COSMETICS,
    WARDROBE,
    PERSONALITY,
    NAME_FINISH
};

struct EditorOptionRule {
    bool enabled = true;
    std::vector<std::string> allowedChoices = {}; // If empty, all choices allowed
    float minRange = 0.0f;
    float maxRange = 100.0f;
};

struct EditorConfig {
    std::string title = "Character Creation";
    std::string finishButtonText = "BEGIN ADVENTURE";
    bool isNewGameCreation = true;
    int costGold = 0;

    std::unordered_map<std::string, EditorOptionRule> rules;

    bool isOptionEnabled(const std::string& key) const {
        auto it = rules.find(key);
        return it != rules.end() && it->second.enabled;
    }

    const EditorOptionRule* getRule(const std::string& key) const {
        auto it = rules.find(key);
        if (it != rules.end()) return &(it->second);
        return nullptr;
    }

    std::vector<std::string> filterChoices(const std::string& key, const std::vector<std::string>& allChoices) const {
        auto it = rules.find(key);
        if (it != rules.end() && !it->second.allowedChoices.empty()) {
            std::vector<std::string> res;
            for (const auto& c : allChoices) {
                if (std::find(it->second.allowedChoices.begin(), it->second.allowedChoices.end(), c) != it->second.allowedChoices.end()) {
                    res.push_back(c);
                }
            }
            if (res.empty()) return allChoices;
            return res;
        }
        return allChoices;
    }

    struct HairstyleDef {
        std::string name;
        int minLengthCm;
    };

    struct Catalogs {
    private:
        struct CatalogData {
            std::vector<std::string> racialTypes;
            std::vector<std::string> minorRacesWithNone;
            std::vector<std::string> bodySizes;
            std::vector<std::string> muscleTiers;
            std::vector<std::string> footTypes;
            std::vector<std::string> legConfigs;
            std::vector<std::string> genitalPlacements;
            std::vector<std::string> bodyHairOptions;
            std::vector<std::string> eyeShapes;
            std::vector<std::string> hornTypes;
            std::vector<std::string> antennaTypes;
            std::vector<std::string> lipSizes;
            std::vector<std::string> wetnessLevels;
            std::vector<std::string> orificeModifiers;
            std::vector<std::string> penetrationModifiers;
            std::vector<std::string> cupSizes;
            std::vector<std::string> breastShapes;
            std::vector<std::string> nippleShapes;
            std::vector<std::string> areolaeShapes;
            std::vector<std::string> girthLevels;
            std::vector<std::string> wingSizes;
            std::vector<std::string> size5;
            std::vector<std::string> allHairStyles;
            std::vector<std::string> fluidFlavours;
            std::vector<std::string> fluidModifiers;
            std::vector<HairstyleDef> hairstyles;
            std::vector<std::string> innateTransformTraits;
            std::vector<std::string> innateTransformRaces;
            std::vector<std::string> crotchTypes;
        };

        static const CatalogData& get() {
            static CatalogData data = loadCatalogs();
            return data;
        }

        static CatalogData loadCatalogs() {
            CatalogData cd;
            std::ifstream file("data/catalogs/character_creation.json");
            if (file.is_open()) {
                try {
                    nlohmann::json j;
                    file >> j;
                    auto loadVec = [&](const char* key, std::vector<std::string>& out) {
                        if (j.contains(key) && j[key].is_array()) {
                            out = j[key].get<std::vector<std::string>>();
                        }
                    };
                    loadVec("racialTypes", cd.racialTypes);
                    loadVec("bodySizes", cd.bodySizes);
                    loadVec("muscleTiers", cd.muscleTiers);
                    loadVec("footTypes", cd.footTypes);
                    loadVec("legConfigs", cd.legConfigs);
                    loadVec("genitalPlacements", cd.genitalPlacements);
                    loadVec("bodyHairOptions", cd.bodyHairOptions);
                    loadVec("eyeShapes", cd.eyeShapes);
                    loadVec("hornTypes", cd.hornTypes);
                    loadVec("antennaTypes", cd.antennaTypes);
                    loadVec("lipSizes", cd.lipSizes);
                    loadVec("wetnessLevels", cd.wetnessLevels);
                    loadVec("orificeModifiers", cd.orificeModifiers);
                    loadVec("penetrationModifiers", cd.penetrationModifiers);
                    loadVec("cupSizes", cd.cupSizes);
                    loadVec("breastShapes", cd.breastShapes);
                    loadVec("nippleShapes", cd.nippleShapes);
                    loadVec("areolaeShapes", cd.areolaeShapes);
                    loadVec("girthLevels", cd.girthLevels);
                    loadVec("wingSizes", cd.wingSizes);
                    loadVec("size5", cd.size5);
                    loadVec("allHairStyles", cd.allHairStyles);
                    loadVec("fluidFlavours", cd.fluidFlavours);
                    loadVec("fluidModifiers", cd.fluidModifiers);
                    loadVec("innateTransformTraits", cd.innateTransformTraits);
                    loadVec("innateTransformRaces", cd.innateTransformRaces);
                    loadVec("crotchTypes", cd.crotchTypes);

                    if (j.contains("hairstyles") && j["hairstyles"].is_array()) {
                        for (const auto& h : j["hairstyles"]) {
                            cd.hairstyles.push_back({ h.value("name", ""), h.value("minLengthCm", 0) });
                        }
                    }
                } catch (...) {}
            }
            if (cd.crotchTypes.empty()) {
                cd.crotchTypes = { "None", "Bovine Udders", "Caprine Udders", "Feline Crotch-Boobs", "Canine Teats" };
            }
            cd.minorRacesWithNone = { "None" };
            cd.minorRacesWithNone.insert(cd.minorRacesWithNone.end(), cd.racialTypes.begin(), cd.racialTypes.end());
            return cd;
        }

    public:
        static const std::vector<std::string>& racialTypes() { return get().racialTypes; }
        static const std::vector<std::string>& minorRacesWithNone() { return get().minorRacesWithNone; }
        static const std::vector<std::string>& bodySizes() { return get().bodySizes; }
        static const std::vector<std::string>& muscleTiers() { return get().muscleTiers; }
        static const std::vector<std::string>& footTypes() { return get().footTypes; }
        static const std::vector<std::string>& legConfigs() { return get().legConfigs; }
        static const std::vector<std::string>& genitalPlacements() { return get().genitalPlacements; }
        static const std::vector<std::string>& bodyHairOptions() { return get().bodyHairOptions; }
        static const std::vector<std::string>& eyeShapes() { return get().eyeShapes; }
        static const std::vector<std::string>& hornTypes() { return get().hornTypes; }
        static const std::vector<std::string>& antennaTypes() { return get().antennaTypes; }
        static const std::vector<std::string>& lipSizes() { return get().lipSizes; }
        static const std::vector<std::string>& wetnessLevels() { return get().wetnessLevels; }
        static const std::vector<std::string>& orificeModifiers() { return get().orificeModifiers; }
        static const std::vector<std::string>& penetrationModifiers() { return get().penetrationModifiers; }
        static const std::vector<std::string>& cupSizes() { return get().cupSizes; }
        static const std::vector<std::string>& breastShapes() { return get().breastShapes; }
        static const std::vector<std::string>& nippleShapes() { return get().nippleShapes; }
        static const std::vector<std::string>& areolaeShapes() { return get().areolaeShapes; }
        static const std::vector<std::string>& girthLevels() { return get().girthLevels; }
        static const std::vector<std::string>& wingSizes() { return get().wingSizes; }
        static const std::vector<std::string>& size5() { return get().size5; }
        static const std::vector<std::string>& allHairStyles() { return get().allHairStyles; }
        static const std::vector<std::string>& fluidFlavours() { return get().fluidFlavours; }
        static const std::vector<std::string>& fluidModifiers() { return get().fluidModifiers; }
        static const std::vector<HairstyleDef>& hairstyles() { return get().hairstyles; }
        static const std::vector<std::string>& innateTransformTraits() { return get().innateTransformTraits; }
        static const std::vector<std::string>& innateTransformRaces() { return get().innateTransformRaces; }
        static const std::vector<std::string>& crotchTypes() { return get().crotchTypes; }
    };

    static const std::vector<HairstyleDef>& getAllHairstyles() {
        return Catalogs::hairstyles();
    }

    static std::vector<std::string> getValidHairstyles(int hairLengthCm) {
        std::vector<std::string> valid;
        for (const auto& h : getAllHairstyles()) {
            if (hairLengthCm >= h.minLengthCm) {
                valid.push_back(h.name);
            }
        }
        if (valid.empty()) valid.push_back("Bald");
        return valid;
    }

    static std::string calculateBodyShape(const std::string& muscle, const std::string& bodySize) {
        auto toLower = [](std::string str) {
            for (char& c : str) c = std::tolower(static_cast<unsigned char>(c));
            return str;
        };
        std::string mStr = toLower(muscle);
        std::string sStr = toLower(bodySize);

        int m = (mStr == "soft") ? 0 : (mStr == "lightly muscled" ? 1 : (mStr == "toned" ? 2 : (mStr == "muscular" ? 3 : 4)));
        int s = (sStr == "skinny") ? 0 : (sStr == "slender") ? 1 : (sStr == "average") ? 2 : (sStr == "muscular" || sStr == "large") ? 3 : 4;

        if (s == 0) return (m >= 3) ? "Wiry / Lean" : (m >= 2 ? "Slender / Toned" : "Frail / Delicate");
        if (s == 1) return (m >= 3) ? "Athletic / Cut" : (m >= 2 ? "Toned / Fit" : "Slender / Graceful");
        if (s == 2) return (m >= 3) ? "Muscular / Athletic" : (m >= 2 ? "Fit / Defined" : "Average / Smooth");
        if (s == 3) return (m >= 4) ? "Heroic / Bodybuilder" : (m >= 2 ? "Powerful / Broad" : "Stocky / Solid");
        return (m >= 3) ? "Colossal / Juggernaut" : (m >= 2 ? "Brawny / Heavy" : "Full-figured / Chubby");
    }

    bool hasAnyOptionInList(const std::vector<std::string>& keys) const {
        for (const auto& k : keys) {
            if (isOptionEnabled(k)) return true;
        }
        return false;
    }

    static EditorConfig newGamePreset() {
        EditorConfig cfg;
        cfg.title = "Character Creation";
        cfg.finishButtonText = "BEGIN ADVENTURE";
        cfg.isNewGameCreation = true;
        
        cfg.rules["gender"] = { .enabled = true };
        cfg.rules["femininity"] = { .enabled = true };
        cfg.rules["orientation"] = { .enabled = true };
        cfg.rules["start_month"] = { .enabled = true };
        cfg.rules["birth_month"] = { .enabled = true };

        cfg.rules["height"] = { .enabled = true, .minRange = 140.0f, .maxRange = 210.0f };
        cfg.rules["body_size"] = { .enabled = true, .allowedChoices = {"Skinny", "Slender", "Average", "Muscular", "Chubby"} };
        cfg.rules["muscle"] = { .enabled = true, .allowedChoices = {"Soft", "Lightly muscled", "Toned", "Muscular", "Ripped"} };
        cfg.rules["skin_tone"] = { .enabled = true, .allowedChoices = {"Fair", "Pale", "Tan", "Olive", "Dark", "Ebony"} };
        cfg.rules["chest_size"] = { .enabled = true, .allowedChoices = {"Flat", "A", "B", "C", "D", "DD"} };
        cfg.rules["genitals"] = { .enabled = true, .minRange = 8.0f, .maxRange = 24.0f };

        cfg.rules["eye_color"] = { .enabled = true, .allowedChoices = {"Blue", "Green", "Brown", "Amber", "Hazel", "Red", "Violet", "Black"} };
        cfg.rules["hair_style"] = { .enabled = true };
        cfg.rules["hair_color"] = { .enabled = true, .allowedChoices = {"Black", "Dark Brown", "Auburn", "Blonde", "Platinum", "Silver", "Red"} };
        cfg.rules["hair_length"] = { .enabled = true, .minRange = 2.0f, .maxRange = 120.0f };
        cfg.rules["ear_type"] = { .enabled = true, .allowedChoices = {"Human"} };

        cfg.rules["wardrobe"] = { .enabled = true };

        cfg.rules["personality_traits"] = { .enabled = true };

        cfg.rules["first_name"] = { .enabled = true };
        cfg.rules["surname"] = { .enabled = true };

        return cfg;
    }

    static EditorConfig hairSalonPreset() {
        EditorConfig cfg;
        cfg.title = "Hair Salon";
        cfg.finishButtonText = "CONFIRM NEW LOOK";
        cfg.isNewGameCreation = false;
        cfg.rules["hair_style"] = { .enabled = true };
        cfg.rules["hair_color"] = { .enabled = true };
        cfg.rules["hair_length"] = { .enabled = true, .minRange = 2.0f, .maxRange = 120.0f };
        return cfg;
    }

    static EditorConfig tattooPiercingPreset() {
        EditorConfig cfg;
        cfg.title = "Body Modification Studio";
        cfg.finishButtonText = "APPLY MODIFICATIONS";
        cfg.isNewGameCreation = false;
        cfg.rules["piercings"] = { .enabled = true };
        cfg.rules["tattoos"] = { .enabled = true };
        return cfg;
    }

    static EditorConfig fullTransformationPreset() {
        EditorConfig cfg;
        cfg.title = "Arcane Body Transformation";
        cfg.finishButtonText = "APPLY TRANSFORMATIONS";
        cfg.isNewGameCreation = false;
        
        cfg.rules["gender"] = { .enabled = true };
        cfg.rules["femininity"] = { .enabled = true };
        cfg.rules["orientation"] = { .enabled = true };
        cfg.rules["height"] = { .enabled = true, .minRange = 100.0f, .maxRange = 260.0f };
        cfg.rules["body_size"] = { .enabled = true };
        cfg.rules["muscle"] = { .enabled = true };
        cfg.rules["skin_tone"] = { .enabled = true };
        cfg.rules["skin_covering"] = { .enabled = true, .allowedChoices = {"Skin", "Fur", "Scales", "Feathers", "Chitin"} };
        cfg.rules["chest_size"] = { .enabled = true, .allowedChoices = {"Flat", "A", "B", "C", "D", "DD", "E", "F", "G", "H"} };
        cfg.rules["genitals"] = { .enabled = true, .minRange = 5.0f, .maxRange = 40.0f };
        cfg.rules["eye_color"] = { .enabled = true };
        cfg.rules["hair_style"] = { .enabled = true };
        cfg.rules["hair_color"] = { .enabled = true };
        cfg.rules["hair_length"] = { .enabled = true, .minRange = 0.0f, .maxRange = 150.0f };
        cfg.rules["ear_type"] = { .enabled = true, .allowedChoices = {"Human", "Cat", "Dog", "Fox", "Elf", "Demon", "Cow", "Rabbit", "Dragon"} };
        cfg.rules["horns"] = { .enabled = true, .allowedChoices = {"None", "Demon", "Dragon", "Goat", "Bull", "Unicorn"} };
        cfg.rules["wings"] = { .enabled = true, .allowedChoices = {"None", "Feathered", "Leathery", "Insectoid", "Energy"} };
        cfg.rules["tails"] = { .enabled = true, .allowedChoices = {"None", "Cat", "Dog", "Fox", "Horse", "Demon", "Dragon"} };
        cfg.rules["piercings"] = { .enabled = true };
        cfg.rules["tattoos"] = { .enabled = true };
        cfg.rules["makeup"] = { .enabled = true };
        cfg.rules["personality_traits"] = { .enabled = true };
        cfg.rules["first_name"] = { .enabled = true };
        cfg.rules["surname"] = { .enabled = true };
        return cfg;
    }
};
