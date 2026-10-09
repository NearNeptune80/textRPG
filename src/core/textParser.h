#pragma once

#include <string>
#include <string_view>

class entity;

class textParser
{
public:
    static std::string interpolate(std::string_view rawText, const entity* player, const entity* target = nullptr);
    static std::string processConditionals(std::string_view text, const entity* player, const entity* target = nullptr);

private:
    static bool evaluateCondition(std::string_view condStr, const entity* player, const entity* target);
    static std::string_view getPronoun(const entity* ent, std::string_view token);
};