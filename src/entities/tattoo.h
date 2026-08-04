#pragma once

#include <string>
#include <vector>

#include "items/enchantment.h"

struct tattoo
{
	std::string id;
	std::string name;
	std::string color;
	bool glowing = false;

	std::vector<enchantment> enchantments;
	std::vector<std::string> tags;
};