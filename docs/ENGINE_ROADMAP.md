# MASTER ENGINE ROADMAP: Phases 1–3 Systems Specification

> **Status**: APPROVED & LOCKED SPECIFICATION  
> **Target Version**: textRPG Engine v2.0 (C++26 Headless Simulation)  
> **Architectural Invariant**: Strict Model-View Separation. All simulation logic lives in headless C++ classes (`src/core/`, `src/state/`, `src/entities/`, `src/map/`, `src/items/`). All visual presentations live in `src/ui/`. All game balance, templates, rooms, jobs, and dialogues live in `data/`.

---

## Executive Overview: Locked Architectural Decisions

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                               PHASED ENGINE ARCHITECTURE                               │
├─────────┬───────────────────────────────────┬──────────────────────────────────────────┤
│ PHASE   │ SUBSYSTEM                         │ CORE MECHANICS & CONSTRAINTS             │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 1A      │ Servant Management & Stamina Loop │ • 24-hour schedule (slots 0..23)         │
│ [DONE]  │ (VERIFIED - Suite 54)             │ • Stats: obedience, morale, affection    │
│         │                                   │ • Work drains stamina; Rest regenerates  │
│         │                                   │ • Zero stamina = severe exhaustion shock │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 1B      │ Customizable Tile Upgrade Trees   │ • NO vault / NO wardrobe presets         │
│ [DONE]  │ & Safe Storage                    │ • Safe tiles persist items indefinitely  │
│         │ (VERIFIED - Suite 55)             │ • Branching upgrades from room_types.json│
│         │                                   │ • Full demolition / teardown to empty    │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 2A      │ Binary Offspring & Nursery        │ • Binary aging: Child -> Adult NPC       │
│         │                                   │ • Interactive birth scene at 100% gest.  │
│         │                                   │ • Persistent OffspringManager            │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 2B      │ Composable Generation, Milking    │ • 3 JSON pools: Body, Outfit, Profession │
│         │ & 1-Tile Client Rooms             │ • Internal anatomy fluid -> Item bottling│
│         │                                   │ • Service tile -> "Get Client" -> Payout │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 3A      │ Universal Affection & Corruption  │ • entity::affection (-100 to 100)        │
│         │                                   │ • entity::corruption (0 to 100)          │
│         │                                   │ • NO trust stat; on ALL NPCs             │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 3B      │ Weather & Item Fluid Soiling      │ • NO fabric translucency                 │
│         │                                   │ • Item soiling tracks fluids & penalties │
│         │                                   │ • Regional weather from weather.json     │
├─────────┼───────────────────────────────────┼──────────────────────────────────────────┤
│ 3C      │ Overworld Spell Casting           │ • spells.json "canCastInOverworld": true │
│         │                                   │ • Exploration Action Grid spell trigger  │
└─────────┴───────────────────────────────────┴──────────────────────────────────────────┘
```

---

## Phase 1A: Servant Management, Hourly Schedule & Stamina [COMPLETED - Suite 54]

### 1. Architectural Design & Psychological Model
Servants are bound characters managed through a centralized `servantManager`. Psychology is driven by three interconnected attributes plus a physical stamina resource:

* **Obedience** (`0.0f` to `100.0f`): Willingness to execute difficult, degrading, or intensive tasks without rebellion.
* **Morale** (`0.0f` to `100.0f`): Mental fortitude, happiness, and resistance to despair.
* **Affection** (`-100.0f` to `100.0f`): Personal emotional attachment toward the player.
* **Stamina** (`0.0f` to `100.0f`): Physical endurance pool. Working drains stamina; unassigned or resting hours recover it.

### 2. C++ Struct & Class Definitions (`src/entities/servantComponent.h`)

```cpp
#pragma once

#include <string>
#include <array>
#include <memory>
#include <nlohmann/json.hpp>

struct ServantJobDefinition
{
    std::string id;
    std::string name;
    std::string description;
    float staminaCostPerHour = 10.0f;
    float staminaRegenPerHour = 0.0f;
    float obedienceDelta = 0.0f;
    float moraleDelta = 0.0f;
    float affectionDelta = 0.0f;
    float goldYieldPerHour = 0.0f;
    std::string itemYieldId = "";
    float itemYieldChance = 0.0f;
    std::vector<std::string> requiredTags;
};

struct ServantPsychology
{
    float obedience = 50.0f;
    float morale = 75.0f;
    float affection = 0.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    bool isExhausted = false;

    void clamp()
    {
        obedience = std::clamp(obedience, 0.0f, 100.0f);
        morale = std::clamp(morale, 0.0f, 100.0f);
        affection = std::clamp(affection, -100.0f, 100.0f);
        stamina = std::clamp(stamina, 0.0f, maxStamina);
        isExhausted = (stamina <= 0.0f);
    }
};

class servantComponent
{
public:
    std::string servantId;
    ServantPsychology stats;
    std::array<std::string, 24> hourlySchedule; // Slots 0..23; "" or "rest" denotes rest

    servantComponent()
    {
        hourlySchedule.fill("rest");
    }

    void setJobForHour(int hour, const std::string& jobId)
    {
        if (hour >= 0 && hour < 24) hourlySchedule[hour] = jobId;
    }

    const std::string& getJobForHour(int hour) const
    {
        static const std::string restJob = "rest";
        if (hour >= 0 && hour < 24) return hourlySchedule[hour];
        return restJob;
    }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};
```

### 3. Hourly Simulation Tick Algorithm (`src/core/servantManager.cpp`)

```
Function: tickHourlyServants(game* g, int currentHour)
For each servant in servantManager:
    jobId = servant.getJobForHour(currentHour)
    jobDef = getJobDefinition(jobId)

    If jobId == "rest" or jobId is empty:
        // Regeneration Loop
        servant.stats.stamina += 15.0f // base rest recovery rate
        servant.stats.morale += 0.5f
        servant.stats.clamp()
        Continue

    If servant.stats.stamina <= 0.0f:
        // Severe Exhaustion Shock
        servant.stats.isExhausted = true
        servant.stats.morale -= 5.0f
        servant.stats.affection -= 3.0f
        servant.stats.obedience -= 2.0f
        servant.stats.clamp()
        // Yields are ZERO while working exhausted
        Log: "[EXHAUSTION] Servant collapses from overwork! Zero yields produced."
    Else:
        // Normal Work Execution
        servant.stats.stamina -= jobDef.staminaCostPerHour
        servant.stats.morale += jobDef.moraleDelta
        servant.stats.obedience += jobDef.obedienceDelta
        servant.stats.affection += jobDef.affectionDelta
        servant.stats.clamp()

        // Distribute Yields
        If jobDef.goldYieldPerHour > 0:
            g->getPlayer()->addStat("currency", jobDef.goldYieldPerHour)
        If not jobDef.itemYieldId.empty() and roll(jobDef.itemYieldChance):
            depositToSafeTileOrPlayer(jobDef.itemYieldId)
```

### 4. JSON Schema (`data/config/servant_jobs.json`)

```json
{
  "version": "1.0",
  "restConfig": {
    "baseStaminaRegenPerHour": 15.0,
    "bedBonusRegenPerHour": 5.0
  },
  "jobs": [
    {
      "id": "maid_cleaning",
      "name": "Mansion Cleaning",
      "description": "Dusting, scrubbing floors, and polishing furnishings.",
      "staminaCostPerHour": 12.0,
      "obedienceDelta": 0.2,
      "moraleDelta": -0.3,
      "affectionDelta": 0.1,
      "goldYieldPerHour": 0.0,
      "itemYieldId": "",
      "itemYieldChance": 0.0,
      "requiredTags": []
    },
    {
      "id": "milking_duty",
      "name": "Milking Station Extraction",
      "description": "Extracting biological fluids in the specialized milking stalls.",
      "staminaCostPerHour": 20.0,
      "obedienceDelta": 0.5,
      "moraleDelta": -1.0,
      "affectionDelta": -0.5,
      "goldYieldPerHour": 0.0,
      "itemYieldId": "item_milk_bottle",
      "itemYieldChance": 0.75,
      "requiredTags": ["lactating"]
    },
    {
      "id": "guard_patrol",
      "name": "Estate Security Patrol",
      "description": "Standing watch around the perimeter walls.",
      "staminaCostPerHour": 15.0,
      "obedienceDelta": 0.3,
      "moraleDelta": 0.0,
      "affectionDelta": 0.2,
      "goldYieldPerHour": 5.0,
      "itemYieldId": "",
      "itemYieldChance": 0.0,
      "requiredTags": ["combat_capable"]
    }
  ]
}
```

### 5. Action Grid & State Integration
* `explorationState` exposes `"Servants"` button when within home estate map boundaries.
* Transitions to `servantState` featuring:
  * Left Panel: List of owned servants with obedience/morale/stamina gauges.
  * Center Panel: 24-hour visual schedule grid (00:00 to 23:00) with color-coded job pills.
  * Bottom Action Grid: Assign Job, Set Full Day Rest, Inspect Anatomy, Release Servant.

### 6. Automated Unit Test Criteria (Test Suite 54)
1. `servantComponent` initializes 24 slots defaulted to `"rest"`.
2. Working `maid_cleaning` for 4 consecutive hours reduces stamina by exactly 48.0 and increases obedience.
3. Allowing a servant to rest for 4 hours recovers 60.0 stamina.
4. Forcing a servant to work at 0.0 stamina triggers `isExhausted`, inflicts morale and affection drops, and yields exactly 0 gold/items.

---

## Phase 1B: Customizable Tile Upgrade Trees & Safe Tile Storage [COMPLETED - Suite 55]

### 1. Architectural Invariant: Safe Storage vs Storage Vault
* **Locked Decision**: **NO storage vault** and **NO wardrobe outfit presets**.
* **Storage Invariant**: Any dropped items on tiles marked `isStorageSafe = true` (or carrying residential/home room types) **never decay**. They remain on the ground indefinitely, functioning as organic, visible room storage.

### 2. Customizable Tile Foundations & Branching Upgrade Trees
Any tile containing the `"customizable"` tag starts as an empty foundation. Standing on the tile exposes build actions for any root room in `data/config/room_types.json`. Once built, the tile branches into tier upgrades or specialized stations based on `availableUpgrades`.

### 3. C++ Struct Updates (`src/map/tile.h`)

```cpp
struct TileCustomization
{
    bool isCustomizable = false;
    std::string currentRoomId = "";       // Empty string = unbuilt foundation
    int upgradeTier = 0;                  // 0 = unbuilt, 1 = Tier 1, 2 = Tier 2, etc.
    std::string stationType = "";         // e.g. "alchemy", "milking", "nursery", "service_room"
    std::vector<std::string> roomTags;
};

// Augmented TileRuntimeData in tile.h
struct TileRuntimeData
{
    // ... existing fields ...
    TileCustomization customization;

    [[nodiscard]] bool getIsEffectiveStorageSafe() const
    {
        if (isStorageSafe) return true;
        if (!customization.currentRoomId.empty()) return true; // Built rooms are always safe
        for (const auto& mod : safetyModifiers)
        {
            if (mod.dangerDelta < 0) return true;
        }
        return false;
    }
};
```

### 4. Branching Upgrade & Teardown Algorithms (`src/map/gameMap.cpp`)

```
Function: upgradeTileRoom(int x, int y, const std::string& targetRoomId, game* g)
    tileData = getRuntimeData(x, y)
    Assert tileData.customization.isCustomizable == true
    roomDef = getRoomDefinition(targetRoomId)

    Assert g->getPlayer()->getStat("currency") >= roomDef.costGold
    g->getPlayer()->modifyBaseStat("currency", -roomDef.costGold)

    tileData.customization.currentRoomId = targetRoomId
    tileData.customization.upgradeTier = roomDef.tier
    tileData.customization.stationType = roomDef.stationType
    tileData.customization.roomTags = roomDef.tags
    tileData.isStorageSafe = roomDef.isStorageSafe

Function: demolishTileRoom(int x, int y, game* g)
    tileData = getRuntimeData(x, y)
    Assert not tileData.customization.currentRoomId.empty()
    roomDef = getRoomDefinition(tileData.customization.currentRoomId)

    // Refund gold percentage
    refund = roomDef.costGold * 0.5f
    g->getPlayer()->modifyBaseStat("currency", refund)

    // Revert to pristine foundation (dropped items are preserved!)
    tileData.customization.currentRoomId = ""
    tileData.customization.upgradeTier = 0
    tileData.customization.stationType = ""
    tileData.customization.roomTags.clear()
```

### 5. JSON Schema (`data/config/room_types.json`)

```json
{
  "version": "1.0",
  "rooms": [
    {
      "id": "room_living_quarters_t1",
      "name": "Basic Living Quarters",
      "tier": 1,
      "category": "Residential",
      "description": "A clean room with basic bedding and safe floor space for personal belongings.",
      "costGold": 250.0,
      "isStorageSafe": true,
      "stationType": "",
      "tags": ["residential", "shelter"],
      "availableUpgrades": ["room_living_quarters_t2", "room_milking_parlor_t1", "room_nursery_t1"]
    },
    {
      "id": "room_living_quarters_t2",
      "name": "Luxurious Quarters",
      "tier": 2,
      "category": "Residential",
      "description": "Plush furnishings and velvet drapes. Enhances servant rest recovery by +5 stamina/hour.",
      "costGold": 600.0,
      "isStorageSafe": true,
      "stationType": "",
      "tags": ["residential", "luxury"],
      "availableUpgrades": ["room_living_quarters_t3"]
    },
    {
      "id": "room_milking_parlor_t1",
      "name": "Milking Parlor",
      "tier": 2,
      "category": "Facility",
      "description": "Equipped with mechanical suction apparatus and sterile storage containers.",
      "costGold": 500.0,
      "isStorageSafe": true,
      "stationType": "milking_parlor",
      "tags": ["facility", "milking"],
      "availableUpgrades": []
    },
    {
      "id": "room_nursery_t1",
      "name": "Offspring Nursery",
      "tier": 2,
      "category": "Facility",
      "description": "Cradles and play mats dedicated to sheltering developing offspring.",
      "costGold": 450.0,
      "isStorageSafe": true,
      "stationType": "nursery",
      "tags": ["facility", "nursery"],
      "availableUpgrades": []
    },
    {
      "id": "room_service_suite_t1",
      "name": "Private Service Parlor",
      "tier": 2,
      "category": "Commercial",
      "description": "A secluded client lounge outfitted for intimate commercial transactions.",
      "costGold": 400.0,
      "isStorageSafe": true,
      "stationType": "service_room",
      "tags": ["facility", "commercial", "service_room"],
      "availableUpgrades": []
    }
  ]
}
```

### 6. Action Grid Hooks
* In `explorationState`, standing on a tile with `customization.isCustomizable == true`:
  * If `currentRoomId.empty()`: Action button `"Build Room"` opens choice list of Tier 1 rooms.
  * If room built:
    * `"Upgrade Room"`: Lists all IDs from `availableUpgrades`.
    * `"Demolish Room"`: Prompts confirmation, refunds 50% gold, reverts tile while **leaving dropped item storage untouched**.

### 7. Automated Unit Test Criteria (Test Suite 55)
1. Items dropped on an empty customizable foundation decay normally unless marked safe.
2. Building `room_living_quarters_t1` sets `isStorageSafe = true`; dropped items never decay across 10,000 simulated game minutes.
3. Branching check: `room_living_quarters_t1` permits upgrading to `room_milking_parlor_t1` or `room_living_quarters_t2`, but rejects `room_living_quarters_t3` directly.
4. Demolishing a room refunds half cost, resets room ID to empty, and retains all ground-dropped items intact.

---

## Phase 2A: Binary Offspring & Nursery System

### 1. Architectural Invariant: Simplified Binary Aging
* **Locked Decision**: Exactly **two growth stages**:
  1. `Child` (resides inside Nursery station).
  2. `Adult NPC` (matures after a configured calendar threshold `adulthoodDays`, e.g., 30 days).
* **Zero intermediate stages**: No infant, toddler, or adolescent sub-states.

### 2. Gestation Interruption & Interactive Birth Trigger
When `processGestationMinutes` decrements `gestationDaysRemaining` to `0`:
1. The engine pauses background ticking.
2. Dispatches an interactive narrative birth scene (`scene_birth_delivery`) via `game::loadScene`.
3. Calls `gestationComponent::giveBirth()`.
4. Passes generated `OffspringInfo` records into `offspringManager::registerBirth()`.

### 3. C++ Struct & Class Definitions (`src/entities/offspringManager.h`)

```cpp
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "entities/gestationComponent.h"

enum class OffspringGrowthStage
{
    CHILD = 0,
    ADULT_NPC = 1
};

struct OffspringRecord
{
    std::string id;
    std::string name;
    std::string motherId;
    std::string fatherId;
    std::string fatherName;
    std::string motherRace;
    std::string fatherRace;
    std::string rolledRace;
    int birthDay = 1;
    int birthMonth = 1;
    int birthYear = 1;
    int daysAlive = 0;
    OffspringGrowthStage stage = OffspringGrowthStage::CHILD;

    bool isAdult() const { return stage == OffspringGrowthStage::ADULT_NPC; }
};

class offspringManager
{
public:
    static void registerBirth(const OffspringInfo& info, const std::string& customName, int day, int month, int year);
    static void advanceDayAll();
    static std::vector<OffspringRecord>& getAllOffspring();
    static OffspringRecord* getOffspring(const std::string& id);
    static std::shared_ptr<entity> promoteToAdultNPC(const std::string& id);

    static nlohmann::json toJson();
    static void fromJson(const nlohmann::json& j);
    static void clear();

    static constexpr int ADULTHOOD_DAYS_THRESHOLD = 30;

private:
    static std::vector<OffspringRecord> s_records;
};
```

### 4. Promotion to Active Companion / NPC
When an offspring record achieves `stage == OffspringGrowthStage::ADULT_NPC`, the player can interact with them at the Nursery:
* `"Recruit as Companion"`: Instantiates an `entity` with inherited genetics and calls `game::addCompanion()`.
* `"Assign to Servants"`: Binds the adult NPC to `servantManager` for mansion duties.

### 5. Automated Unit Test Criteria (Test Suite 56)
1. Reaching 0 gestation days fires `giveBirth()`, creating an `OffspringRecord` marked `stage = CHILD`.
2. Simulating 29 in-game days keeps stage at `CHILD`.
3. Simulating the 30th day triggers automatic transition to `ADULT_NPC`.
4. `promoteToAdultNPC` produces a fully valid `entity` with matching racial traits and stats capable of entering party combat.

---

## Phase 2B: Composable NPC Templates, Fluid Bottling & 1-Tile Client Rooms

### 1. Composable Procedural NPC Generation (3 Mix-and-Match Pools)
Procedural generation is refactored from rigid monolithic enemy JSONs into three independent, modular pools:

```
┌─────────────────────────┐    ┌─────────────────────────┐    ┌─────────────────────────┐
│   body_templates.json   │    │  outfit_templates.json  │    │    professions.json     │
├─────────────────────────┤    ├─────────────────────────┤    ├─────────────────────────┤
│ • Species & Morphs      │    │ • 35 Slot Gear Sets     │    │ • Title & Profession    │
│ • Genital Metrics       │ +  │ • Clothing Displacements│ +  │ • Gold Budget Range     │
│ • Height & Muscle       │    │ • Style Tags (Maid,     │    │ • Fetish Desire Matrix  │
│ • Fluid Capacities      │    │   Formal, Warrior)      │    │ • Combat Maneuvers      │
└─────────────────────────┘    └─────────────────────────┘    └─────────────────────────┘
                                           │
                                           ▼
                       ┌───────────────────────────────────────┐
                       │ npcGenerator::compose(body, outfit,   │
                       │                       profession)     │
                       └───────────────────────────────────────┘
```

#### JSON Schemas

**`data/catalogs/body_templates.json`**:
```json
{
  "bodyTemplates": [
    {
      "id": "body_canine_male_athletic",
      "species": "Dog-morph",
      "genderArchetype": "MALE",
      "heightCm": 182.0,
      "bodyBuild": "Toned",
      "breasts": { "cupSize": 0 },
      "groin": {
        "type": "Penis",
        "lengthCm": 18.0,
        "diameterCm": 4.2,
        "knot": true,
        "currentFluidMl": 15.0,
        "maxFluidMl": 30.0
      },
      "tags": ["canine", "fur", "tail", "athletic"]
    }
  ]
}
```

**`data/catalogs/outfit_templates.json`**:
```json
{
  "outfits": [
    {
      "id": "outfit_merchant_traveler",
      "name": "Merchant Traveling Attire",
      "tags": ["civilian", "merchant", "travel"],
      "items": [
        { "slot": "TORSO_OVER", "itemId": "item_linen_shirt" },
        { "slot": "LEGS_OUTER", "itemId": "item_leather_trousers" },
        { "slot": "FEET", "itemId": "item_travelers_boots" }
      ]
    }
  ]
}
```

**`data/catalogs/professions.json`**:
```json
{
  "professions": [
    {
      "id": "prof_wealthy_patron",
      "title": "Wealthy Patron",
      "goldBudgetMin": 80.0,
      "goldBudgetMax": 250.0,
      "allowedBodyTags": ["civilian", "human", "canine", "feline"],
      "allowedOutfitTags": ["formal", "merchant", "noble"],
      "fetishDesires": {
        "domination": 3,
        "lactation": 4,
        "oral": 3
      }
    }
  ]
}
```

### 2. Anatomy Fluid Harvesting & Bottling Engine
* Converts stored fluid from anatomy sockets (`anatomyComponent::drainFluid`) into concrete items.
* Conversion Matrix:
  * `BREASTS` Milk: Every `50.0 ml` -> 1x `item_milk_bottle` (Value: 15¤).
  * `GROIN` Cum: Every `15.0 ml` -> 1x `item_cum_flask` (Value: 25¤).
  * `GROIN` Girlcum: Every `15.0 ml` -> 1x `item_girlcum_vial` (Value: 20¤).
* Invoked via:
  1. Milking Parlor station action button (`"Extract Personal Fluids"`).
  2. Hourly servant milking duties.

### 3. 1-Tile Client Solicitation (Service Rooms)
* **Tile Tag**: `service_room`.
* **Action Grid Hook**: When player stands on a tile with `stationType == "service_room"`:
  * Button `"Get Client"` is active.
* **Execution Flow**:
  1. Engine samples `professions.json` for a commercial client.
  2. Queries compatible `body_templates.json` and `outfit_templates.json`.
  3. Composes client `entity` and names them via `names.json`.
  4. Sets `activeTargetNPC` to the generated client.
  5. Transitions directly into `sexState` with initial client stance and dominance score.
  6. On `END_SEX_SCENE`: Calculates satisfaction score based on matched fetishes; transfers gold budget from client into player wallet.

### 4. Automated Unit Test Criteria (Test Suite 57)
1. `npcGenerator::compose` correctly combines body, outfit, and profession tags into an entity with matching equipment and stats.
2. Draining 100ml milk from a character with full breasts yields exactly 2x `item_milk_bottle` and zeroes breast fluid volume.
3. Standing on a `service_room` tile produces the `"Get Client"` button; clicking it instantiates a client and transitions to `sexState`.
4. Concluding the client scene awards gold within `[goldBudgetMin, goldBudgetMax]`.

---

## Phase 3A: Universal Affection & Corruption Meters

### 1. Architectural Decisions
* **Universal Presence**: Every entity (Player, Companions, Named NPCs, Procedural Foes, Commercial Clients) tracks:
  * `affection` (`-100.0f` to `100.0f`): Initialized to `0.0f` (neutral).
  * `corruption` (`0.0f` to `100.0f`): Initialized to `0.0f` (pure).
* **NO `trust` stat**: Explicitly excluded to avoid stat bloat.

### 2. C++ Entity Class Augmentation (`src/entities/entity.h`)

```cpp
class entity
{
public:
    // ... existing fields ...
    float affection = 0.0f;
    float corruption = 0.0f;

    void modifyAffection(float delta)
    {
        affection = std::clamp(affection + delta, -100.0f, 100.0f);
    }

    void modifyCorruption(float delta)
    {
        corruption = std::clamp(corruption + delta, 0.0f, 100.0f);
    }
};
```

### 3. Serialization & UI Presentation
* Serialized inside `entity::toJson()` and deserialized in `entity::fromJson()`.
* **Anatomy & Character Card UI**:
  * Displays color-coded pills:
    * Affection: Cyan (`Affection: +45 [Friendly]`) or Crimson (`Affection: -60 [Hostile]`).
    * Corruption: Deep Purple (`Corruption: 80% [Depraved]`).
* **Condition Evaluation Engine (`game::checkSingleCondition`)**:
  * Added `MIN_AFFECTION`, `MAX_AFFECTION`, `MIN_CORRUPTION`, `MAX_CORRUPTION` condition tokens.

### 4. Automated Unit Test Criteria (Test Suite 58)
1. Affection clamps strictly between -100.0 and +100.0.
2. Corruption clamps strictly between 0.0 and 100.0.
3. Both stats persist accurately through JSON serialization and load cycles.
4. Condition evaluation unlocks dialogue branches when `MIN_AFFECTION` threshold is met.

---

## Phase 3B: Weather & Item Fluid Soiling

### 1. Architectural Invariant: Fluid Soiling vs Translucency
* **Locked Decision**: **NO fabric translucency**.
* **Soiling Engine**: Items explicitly track accumulated fluid stains and dirtiness. Dirty clothing inflicts charisma/charm penalties until cleansed at a water basin or bathhouse.

### 2. Item Soiling Struct Definition (`src/items/item.h`)

```cpp
struct ItemFluidSoiling
{
    bool isDirty = false;
    float cumStainMl = 0.0f;
    float milkStainMl = 0.0f;
    float girlcumStainMl = 0.0f;

    bool hasStains() const
    {
        return isDirty || (cumStainMl > 0.0f) || (milkStainMl > 0.0f) || (girlcumStainMl > 0.0f);
    }

    void clean()
    {
        isDirty = false;
        cumStainMl = 0.0f;
        milkStainMl = 0.0f;
        girlcumStainMl = 0.0f;
    }
};

// Added to struct item in item.h:
struct item
{
    // ... existing fields ...
    ItemFluidSoiling soiling;
};
```

### 3. Fluid Soiling Triggers in `sexState`
* Internal or external ejaculations/lactations check covered or adjacent equipped items on target slots.
* Adds fluid volume to `soiling.cumStainMl` or `soiling.milkStainMl`.
* Sets `soiling.isDirty = true`.
* When equipped, a dirty item applies a `-2.0 Charm` penalty per soiled piece.

### 4. Regional Weather Engine (`src/core/weatherManager.h`)
* Regional weather loaded from `data/config/weather.json`.
* Weather states: `CLEAR`, `CLOUDY`, `RAIN`, `STORM`, `FOG`, `SNOW`, `HEATWAVE`.
* Weather effects:
  * `RAIN` / `STORM`: Outdoor movement incurs +1 stamina drain per step.
  * `FOG`: Reduces radar minimap discovery radius from 3 tiles to 1 tile.

### 5. Automated Unit Test Criteria (Test Suite 59)
1. Ejaculating onto a clothed chest stains `CHEST_WEAR`, recording cum volume and marking `isDirty = true`.
2. Washing at a water tile cleanses all stains and resets `isDirty`.
3. Weather transitions update on map change based on region tables.
4. Radar discovery radius drops to 1 tile during `FOG`.

---

## Phase 3C: Overworld Spell Casting

### 1. Specification & Data Extension
Spells in `data/spells.json` declare whether they can be cast outside of combat:

```json
{
  "id": "cleanse",
  "name": "Purifying Cleanse",
  "school": "Restoration",
  "mpCost": 20,
  "staminaCost": 10.0,
  "heal": 40.0,
  "canCastInOverworld": true,
  "overworldEffect": {
    "type": "HEAL_AND_PURGE",
    "magnitude": 40.0
  }
}
```

### 2. Overworld Spell Dispatcher (`src/core/game.cpp`)
* In `explorationState`, Action Grid populates available overworld spells under a `"Spells"` category.
* Supported Overworld Effects:
  * `HEAL_AND_PURGE`: Restores player/party HP and removes negative debuffs.
  * `TELEPORT_WAYPOINT`: Warps player to active town hub.
  * `ILLUMINATE`: Dispels darkness penalty in cavern/dungeon maps for 60 minutes.

### 3. Automated Unit Test Criteria (Test Suite 60)
1. Spells with `"canCastInOverworld": false` are omitted from the exploration action grid.
2. Casting `cleanse` in the overworld deducts 20 MP and restores 40 HP without entering combat.
3. Insufficient MP disables overworld spell buttons with clear tooltips.

---

## Phase Sign-Off Matrix

```
┌────────────────────────────────────────────────────────────────────────┐
│                        PHASE SIGN-OFF CHECKLIST                        │
├─────────┬──────────────────────────────────────────┬───────────────────┤
│ PHASE   │ VERIFICATION TARGET                      │ STATUS            │
├─────────┼──────────────────────────────────────────┼───────────────────┤
│ 1A      │ 24h Servant Timetable & Stamina Loop     │ Ready for C++ Dev │
│ 1B      │ Customizable Tile Trees & Safe Ground    │ Ready for C++ Dev │
│ 2A      │ Binary Offspring & Nursery Engine        │ Ready for C++ Dev │
│ 2B      │ Composable NPCs, Milking & 1-Tile Rooms  │ Ready for C++ Dev │
│ 3A      │ Universal Affection & Corruption Meters  │ Ready for C++ Dev │
│ 3B      │ Weather & Item Fluid Soiling             │ Ready for C++ Dev │
│ 3C      │ Overworld Utility Spell Casting          │ Ready for C++ Dev │
└─────────┴──────────────────────────────────────────┴───────────────────┘
```

