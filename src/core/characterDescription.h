#pragma once

#include <string>
#include <vector>

class entity;

class characterDescription
{
public:
    static std::string generateFullDescription(const entity* ent, const entity* viewer = nullptr);
    static std::string generateSummary(const entity* ent);

    // Sectional builders
    static std::string buildOverviewSection(const entity* ent);
    static std::string buildHeadAndFaceSection(const entity* ent);
    static std::string buildTorsoAndBreastsSection(const entity* ent);
    static std::string buildGenitalsAndRearSection(const entity* ent);
    static std::string buildExtremitiesSection(const entity* ent);
    static std::string buildClothingAndTattoosSection(const entity* ent);
};
