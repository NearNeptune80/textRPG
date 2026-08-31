#include "state/transformationState.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <nlohmann/json.hpp>

#include "core/game.h"
#include "entities/entity.h"
#include "state/explorationState.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

static std::string getPresetsDirectory()
{
    std::string dir = "data/presets";
    if (!fs::exists(dir))
    {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
        {
            dir = "presets";
            fs::create_directories(dir, ec);
        }
    }
    return dir;
}

transformationState::transformationState(TransformationTab initialTab)
    : currentTab(initialTab)
{
}

void transformationState::initialise(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void transformationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void transformationState::onExit(game* gameContext) {}

void transformationState::update(game* gameContext, float deltaTime) {}

void transformationState::setTab(TransformationTab tab, game* gameContext)
{
    currentTab = tab;
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void transformationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.key == SDLK_ESCAPE)
        {
            gameContext->changeState(std::make_unique<explorationState>());
        }
        else if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9)
        {
            int tabIdx = event.key.key - SDLK_1;
            if (tabIdx >= 0 && tabIdx <= 10)
            {
                setTab(static_cast<TransformationTab>(tabIdx), gameContext);
            }
        }
        else if (event.key.key == SDLK_0)
        {
            setTab(TransformationTab::APPENDAGES, gameContext);
        }
    }
}

void transformationState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}

void transformationState::savePreset(game* gameContext, const std::string& name)
{
    if (!gameContext) return;
    entity* player = gameContext->getPlayer();
    if (!player) return;

    std::string cleanName = name.empty() ? "Custom_Form" : name;
    for (char& c : cleanName)
    {
        if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }

    std::string dir = getPresetsDirectory();
    std::string filePath = dir + "/" + cleanName + ".json";

    json j;
    j["presetName"] = cleanName;
    j["heightMeters"] = player->anatomy.heightMeters;

    json partsJson = json::object();
    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        bodySlot slot = static_cast<bodySlot>(i);
        if (const bodyPart* p = player->anatomy.getPart(slot))
        {
            json pj;
            pj["id"] = p->id;
            pj["name"] = p->name;
            pj["race"] = p->race;
            pj["count"] = p->count;
            pj["covering"] = coveringTypeToString(p->covering);
            pj["primaryColor"] = p->primaryColor;
            pj["secondaryColor"] = p->secondaryColor;
            pj["length"] = p->length;
            pj["diameter"] = p->diameter;
            pj["cupSize"] = p->cupSize;
            pj["style"] = p->style;
            pj["currentFluidMl"] = p->currentFluidMl;
            pj["maxFluidMl"] = p->maxFluidMl;
            pj["fluidRegenPerHour"] = p->fluidRegenPerHour;
            pj["isLactating"] = p->isLactating;

            if (p->orifice.exists)
            {
                pj["orifice"] = {
                    {"exists", p->orifice.exists},
                    {"elasticity", p->orifice.elasticity},
                    {"currentStretch", p->orifice.currentStretch},
                    {"maxCapacityMl", p->orifice.maxCapacityMl},
                    {"depthCm", p->orifice.depthCm},
                    {"wetnessLevel", p->orifice.wetnessLevel},
                    {"storedFluids", p->orifice.storedFluids}
                };
            }
            partsJson[bodySlotToString(slot)] = pj;
        }
    }
    j["parts"] = partsJson;

    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << j.dump(4);
        statusMessage = "Preset '" + cleanName + "' saved successfully.";
    }
    else
    {
        statusMessage = "Failed to save preset.";
    }
}

void transformationState::loadPreset(game* gameContext, const std::string& name)
{
    if (!gameContext) return;
    entity* player = gameContext->getPlayer();
    if (!player) return;

    std::string dir = getPresetsDirectory();
    std::string filePath = dir + "/" + name + ".json";
    if (!fs::exists(filePath))
    {
        filePath = dir + "/" + name;
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        statusMessage = "Could not open preset file: " + name;
        return;
    }

    try
    {
        json j;
        file >> j;

        player->anatomy.heightMeters = j.value("heightMeters", 1.75f);
        if (j.contains("parts"))
        {
            // Clear current parts first
            for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
            {
                player->anatomy.removePart(static_cast<bodySlot>(i));
            }

            for (auto& [slotStr, pj] : j["parts"].items())
            {
                bodySlot slot = stringToBodySlot(slotStr);
                bodyPart p;
                p.id = pj.value("id", "");
                p.name = pj.value("name", "");
                p.race = pj.value("race", "Human");
                p.count = pj.value("count", 1);
                p.covering = stringToCoveringType(pj.value("covering", "SKIN"));
                p.primaryColor = pj.value("primaryColor", "Fair");
                p.secondaryColor = pj.value("secondaryColor", "");
                p.length = pj.value("length", 0.0f);
                p.diameter = pj.value("diameter", 0.0f);
                p.cupSize = pj.value("cupSize", 0);
                p.style = pj.value("style", "");
                p.currentFluidMl = pj.value("currentFluidMl", 0.0f);
                p.maxFluidMl = pj.value("maxFluidMl", 0.0f);
                p.fluidRegenPerHour = pj.value("fluidRegenPerHour", 0.0f);
                p.isLactating = pj.value("isLactating", false);

                if (pj.contains("orifice"))
                {
                    const auto& orf = pj["orifice"];
                    p.orifice.exists = orf.value("exists", true);
                    p.orifice.elasticity = orf.value("elasticity", 50.0f);
                    p.orifice.currentStretch = orf.value("currentStretch", 0.0f);
                    p.orifice.maxCapacityMl = orf.value("maxCapacityMl", 100.0f);
                    p.orifice.depthCm = orf.value("depthCm", 15.0f);
                    p.orifice.wetnessLevel = orf.value("wetnessLevel", 1);
                    if (orf.contains("storedFluids"))
                    {
                        p.orifice.storedFluids = orf["storedFluids"].get<std::unordered_map<std::string, float>>();
                    }
                }

                player->anatomy.setPart(slot, p);
            }
        }

        statusMessage = "Preset '" + name + "' loaded successfully!";
    }
    catch (...)
    {
        statusMessage = "Error parsing preset JSON.";
    }
}

void transformationState::deletePreset(const std::string& name)
{
    std::string dir = getPresetsDirectory();
    std::string filePath = dir + "/" + name + ".json";
    if (!fs::exists(filePath)) filePath = dir + "/" + name;

    if (fs::exists(filePath))
    {
        std::error_code ec;
        fs::remove(filePath, ec);
        statusMessage = "Deleted preset '" + name + "'.";
    }
}

std::vector<std::string> transformationState::getPresetNames() const
{
    std::vector<std::string> names;
    std::string dir = getPresetsDirectory();
    if (!fs::exists(dir)) return names;

    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            names.push_back(entry.path().stem().string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

void transformationState::resetToHuman(game* gameContext)
{
    if (!gameContext) return;
    entity* p = gameContext->getPlayer();
    if (!p) return;

    for (size_t i = 0; i < BODY_SLOT_COUNT; ++i)
    {
        p->anatomy.removePart(static_cast<bodySlot>(i));
    }

    p->anatomy.heightMeters = 1.78f;

    // Torso & Skin
    bodyPart torso;
    torso.id = "torso_human"; torso.name = "Torso"; torso.race = "Human"; torso.covering = CoveringType::SKIN; torso.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::TORSO, torso);

    // Head, Face & Hair
    bodyPart head; head.id = "head_human"; head.name = "Head"; head.race = "Human"; head.covering = CoveringType::SKIN; head.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::HEAD, head);

    bodyPart hair; hair.id = "hair_human"; hair.name = "Hair"; hair.race = "Human"; hair.covering = CoveringType::HAIR_COVERING; hair.primaryColor = "Brown"; hair.style = "Short"; hair.length = 15.0f;
    p->anatomy.setPart(bodySlot::HAIR, hair);

    bodyPart eyes; eyes.id = "eyes_human"; eyes.name = "Eyes"; eyes.race = "Human"; eyes.covering = CoveringType::IRIS; eyes.primaryColor = "Blue";
    p->anatomy.setPart(bodySlot::EYES, eyes);

    bodyPart ears; ears.id = "ears_human"; ears.name = "Ears"; ears.race = "Human"; ears.covering = CoveringType::SKIN; ears.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::EARS, ears);

    bodyPart mouth; mouth.id = "mouth_human"; mouth.name = "Mouth"; mouth.race = "Human"; mouth.covering = CoveringType::FLESH; mouth.primaryColor = "Rosy";
    mouth.orifice.exists = true; mouth.orifice.depthCm = 18.0f; mouth.orifice.elasticity = 40.0f; mouth.orifice.wetnessLevel = 2;
    p->anatomy.setPart(bodySlot::MOUTH, mouth);

    // Breasts
    bodyPart breasts; breasts.id = "breasts_human"; breasts.name = "Breasts"; breasts.race = "Human"; breasts.cupSize = 0; breasts.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::BREASTS, breasts);

    // Ass & Anus
    bodyPart ass; ass.id = "ass_human"; ass.name = "Ass"; ass.race = "Human"; ass.primaryColor = "Fair";
    ass.orifice.exists = true; ass.orifice.depthCm = 20.0f; ass.orifice.elasticity = 30.0f; ass.orifice.wetnessLevel = 1;
    p->anatomy.setPart(bodySlot::ASS, ass);

    // Genitalia
    bodyPart groin; groin.id = "groin_human"; groin.name = "Groin"; groin.race = "Human"; groin.primaryColor = "Fair";
    if (p->genderArchetype == GenderArchetype::FEMALE)
    {
        groin.name = "Vagina";
        groin.orifice.exists = true; groin.orifice.depthCm = 15.0f; groin.orifice.elasticity = 50.0f; groin.orifice.wetnessLevel = 2;
    }
    else
    {
        groin.name = "Penis";
        groin.length = 16.0f; groin.diameter = 3.8f; groin.currentFluidMl = 15.0f; groin.maxFluidMl = 30.0f;
    }
    p->anatomy.setPart(bodySlot::GROIN, groin);

    // Limbs
    bodyPart arms; arms.id = "arms_human"; arms.name = "Arms"; arms.race = "Human"; arms.count = 2; arms.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::ARMS, arms);

    bodyPart legs; legs.id = "legs_human"; legs.name = "Legs"; legs.race = "Human"; legs.count = 2; legs.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::LEGS, legs);

    bodyPart feet; feet.id = "feet_human"; feet.name = "Feet"; feet.race = "Human"; feet.count = 2; feet.primaryColor = "Fair";
    p->anatomy.setPart(bodySlot::FEET, feet);

    statusMessage = "Body reset to standard Human baseline.";
}

void transformationState::randomizeForm(game* gameContext)
{
    if (!gameContext) return;
    entity* p = gameContext->getPlayer();
    if (!p) return;

    static std::mt19937 rng(std::random_device{}());
    static const std::vector<std::string> races = { "Demon", "Cat-morph", "Dog-morph", "Wolf-morph", "Fox-morph", "Harpy", "Bovine-morph", "Dragon-morph", "Elf" };
    static const std::vector<std::string> hairColors = { "Crimson", "Blonde", "Raven Black", "Platinum", "Lilac", "Pink", "Silver", "Amber" };
    static const std::vector<std::string> eyeColors = { "Gold", "Violet", "Emerald Green", "Sapphire Blue", "Ruby Red", "Silver" };
    static const std::vector<std::string> skinColors = { "Porcelain", "Tanned", "Pale", "Dark", "Ebony", "Rosy", "Lilac", "Red" };

    std::uniform_int_distribution<size_t> rDist(0, races.size() - 1);
    std::uniform_int_distribution<size_t> hcDist(0, hairColors.size() - 1);
    std::uniform_int_distribution<size_t> ecDist(0, eyeColors.size() - 1);
    std::uniform_int_distribution<size_t> scDist(0, skinColors.size() - 1);
    std::uniform_int_distribution<int> cupDist(0, 10);
    std::uniform_real_distribution<float> hDist(1.50f, 2.15f);

    std::string primaryRace = races[rDist(rng)];
    p->anatomy.heightMeters = hDist(rng);

    // Torso
    bodyPart torso; torso.id = "torso_" + primaryRace; torso.name = "Torso"; torso.race = primaryRace; torso.covering = (primaryRace.find("morph") != std::string::npos ? CoveringType::FUR : CoveringType::SKIN); torso.primaryColor = skinColors[scDist(rng)];
    p->anatomy.setPart(bodySlot::TORSO, torso);

    // Head, Hair & Eyes
    bodyPart hair; hair.id = "hair_" + primaryRace; hair.name = "Hair"; hair.race = primaryRace; hair.covering = CoveringType::HAIR_COVERING; hair.primaryColor = hairColors[hcDist(rng)]; hair.style = "Long Wavy"; hair.length = 35.0f;
    p->anatomy.setPart(bodySlot::HAIR, hair);

    bodyPart eyes; eyes.id = "eyes_" + primaryRace; eyes.name = "Eyes"; eyes.race = primaryRace; eyes.covering = CoveringType::IRIS; eyes.primaryColor = eyeColors[ecDist(rng)];
    p->anatomy.setPart(bodySlot::EYES, eyes);

    bodyPart ears; ears.id = "ears_" + primaryRace; ears.name = "Ears"; ears.race = primaryRace; ears.primaryColor = hair.primaryColor;
    p->anatomy.setPart(bodySlot::EARS, ears);

    // Horns (if Demon or Dragon or Bovine)
    if (primaryRace == "Demon" || primaryRace == "Dragon-morph" || primaryRace == "Bovine-morph")
    {
        bodyPart horns; horns.id = "horns_" + primaryRace; horns.name = "Horns"; horns.race = primaryRace; horns.count = 2; horns.length = 22.0f; horns.primaryColor = "Obsidian Black";
        p->anatomy.setPart(bodySlot::HORNS, horns);
    }
    else
    {
        p->anatomy.removePart(bodySlot::HORNS);
    }

    // Wings (if Demon, Harpy or Dragon)
    if (primaryRace == "Demon" || primaryRace == "Harpy" || primaryRace == "Dragon-morph")
    {
        bodyPart wings; wings.id = "wings_" + primaryRace; wings.name = "Wings"; wings.race = primaryRace; wings.count = 2; wings.primaryColor = primaryRace == "Harpy" ? hair.primaryColor : "Dark Violet";
        p->anatomy.setPart(bodySlot::WINGS, wings);
    }
    else
    {
        p->anatomy.removePart(bodySlot::WINGS);
    }

    // Tail
    bodyPart tail; tail.id = "tail_" + primaryRace; tail.name = "Tail"; tail.race = primaryRace; tail.count = 1; tail.length = 90.0f; tail.diameter = 6.0f; tail.primaryColor = hair.primaryColor;
    p->anatomy.setPart(bodySlot::TAIL, tail);

    // Breasts
    bodyPart breasts; breasts.id = "breasts_" + primaryRace; breasts.name = "Breasts"; breasts.race = primaryRace; breasts.cupSize = cupDist(rng); breasts.primaryColor = torso.primaryColor;
    if (breasts.cupSize >= 3)
    {
        breasts.isLactating = true;
        breasts.currentFluidMl = 250.0f;
        breasts.maxFluidMl = 1000.0f;
    }
    p->anatomy.setPart(bodySlot::BREASTS, breasts);

    statusMessage = "Transformed into a chaotic " + p->anatomy.getRacialTitle() + " form!";
}
