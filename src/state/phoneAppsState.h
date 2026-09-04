#pragma once

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "state/iGameState.h"

class game;

enum class PhoneAppMode
{
    HOME,
    QUESTS,
    PERKS,
    SPELLS,
    FETISHES,
    STATS,
    SELFIE,
    CONTACTS,
    ENCYCLOPEDIA,
    TRANSFORM,
    MAPS,
    COMBAT_MOVES,
    MASTURBATE,
    WAIT_REST,
    ELEMENTAL
};

inline std::string phoneAppModeToString(PhoneAppMode mode)
{
    switch (mode)
    {
        case PhoneAppMode::HOME:         return "Smartphone Home";
        case PhoneAppMode::QUESTS:       return "Quests";
        case PhoneAppMode::PERKS:        return "Perk Tree";
        case PhoneAppMode::SPELLS:       return "Spells & Magic";
        case PhoneAppMode::FETISHES:     return "Fetishes & Desires";
        case PhoneAppMode::STATS:        return "Character Stats";
        case PhoneAppMode::SELFIE:       return "Selfie & Appearance";
        case PhoneAppMode::CONTACTS:     return "Contacts";
        case PhoneAppMode::ENCYCLOPEDIA: return "Encyclopedia";
        case PhoneAppMode::TRANSFORM:    return "Transformations";
        case PhoneAppMode::MAPS:         return "World & Local Maps";
        case PhoneAppMode::COMBAT_MOVES: return "Combat Moves";
        case PhoneAppMode::MASTURBATE:   return "Solo Intimacy";
        case PhoneAppMode::WAIT_REST:    return "Wait & Rest";
        case PhoneAppMode::ELEMENTAL:    return "Elemental Companion";
        default:                         return "Phone";
    }
}

enum class FetishDesireLevel
{
    HATE = 0,
    DISLIKE = 1,
    NEUTRAL = 2,
    LIKE = 3,
    LOVE = 4
};

inline std::string fetishDesireLevelToString(FetishDesireLevel level)
{
    switch (level)
    {
        case FetishDesireLevel::HATE:    return "Hate";
        case FetishDesireLevel::DISLIKE: return "Dislike";
        case FetishDesireLevel::NEUTRAL: return "Neutral";
        case FetishDesireLevel::LIKE:    return "Like";
        case FetishDesireLevel::LOVE:    return "Love";
        default:                         return "Neutral";
    }
}

enum class QuestCategoryFilter
{
    ALL = 0,
    MAIN = 1,
    SIDE = 2
};

inline std::string questCategoryFilterToString(QuestCategoryFilter filter)
{
    switch (filter)
    {
        case QuestCategoryFilter::ALL:  return "All";
        case QuestCategoryFilter::MAIN: return "Main";
        case QuestCategoryFilter::SIDE: return "Side";
        default:                        return "All";
    }
}

enum class QuestFilter
{
    ALL = 0,
    ACTIVE = 1,
    COMPLETED = 2
};

inline std::string questFilterToString(QuestFilter filter)
{
    switch (filter)
    {
        case QuestFilter::ALL:       return "All";
        case QuestFilter::ACTIVE:    return "Active";
        case QuestFilter::COMPLETED: return "Completed";
        default:                     return "All";
    }
}

class phoneAppsState : public iGameState
{
public:
    explicit phoneAppsState(PhoneAppMode mode = PhoneAppMode::HOME);
    ~phoneAppsState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    PhoneAppMode getAppMode() const { return m_mode; }
    void setAppMode(PhoneAppMode mode) { m_mode = mode; m_selectedItemIndex = 0; m_selectedQuestIndex = 0; m_feedbackText.clear(); m_questScrollY = 0.0f; m_contactsExpandedIdx = -1; }

    int getSelectedCategory() const { return m_selectedCategory; }
    void setSelectedCategory(int idx) { m_selectedCategory = idx; m_selectedItemIndex = 0; }

    int getSelectedItemIndex() const { return m_selectedItemIndex; }
    void setSelectedItemIndex(int idx) { m_selectedItemIndex = idx; }

    QuestCategoryFilter getQuestCategoryFilter() const { return m_questCategoryFilter; }
    void setQuestCategoryFilter(QuestCategoryFilter cat) { m_questCategoryFilter = cat; m_selectedQuestIndex = 0; }

    bool isShowCompleted() const { return m_showCompletedQuests; }
    void setShowCompleted(bool val) { m_showCompletedQuests = val; m_selectedQuestIndex = 0; }
    void toggleShowCompleted() { m_showCompletedQuests = !m_showCompletedQuests; m_selectedQuestIndex = 0; }

    const std::string& getExpandedQuestId() const { return m_expandedQuestId; }
    void setExpandedQuestId(const std::string& qid) { m_expandedQuestId = qid; }
    void toggleExpandedQuest(const std::string& qid) { if (m_expandedQuestId == qid) m_expandedQuestId.clear(); else m_expandedQuestId = qid; }

    float getQuestScrollY() const { return m_questScrollY; }
    void setQuestScrollY(float sy) { m_questScrollY = sy; }
    float getQuestMaxScrollY() const { return m_questMaxScrollY; }
    void setQuestMaxScrollY(float msy) { m_questMaxScrollY = msy; }
    void scrollQuestList(float delta) { m_questScrollY = std::clamp(m_questScrollY + delta, 0.0f, m_questMaxScrollY); }

    QuestFilter getQuestFilter() const { return m_questFilter; }
    void setQuestFilter(QuestFilter filter) { m_questFilter = filter; m_selectedQuestIndex = 0; }
    void cycleQuestFilter() {
        int next = (static_cast<int>(m_questFilter) + 1) % 3;
        setQuestFilter(static_cast<QuestFilter>(next));
    }

    int getSelectedQuestIndex() const { return m_selectedQuestIndex; }
    void setSelectedQuestIndex(int idx) { m_selectedQuestIndex = idx; }

    int getQuestPage() const { return m_questPage; }
    void setQuestPage(int page) { m_questPage = page; }

    const std::string& getFeedbackText() const { return m_feedbackText; }
    void setFeedbackText(const std::string& text) { m_feedbackText = text; }

    int getStatsTab() const { return m_statsTab; }
    void setStatsTab(int tab) { m_statsTab = tab; }

    int getEncyclopediaCategory() const { return m_encyclopediaCategory; }
    void setEncyclopediaCategory(int cat) { m_encyclopediaCategory = cat; }

    int getMapsView() const { return m_mapsView; }
    void setMapsView(int view) { m_mapsView = view; }

    int getContactsSelectedIdx() const { return m_contactsSelectedIdx; }
    void setContactsSelectedIdx(int idx) { m_contactsSelectedIdx = idx; }

    int getContactsExpandedIdx() const { return m_contactsExpandedIdx; }
    void setContactsExpandedIdx(int idx) { m_contactsExpandedIdx = idx; }
    void toggleContactsExpanded(int idx) { m_contactsExpandedIdx = (m_contactsExpandedIdx == idx) ? -1 : idx; }

    int getPerksCategory() const { return m_perksCategory; }
    void setPerksCategory(int cat) { m_perksCategory = cat; }

    int getSpellsSchool() const { return m_spellsSchool; }
    void setSpellsSchool(int school) { m_spellsSchool = school; }

    int getSelectedCombatSlot() const { return m_selectedCombatSlot; }
    void setSelectedCombatSlot(int slot) { m_selectedCombatSlot = slot; }

    int getCombatCategory() const { return m_combatCategory; }
    void setCombatCategory(int cat) { m_combatCategory = cat; }

    bool isElementalSummoned() const { return m_elementalSummoned; }
    void setElementalSummoned(bool val) { m_elementalSummoned = val; }
    void toggleElementalSummoned() { m_elementalSummoned = !m_elementalSummoned; }

    bool isElementalActiveForm() const { return m_elementalActiveForm; }
    void setElementalActiveForm(bool val) { m_elementalActiveForm = val; }
    void toggleElementalActiveForm() { m_elementalActiveForm = !m_elementalActiveForm; }

    FetishDesireLevel getFetishDesire(const std::string& fetishKey) const
    {
        auto it = m_fetishDesires.find(fetishKey);
        if (it != m_fetishDesires.end()) return it->second;
        return FetishDesireLevel::NEUTRAL;
    }
    void setFetishDesire(const std::string& fetishKey, FetishDesireLevel level)
    {
        m_fetishDesires[fetishKey] = level;
    }
    const std::unordered_map<std::string, FetishDesireLevel>& getAllFetishDesires() const { return m_fetishDesires; }

    const nlohmann::json& getAppData() const { return m_appData; }

    void loadData(PhoneAppMode mode);

private:
    PhoneAppMode m_mode = PhoneAppMode::HOME;
    int m_selectedCategory = 0;
    int m_selectedItemIndex = 0;
    QuestCategoryFilter m_questCategoryFilter = QuestCategoryFilter::ALL;
    bool m_showCompletedQuests = false;
    std::string m_expandedQuestId;
    float m_questScrollY = 0.0f;
    float m_questMaxScrollY = 0.0f;
    QuestFilter m_questFilter = QuestFilter::ALL;
    int m_selectedQuestIndex = 0;
    int m_questPage = 0;
    std::string m_feedbackText;
    nlohmann::json m_appData;

    int m_statsTab = 0; // 0: Core, 1: Body, 2: Sex, 3: Pregnancy
    int m_encyclopediaCategory = 0; // 0: Species, 1: Weapons, 2: Clothing, 3: Items
    int m_mapsView = 0; // 0: World, 1: Local, 2: POI
    int m_contactsSelectedIdx = -1; // -1: List, >=0: Contact detail
    int m_contactsExpandedIdx = -1; // -1: None expanded, >=0: Index of expanded card in list
    int m_perksCategory = 0; // 0: All, 1: Physical, 2: Arcane, 3: Social, 4: Demonic
    int m_spellsSchool = 0; // 0: All, 1: Arcane, 2: Fire, 3: Restoration, 4: Translocation
    int m_selectedCombatSlot = 0;
    int m_combatCategory = 0; // 0: All, 1: Physical, 2: Spells, 3: Tactical
    bool m_elementalSummoned = false;
    bool m_elementalActiveForm = false;
    std::unordered_map<std::string, FetishDesireLevel> m_fetishDesires;
};
