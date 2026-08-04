#pragma once
#include <string>
#include "enums.h"

enum class mutationType
{
	GROWTH_LENGTH,
	GROWTH_DIAMETER,
	GROWTH_CUP,
	CHANGE_RACE,
	CHANGE_COLOR,
	ADD_PART,
	REMOVE_PART
};

struct anatomyMutation
{
	std::string id;
	bodySlot targetSlot;
	mutationType type;

	float amountPerMinute{0.0f}; // Used for continuous growth
	float targetValueFloat{0.0f};
	std::string targetValueString{""};

	int minutesRemaining{0};
};