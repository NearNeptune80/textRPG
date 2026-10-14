#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct BodyTemplate
{
    std::string id;
    std::string species = "Human";
    std::string genderArchetype = "FEMALE";
    float heightCm = 170.0f;
    std::string bodyBuild = "Average";
    int breastCupSize = 0;
    float breastFluidMl = 0.0f;
    float breastMaxFluidMl = 0.0f;
    bool isLactating = false;
    std::string groinType = "Vagina";
    float groinLengthCm = 0.0f;
    float groinDiameterCm = 0.0f;
    bool knot = false;
    int groinWetness = 1;
    float groinFluidMl = 0.0f;
    float groinMaxFluidMl = 0.0f;
    std::vector<std::string> tags;
};

struct OutfitItem
{
    std::string slot;
    std::string itemId;
};

struct OutfitTemplate
{
    std::string id;
    std::string name;
    std::vector<std::string> tags;
    std::vector<OutfitItem> items;
};

struct ProfessionTemplate
{
    std::string id;
    std::string title;
    float goldBudgetMin = 50.0f;
    float goldBudgetMax = 150.0f;
    std::vector<std::string> allowedBodyTags;
    std::vector<std::string> allowedOutfitTags;
    std::unordered_map<std::string, int> fetishDesires;
    std::vector<std::string> combatManeuvers;
};

struct NamePools
{
    std::vector<std::string> firstNamesMale;
    std::vector<std::string> firstNamesFemale;
    std::vector<std::string> firstNamesNeutral;
    std::vector<std::string> surnames;
};

class composableTemplateManager
{
public:
    static bool loadAllTemplates(const std::string& templatesDir = "data/templates");
    static const BodyTemplate* getBodyTemplate(const std::string& id);
    static const OutfitTemplate* getOutfitTemplate(const std::string& id);
    static const ProfessionTemplate* getProfessionTemplate(const std::string& id);
    static std::string getRandomName(const std::string& genderArchetype = "FEMALE");

    static const std::unordered_map<std::string, BodyTemplate>& getAllBodyTemplates();
    static const std::unordered_map<std::string, OutfitTemplate>& getAllOutfitTemplates();
    static const std::unordered_map<std::string, ProfessionTemplate>& getAllProfessions();

private:
    static std::unordered_map<std::string, BodyTemplate> s_bodyTemplates;
    static std::unordered_map<std::string, OutfitTemplate> s_outfitTemplates;
    static std::unordered_map<std::string, ProfessionTemplate> s_professions;
    static NamePools s_namePools;
    static bool s_loaded;
};

