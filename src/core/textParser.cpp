#include "core/textParser.h"

#include <format>
#include <algorithm>
#include "entities/entity.h"
#include "entities/bodyPart.h"

std::string_view textParser::getPronoun(const entity* ent, std::string_view token)
{
    if (!ent) return token;

    bool isFemale = ent->anatomy.isFeminine();

    if (token == "he/she" || token == "she/he") return isFemale ? "she" : "he";
    if (token == "He/She" || token == "She/He") return isFemale ? "She" : "He";
    if (token == "him/her" || token == "her/him") return isFemale ? "her" : "him";
    if (token == "his/her" || token == "her/his") return isFemale ? "her" : "his";
    if (token == "His/Her" || token == "Her/His") return isFemale ? "Her" : "His";
    if (token == "himself/herself") return isFemale ? "herself" : "himself";

    return token;
}

bool textParser::evaluateCondition(std::string_view condStr, const entity* player, const entity* target)
{
    while (!condStr.empty() && (condStr.front() == ' ' || condStr.front() == '\t')) condStr.remove_prefix(1);
    while (!condStr.empty() && (condStr.back() == ' ' || condStr.back() == '\t')) condStr.remove_suffix(1);

    if (condStr.starts_with("NOT ") || condStr.starts_with("not "))
    {
        return !evaluateCondition(condStr.substr(4), player, target);
    }
    if (condStr.starts_with("!"))
    {
        return !evaluateCondition(condStr.substr(1), player, target);
    }

    if (condStr == "HAS_PENIS") return player && player->anatomy.hasPenis();
    if (condStr == "HAS_VAGINA") return player && player->anatomy.hasVagina();
    if (condStr == "HAS_BREASTS") return player && player->anatomy.hasBreasts();
    if (condStr == "IS_FEMALE" || condStr == "IS_FEMININE") return player && player->anatomy.isFeminine();
    if (condStr == "IS_MALE" || condStr == "IS_MASCULINE") return player && !player->anatomy.isFeminine();
    if (condStr == "IS_PREGNANT") return player && player->gestation.isPregnant;

    if (condStr == "TARGET_HAS_PENIS") return target && target->anatomy.hasPenis();
    if (condStr == "TARGET_HAS_VAGINA") return target && target->anatomy.hasVagina();
    if (condStr == "TARGET_IS_FEMALE") return target && target->anatomy.isFeminine();

    if (condStr.starts_with("RACE_IS:"))
    {
        std::string_view expected = condStr.substr(8);
        return player && player->anatomy.getDominantRace() == expected;
    }
    if (condStr.starts_with("TARGET_RACE_IS:"))
    {
        std::string_view expected = condStr.substr(15);
        return target && target->anatomy.getDominantRace() == expected;
    }
    if (condStr.starts_with("GENDER_IS:"))
    {
        std::string_view expected = condStr.substr(10);
        return player && genderArchetypeToString(player->anatomy.getGenderArchetype()) == expected;
    }

    if (condStr.starts_with("STAT_MIN:"))
    {
        size_t colon = condStr.find(':', 9);
        if (colon != std::string_view::npos)
        {
            std::string stat(condStr.substr(9, colon - 9));
            float reqVal = 0.0f;
            try { reqVal = std::stof(std::string(condStr.substr(colon + 1))); } catch (...) {}
            return player && player->getStat(stat) >= reqVal;
        }
    }
    if (condStr.starts_with("STAT_MAX:"))
    {
        size_t colon = condStr.find(':', 9);
        if (colon != std::string_view::npos)
        {
            std::string stat(condStr.substr(9, colon - 9));
            float reqVal = 0.0f;
            try { reqVal = std::stof(std::string(condStr.substr(colon + 1))); } catch (...) {}
            return player && player->getStat(stat) <= reqVal;
        }
    }

    if (condStr.starts_with("VAR_MIN:") || condStr.starts_with("QUEST_MIN:"))
    {
        size_t colon = condStr.find(':', 8);
        if (colon != std::string_view::npos)
        {
            std::string varName(condStr.substr(8, colon - 8));
            int reqVal = 0;
            try { reqVal = std::stoi(std::string(condStr.substr(colon + 1))); } catch (...) {}
            return player && player->quests.getQuestStage(varName) >= reqVal;
        }
    }
    if (condStr.starts_with("VAR_EQUALS:") || condStr.starts_with("QUEST_STAGE:"))
    {
        size_t prefixLen = condStr.starts_with("VAR_") ? 11 : 12;
        size_t colon = condStr.find(':', prefixLen);
        if (colon != std::string_view::npos)
        {
            std::string varName(condStr.substr(prefixLen, colon - prefixLen));
            int reqVal = 0;
            try { reqVal = std::stoi(std::string(condStr.substr(colon + 1))); } catch (...) {}
            return player && player->quests.getQuestStage(varName) == reqVal;
        }
    }

    if (condStr.starts_with("HAS_ITEM:"))
    {
        std::string itemId(condStr.substr(9));
        if (player)
        {
            for (const auto& it : player->inventory.backpack)
            {
                if (it && it->id == itemId) return true;
            }
        }
        return false;
    }

    return true;
}

std::string textParser::processConditionals(std::string_view text, const entity* player, const entity* target)
{
    std::string str(text);
    while (true)
    {
        size_t ifPos = str.find("[IF ");
        if (ifPos == std::string::npos) break;

        size_t condEnd = str.find(']', ifPos + 4);
        if (condEnd == std::string::npos) break;

        std::string_view condStr = std::string_view(str).substr(ifPos + 4, condEnd - (ifPos + 4));

        size_t endifPos = str.find("[ENDIF]", condEnd + 1);
        if (endifPos == std::string::npos) break;

        size_t elsePos = str.find("[ELSE]", condEnd + 1);
        bool hasElse = (elsePos != std::string::npos && elsePos < endifPos);

        bool condResult = evaluateCondition(condStr, player, target);
        std::string replacement;
        if (condResult)
        {
            size_t trueEnd = hasElse ? elsePos : endifPos;
            replacement = str.substr(condEnd + 1, trueEnd - (condEnd + 1));
        }
        else if (hasElse)
        {
            replacement = str.substr(elsePos + 6, endifPos - (elsePos + 6));
        }

        size_t totalLen = (endifPos + 7) - ifPos;
        str.replace(ifPos, totalLen, replacement);
    }
    return str;
}

std::string textParser::interpolate(std::string_view rawText, const entity* player, const entity* target)
{
    if (rawText.empty()) return "";

    std::string processed = processConditionals(rawText, player, target);
    std::string result;
    result.reserve(processed.size() + 64);

    size_t i = 0;
    while (i < processed.size())
    {
        char openChar = processed[i];
        if (openChar == '{' || openChar == '[')
        {
            char closeChar = (openChar == '{') ? '}' : ']';
            size_t end = processed.find(closeChar, i + 1);
            if (end != std::string::npos)
            {
                std::string tag(processed.substr(i + 1, end - (i + 1)));
                std::string tagLower = tag;
                std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
                bool resolved = false;

                if (tagLower.starts_with("player.") && player)
                {
                    std::string sub = tagLower.substr(7);
                    if (sub == "name") { result += player->name; resolved = true; }
                    else if (sub == "level") { result += std::to_string(player->stats.level); resolved = true; }
                    else if (sub == "race") { result += player->anatomy.getDominantRace(); resolved = true; }
                    else if (sub == "gender") { result += genderArchetypeToString(player->anatomy.getGenderArchetype()); resolved = true; }
                    else if (sub == "cupsize")
                    {
                        const auto* br = player->anatomy.getPart(bodySlot::BREASTS);
                        result += br ? bodyPart::getCupSizeName(br->cupSize) : "Flat";
                        resolved = true;
                    }
                    else if (sub.starts_with("stat:"))
                    {
                        std::string statName = tag.substr(12);
                        result += std::format("{:.0f}", player->getStat(statName));
                        resolved = true;
                    }
                    else if (sub.starts_with("var:"))
                    {
                        std::string varName = tag.substr(11);
                        result += std::to_string(player->quests.getQuestStage(varName));
                        resolved = true;
                    }
                    else
                    {
                        result += getPronoun(player, sub);
                        resolved = true;
                    }
                }
                else if (tagLower.starts_with("target.") && target)
                {
                    std::string sub = tagLower.substr(7);
                    if (sub == "name") { result += target->name; resolved = true; }
                    else if (sub == "level") { result += std::to_string(target->stats.level); resolved = true; }
                    else if (sub == "race") { result += target->anatomy.getDominantRace(); resolved = true; }
                    else if (sub == "gender") { result += genderArchetypeToString(target->anatomy.getGenderArchetype()); resolved = true; }
                    else if (sub == "cupsize")
                    {
                        const auto* br = target->anatomy.getPart(bodySlot::BREASTS);
                        result += br ? bodyPart::getCupSizeName(br->cupSize) : "Flat";
                        resolved = true;
                    }
                    else if (sub.starts_with("stat:"))
                    {
                        std::string statName = tag.substr(12);
                        result += std::format("{:.0f}", target->getStat(statName));
                        resolved = true;
                    }
                    else
                    {
                        result += getPronoun(target, sub);
                        resolved = true;
                    }
                }

                if (resolved)
                {
                    i = end + 1;
                    continue;
                }
            }
        }
        result.push_back(processed[i]);
        i++;
    }

    return result;
}