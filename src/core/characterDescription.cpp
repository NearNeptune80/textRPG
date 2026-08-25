#include "core/characterDescription.h"

#include <format>
#include <sstream>

#include "entities/entity.h"

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
    std::ostringstream ss;

    std::string archetypeName = genderArchetypeToString(ent->anatomy.getGenderArchetype());
    std::string racialTitle = ent->anatomy.getRacialTitle();

    ss << std::format("{} is a {} {}, standing approximately {:.2f} meters tall with a {} figure.\n",
                      ent->name, archetypeName, racialTitle, ent->anatomy.heightMeters,
                      ent->anatomy.isFeminine() ? "graceful, feminine" : "solid, masculine");

    // Dominance / demeanor
    float lust = ent->getStat("lust");
    if (lust >= 75.0f)
    {
        ss << std::format("{}'s eyes are clouded with intense desire, breathing heavily with arousal.\n", ent->name);
    }
    else if (lust >= 30.0f)
    {
        ss << std::format("{} has a flushed look and a slightly restless, aroused posture.\n", ent->name);
    }

    if (ent->gestation.isPregnant)
    {
        ss << std::format("{} has a visibly swollen, gravid belly, carrying an unborn litter ({} days remaining).\n",
                          ent->name, ent->gestation.gestationDaysRemaining);
    }

    return ss.str();
}

std::string characterDescription::buildHeadAndFaceSection(const entity* ent)
{
    if (!ent) return "";
    std::ostringstream ss;

    const bodyPart* hair = ent->anatomy.getPart(bodySlot::HAIR);
    const bodyPart* eyes = ent->anatomy.getPart(bodySlot::EYES);
    const bodyPart* ears = ent->anatomy.getPart(bodySlot::EARS);
    const bodyPart* horns = ent->anatomy.getPart(bodySlot::HORNS);

    if (hair)
    {
        ss << std::format("{} possesses {} {} hair",
                          ent->name,
                          hair->primaryColor.empty() ? "dark" : hair->primaryColor,
                          hair->style.empty() ? "flowing" : hair->style);
    }

    if (eyes)
    {
        ss << std::format(" and striking {} eyes.\n", eyes->primaryColor.empty() ? "amber" : eyes->primaryColor);
    }
    else
    {
        ss << ".\n";
    }

    if (ears)
    {
        ss << std::format("{} has {} {} ears atop their head.\n",
                          ent->name, ears->primaryColor, ears->race);
    }

    if (horns)
    {
        ss << std::format("A pair of {} {} horns curve outward from their brow.\n",
                          horns->primaryColor, horns->race);
    }

    return ss.str();
}

std::string characterDescription::buildTorsoAndBreastsSection(const entity* ent)
{
    if (!ent) return "";
    std::ostringstream ss;

    const bodyPart* torso = ent->anatomy.getPart(bodySlot::TORSO);
    const bodyPart* breasts = ent->anatomy.getPart(bodySlot::BREASTS);

    if (torso)
    {
        ss << std::format("Their torso is covered in smooth {} {}.\n",
                          torso->primaryColor, getCoveringNoun(torso->covering));
    }

    if (breasts)
    {
        std::string cupName = bodyPart::getCupSizeName(breasts->cupSize);
        if (breasts->cupSize > 0)
        {
            ss << std::format("{} has soft, noticeable {}-cup breasts", ent->name, cupName);
            if (breasts->isLactating || breasts->currentFluidMl > 0.0f)
            {
                ss << std::format(", visibly engorged with milk ({:.0f}/{:.0f} ml)",
                                  breasts->currentFluidMl, breasts->maxFluidMl);
            }
            ss << ".\n";
        }
        else
        {
            ss << std::format("{} has a smooth, flat chest.\n", ent->name);
        }
    }

    return ss.str();
}

std::string characterDescription::buildGenitalsAndRearSection(const entity* ent)
{
    if (!ent) return "";
    std::ostringstream ss;

    const bodyPart* groin = ent->anatomy.getPart(bodySlot::GROIN);
    const bodyPart* ass = ent->anatomy.getPart(bodySlot::ASS);

    if (ent->anatomy.hasPenis())
    {
        float len = groin ? groin->length : 15.0f;
        float diam = groin ? groin->diameter : 3.5f;
        float cum = groin ? groin->currentFluidMl : 5.0f;
        float maxCum = groin ? groin->maxFluidMl : 15.0f;

        ss << std::format("Between their legs rests a {:.1f}cm long, {:.1f}cm thick penis, holding {:.0f}/{:.0f}ml of seed.\n",
                          len, diam, cum, maxCum);
    }

    if (ent->anatomy.hasVagina())
    {
        int wetness = (groin && groin->orifice.exists) ? groin->orifice.wetnessLevel : 1;
        static const std::vector<std::string> wetnessWords = { "dry", "moist", "wet", "very wet", "dripping", "soaked" };
        std::string wetWord = (wetness >= 0 && wetness < static_cast<int>(wetnessWords.size())) ? wetnessWords[wetness] : "moist";

        ss << std::format("They possess a lush, {} vagina inviting penetration.\n", wetWord);
    }

    if (ass)
    {
        ss << std::format("Their rear features a well-formed ass with {} skin.\n", ass->primaryColor);
    }

    return ss.str();
}

std::string characterDescription::buildExtremitiesSection(const entity* ent)
{
    if (!ent) return "";
    std::ostringstream ss;

    const bodyPart* tail = ent->anatomy.getPart(bodySlot::TAIL);
    const bodyPart* wings = ent->anatomy.getPart(bodySlot::WINGS);
    const bodyPart* tentacles = ent->anatomy.getPart(bodySlot::TENTACLES);

    if (tail)
    {
        ss << std::format("Behind them sways a graceful {} {} tail.\n",
                          tail->primaryColor, tail->race);
    }

    if (wings)
    {
        ss << std::format("From their back extends a pair of impressive {} {} wings.\n",
                          wings->primaryColor, wings->race);
    }

    if (tentacles)
    {
        ss << std::format("Several writhing {} tentacles extend from their lower body.\n",
                          tentacles->primaryColor);
    }

    return ss.str();
}

std::string characterDescription::buildClothingAndTattoosSection(const entity* ent)
{
    if (!ent) return "";
    std::ostringstream ss;

    // List worn equipment
    std::vector<std::string> worn;
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
        ss << "Wearing: ";
        for (size_t i = 0; i < worn.size(); ++i)
        {
            ss << worn[i] << (i + 1 < worn.size() ? ", " : ".\n");
        }
    }
    else
    {
        ss << "Currently wearing no clothing.\n";
    }

    return ss.str();
}

std::string characterDescription::generateFullDescription(const entity* ent, const entity* viewer)
{
    if (!ent) return "Nothing here to see.";

    std::ostringstream ss;
    ss << buildOverviewSection(ent) << "\n";
    ss << buildHeadAndFaceSection(ent);
    ss << buildTorsoAndBreastsSection(ent);
    ss << buildGenitalsAndRearSection(ent);
    ss << buildExtremitiesSection(ent);
    ss << buildClothingAndTattoosSection(ent);

    return ss.str();
}
