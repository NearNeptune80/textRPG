#include "entities/offspringManager.h"
#include "entities/entity.h"
#include <format>
#include <algorithm>
#include <chrono>

using json = nlohmann::json;

std::vector<OffspringRecord> offspringManager::s_records;

json OffspringRecord::toJson() const
{
    json j;
    j["id"] = id;
    j["name"] = name;
    j["motherId"] = motherId;
    j["fatherId"] = fatherId;
    j["fatherName"] = fatherName;
    j["motherRace"] = motherRace;
    j["fatherRace"] = fatherRace;
    j["rolledRace"] = rolledRace;
    j["birthDay"] = birthDay;
    j["birthMonth"] = birthMonth;
    j["birthYear"] = birthYear;
    j["daysAlive"] = daysAlive;
    j["stage"] = static_cast<int>(stage);
    return j;
}

OffspringRecord OffspringRecord::fromJson(const json& j)
{
    OffspringRecord r;
    r.id = j.value("id", "");
    r.name = j.value("name", "Unnamed Child");
    r.motherId = j.value("motherId", "");
    r.fatherId = j.value("fatherId", "");
    r.fatherName = j.value("fatherName", "Unknown");
    r.motherRace = j.value("motherRace", "Human");
    r.fatherRace = j.value("fatherRace", "Human");
    r.rolledRace = j.value("rolledRace", "Human");
    r.birthDay = j.value("birthDay", 1);
    r.birthMonth = j.value("birthMonth", 1);
    r.birthYear = j.value("birthYear", 1);
    r.daysAlive = j.value("daysAlive", 0);
    r.stage = static_cast<OffspringGrowthStage>(j.value("stage", 0));
    return r;
}

void offspringManager::registerBirth(const OffspringInfo& info, const std::string& customName, int day, int month, int year)
{
    static uint64_t counter = 1;
    OffspringRecord rec;
    rec.id = std::format("child_{}_{}", std::chrono::steady_clock::now().time_since_epoch().count(), counter++);
    rec.name = customName.empty() ? ("Child of " + (info.fatherName.empty() ? "Unknown" : info.fatherName)) : customName;
    rec.motherId = info.motherId;
    rec.fatherId = info.fatherId;
    rec.fatherName = info.fatherName;
    rec.motherRace = info.motherRace;
    rec.fatherRace = info.fatherRace;
    rec.rolledRace = info.rolledRace.empty() ? info.motherRace : info.rolledRace;
    rec.birthDay = day;
    rec.birthMonth = month;
    rec.birthYear = year;
    rec.daysAlive = 0;
    rec.stage = OffspringGrowthStage::CHILD;

    s_records.push_back(rec);
}

void offspringManager::advanceDayAll()
{
    for (auto& rec : s_records)
    {
        rec.daysAlive++;
        if (rec.daysAlive >= ADULTHOOD_DAYS_THRESHOLD)
        {
            rec.stage = OffspringGrowthStage::ADULT_NPC;
        }
    }
}

std::vector<OffspringRecord>& offspringManager::getAllOffspring()
{
    return s_records;
}

OffspringRecord* offspringManager::getOffspring(const std::string& id)
{
    for (auto& rec : s_records)
    {
        if (rec.id == id) return &rec;
    }
    return nullptr;
}

std::shared_ptr<entity> offspringManager::promoteToAdultNPC(const std::string& id)
{
    auto it = std::find_if(s_records.begin(), s_records.end(), [&](const OffspringRecord& r) {
        return r.id == id;
    });

    if (it == s_records.end()) return nullptr;

    OffspringRecord rec = *it;
    s_records.erase(it);

    auto adult = std::make_shared<entity>();
    adult->id = rec.id;
    adult->name = rec.name;

    // Baseline stats
    adult->stats.setBaseStat("physique", 12.0f);
    adult->stats.setBaseStat("finesse", 12.0f);
    adult->stats.setBaseStat("willpower", 12.0f);
    adult->stats.setBaseStat("arcane", 10.0f);
    adult->stats.setBaseStat("healthMax", 100.0f);
    adult->stats.setBaseStat("health", 100.0f);
    adult->stats.setBaseStat("manaMax", 50.0f);
    adult->stats.setBaseStat("mana", 50.0f);
    adult->stats.setBaseStat("currency", 50.0f);

    // Setup racial anatomy parts
    std::string race = rec.rolledRace.empty() ? "Human" : rec.rolledRace;

    bodyPart head;
    head.id = "head_" + race;
    head.name = race + " Head";
    head.race = race;
    adult->anatomy.setPart(bodySlot::HEAD, head);

    bodyPart torso;
    torso.id = "torso_" + race;
    torso.name = race + " Torso";
    torso.race = race;
    adult->anatomy.setPart(bodySlot::TORSO, torso);

    bodyPart breasts;
    breasts.id = "breasts_" + race;
    breasts.name = "Breasts";
    breasts.race = race;
    breasts.cupSize = 1;
    adult->anatomy.setPart(bodySlot::BREASTS, breasts);

    bodyPart groin;
    groin.id = "groin_" + race;
    groin.name = "Groin";
    groin.race = race;
    groin.tags.push_back("vagina");
    adult->anatomy.setPart(bodySlot::GROIN, groin);

    return adult;
}

json offspringManager::toJson()
{
    json arr = json::array();
    for (const auto& rec : s_records)
    {
        arr.push_back(rec.toJson());
    }
    return arr;
}

void offspringManager::fromJson(const json& j)
{
    s_records.clear();
    if (!j.is_array()) return;

    for (const auto& item : j)
    {
        s_records.push_back(OffspringRecord::fromJson(item));
    }
}

void offspringManager::clear()
{
    s_records.clear();
}

