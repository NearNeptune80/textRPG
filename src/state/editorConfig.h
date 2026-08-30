#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

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

    static const std::vector<HairstyleDef>& getAllHairstyles() {
        static const std::vector<HairstyleDef> s_styles = {
            { "Bald", 0 },
            { "Stubble", 1 },
            { "Short", 3 },
            { "Messy", 3 },
            { "Slicked-back", 5 },
            { "Pixie-cut", 5 },
            { "Mohawk", 5 },
            { "Afro", 5 },
            { "Sidecut", 5 },
            { "Bob Cut", 10 },
            { "Straight", 15 },
            { "Wavy", 15 },
            { "Curly", 15 },
            { "Topknot", 15 },
            { "Dreadlocks", 15 },
            { "Shoulder-length", 20 },
            { "Ponytail", 20 },
            { "Low Ponytail", 20 },
            { "Bun", 25 },
            { "Chignon", 25 },
            { "Braided", 25 },
            { "Twin Tails", 25 },
            { "Twin Braids", 25 },
            { "Crown Braid", 30 },
            { "Hime Cut", 30 },
            { "Floor-length", 80 }
        };
        return s_styles;
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
        
        // Identity
        cfg.rules["gender"] = { .enabled = true };
        cfg.rules["femininity"] = { .enabled = true };
        cfg.rules["orientation"] = { .enabled = true };
        cfg.rules["start_month"] = { .enabled = true };

        // Body
        cfg.rules["height"] = { .enabled = true, .minRange = 140.0f, .maxRange = 210.0f };
        cfg.rules["body_size"] = { .enabled = true, .allowedChoices = {"Skinny", "Slender", "Average", "Muscular", "Chubby"} };
        cfg.rules["muscle"] = { .enabled = true, .allowedChoices = {"Soft", "Lightly muscled", "Toned", "Muscular", "Ripped"} };
        cfg.rules["skin_tone"] = { .enabled = true, .allowedChoices = {"Fair", "Pale", "Tan", "Olive", "Dark", "Ebony"} };
        cfg.rules["chest_size"] = { .enabled = true, .allowedChoices = {"Flat", "A", "B", "C", "D", "DD"} };
        cfg.rules["genitals"] = { .enabled = true, .minRange = 8.0f, .maxRange = 24.0f };

        // Face & Hair
        cfg.rules["eye_color"] = { .enabled = true, .allowedChoices = {"Blue", "Green", "Brown", "Amber", "Hazel", "Red", "Violet", "Black"} };
        cfg.rules["hair_style"] = { .enabled = true };
        cfg.rules["hair_color"] = { .enabled = true, .allowedChoices = {"Black", "Dark Brown", "Auburn", "Blonde", "Platinum", "Silver", "Red"} };
        cfg.rules["hair_length"] = { .enabled = true, .minRange = 2.0f, .maxRange = 120.0f };
        cfg.rules["ear_type"] = { .enabled = true, .allowedChoices = {"Human"} };

        // Wardrobe
        cfg.rules["wardrobe"] = { .enabled = true };

        // Personality
        cfg.rules["personality_traits"] = { .enabled = true };

        // Name & Finish
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
        
        // All options enabled with full exotic choices
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
