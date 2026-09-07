#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "settings/gameSettings.h"
#include "quest/quest.h"

struct ContentDropdownBlock
{
    std::string id;
    std::string tag;
    std::string title;
    std::string content;
};

struct NarrativeSegment
{
    bool isDropdown = false;
    std::string text;
    ContentDropdownBlock dropdown;
};

class ContentFilterManager
{
public:
    static std::string normalizeTag(const std::string& tag);

    static bool isTagAllowed(const ContentSettings& settings, const std::string& tag);

    static std::vector<std::string> getDisallowedTags(const ContentSettings& settings, const std::vector<std::string>& tags);

    static bool isSceneAllowed(const ContentSettings& settings, const questScene& scene);

    static bool isChoiceAllowed(const ContentSettings& settings, const dialogueChoice& choice);

    static std::vector<NarrativeSegment> parseNarrativeSegments(const ContentSettings& settings, const std::string& rawText);

    static std::string filterPlainText(const ContentSettings& settings, const std::string& rawText);

    static std::string getTagDisplayName(const std::string& tag);
};
