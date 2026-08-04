#include "textParser.h"
#include "entity.h"
#include <regex>
#include <algorithm>

std::string textParser::getPronoun(const entity* ent, const std::string& token)
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

std::string textParser::interpolate(const std::string& rawText, const entity* player, const entity* target)
{
    if (rawText.empty()) return rawText;

    std::string result = rawText;

    // Helper lambda for tag replacement
    auto replaceTag = [&](const std::string& tag, const std::string& replacement) {
        size_t pos = 0;
        while ((pos = result.find(tag, pos)) != std::string::npos) {
            result.replace(pos, tag.length(), replacement);
            pos += replacement.length();
        }
    };

    // Player Tokens
    if (player)
    {
        replaceTag("{player.name}", player->name);
        replaceTag("{player.level}", std::to_string(player->stats.level));
        replaceTag("{player.he/she}", getPronoun(player, "he/she"));
        replaceTag("{player.He/She}", getPronoun(player, "He/She"));
        replaceTag("{player.him/her}", getPronoun(player, "him/her"));
        replaceTag("{player.his/her}", getPronoun(player, "his/her"));
        replaceTag("{player.His/Her}", getPronoun(player, "His/Her"));
    }

    // Target NPC Tokens
    if (target)
    {
        replaceTag("{target.name}", target->name);
        replaceTag("{target.level}", std::to_string(target->stats.level));
        replaceTag("{target.he/she}", getPronoun(target, "he/she"));
        replaceTag("{target.He/She}", getPronoun(target, "He/She"));
        replaceTag("{target.him/her}", getPronoun(target, "him/her"));
        replaceTag("{target.his/her}", getPronoun(target, "his/her"));
        replaceTag("{target.His/Her}", getPronoun(target, "His/Her"));
    }

    return result;
}