# textRPG — Complete UI & JSON Development Specification

> **Version**: 1.0 (Modern C++26 Headless Engine)  
> **Purpose**: This standalone specification provides 100% of the architectural, data schema, enum, command, and snapshot getter definitions needed to build frontends (GUIs/clients) and content (quests, items, maps, NPCs, themes) without loading the C++ codebase into model contexts.

---

## 1. Architectural Model & Headless Decoupling

The engine follows a strict **Headless Simulation** pattern:
```
┌─────────────────────────────────────────────────────────────────────────┐
│                           HEADLESS ENGINE                               │
│  (game.h / game.cpp / activeGameState / timeManager / eventBus)        │
│   - Simulation state (Anatomy, Inventory, Stats, Map, Quests, Combat)   │
│   - Zero rendering code, zero window handles, zero coordinate logic     │
└───────────────────┬─────────────────────────────────▲───────────────────┘
                    │                                 │
         Read-Only Snapshot Getters              UICommand Dispatch
                    │                                 │
┌───────────────────▼─────────────────────────────────┴───────────────────┐
│                             UI CLIENT                                   │
│  (uiRenderer / GUI widgets / CLI test harness / External frontends)     │
│   - Reads state via snapshot getters to display panels                  │
│   - Dispatches user intent exclusively via game::handleCommand()        │
└─────────────────────────────────────────────────────────────────────────┘
```

### Core Execution Flow
1. **Per-Frame Simulation**: `game.update(deltaTime)` advances active game state.
2. **Time Passage**: `timeManager.advanceTime(minutes)` publishes `gameEvent::timeAdvanced`, triggering mutation growth, fluid regeneration, orifice stretch recovery, pregnancy incubation, and daily merchant restocks (06:00).
3. **Action Grid**: `ActionGridManager::refresh(gameContext)` populates `game.activeButtons` dynamically based on state.
4. **Command Dispatch**: `game.handleCommand(UICommand)` routes inputs to `activeGameState->handleCommand()`.

---

## 2. Enums & Canonical String Mappings

### 2.1. Body Sockets (`bodySlot`)
There are **24 anatomical sockets** (`BODY_SLOT_COUNT = 24`):
```cpp
enum class bodySlot {
    HAIR, HEAD, EYES, EARS, MOUTH, NECK,
    HORNS, ANTENNAE,
    TORSO, BREASTS, NIPPLES, STOMACH, BACK,
    ARMS, HANDS, FINGERS,
    HIPS, GROIN, ASS, TAIL,
    LEGS, FEET,
    WINGS, TENTACLES
};
```
* **String Values**: `"HAIR"`, `"HEAD"`, `"EYES"`, `"EARS"`, `"MOUTH"`, `"NECK"`, `"HORNS"`, `"ANTENNAE"`, `"TORSO"`, `"BREASTS"`, `"NIPPLES"`, `"STOMACH"`, `"BACK"`, `"ARMS"`, `"HANDS"`, `"FINGERS"`, `"HIPS"`, `"GROIN"`, `"ASS"`, `"TAIL"`, `"LEGS"`, `"FEET"`, `"WINGS"`, `"TENTACLES"`.

### 2.2. Equipment Slots (`equipSlot`)
Organised in a **6×6 grid** (`EQUIP_SLOT_COUNT = 35`):
* **Row 1**: `EYEWEAR`, `HEADWEAR`, `HAIR_WEAR`, `HORNS_SLOT`, `WEAPON_MAIN`, `WEAPON_OFF`
* **Row 2**: `MOUTHWEAR`, `TORSO_OVER`, `NECKWEAR`, `WINGS_SLOT`, `PIERCING_EAR`, `PIERCING_NOSE`
* **Row 3**: `WRISTS`, `TORSO_UNDER`, `CHEST_WEAR`, `NIPPLES_WEAR`, `PIERCING_LIP`, `PIERCING_TONGUE`
* **Row 4**: `HANDS`, `HIPS_WEAR`, `STOMACH_WEAR`, `FINGER_PRIMARY`, `PIERCING_NIPPLE`, `PIERCING_NAVEL`
* **Row 5**: `ANKLES`, `LEGS_OUTER`, `GROIN_OVER`, `TAIL_SLOT`, `PIERCING_COCK`, `PIERCING_VAGINA`
* **Row 6**: `CALVES`, `FEET`, `ASS_WEAR`, `PENIS_WEAR`, `VAGINA_WEAR`
* **Sentinel**: `NONE`

### 2.3. Gender Archetypes (`GenderArchetype`)
Determined dynamically from organ presence (`hasPenis()`, `hasVagina()`, `hasBreasts()`) and presentation:
* `MALE`: Penis present, no vagina, flat breasts.
* `FEMALE`: Vagina present, no penis, breasts present (A-cup+).
* `HERMAPHRODITE`: Both penis and vagina present.
* `GYNOMORPH`: Penis present, no vagina, breasts present & feminine presentation.
* `ANDROMORPH`: Vagina present, no penis, masculine presentation & flat chest.
* `ASEXUAL_NULL`: Neither penis nor vagina present.

### 2.4. Sexual Orientations (`SexualOrientation`)
* `HETEROSEXUAL`, `BISEXUAL`, `HOMOSEXUAL`, `ASEXUAL`.

### 2.5. Clothing Displacement Modes (`DisplacementMode`)
* `NONE`: Normal worn coverage.
* `UNBUTTON`: Unfastened (e.g. shirt unbuttoned exposing torso/breasts).
* `PULL_ASIDE`: Shifted sideways (e.g. panties pulled aside exposing groin/ass).
* `LIFT_UP`: Lifted (e.g. bra lifted exposing breasts/nipples).
* `PULL_DOWN`: Pulled down (e.g. pants pulled down exposing legs/groin/ass).
* `OPEN`: Flapped open.

### 2.6. Interactive Sex Stances (`SexStance`)
* `MISSIONARY`: Face-to-face intimate stance.
* `FROM_BEHIND`: Rear entry / doggystyle.
* `KNEELING`: Oral / submissive stance.
* `STANDING`: Wall-press / standing encounter.
* `LAP_SITTING`: Straddling / seated stance.

### 2.7. Time Phase (`TimePhase`)
* `DAWN` (05:00–07:59), `DAY` (08:00–17:59), `DUSK` (18:00–20:59), `NIGHT` (21:00–04:59).

---

## 3. UI Command Dispatch (`UICommand`)

All UI inputs are packaged into a `UICommand` struct and dispatched via `game->handleCommand(cmd)`:

```cpp
struct UICommand {
    CommandType type;
    int intPayload1 = 0;
    int intPayload2 = 0;
    std::string stringPayload = "";
};
```

### Command Types Reference

| `CommandType` | `intPayload1` | `intPayload2` | `stringPayload` | Purpose |
|---|---|---|---|---|
| `MOVE_PLAYER` | Target X | Target Y | — | Move player on the world map grid |
| `SELECT_DIALOGUE_CHOICE` | Choice Index | — | — | Select a choice in `eventState` |
| `SELECT_INVENTORY_SLOT` | Slot Index | Side (0=Player, 1=Tile) | — | Highlight item in `inventoryState` |
| `EQUIP_ITEM` | Backpack Index | — | — | Equip item from backpack |
| `UNEQUIP_SLOT` | `equipSlot` (int) | — | — | Unequip equipped item |
| `DROP_ITEM` | Stacked Index | Quantity | — | Drop item to ground |
| `PICKUP_ITEM` | Ground Index | Quantity | — | Pickup item from ground |
| `EXECUTE_COMBAT_ACTION` | Slot Index | — | Action ID / `"WIN"` / `"DEFEAT"` | Execute or queue combat move |
| `SELECT_RESOLUTION_TARGET`| Enemy Index | — | — | Target enemy in resolution hub |
| `LOOT_ENEMY` | — | — | — | Loot gold/items from target enemy |
| `STRIP_ENEMY` | — | — | — | Strip equipped items from enemy |
| `INTERACTIVE_SEX` | — | — | — | Launch interactive sex with enemy |
| `SUBJUGATE_ENEMY` | — | — | — | Enslave/subjugate enemy |
| `RELEASE_ENEMY` | — | — | — | Spare and release enemy |
| `EXECUTE_SEX_ACTION` | Action Index | — | — | Execute sex move in `sexState` |
| `CHANGE_SEX_STANCE` | `SexStance` (int)| — | — | Change physical positioning |
| `END_SEX_SCENE` | — | — | — | Conclude sex scene & restore clothes |
| `CLOSE_MENU` | — | — | — | Return to `explorationState` |

---

## 4. Snapshot Getters API (Read-Only UI Access)

Frontends retrieve state directly from `game` without modifying logic:

```cpp
// General Engine State
const questScene& game::getCurrentScene() const;
entity* game::getPlayer() const;
const gameMap* game::getActiveMap() const;
const std::vector<actionButton>& game::getActiveActionButtons() const;
const timeManager& game::getTime() const;

// Inventory Stacked Views
std::vector<InventorySlot> game::getPlayerInventoryStacked() const;
std::vector<InventorySlot> game::getTileInventoryStacked() const;

// Character Status & Attributes
float entity::getStat(const std::string& statName) const; // Returns effective stat factoring buffs
GenderArchetype anatomyComponent::getGenderArchetype() const;
std::string anatomyComponent::getRacialTitle() const;
bool anatomyComponent::isDualHybrid() const;
bool anatomyComponent::isChaoticChimera() const;
std::unordered_map<std::string, float> anatomyComponent::calculateRacePercentages() const;

// Clothing Exposure
bool inventoryComponent::isSlotExposed(bodySlot slot) const;
DisplacementMode inventoryComponent::getDisplacement(equipSlot slot) const;

// Character Prose Description
std::string characterDescription::generateFullDescription(const entity* ent, const entity* viewer = nullptr);

// Combat Snapshot (when dynamic_cast<CombatState*>(game.getActiveState()))
const std::vector<CombatParticipant>& combatEngine::getPlayerParty() const;
const std::vector<CombatParticipant>& combatEngine::getEnemyParty() const;
const std::vector<std::string>& combatEngine::getCombatLog() const;

// Interactive Sex Snapshot (when dynamic_cast<sexState*>(game.getActiveState()))
SexStance sexState::getStance() const;
float sexState::getPlayerDominance() const; // -100 to +100
bool sexState::isPlayerDominant() const;
float sexState::getPlayerArousal() const;   // 0 to 100
float sexState::getPartnerArousal() const;  // 0 to 100
entity* sexState::getPartner() const;
const std::string& sexState::getNarrativeLog() const;
std::vector<SexAction> sexState::getAvailableActions() const;

// Encounter Resolution Snapshot (when dynamic_cast<encounterResolutionState*>(game.getActiveState()))
const std::vector<DefeatedEnemyRecord>& encounterResolutionState::getDefeatedRecords() const;
size_t encounterResolutionState::getSelectedIndex() const;
const std::string& encounterResolutionState::getResolutionLog() const;
```

---

## 5. JSON File Specifications

### 5.1. Items (`data/items.json`)
```json
{
  "items": [
    {
      "id": "item_linen_shirt",
      "name": "Linen Shirt",
      "description": "A light, breathable woven linen shirt.",
      "baseValue": 15,
      "isConsumable": false,
      "isEquippable": true,
      "isStackable": false,
      "count": 1,
      "targetSlot": "TORSO_UNDER",
      "requiredTags": [],
      "forbiddenTags": ["wings"],
      "statModifiers": [
        { "statName": "physique", "flatValue": 1.0, "percentValue": 0.0 }
      ],
      "supportedDisplacements": {
        "UNBUTTON": ["TORSO", "BREASTS"],
        "LIFT_UP": ["TORSO", "BREASTS", "STOMACH"],
        "OPEN": ["TORSO", "BREASTS"]
      }
    }
  ]
}
```

### 5.2. Quests & Dialogue Scenes (`data/quests/*.json`)
```json
{
  "id": "root_delivery",
  "name": "Canis Root Delivery",
  "description": "Deliver the Canis Root to the stranger in the town district.",
  "triggers": [
    {
      "id": "trig_stranger_start",
      "mapId": "overworld",
      "x": 1,
      "y": 1,
      "label": "Talk to Stranger",
      "sceneId": "quest_intro_01",
      "conditions": [
        { "type": "QUEST_STAGE", "target": "root_delivery", "requiredValue": 0 }
      ]
    }
  ],
  "stages": {
    "0": "Find the stranger in the town district.",
    "1": "Hand over the Canis Root.",
    "2": "Quest completed!"
  },
  "scenes": [
    {
      "id": "quest_intro_01",
      "speakerName": "Hooded Stranger",
      "bodyText": "Did you bring the Canis Root I requested?",
      "choices": [
        {
          "label": "Hand over Canis Root",
          "requirements": [
            { "type": "HAS_ITEM", "target": "item_canis_root", "requiredValue": 1 }
          ],
          "results": [
            { "action": "REMOVE_ITEM", "target": "item_canis_root", "amount": 1 },
            { "action": "SET_QUEST", "target": "root_delivery", "amount": 1 },
            { "action": "ADD_STAT", "target": "currency", "amount": 50 }
          ],
          "nextSceneId": "quest_intro_02"
        },
        {
          "label": "Not yet.",
          "requirements": [],
          "results": [],
          "nextSceneId": "EXIT"
        }
      ]
    }
  ]
}
```

#### Supported Condition Types
* `HAS_ITEM`: Checks backpack item count (`target` = itemId, `requiredValue` = count).
* `QUEST_STAGE` / `QUEST_FLAG`: Checks player quest integer (`target` = questId, `requiredValue` = stage).
* `TIME_PHASE`: Checks time (`target` = `"DAY"` / `"NIGHT"` / `"DAWN"` / `"DUSK"`).
* `TIME_HOUR_BETWEEN`: Checks hour (`minValue` = startHour, `maxValue` = endHour).
* `STAT_MIN` / `STAT_MAX` / `STAT_CHECK`: Checks effective stats.
* `EQUIPPED_SLOT`: Checks equipped slot (`target` = equipSlot, optional `stringValue` = itemId).
* `HAS_TAG`: Checks body tags (`target` = tag).
* `HAS_MUTATION`: Checks active mutations (`target` = mutationId).
* `IS_PREGNANT`: Boolean check on `gestation.isPregnant`.
* `GENDER_IS`: Checks gender archetype (`target` = `"Male"`, `"Female"`, `"Hermaphrodite"`, etc.).
* `RACE_IS` / `DOMINANT_RACE`: Checks dominant race (`target` = raceName).
* `DOMINANCE_BETWEEN`: Checks dominance score (`minValue` to `maxValue`).

#### Boolean Condition Trees
Conditions support nested logic via `"op"`: `"AND"`, `"OR"`, `"NOT"`, `"LEAF"`:
```json
{
  "op": "AND",
  "children": [
    { "type": "STAT_MIN", "target": "physique", "requiredValue": 20 },
    {
      "op": "OR",
      "children": [
        { "type": "HAS_TAG", "target": "feminine" },
        { "type": "IS_PREGNANT" }
      ]
    }
  ]
}
```

#### Supported Action Effects (`gameEffect`)
* `TELEPORT`: Warps player (`target` = mapId, `x`, `y`).
* `GIVE_ITEM`: Adds item (`target` = itemId, `amount` = count).
* `REMOVE_ITEM`: Removes item (`target` = itemId, `amount` = count).
* `TRANSFER_CURRENCY`: Moves gold between player and target (`amount` or `floatAmount`).
* `MODIFY_STAT` / `ADD_STAT`: Adjusts base stat (`target` = statName, `amount` = delta).
* `TRANSFORM_PART`: Alters body part (`target` = bodySlot, `stringVal` = race, `amount` = sizeDelta).
* `FILL_FLUID`: Adds fluid (`target` = fluidType e.g. `"milk"`/`"cum"`, `secondaryTarget` = bodySlot, `amount` = ml).
* `DRAIN_FLUID`: Drains fluid (`secondaryTarget` = bodySlot, `amount` = ml).
* `STRETCH_ORIFICE`: Permanently/temporarily dilates orifice (`target` = bodySlot, `amount` = diameterCm).
* `IMPREGNATE`: Initiates pregnancy (`target` = `"player"`/`"npc"`, `secondaryTarget` = fatherId, `stringVal` = fatherRace, `amount` = litterSize).
* `INDUCE_BIRTH`: Triggers birth immediately (`target` = `"player"`/`"npc"`).
* `DISPLACE_CLOTHING`: Sets displacement (`target` = equipSlot, `stringVal` = DisplacementMode).
* `RESTORE_CLOTHING`: Resets all active clothing displacements.
* `SET_FLAG` / `SET_QUEST`: Updates quest stage flag (`target` = questId, `amount` = stage).
* `CALL_SUB_SCENE`: Pushes subroutine scene onto stack (`target` = sceneId).
* `RANDOM_BRANCH`: Evaluates weighted array of branches (`branches`: `["scene_a", "scene_b"]`, `weights`: `[70, 30]`).
* `SPAWN_NPC`: Spawns persistent NPC on map (`target` = templateId, `x`, `y`).
* `DESPAWN_NPC`: Removes persistent NPC from current tile (`target` = npcId).

### 5.3. NPC Templates (`data/npc_templates.json`)
```json
{
  "templates": [
    {
      "id": "tpl_alley_bandit",
      "name": "Alleyway Bandit",
      "levelMin": 1,
      "levelMax": 3,
      "baseStats": {
        "health": 50.0,
        "mana": 30.0,
        "lust": 100.0,
        "physique": 14.0,
        "arcane": 8.0,
        "corruption": 10.0
      },
      "tags": ["masculine", "hostile"],
      "possibleRaces": ["Human", "Orc"],
      "guaranteedItems": ["item_linen_shirt", "item_leather_trousers", "item_leather_boots"],
      "randomItems": ["item_canis_root"]
    }
  ]
}
```

### 5.4. World Maps (`data/maps/*.json`)
```json
{
  "id": "overworld",
  "name": "Town District",
  "width": 5,
  "height": 5,
  "tiles": [
    [2, 2, 2, 2, 2],
    [2, 1, 1, 1, 2],
    [2, 1, 3, 1, 2],
    [2, 1, 1, 1, 2],
    [2, 2, 2, 2, 2]
  ],
  "dangerLevels": [
    [0, 0, 0, 0, 0],
    [0, 0, 1, 0, 0],
    [0, 1, 2, 1, 0],
    [0, 0, 1, 0, 0],
    [0, 0, 0, 0, 0]
  ],
  "warps": [
    { "x": 2, "y": 2, "targetMap": "house_01", "targetX": 1, "targetY": 1 }
  ],
  "triggers": []
}
```
* **Tile Types**: `0` = Void, `1` = Floor, `2` = Wall, `3` = Door.

### 5.5. Game Settings (`data/settings.json`)
```json
{
  "demographics": {
    "percentHetero": 40.0,
    "percentBi": 30.0,
    "percentHomo": 20.0,
    "percentAsexual": 10.0,
    "percentMale": 30.0,
    "percentFemale": 40.0,
    "percentHermaphrodite": 15.0,
    "percentGynomorph": 7.0,
    "percentAndromorph": 5.0,
    "percentNull": 3.0
  },
  "content": {
    "pregnancyEnabled": true,
    "lactationEnabled": true,
    "fluidMultiplier": 1.0,
    "transformationSpeedMultiplier": 1.0
  },
  "gameplay": {
    "difficultyMultiplier": 1.0,
    "currencyLossOnDefeatPercent": 0.15,
    "autoSaveOnMapChange": true,
    "autoSaveOnSceneExit": true,
    "maxAutoSaves": 3
  },
  "display": {
    "descriptionVerbosity": 0,
    "activeTheme": "default"
  }
}
```

### 5.6. UI Theme (`data/themes/theme.json`)
```json
{
  "themeName": "Dark Velvet",
  "colors": {
    "bgDark": [30, 30, 30, 255],
    "bgPanel": [30, 28, 35, 255],
    "bgHeader": [45, 45, 52, 255],
    "bgSlot": [40, 38, 48, 255],
    "bgSlotOccupied": [50, 55, 75, 255],
    "bgSlotSelected": [70, 60, 95, 255],
    "bgButton": [70, 100, 140, 255],
    "bgButtonDisabled": [45, 45, 52, 255],
    "borderNormal": [60, 55, 65, 255],
    "borderSelected": [255, 215, 0, 255],
    "borderButton": [100, 140, 190, 255],
    "borderButtonDisabled": [65, 65, 75, 255],
    "textPrimary": [255, 255, 255, 255],
    "textSecondary": [220, 225, 240, 255],
    "textMuted": [130, 130, 145, 255],
    "textGold": [255, 215, 0, 255],
    "textAccent": [180, 150, 220, 255],
    "health": [255, 60, 90, 255],
    "mana": [220, 130, 255, 255],
    "lust": [230, 50, 150, 255],
    "physique": [255, 50, 120, 255],
    "arcane": [180, 110, 255, 255],
    "corruption": [100, 200, 255, 255],
    "currency": [255, 215, 0, 255],
    "gems": [255, 100, 220, 255],
    "enemy": [255, 120, 170, 255],
    "friendly": [100, 210, 255, 255],
    "companion": [120, 240, 150, 255]
  }
}
```

---

## 6. Anatomy, Fluids & Transformation Mechanics

### 6.1. 3-Tier Racial Classification
Body sockets are weighted when calculating racial composition:
* **Weight 3.0×**: `HEAD`, `TORSO`, `GROIN`
* **Weight 2.0×**: `TAIL`, `EARS`, `WINGS`, `HORNS`
* **Weight 1.0×**: All remaining sockets

**Tiers**:
1. `DOMINANT_MORPH` (Top race $\ge 50.0\%$): Display title is `"<Race>-Morph"` or `"Pure <Race>"` (at 100%).
2. `DUAL_HYBRID` (Top two races both $\ge 40.0\%$): Display title is `"<Race1>-<Race2> Hybrid"`.
3. `CHAOTIC_CHIMERA` ($< 40.0\%$ across 3+ species): Display title is `"Chaotic Chimera"`.

### 6.2. Fluids & Orifices
* `OrificeData` exists on `MOUTH` (throat), `BREASTS` (nipples/lactation), `GROIN` (vagina), `ASS` (anus).
* Tracks `elasticity` (0–100), `currentStretch` (0–100), `maxCapacityMl`, `depthCm`, `wetnessLevel` (0–5), `storedFluids` (`"cum"`, `"milk"`, `"girlcum"`).
* Penetration stretches orifices when `penisDiameter > currentStretch`.
* Time passage gradually recovers elasticity and restores orifice tightness.
* Lactation and semen refill hourly based on `fluidRegenPerHour` up to `maxFluidMl`.

### 6.3. Pregnancy & Gestation Pipeline
* Conception triggers on ejaculation into `GROIN` orifice if `hasVagina()` and `pregnancyEnabled`.
* Incubation counts down daily (`totalGestationDays = 30`).
* Litter size: 70% chance of 1, 20% chance of 2, 10% chance of 3.
* Offspring race inheritance: 50% father race, 45% mother race, 5% hybrid.
* At day 0, `giveBirth()` returns `std::vector<OffspringInfo>` and clears pregnancy state.

---

## 7. Interactive CYOA Sex State Machine

When `activeGameState` is `sexState`:
1. **Dominance Continuum**: Player dominance ranges from `-100` (Max Submissive) to `+100` (Max Dominant).
2. **Dominant Turn**: Player chooses active offensive/erotic actions (`Kiss`, `Caress Breasts`, `Give Oral`, `Receive Oral`, `Vaginal Penetration`, `Anal Penetration`, `Mammary Sex`, `Spank Buttocks`).
3. **Submissive Turn**: When `playerDominance < 0`, action menu presents submissive reactions:
   - `[Plead / Suggest Action]`: Suggest gentle treatment.
   - `[Endure Passively]`: Surrender control, increasing arousal.
   - `[Beg for Climax]`: Request partner to allow orgasm.
   - `[Struggle for Control]`: Contest physical strength (`physique` check) to reverse dominance.
4. **Partner AI**: When player is submissive or takes passive turns, NPC partner executes proactive dominant actions.
5. **Proximity & Stance Rules**: Actions enforce physical adjacency (e.g. vaginal penetration requires `MISSIONARY`, `FROM_BEHIND`, or `STANDING`).
6. **Automatic Clothing Management**: Triggering erotic actions automatically applies partial displacements (`LEGS_OUTER` $\rightarrow$ `PULL_DOWN`, `GROIN_OVER` $\rightarrow$ `PULL_ASIDE`, `CHEST_WEAR` $\rightarrow$ `PULL_ASIDE`). All clothing is 100% restored upon exiting the scene.

---

## 8. UI Layout Architecture (5-Pane System)

The standard graphical frontend is structured into 5 decoupled regions:

```
┌─────────────────────────────────────────────────────────────────────────┐
│ [TOP BAR] Time, Date, Phase | Location Map Name | Player Gold           │
├──────────────┬───────────────────────────────────────────┬──────────────┤
│ [LEFT PANE]  │ [CENTER PANE]                             │ [RIGHT PANE] │
│ Character    │ Dynamic Contextual View:                  │ World Info / │
│ Status:      │  - Exploration (Grid, Surrounding Tiles)  │ Target NPC   │
│  - HP Bar    │  - Narrative Scene (Speaker, Body, Choices│ Status       │
│  - Mana Bar  │  - Interactive Sex (Log, Stance, Arousal) │  - Name/Race │
│  - Lust Bar  │  - Turn-Based Combat (Combat Log, Parties)│  - Bars/Stats│
│  - Stats     │  - Encounter Resolution Hub (Loot/Strip)  │  - Equipped  │
│  - Anatomy   │  - Inventory / Equipment (Backpack/Ground)│    Items     │
├──────────────┴───────────────────────────────────────────┴──────────────┤
│ [BOTTOM ACTION GRID]                                                    │
│ 10 Action Buttons per page (Paged 0..N) + Navigation Controls           │
│ Driven entirely by game->getActiveActionButtons()                       │
└─────────────────────────────────────────────────────────────────────────┘
```
