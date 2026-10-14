#include "entities/composableTemplateManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include "common/randomEngine.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

std::unordered_map<std::string, BodyTemplate> composableTemplateManager::s_bodyTemplates;
std::unordered_map<std::string, OutfitTemplate> composableTemplateManager::s_outfitTemplates;
std::unordered_map<std::string, ProfessionTemplate> composableTemplateManager::s_professions;
NamePools composableTemplateManager::s_namePools;
bool composableTemplateManager::s_loaded = false;

static json loadJsonFile(const std::string& path, const std::string& fallbackPath = "")
{
    if (fs::exists(path))
    {
        std::ifstream f(path);
        if (f.is_open())
        {
            try { return json::parse(f); } catch (...) {}
        }
    }
    if (!fallbackPath.empty() && fs::exists(fallbackPath))
    {
        std::ifstream f(fallbackPath);
        if (f.is_open())
        {
            try { return json::parse(f); } catch (...) {}
        }
    }
    return json::object();
}

bool composableTemplateManager::loadAllTemplates(const std::string& templatesDir)
{
    s_bodyTemplates.clear();
    s_outfitTemplates.clear();
    s_professions.clear();

    // 1. Body Templates
    json bodyData = loadJsonFile(templatesDir + "/body_templates.json", "data/catalogs/body_templates.json");
    if (bodyData.contains("bodyTemplates") && bodyData["bodyTemplates"].is_array())
    {
        for (const auto& item : bodyData["bodyTemplates"])
        {
            BodyTemplate bt;
            bt.id = item.value("id", "");
            bt.species = item.value("species", "Human");
            bt.genderArchetype = item.value("genderArchetype", "FEMALE");
            bt.heightCm = item.value("heightCm", 170.0f);
            bt.bodyBuild = item.value("bodyBuild", "Average");

            if (item.contains("breasts") && item["breasts"].is_object())
            {
                bt.breastCupSize = item["breasts"].value("cupSize", 0);
                bt.breastFluidMl = item["breasts"].value("currentFluidMl", 0.0f);
                bt.breastMaxFluidMl = item["breasts"].value("maxFluidMl", 0.0f);
                bt.isLactating = item["breasts"].value("isLactating", false);
            }

            if (item.contains("groin") && item["groin"].is_object())
            {
                bt.groinType = item["groin"].value("type", "Vagina");
                bt.groinLengthCm = item["groin"].value("lengthCm", 0.0f);
                bt.groinDiameterCm = item["groin"].value("diameterCm", 0.0f);
                bt.knot = item["groin"].value("knot", false);
                bt.groinWetness = item["groin"].value("wetness", 1);
                bt.groinFluidMl = item["groin"].value("currentFluidMl", 0.0f);
                bt.groinMaxFluidMl = item["groin"].value("maxFluidMl", 0.0f);
            }

            if (item.contains("tags") && item["tags"].is_array())
            {
                bt.tags = item["tags"].get<std::vector<std::string>>();
            }

            s_bodyTemplates[bt.id] = bt;
        }
    }

    // 2. Outfit Templates
    json outfitData = loadJsonFile(templatesDir + "/outfit_templates.json", "data/catalogs/outfit_templates.json");
    if (outfitData.contains("outfits") && outfitData["outfits"].is_array())
    {
        for (const auto& item : outfitData["outfits"])
        {
            OutfitTemplate ot;
            ot.id = item.value("id", "");
            ot.name = item.value("name", "Attire");
            if (item.contains("tags") && item["tags"].is_array())
            {
                ot.tags = item["tags"].get<std::vector<std::string>>();
            }
            if (item.contains("items") && item["items"].is_array())
            {
                for (const auto& itEntry : item["items"])
                {
                    OutfitItem oi;
                    oi.slot = itEntry.value("slot", "NONE");
                    oi.itemId = itEntry.value("itemId", "");
                    ot.items.push_back(oi);
                }
            }
            s_outfitTemplates[ot.id] = ot;
        }
    }

    // 3. Professions
    json profData = loadJsonFile(templatesDir + "/professions.json", "data/catalogs/professions.json");
    if (profData.contains("professions") && profData["professions"].is_array())
    {
        for (const auto& item : profData["professions"])
        {
            ProfessionTemplate pt;
            pt.id = item.value("id", "");
            pt.title = item.value("title", "Adventurer");
            pt.goldBudgetMin = item.value("goldBudgetMin", 50.0f);
            pt.goldBudgetMax = item.value("goldBudgetMax", 150.0f);
            if (item.contains("allowedBodyTags") && item["allowedBodyTags"].is_array())
            {
                pt.allowedBodyTags = item["allowedBodyTags"].get<std::vector<std::string>>();
            }
            if (item.contains("allowedOutfitTags") && item["allowedOutfitTags"].is_array())
            {
                pt.allowedOutfitTags = item["allowedOutfitTags"].get<std::vector<std::string>>();
            }
            if (item.contains("fetishDesires") && item["fetishDesires"].is_object())
            {
                for (const auto& [k, v] : item["fetishDesires"].items())
                {
                    pt.fetishDesires[k] = v.get<int>();
                }
            }
            if (item.contains("combatManeuvers") && item["combatManeuvers"].is_array())
            {
                pt.combatManeuvers = item["combatManeuvers"].get<std::vector<std::string>>();
            }
            s_professions[pt.id] = pt;
        }
    }

    // 4. Names
    json nameData = loadJsonFile(templatesDir + "/names.json", "data/catalogs/names.json");
    if (nameData.contains("firstNamesMale") && nameData["firstNamesMale"].is_array())
    {
        s_namePools.firstNamesMale = nameData["firstNamesMale"].get<std::vector<std::string>>();
    }
    if (nameData.contains("firstNamesFemale") && nameData["firstNamesFemale"].is_array())
    {
        s_namePools.firstNamesFemale = nameData["firstNamesFemale"].get<std::vector<std::string>>();
    }
    if (nameData.contains("firstNamesNeutral") && nameData["firstNamesNeutral"].is_array())
    {
        s_namePools.firstNamesNeutral = nameData["firstNamesNeutral"].get<std::vector<std::string>>();
    }
    if (nameData.contains("surnames") && nameData["surnames"].is_array())
    {
        s_namePools.surnames = nameData["surnames"].get<std::vector<std::string>>();
    }

    s_loaded = true;
    return (!s_bodyTemplates.empty() && !s_professions.empty());
}

const BodyTemplate* composableTemplateManager::getBodyTemplate(const std::string& id)
{
    if (!s_loaded) loadAllTemplates();
    auto it = s_bodyTemplates.find(id);
    return (it != s_bodyTemplates.end()) ? &it->second : nullptr;
}

const OutfitTemplate* composableTemplateManager::getOutfitTemplate(const std::string& id)
{
    if (!s_loaded) loadAllTemplates();
    auto it = s_outfitTemplates.find(id);
    return (it != s_outfitTemplates.end()) ? &it->second : nullptr;
}

const ProfessionTemplate* composableTemplateManager::getProfessionTemplate(const std::string& id)
{
    if (!s_loaded) loadAllTemplates();
    auto it = s_professions.find(id);
    return (it != s_professions.end()) ? &it->second : nullptr;
}

std::string composableTemplateManager::getRandomName(const std::string& genderArchetype)
{
    if (!s_loaded) loadAllTemplates();

    std::string first = "Alex";
    if ((genderArchetype == "MALE" || genderArchetype == "ANDROMORPH") && !s_namePools.firstNamesMale.empty())
    {
        first = s_namePools.firstNamesMale[dice::rollInt(0, static_cast<int>(s_namePools.firstNamesMale.size()) - 1)];
    }
    else if ((genderArchetype == "FEMALE" || genderArchetype == "GYNOMORPH") && !s_namePools.firstNamesFemale.empty())
    {
        first = s_namePools.firstNamesFemale[dice::rollInt(0, static_cast<int>(s_namePools.firstNamesFemale.size()) - 1)];
    }
    else if (!s_namePools.firstNamesNeutral.empty())
    {
        first = s_namePools.firstNamesNeutral[dice::rollInt(0, static_cast<int>(s_namePools.firstNamesNeutral.size()) - 1)];
    }

    std::string last = "";
    if (!s_namePools.surnames.empty())
    {
        last = s_namePools.surnames[dice::rollInt(0, static_cast<int>(s_namePools.surnames.size()) - 1)];
    }

    return last.empty() ? first : (first + " " + last);
}

const std::unordered_map<std::string, BodyTemplate>& composableTemplateManager::getAllBodyTemplates()
{
    if (!s_loaded) loadAllTemplates();
    return s_bodyTemplates;
}

const std::unordered_map<std::string, OutfitTemplate>& composableTemplateManager::getAllOutfitTemplates()
{
    if (!s_loaded) loadAllTemplates();
    return s_outfitTemplates;
}

const std::unordered_map<std::string, ProfessionTemplate>& composableTemplateManager::getAllProfessions()
{
    if (!s_loaded) loadAllTemplates();
    return s_professions;
}

