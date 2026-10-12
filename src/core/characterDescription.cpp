#include "core/characterDescription.h"

#include <format>
#include <fstream>
#include <random>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "entities/entity.h"

namespace {
    struct DescriptionTemplateManager
    {
        std::unordered_map<std::string, std::vector<std::string>> templates;
        bool loaded = false;

        void load(const std::string& path = "data/config/body_part_descriptions.json")
        {
            std::ifstream f(path);
            if (!f.is_open()) return;
            try {
                nlohmann::json j;
                f >> j;
                if (j.contains("templates") && j["templates"].is_object()) {
                    for (auto& [key, val] : j["templates"].items()) {
                        if (val.is_array()) {
                            templates[key] = val.get<std::vector<std::string>>();
                        }
                    }
                }
                loaded = true;
            } catch (...) {}
        }

        std::string getRandom(const std::string& category, const std::string& fallback)
        {
            if (!loaded) load();
            auto it = templates.find(category);
            if (it != templates.end() && !it->second.empty()) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
                return it->second[dist(gen)];
            }
            return fallback;
        }
    };

    static DescriptionTemplateManager s_descMgr;

    static std::string formatTemplate(std::string tmpl, const std::unordered_map<std::string, std::string>& vars)
    {
        for (const auto& [k, v] : vars) {
            std::string tag = "{" + k + "}";
            size_t pos = 0;
            while ((pos = tmpl.find(tag, pos)) != std::string::npos) {
                tmpl.replace(pos, tag.length(), v);
                pos += v.length();
            }
        }
        return tmpl;
    }
}

std::string characterDescription::generateSummary(const entity* ent)
{
    if (!ent) return "Nobody.";

    std::string archetypeName = genderArchetypeToString(ent->anatomy.getGenderArchetype());
    std::string racialTitle = ent->anatomy.getRacialTitle();

    return std::format("{} is a {} {} standing {:.2f}m tall.",
                       ent->name, archetypeName, racialTitle, ent->anatomy.heightMeters);
}

std::string characterDescription::buildOverviewSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    std::string archetypeName = genderArchetypeToString(ent->anatomy.getGenderArchetype());
    std::string racialTitle = ent->anatomy.getRacialTitle();
    std::string heightStr = std::format("{:.2f}", ent->anatomy.heightMeters);

    std::string fallback = "{name} is a {archetype} {race}, standing approximately {height} meters tall with a {figure} figure.";
    std::string tmplKey = ent->anatomy.isFeminine() ? "overview_feminine" : "overview_masculine";
    std::string chosen = s_descMgr.getRandom(tmplKey, fallback);
    s += formatTemplate(chosen, {
        {"name", ent->name},
        {"archetype", archetypeName},
        {"race", racialTitle},
        {"height", heightStr},
        {"figure", ent->anatomy.isFeminine() ? "graceful, feminine" : "solid, masculine"}
    }) + "\n";

    float lust = ent->getStat("lust");
    if (lust >= 75.0f)
    {
        s += std::format("{}'s eyes are clouded with intense desire, breathing heavily with arousal.\n", ent->name);
    }
    else if (lust >= 30.0f)
    {
        s += std::format("{} has a flushed look and a slightly restless, aroused posture.\n", ent->name);
    }

    if (ent->gestation.isPregnant)
    {
        s += std::format("{} has a visibly swollen, gravid belly, carrying an unborn litter ({} days remaining).\n",
                         ent->name, ent->gestation.gestationDaysRemaining);
    }

    return s;
}

std::string characterDescription::buildHeadAndFaceSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    const bodyPart* hair = ent->anatomy.getPart(bodySlot::HAIR);
    const bodyPart* eyes = ent->anatomy.getPart(bodySlot::EYES);
    const bodyPart* ears = ent->anatomy.getPart(bodySlot::EARS);
    const bodyPart* horns = ent->anatomy.getPart(bodySlot::HORNS);

    std::string hColor = hair ? (hair->primaryColor.empty() ? "dark" : hair->primaryColor) : "";
    std::string hStyle = hair ? (hair->style.empty() ? "flowing" : hair->style) : "";
    std::string eColor = eyes ? (eyes->primaryColor.empty() ? "amber" : eyes->primaryColor) : "";

    if (hair && eyes)
    {
        std::string fallback = "{name} possesses {color} {style} hair and striking {eyeColor} eyes.";
        std::string chosen = s_descMgr.getRandom("hair_and_eyes", fallback);
        s += formatTemplate(chosen, {
            {"name", ent->name},
            {"color", hColor},
            {"style", hStyle},
            {"eyeColor", eColor}
        }) + "\n";
    }
    else if (hair)
    {
        std::string fallback = "{name} possesses {color} {style} hair.";
        std::string chosen = s_descMgr.getRandom("hair_only", fallback);
        s += formatTemplate(chosen, {
            {"name", ent->name},
            {"color", hColor},
            {"style", hStyle}
        }) + "\n";
    }
    else if (eyes)
    {
        std::string fallback = "{name} possesses striking {eyeColor} eyes.";
        std::string chosen = s_descMgr.getRandom("eyes_only", fallback);
        s += formatTemplate(chosen, {
            {"name", ent->name},
            {"eyeColor", eColor}
        }) + "\n";
    }

    if (ears)
    {
        std::string fallback = "{name} has {color} {race} ears atop their head.";
        std::string chosen = s_descMgr.getRandom("ears", fallback);
        s += formatTemplate(chosen, {
            {"name", ent->name},
            {"color", ears->primaryColor},
            {"race", ears->race}
        }) + "\n";
    }

    if (horns)
    {
        std::string fallback = "A pair of {color} {race} horns curve outward from their brow.";
        std::string chosen = s_descMgr.getRandom("horns", fallback);
        s += formatTemplate(chosen, {
            {"color", horns->primaryColor},
            {"race", horns->race}
        }) + "\n";
    }

    return s;
}

std::string characterDescription::buildTorsoAndBreastsSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    const bodyPart* torso = ent->anatomy.getPart(bodySlot::TORSO);
    const bodyPart* breasts = ent->anatomy.getPart(bodySlot::BREASTS);

    if (torso)
    {
        std::string fallback = "Their torso is covered in smooth {color} {covering}.";
        std::string chosen = s_descMgr.getRandom("torso", fallback);
        s += formatTemplate(chosen, {
            {"color", torso->primaryColor},
            {"covering", getCoveringNoun(torso->covering)}
        }) + "\n";
    }

    if (breasts)
    {
        std::string cupName = bodyPart::getCupSizeName(breasts->cupSize);
        if (breasts->cupSize > 0)
        {
            std::string fallback = "{name} has soft, noticeable {cup}-cup breasts";
            std::string chosen = s_descMgr.getRandom("breasts", fallback);
            s += formatTemplate(chosen, {
                {"name", ent->name},
                {"cup", cupName}
            });

            if (breasts->isLactating || breasts->currentFluidMl > 0.0f)
            {
                std::string lacFallback = ", visibly engorged with milk ({fluid}/{maxFluid} ml)";
                std::string lacChosen = s_descMgr.getRandom("breasts_lactating", lacFallback);
                s += formatTemplate(lacChosen, {
                    {"fluid", std::format("{:.0f}", breasts->currentFluidMl)},
                    {"maxFluid", std::format("{:.0f}", breasts->maxFluidMl)}
                });
            }
            s += ".\n";
        }
        else
        {
            std::string fallback = "{name} has a smooth, flat chest.";
            std::string chosen = s_descMgr.getRandom("flat_chest", fallback);
            s += formatTemplate(chosen, {{"name", ent->name}}) + "\n";
        }
    }

    return s;
}

std::string characterDescription::buildGenitalsAndRearSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    const bodyPart* groin = ent->anatomy.getPart(bodySlot::GROIN);
    const bodyPart* ass = ent->anatomy.getPart(bodySlot::ASS);

    if (ent->anatomy.hasPenis())
    {
        float len = groin ? groin->length : 15.0f;
        float diam = groin ? groin->diameter : 3.5f;
        float cum = groin ? groin->currentFluidMl : 5.0f;
        float maxCum = groin ? groin->maxFluidMl : 15.0f;

        std::string fallback = "Between their legs rests a {length}cm long, {diameter}cm thick penis, holding {fluid}/{maxFluid}ml of seed.";
        std::string chosen = s_descMgr.getRandom("penis", fallback);
        s += formatTemplate(chosen, {
            {"length", std::format("{:.1f}", len)},
            {"diameter", std::format("{:.1f}", diam)},
            {"fluid", std::format("{:.0f}", cum)},
            {"maxFluid", std::format("{:.0f}", maxCum)}
        }) + "\n";
    }

    if (ent->anatomy.hasVagina())
    {
        int wetness = (groin && groin->orifice.exists) ? groin->orifice.wetnessLevel : 1;
        static const std::string wetnessWords[6] = { "dry", "moist", "wet", "very wet", "dripping", "soaked" };
        std::string wetWord = (wetness >= 0 && wetness < 6) ? wetnessWords[wetness] : "moist";

        std::string fallback = "They possess a lush, {wetness} vagina inviting penetration.";
        std::string chosen = s_descMgr.getRandom("vagina", fallback);
        s += formatTemplate(chosen, {{"wetness", wetWord}}) + "\n";
    }

    if (ass)
    {
        std::string fallback = "Their rear features a well-formed ass with {color} skin.";
        std::string chosen = s_descMgr.getRandom("ass", fallback);
        s += formatTemplate(chosen, {{"color", ass->primaryColor}}) + "\n";
    }

    return s;
}

std::string characterDescription::buildExtremitiesSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    const bodyPart* tail = ent->anatomy.getPart(bodySlot::TAIL);
    const bodyPart* wings = ent->anatomy.getPart(bodySlot::WINGS);
    const bodyPart* tentacles = ent->anatomy.getPart(bodySlot::TENTACLES);

    if (tail)
    {
        std::string fallback = "Behind them sways a graceful {color} {race} tail.";
        std::string chosen = s_descMgr.getRandom("tail", fallback);
        s += formatTemplate(chosen, {
            {"color", tail->primaryColor},
            {"race", tail->race}
        }) + "\n";
    }

    if (wings)
    {
        std::string fallback = "From their back extends a pair of impressive {color} {race} wings.";
        std::string chosen = s_descMgr.getRandom("wings", fallback);
        s += formatTemplate(chosen, {
            {"color", wings->primaryColor},
            {"race", wings->race}
        }) + "\n";
    }

    if (tentacles)
    {
        std::string fallback = "Several writhing {color} tentacles extend from their lower body.";
        std::string chosen = s_descMgr.getRandom("tentacles", fallback);
        s += formatTemplate(chosen, {
            {"color", tentacles->primaryColor}
        }) + "\n";
    }

    return s;
}

std::string characterDescription::buildClothingAndTattoosSection(const entity* ent)
{
    if (!ent) return "";
    std::string s;
    s.reserve(256);

    std::vector<std::string_view> worn;
    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        auto itemPtr = ent->inventory.equipped[i];
        if (itemPtr)
        {
            worn.push_back(itemPtr->name);
        }
    }

    if (!worn.empty())
    {
        s += "Wearing: ";
        for (size_t i = 0; i < worn.size(); ++i)
        {
            s += worn[i];
            s += (i + 1 < worn.size() ? ", " : ".\n");
        }
    }
    else
    {
        s += "Currently wearing no clothing.\n";
    }

    return s;
}

std::string characterDescription::generateFullDescription(const entity* ent, const entity* viewer)
{
    if (!ent) return "Nothing here to see.";

    std::string s;
    s.reserve(1024);
    s += buildOverviewSection(ent);
    s += "\n";
    s += buildHeadAndFaceSection(ent);
    s += buildTorsoAndBreastsSection(ent);
    s += buildGenitalsAndRearSection(ent);
    s += buildExtremitiesSection(ent);
    s += buildClothingAndTattoosSection(ent);

    return s;
}
