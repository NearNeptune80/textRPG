#include "core/contentFilterManager.h"
#include <algorithm>
#include <cctype>
#include <sstream>

std::string ContentFilterManager::normalizeTag(const std::string& tag)
{
    std::string result;
    result.reserve(tag.size());
    for (char c : tag)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
        {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        else if (c == ' ' || c == '-' || c == '_')
        {
            if (!result.empty() && result.back() != '_')
            {
                result.push_back('_');
            }
        }
    }
    while (!result.empty() && result.back() == '_')
    {
        result.pop_back();
    }
    return result;
}

std::string ContentFilterManager::getTagDisplayName(const std::string& tag)
{
    std::string norm = normalizeTag(tag);
    if (norm == "non_con" || norm == "noncon" || norm == "rape") return "Non-Consent";
    if (norm == "extreme" || norm == "sadism" || norm == "sadistic" || norm == "gore") return "Extreme Content";
    if (norm == "public_sex" || norm == "publicsex" || norm == "exhibitionism") return "Public Sex";
    if (norm == "pregnancy" || norm == "impregnation" || norm == "insemination") return "Pregnancy";
    if (norm == "lactation" || norm == "milk") return "Lactation";
    if (norm == "watersports" || norm == "urination" || norm == "piss") return "Watersports";
    if (norm == "spitting" || norm == "oral_degradation") return "Spitting";
    if (norm == "forced_tf" || norm == "forcedtf" || norm == "involuntary_tf") return "Forced Transformation";
    if (norm == "tentacles" || norm == "monsters" || norm == "tentacle") return "Tentacles";
    if (norm == "bdsm" || norm == "bondage" || norm == "restraints") return "BDSM";
    if (norm == "incest") return "Incest";
    if (norm == "size_diff" || norm == "size_difference") return "Size Difference";
    if (norm == "prolapse" || norm == "stretching") return "Prolapse";
    if (norm == "aphrodisiacs" || norm == "drugs") return "Aphrodisiacs";

    // Capitalize normalized tag as fallback
    if (!norm.empty())
    {
        norm[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(norm[0])));
    }
    return norm;
}

bool ContentFilterManager::isTagAllowed(const ContentSettings& settings, const std::string& tag)
{
    std::string norm = normalizeTag(tag);
    if (norm.empty()) return true;

    // Explicit Core Content Toggles
    if ((norm == "non_con" || norm == "noncon" || norm == "rape") && !settings.nonConEnabled) return false;
    if ((norm == "extreme" || norm == "sadism" || norm == "sadistic" || norm == "gore" || norm == "snuff") && !settings.extremeContentEnabled) return false;
    if ((norm == "public_sex" || norm == "publicsex" || norm == "exhibitionism") && !settings.publicSexEnabled) return false;
    if ((norm == "pregnancy" || norm == "impregnation" || norm == "insemination") && !settings.pregnancyEnabled) return false;
    if ((norm == "lactation" || norm == "milk" || norm == "nursing") && !settings.lactationEnabled) return false;
    if ((norm == "watersports" || norm == "urination" || norm == "piss") && !settings.watersportsEnabled) return false;
    if ((norm == "spitting" || norm == "oral_degradation") && !settings.spittingEnabled) return false;
    if ((norm == "forced_tf" || norm == "forcedtf" || norm == "involuntary_tf") && !settings.forcedTfEnabled) return false;
    if ((norm == "tentacles" || norm == "monsters" || norm == "tentacle") && !settings.tentaclesEnabled) return false;
    if ((norm == "bdsm" || norm == "bondage" || norm == "restraints") && !settings.bdsmEnabled) return false;
    if (norm == "incest" && !settings.incestEnabled) return false;
    if ((norm == "size_diff" || norm == "size_difference" || norm == "macro" || norm == "micro") && !settings.sizeDifferenceEnabled) return false;
    if ((norm == "prolapse" || norm == "stretching") && !settings.prolapseEnabled) return false;
    if ((norm == "aphrodisiacs" || norm == "drugs") && !settings.aphrodisiacsEnabled) return false;

    // Fetish Preferences Lookup (Rating 0 = Disabled)
    for (const auto& [fetName, rating] : settings.fetishPreferences)
    {
        std::string normFet = normalizeTag(fetName);
        if (normFet == norm || normFet.find(norm) != std::string::npos || norm.find(normFet) != std::string::npos)
        {
            if (rating == 0) return false;
        }
    }

    return true;
}

std::vector<std::string> ContentFilterManager::getDisallowedTags(const ContentSettings& settings, const std::vector<std::string>& tags)
{
    std::vector<std::string> disallowed;
    for (const auto& t : tags)
    {
        if (!isTagAllowed(settings, t))
        {
            disallowed.push_back(t);
        }
    }
    return disallowed;
}

bool ContentFilterManager::isSceneAllowed(const ContentSettings& settings, const questScene& scene)
{
    if (scene.contentTags.empty()) return true;
    for (const auto& t : scene.contentTags)
    {
        if (!isTagAllowed(settings, t)) return false;
    }
    return true;
}

bool ContentFilterManager::isChoiceAllowed(const ContentSettings& settings, const dialogueChoice& choice)
{
    if (choice.contentTags.empty()) return true;
    for (const auto& t : choice.contentTags)
    {
        if (!isTagAllowed(settings, t)) return false;
    }
    return true;
}

std::vector<NarrativeSegment> ContentFilterManager::parseNarrativeSegments(const ContentSettings& settings, const std::string& rawText)
{
    std::vector<NarrativeSegment> segments;
    if (rawText.empty()) return segments;

    size_t curPos = 0;
    int blockIdx = 0;

    while (curPos < rawText.size())
    {
        size_t tagStart = rawText.find("[content:", curPos);
        if (tagStart == std::string::npos)
        {
            NarrativeSegment seg;
            seg.isDropdown = false;
            seg.text = rawText.substr(curPos);
            if (!seg.text.empty()) segments.push_back(seg);
            break;
        }

        if (tagStart > curPos)
        {
            NarrativeSegment seg;
            seg.isDropdown = false;
            seg.text = rawText.substr(curPos, tagStart - curPos);
            if (!seg.text.empty()) segments.push_back(seg);
        }

        size_t tagEnd = rawText.find(']', tagStart);
        if (tagEnd == std::string::npos)
        {
            NarrativeSegment seg;
            seg.isDropdown = false;
            seg.text = rawText.substr(tagStart);
            segments.push_back(seg);
            break;
        }

        std::string meta = rawText.substr(tagStart + 9, tagEnd - (tagStart + 9));
        std::string tag = meta;
        std::string title = "";

        size_t colonPos = meta.find(':');
        if (colonPos != std::string::npos)
        {
            tag = meta.substr(0, colonPos);
            title = meta.substr(colonPos + 1);
        }
        if (title.empty())
        {
            title = getTagDisplayName(tag);
        }

        size_t closeTag = rawText.find("[/content]", tagEnd + 1);
        if (closeTag == std::string::npos)
        {
            NarrativeSegment seg;
            seg.isDropdown = false;
            seg.text = rawText.substr(tagEnd + 1);
            segments.push_back(seg);
            break;
        }

        std::string contentText = rawText.substr(tagEnd + 1, closeTag - (tagEnd + 1));
        curPos = closeTag + 10;
        blockIdx++;

        bool allowed = isTagAllowed(settings, tag);
        if (allowed)
        {
            NarrativeSegment seg;
            seg.isDropdown = false;
            seg.text = contentText;
            segments.push_back(seg);
        }
        else
        {
            if (settings.contentFilterMode == ContentFilterMode::BLOCK_AND_SKIP)
            {
                // Redact completely
                NarrativeSegment seg;
                seg.isDropdown = false;
                seg.text = " [Content omitted: " + title + "] ";
                segments.push_back(seg);
            }
            else
            {
                // DROPDOWN or WARN_CONFIRM: render as collapsible dropdown segment
                NarrativeSegment seg;
                seg.isDropdown = true;
                seg.dropdown.id = "dropdown_" + std::to_string(blockIdx) + "_" + normalizeTag(tag);
                seg.dropdown.tag = tag;
                seg.dropdown.title = title;
                seg.dropdown.content = contentText;
                segments.push_back(seg);
            }
        }
    }

    return segments;
}

std::string ContentFilterManager::filterPlainText(const ContentSettings& settings, const std::string& rawText)
{
    auto segments = parseNarrativeSegments(settings, rawText);
    std::string result;
    for (const auto& seg : segments)
    {
        if (!seg.isDropdown)
        {
            result += seg.text;
        }
        else
        {
            result += "\n[Content Warning: " + seg.dropdown.title + "]\n" + seg.dropdown.content + "\n";
        }
    }
    return result;
}
