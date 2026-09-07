#include "entities/npcGenerator.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "common/randomEngine.h"
#include "entities/entity.h"
#include "items/itemDatabase.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

std::unordered_map<std::string, NPCTemplate> npcGenerator::registry;

bool npcGenerator::loadTemplates(const std::string& path)
{
    registry.clear();
    fs::path p(path);
    if (!fs::exists(p))
    {
        std::cerr << "[npcGenerator] Path does not exist: " << path << "\n";
        return false;
    }

    auto parseSingleTemplate = [](const json& tJson) {
        NPCTemplate tpl;
        tpl.id = tJson.at("id").get<std::string>();
        tpl.name = tJson.value("name", "Unknown NPC");
        tpl.levelMin = tJson.value("levelMin", 1);
        tpl.levelMax = tJson.value("levelMax", 1);

        if (tJson.contains("baseStats"))
        {
            for (auto& [key, val] : tJson["baseStats"].items())
            {
                tpl.baseStats[key] = val.get<float>();
            }
        }

        tpl.tags = tJson.value("tags", std::vector<std::string>{});
        tpl.possibleRaces = tJson.value("possibleRaces", std::vector<std::string>{"Human"});
        tpl.guaranteedItems = tJson.value("guaranteedItems", std::vector<std::string>{});
        tpl.randomItems = tJson.value("randomItems", std::vector<std::string>{});
        if (tJson.contains("perks") && tJson["perks"].is_array())
        {
            tpl.perks = tJson["perks"].get<std::vector<std::string>>();
        }
        else if (tJson.contains("unlockedPerks") && tJson["unlockedPerks"].is_array())
        {
            tpl.perks = tJson["unlockedPerks"].get<std::vector<std::string>>();
        }

        registry[tpl.id] = tpl;
    };

    auto loadSingleFile = [&](const fs::path& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        try
        {
            json data;
            file >> data;

            if (data.is_object() && data.contains("templates") && data["templates"].is_array())
            {
                for (const auto& tJson : data["templates"])
                {
                    parseSingleTemplate(tJson);
                }
            }
            else if (data.is_array())
            {
                for (const auto& tJson : data)
                {
                    parseSingleTemplate(tJson);
                }
            }
            else if (data.is_object() && data.contains("id"))
            {
                parseSingleTemplate(data);
            }
        }
        catch (const json::exception& e)
        {
            std::cerr << "NPC Template JSON Error (" << filePath.string() << "): " << e.what() << "\n";
        }
    };

    if (fs::is_directory(p))
    {
        for (const auto& entry : fs::recursive_directory_iterator(p))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                loadSingleFile(entry.path());
            }
        }
    }
    else if (fs::is_regular_file(p))
    {
        loadSingleFile(p);
    }

    return !registry.empty();
}

const NPCTemplate* npcGenerator::getTemplate(const std::string& templateId)
{
    auto it = registry.find(templateId);
    if (it != registry.end())
    {
        return &it->second;
    }
    return nullptr;
}

bool npcGenerator::hasTemplate(const std::string& templateId)
{
    return registry.contains(templateId);
}

void npcGenerator::applyDemographicConfiguration(entity* npc, const GameSettings* settings)
{
    if (!npc) return;

    DemographicSettings defaultDemo;
    const DemographicSettings& demo = settings ? settings->demographics : defaultDemo;

    float rollSex = dice::roll01();
    float rollArch = dice::roll01();
    float rollAge = dice::roll01();
    float rollFurry = dice::roll01();

    npc->orientation = demo.rollSexuality(rollSex);
    npc->genderArchetype = demo.rollGenderArchetype(rollArch);
    npc->age = demo.rollAge(rollAge);

    std::string furryStage = demo.rollFurryStage(rollFurry);

    std::string race = npc->anatomy.getDominantRace();
    if (race.empty()) race = "Human";

    CoveringType mainCovering = (furryStage == "Anthro" || furryStage == "Feral")
                                ? CoveringType::FUR : CoveringType::SKIN;

    // Build anatomical parts matching gender archetype and furry configuration
    bodyPart torso; torso.id = "part_torso_" + race; torso.name = "Torso"; torso.race = race; torso.primaryColor = "Fair"; torso.covering = mainCovering;
    bodyPart head; head.id = "part_head_" + race; head.name = "Face"; head.race = race; head.primaryColor = "Fair"; head.covering = mainCovering;
    bodyPart legs; legs.id = "part_legs_" + race; legs.name = "Legs"; legs.race = race; legs.count = 2; legs.covering = mainCovering;
    bodyPart feet; feet.id = "part_feet_" + race; feet.name = "Feet"; feet.race = race; feet.count = 2; feet.covering = mainCovering;
    bodyPart arms; arms.id = "part_arms_" + race; arms.name = "Arms"; arms.race = race; arms.count = 2; arms.covering = mainCovering;
    bodyPart hair; hair.id = "part_hair_" + race; hair.name = "Hair"; hair.race = race; hair.primaryColor = "Brown"; hair.covering = CoveringType::HAIR_COVERING;
    bodyPart eyes; eyes.id = "part_eyes_" + race; eyes.name = "Eyes"; eyes.race = race; eyes.primaryColor = "Blue"; eyes.covering = CoveringType::IRIS;

    bodyPart breasts; breasts.id = "part_breasts_" + race; breasts.name = "Breasts"; breasts.race = race; breasts.covering = mainCovering;
    bodyPart groin; groin.id = "part_groin_" + race; groin.name = "Groin"; groin.race = race; groin.covering = mainCovering;
    bodyPart ass; ass.id = "part_ass_" + race; ass.name = "Ass"; ass.race = race; ass.covering = mainCovering;
    ass.orifice.exists = true; ass.orifice.elasticity = 60.0f; ass.orifice.maxCapacityMl = 80.0f;

    if (furryStage == "Partial" || furryStage == "Anthro" || furryStage == "Feral")
    {
        bodyPart tail;
        tail.id = "part_tail_" + race;
        tail.name = "Tail";
        tail.race = race;
        tail.covering = CoveringType::FUR;
        tail.tags.push_back("tail");
        npc->anatomy.setPart(bodySlot::TAIL, tail);

        if (furryStage == "Partial")
        {
            head.tags.push_back("animal_ears");
        }
        else if (furryStage == "Anthro")
        {
            torso.tags.push_back("anthro");
            head.tags.push_back("muzzle");
        }
        else if (furryStage == "Feral")
        {
            torso.tags.push_back("feral");
            legs.tags.push_back("digitigrade");
        }
    }

    switch (npc->genderArchetype)
    {
        case GenderArchetype::MALE:
            breasts.cupSize = 0;
            groin.name = "Penis"; groin.length = 15.0f; groin.diameter = 3.5f;
            groin.currentFluidMl = 10.0f; groin.maxFluidMl = 20.0f; groin.fluidRegenPerHour = 2.0f;
            groin.tags.push_back("penis"); groin.tags.push_back("has_penis");
            torso.tags.push_back("masculine");
            break;

        case GenderArchetype::FEMALE:
            breasts.cupSize = 3; // C cup
            groin.name = "Vagina"; groin.orifice.exists = true; groin.orifice.elasticity = 75.0f; groin.orifice.maxCapacityMl = 120.0f;
            groin.tags.push_back("vagina"); groin.tags.push_back("has_vagina");
            torso.tags.push_back("feminine");
            break;

        case GenderArchetype::HERMAPHRODITE:
            breasts.cupSize = 3;
            groin.name = "Hermaphrodite Genitals"; groin.length = 14.0f; groin.diameter = 3.2f;
            groin.currentFluidMl = 10.0f; groin.maxFluidMl = 20.0f; groin.fluidRegenPerHour = 2.0f;
            groin.orifice.exists = true; groin.orifice.elasticity = 75.0f; groin.orifice.maxCapacityMl = 120.0f;
            groin.tags.push_back("penis"); groin.tags.push_back("vagina"); groin.tags.push_back("hermaphrodite");
            torso.tags.push_back("feminine"); torso.tags.push_back("masculine");
            break;

        case GenderArchetype::GYNOMORPH:
            breasts.cupSize = 4; // D cup
            groin.name = "Penis"; groin.length = 16.0f; groin.diameter = 3.6f;
            groin.currentFluidMl = 12.0f; groin.maxFluidMl = 25.0f; groin.fluidRegenPerHour = 2.5f;
            groin.tags.push_back("penis"); groin.tags.push_back("gynomorph");
            torso.tags.push_back("feminine");
            break;

        case GenderArchetype::ANDROMORPH:
            breasts.cupSize = 0;
            groin.name = "Vagina"; groin.orifice.exists = true; groin.orifice.elasticity = 70.0f; groin.orifice.maxCapacityMl = 100.0f;
            groin.tags.push_back("vagina"); groin.tags.push_back("andromorph");
            torso.tags.push_back("masculine");
            break;

        case GenderArchetype::ASEXUAL_NULL:
        default:
            breasts.cupSize = 0;
            groin.name = "Smooth Groin";
            groin.tags.push_back("null");
            break;
    }

    ContentSettings defaultContent;
    const ContentSettings& content = settings ? settings->content : defaultContent;

    // Roll 28 Lilith's Throne Fetishes based on frequency scale:
    // 0=Never (0%), 1=V.Rare (5%), 2=Rare (15%), 3=Average (35%), 4=Common (60%), 5=V.Common (85%), 6=Always (100%)
    static constexpr float freqProbabilities[] = { 0.0f, 0.05f, 0.15f, 0.35f, 0.60f, 0.85f, 1.0f };
    for (const auto& [fetName, freqRating] : content.fetishPreferences)
    {
        int rating = std::clamp(freqRating, 0, 6);
        float prob = freqProbabilities[rating];
        if (prob > 0.0f && (prob >= 1.0f || dice::roll01() < prob))
        {
            int desire = (dice::roll01() < 0.25f) ? 4 : 3; // 3=Like, 4=Love
            npc->setFetishDesire(fetName, desire);
        }
        else
        {
            npc->setFetishDesire(fetName, 0);
        }
    }

    // Enforce Content Options Toggles on NPC generation
    if (content.lactationState == ContentToggleState::OFF)
    {
        breasts.currentFluidMl = 0.0f;
        breasts.maxFluidMl = 0.0f;
        breasts.fluidRegenPerHour = 0.0f;
    }
    else if (content.lactationState == ContentToggleState::ON &&
             (npc->genderArchetype == GenderArchetype::FEMALE || npc->genderArchetype == GenderArchetype::HERMAPHRODITE))
    {
        if (dice::roll01() < 0.35f || npc->hasFetish("Lactation") || npc->hasFetish("Milk lover"))
        {
            breasts.currentFluidMl = 150.0f;
            breasts.maxFluidMl = 500.0f;
            breasts.fluidRegenPerHour = 25.0f;
            breasts.tags.push_back("lactating");
        }
    }

    if (content.watersportsState == ContentToggleState::OFF)
    {
        npc->setFetishDesire("Watersports", 0);
    }
    if (content.tentaclesState == ContentToggleState::OFF)
    {
        npc->setFetishDesire("Tentacles", 0);
    }
    if (content.bdsmState == ContentToggleState::OFF)
    {
        npc->setFetishDesire("BDSM / Sadism", 0);
        npc->setFetishDesire("Bondage", 0);
        npc->setFetishDesire("Masochism", 0);
    }
    if (content.extremeContentState == ContentToggleState::OFF)
    {
        npc->setFetishDesire("BDSM / Sadism", 0);
    }
    if (content.pregnancyState == ContentToggleState::OFF)
    {
        npc->setFetishDesire("Pregnancy", 0);
        npc->setFetishDesire("Insemination", 0);
    }

    npc->anatomy.setPart(bodySlot::HEAD, head);
    npc->anatomy.setPart(bodySlot::HAIR, hair);
    npc->anatomy.setPart(bodySlot::EYES, eyes);
    npc->anatomy.setPart(bodySlot::TORSO, torso);
    npc->anatomy.setPart(bodySlot::BREASTS, breasts);
    npc->anatomy.setPart(bodySlot::ARMS, arms);
    npc->anatomy.setPart(bodySlot::GROIN, groin);
    npc->anatomy.setPart(bodySlot::ASS, ass);
    npc->anatomy.setPart(bodySlot::LEGS, legs);
    npc->anatomy.setPart(bodySlot::FEET, feet);
}

std::shared_ptr<entity> npcGenerator::generateFromTemplate(const std::string& templateId, const GameSettings* settings)
{
    auto it = registry.find(templateId);
    if (it == registry.end()) return nullptr;

    const auto& tpl = it->second;
    static int genCounter = 1;

    auto npc = std::make_shared<entity>("npc_gen_" + std::to_string(genCounter++), tpl.name);

    npc->stats.level = dice::rollInt(tpl.levelMin, tpl.levelMax);

    for (const auto& [sName, val] : tpl.baseStats)
    {
        npc->stats.setBaseStat(sName, val);
    }

    const std::string* racePtr = dice::choose(tpl.possibleRaces);
    std::string race = racePtr ? *racePtr : "Human";
    bodyPart torso; torso.id = "part_torso_" + race; torso.name = "Torso"; torso.race = race;
    npc->anatomy.setPart(bodySlot::TORSO, torso);

    // Apply demographic settings (Sexuality & Gender Archetype)
    applyDemographicConfiguration(npc.get(), settings);

    std::vector<std::string> bodyTags = npc->anatomy.getAllTags();
    for (const auto& itemId : tpl.guaranteedItems)
    {
        auto itemPtr = itemDatabase::getItem(itemId);
        if (itemPtr)
        {
            npc->inventory.addItem(itemPtr);
            if (itemPtr->isEquippable && itemPtr->targetSlot != equipSlot::NONE)
            {
                size_t backpackIdx = npc->inventory.backpack.size() - 1;
                npc->inventory.equipItem(backpackIdx, itemPtr->targetSlot, bodyTags);
            }
        }
    }

    for (const auto& itemId : tpl.randomItems)
    {
        if (dice::rollPercent(50.0f))
        {
            auto itemPtr = itemDatabase::getItem(itemId);
            if (itemPtr) npc->inventory.addItem(itemPtr);
        }
    }

    for (const auto& perkId : tpl.perks)
    {
        npc->unlockPerk(perkId);
    }

    return npc;
}

std::shared_ptr<entity> npcGenerator::generateRandomNPC(const GameSettings* settings)
{
    if (registry.empty()) return nullptr;

    auto it = registry.begin();
    std::advance(it, dice::rollInt<size_t>(0, registry.size() - 1));
    return generateFromTemplate(it->first, settings);
}