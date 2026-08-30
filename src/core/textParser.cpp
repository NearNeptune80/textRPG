#include "core/textParser.h"

#include <format>
#include "entities/entity.h"

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

std::string textParser::interpolate(std::string_view rawText, const entity* player, const entity* target)
{
    if (rawText.empty()) return "";

    std::string result;
    result.reserve(rawText.size() + 64);

    size_t i = 0;
    while (i < rawText.size())
    {
        if (rawText[i] == '{')
        {
            size_t end = rawText.find('}', i + 1);
            if (end != std::string_view::npos)
            {
                std::string_view tag = rawText.substr(i + 1, end - (i + 1));
                bool resolved = false;

                if (tag.starts_with("player.") && player)
                {
                    std::string_view sub = tag.substr(7);
                    if (sub == "name") { result += player->name; resolved = true; }
                    else if (sub == "level") { result += std::to_string(player->stats.level); resolved = true; }
                    else
                    {
                        result += getPronoun(player, sub);
                        resolved = true;
                    }
                }
                else if (tag.starts_with("target.") && target)
                {
                    std::string_view sub = tag.substr(7);
                    if (sub == "name") { result += target->name; resolved = true; }
                    else if (sub == "level") { result += std::to_string(target->stats.level); resolved = true; }
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
        result.push_back(rawText[i]);
        i++;
    }

    return result;
}