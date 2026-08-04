#pragma once

#include <string>

class entity;

class textParser
{
public:
	static std::string interpolate(const std::string& rawText, const entity* player, const entity* target = nullptr);

private:
	static std::string getPronoun(const entity* ent, const std::string& token);
};