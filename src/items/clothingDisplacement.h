#pragma once

#include <string>

/**
 * Partial displacement modes for clothing items (e.g. unbuttoning a shirt, pulling aside panties).
 */
enum class DisplacementMode
{
	NONE,
	UNBUTTON,
	PULL_ASIDE,
	LIFT_UP,
	PULL_DOWN,
	OPEN
};

inline std::string displacementModeToString(DisplacementMode mode)
{
	switch (mode)
	{
		case DisplacementMode::UNBUTTON:   return "UNBUTTON";
		case DisplacementMode::PULL_ASIDE: return "PULL_ASIDE";
		case DisplacementMode::LIFT_UP:    return "LIFT_UP";
		case DisplacementMode::PULL_DOWN:  return "PULL_DOWN";
		case DisplacementMode::OPEN:       return "OPEN";
		case DisplacementMode::NONE:
		default:                           return "NONE";
	}
}

inline DisplacementMode stringToDisplacementMode(const std::string& str)
{
	if (str == "UNBUTTON")   return DisplacementMode::UNBUTTON;
	if (str == "PULL_ASIDE") return DisplacementMode::PULL_ASIDE;
	if (str == "LIFT_UP")    return DisplacementMode::LIFT_UP;
	if (str == "PULL_DOWN")  return DisplacementMode::PULL_DOWN;
	if (str == "OPEN")       return DisplacementMode::OPEN;
	return DisplacementMode::NONE;
}