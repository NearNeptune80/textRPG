# MASTER_ROADMAP.md - textRPG Engine Architecture & Implementation Plan
> **Standard**: C++26 | **Architecture**: 100% Decoupled Headless Simulation | **Inspiration**: Lilith's Throne (LT)

---

## Architecture & Design Rules (Zero Deviation)
1. **Model-View Separation**: The simulation layer (`src/core/`, `src/state/`, `src/entities/`, `src/combat/`, `src/map/`, `src/items/`, `src/quest/`, `src/save/`, `src/sex/`, `src/settings/`) MUST NEVER include SDL headers, window contexts, renderer pointers, or pixel calculations.
2. **Trivial UI Integration**: Any UI view (SDL3, CLI, ImGui) queries pure read-only getters on `game` and dispatches `UICommand` events. UI layouts can be replaced without modifying core simulation logic.
3. **Data-Driven Logic**: 100% of quests, scenes, items, races, body parts, mutations, and NPC templates are stored in `data/*.json`.
4. **Memory Management**: RAII everywhere. Zero raw `new`/`delete`. Use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` for shared registry items.
5. **Naming & Style**: `camelCase` for methods/variables/classes, `UPPER_CASE` for enums. British English standard in comments and descriptions (`colour`, `initialise`, `serialise`, `behaviour`).

---

## Milestone 1: Architectural Decoupling, Core Cleanup & Headless CLI Harness
**Goal**: Complete headless decoupling of the core engine, eliminate raw pointer leaks, and create a dedicated CLI test harness.

### Micro-Tasks:
- [ ] **Task 1.1: Core Engine Memory Clean-up & RAII**
  - **Target Files**: `src/core/game.h`, `src/core/game.cpp`
  - **Details**:
    - Replace raw `entity* Player` with `std::unique_ptr<entity> playerEntity`.
    - Replace raw `gameMap* map` with `gameMap* activeMap` pointing to entries in `mapCache` (`std::unordered_map<std::string, gameMap>`).
    - Remove raw `new`/`delete` calls in `game::init()` and `game::~game()`.
    - Ensure all state transitions via `changeState(std::unique_ptr<iGameState>)` cleanly trigger `onExit()` and `onEnter()`.
  - **Verification**: Engine initializes and destructs cleanly without memory leaks or dangling pointers.

- [ ] **Task 1.2: Decoupled State & UICommand Routing**
  - **Target Files**: `src/state/iGameState.h`, `src/state/explorationState.cpp`, `src/state/inventoryState.cpp`, `src/state/eventState.cpp`, `src/state/combatState.cpp`
  - **Details**:
    - Ensure `iGameState::handleCommand(game* g, const UICommand& cmd)` is the primary driver of state logic.
    - Remove any SDL layout/coordinate calculations from states.
  - **Verification**: States respond predictably to `UICommand` packets.

- [ ] **Task 1.3: Universal State Snapshot / Getter API**
  - **Target Files**: `src/core/game.h`, `src/core/game.cpp`
  - **Details**:
    - Expose lightweight, read-only data accessors for any UI:
      - `const questScene& getCurrentScene() const;`
      - `const entity* getPlayer() const;`
      - `std::vector<InventorySlot> getPlayerInventoryStacked() const;`
      - `std::vector<InventorySlot> getTileInventoryStacked() const;`
      - `const gameMap* getActiveMap() const;`
      - `const std::vector<actionButton>& getActiveActionButtons() const;`
      - `const timeManager& getTime() const;`
  - **Verification**: UI renderer can fetch complete game state without touching internal mutable fields.

- [ ] **Task 1.4: Headless CLI Test Runner Target**
  - **Target Files**: `src/main_cli.cpp`, `CMakeLists.txt`
  - **Details**:
    - Add `textRPG_cli` executable target in `CMakeLists.txt` linking only core simulation files (no SDL3).
    - Implement interactive text-based test harness in `src/main_cli.cpp` to inspect player stats, trigger scenes, move on maps, and advance time from terminal stdin.
  - **Verification**: `textRPG_cli` builds and runs in a pure terminal environment without SDL.

---

## Milestone 2: Settings Engine & Demographic Configuration
**Goal**: Implement comprehensive game settings and customizable demographic generation distributions.

### Micro-Tasks:
- [ ] **Task 2.1: Game Settings Data Structure**
  - **Target Files**: `src/settings/gameSettings.h`
  - **Details**:
    - Define `struct GameSettings`:
      - **Sexuality Distribution (%)**: `percentHetero` (40), `percentBi` (30), `percentHomo` (20), `percentAsexual` (10).
      - **Gender Archetype Distribution (%)**: `percentMale` (30), `percentFemale` (40), `percentHermaphrodite` (15), `percentGynomorph` (7), `percentAndromorph` (5), `percentNull` (3).
      - **Content Toggles**: `pregnancyEnabled` (true), `lactationEnabled` (true), `fluidMultiplier` (1.0f), `transformationSpeed` (1.0f).
      - **Gameplay & Difficulty**: `difficultyMultiplier` (1.0f), `currencyLossOnDefeatPercent` (0.15f), `autoSaveMode` (1).
      - **Display**: `descriptionVerbosity` (0 = Full, 1 = Condensed), `activeTheme` ("default").
  - **Verification**: Struct defaults compile with valid 100% distribution baselines.

- [ ] **Task 2.2: Settings Manager & JSON Persistence**
  - **Target Files**: `src/settings/settingsManager.h`, `src/settings/settingsManager.cpp`, `data/settings.json`
  - **Details**:
    - Implement `settingsManager::load(const std::string& path)` and `settingsManager::save(const std::string& path)`.
    - Validate that percentage distributions normalize to 100.0f.
  - **Verification**: Modifying settings in memory serializes to `data/settings.json` and reloads accurately.

- [ ] **Task 2.3: Integrate Settings with Engine & Generation**
  - **Target Files**: `src/core/game.h`, `src/core/game.cpp`
  - **Details**:
    - Embed `GameSettings settings` into `game`.
    - Load `data/settings.json` during `game::init()`.
  - **Verification**: Spawning functions access global settings for rolls.

---

## Milestone 3: Comprehensive Anatomy, Fluids, Gestation & Transformations
**Goal**: Achieve Lilith's Throne-grade anatomical depth, bodily fluid mechanics, gestation/lineage, 3-tier transformations, and procedural character descriptions.

### Micro-Tasks:
- [ ] **Task 3.1: Complete Sockets & Gender Archetype Engine**
  - **Target Files**: `src/common/enums.h`, `src/entities/bodyPart.h`, `src/entities/anatomyComponent.h`, `src/entities/anatomyComponent.cpp`
  - **Details**:
    - Ensure all 24 sockets (`HAIR`, `HEAD`, `EYES`, `EARS`, `MOUTH`, `NECK`, `HORNS`, `ANTENNAE`, `TORSO`, `BREASTS`, `NIPPLES`, `STOMACH`, `BACK`, `ARMS`, `HANDS`, `FINGERS`, `HIPS`, `GROIN`, `ASS`, `TAIL`, `LEGS`, `FEET`, `WINGS`, `TENTACLES`) are fully queryable.
    - Implement `GenderArchetype getGenderArchetype() const` returning `MALE`, `FEMALE`, `HERMAPHRODITE`, `GYNOMORPH`, `ANDROMORPH`, or `ASEXUAL_NULL` based on breast presence and genital configuration.
  - **Verification**: Unit tests verify archetype classification for all combinations of genitals/breasts.

- [ ] **Task 3.2: Bodily Fluids & Orifice Mechanics**
  - **Target Files**: `src/entities/bodyPart.h`, `src/entities/anatomyComponent.h`, `src/entities/anatomyComponent.cpp`
  - **Details**:
    - Add fluid stores to relevant parts:
      - `BREASTS`: `currentMilk` (ml), `maxMilk` (ml), `lactationRatePerHour` (ml).
      - `GROIN` (Penis): `currentCum` (ml), `maxCum` (ml), `cumRegenPerHour` (ml).
      - `GROIN` (Vagina): `currentGirlcum` (ml), `wetnessLevel` (0-5).
    - Add orifice attributes:
      - `Orifice`: `elasticity` (0-100), `currentStretch` (0-100), `capacity` (ml), `storedFluids` (`std::unordered_map<std::string, float>`).
      - Sockets with Orifices: `MOUTH` (Throat), `BREASTS` (Nipples), `GROIN` (Vagina), `ASS` (Anus).
    - Methods: `transferFluid()`, `stretchOrifice()`, `recoverStretch(int minutesPassed)`.
  - **Verification**: Fluid drainage, storage inside partner orifices, and stretch decay over time function accurately.

- [ ] **Task 3.3: Pregnancy & Gestation Component**
  - **Target Files**: `src/entities/gestationComponent.h`, `src/entities/gestationComponent.cpp`, `src/entities/entity.h`
  - **Details**:
    - Create `gestationComponent` tracking:
      - `isPregnant` (bool), `conceptionDay` (int), `fatherId` (string), `fatherRace` (string), `motherRace` (string).
      - `gestationDaysRemaining` (int), `litterSize` (int), `incubatedOffspringRaces` (`std::vector<std::string>`).
    - Implement `impregnate(const entity* mother, const entity* father, int customLitterSize)`.
    - Implement `processGestation(int daysPassed)` and `giveBirth(entity* mother)`.
  - **Verification**: Impregnation rolls litter size, advances daily, and produces offspring upon gestation completion.

- [ ] **Task 3.4: 3-Tier Racial Transformation & Hybrid Model**
  - **Target Files**: `src/entities/anatomyComponent.cpp`
  - **Details**:
    - Weighted socket calculation: Head (weight 3), Torso (weight 3), Groin (weight 3), Tail (2), Ears (2), Wings (2), others (1).
    - Classify into:
      - **Dominant Morph**: Single race $\ge 50\%$ total points (e.g. "Cat-Morph", "Pure Feline").
      - **Dual-Hybrid**: Top 2 races both $\ge 40\%$ (e.g. "Cat-Fox Hybrid").
      - **Chaotic Chimera**: No race reaches $40\%$, traits spread across $\ge 3$ species.
  - **Verification**: Mutating parts shifts race classification accurately across all 3 tiers.

- [ ] **Task 3.5: Modular Procedural Character Description Engine**
  - **Target Files**: `src/core/characterDescription.h`, `src/core/characterDescription.cpp`
  - **Details**:
    - Assemble head-to-toe narrative description:
      1. General: Height, body build, gender presentation, dominant race tier.
      2. Head & Face: Hair, horns, antennae, ears, eyes.
      3. Upper Body: Torso, skin/fur/scales covering, breasts (cup size, lactation), nipples.
      4. Lower Body: Genitals (length, girth, wetness, fluid capacity), rear (ass size).
      5. Extremities: Limbs, tail, wings, tentacles.
      6. Clothing & Tattoos: Worn layers and active displacements.
    - Support configurable synonym pools and tag interpolation.
  - **Verification**: Generating description for any NPC produces formatted, grammatical, multi-paragraph text.

---

## Milestone 4: Items, Layering, Granular Partial Displacement & Economy
**Goal**: Implement complete clothing layering, per-item partial displacement modes, and dynamic merchant economy.

### Micro-Tasks:
- [ ] **Task 4.1: Clothing Partial Displacement Engine**
  - **Target Files**: `src/items/item.h`, `src/items/clothingDisplacement.h`, `src/items/inventory.h`, `src/items/inventory.cpp`
  - **Details**:
    - Enum `DisplacementMode { NONE, UNBUTTON, PULL_ASIDE, LIFT_UP, PULL_DOWN, OPEN }`.
    - In `item`: `std::unordered_map<DisplacementMode, std::vector<bodySlot>> supportedDisplacements`.
    - In `inventoryComponent`:
      - `std::unordered_map<equipSlot, DisplacementMode> activeDisplacements;`
      - `void setDisplacement(equipSlot slot, DisplacementMode mode);`
      - `void resetAllDisplacements();`
      - `bool isSlotExposed(bodySlot slot) const;`
  - **Verification**: Displacing a shirt exposes torso/breasts without unequipping it; resetting restores full coverage.

- [ ] **Task 4.2: Data-Driven Item Database & JSON Overhaul**
  - **Target Files**: `src/items/itemDatabase.cpp`, `data/items.json`
  - **Details**:
    - Expand `data/items.json` schema to include displacement definitions, required/forbidden tags, and stat modifiers.
    - Validate JSON parser loading 100% of fields into `item` definitions.
  - **Verification**: All items load into registry with displacement metadata.

- [ ] **Task 4.3: Deep Merchant Economy & Daily Restock**
  - **Target Files**: `src/items/merchantValuation.h`, `src/items/merchantValuation.cpp`, `src/entities/entity.h`
  - **Details**:
    - Dynamic buy/sell calculations factoring player `tradePerkModifier`, merchant affinity, and item condition.
    - Implement `merchantRestock(entity* merchant, int currentDay)` replenishing gold and item stock at 06:00 daily.
  - **Verification**: Buying/selling adjusts currency accurately; restock triggers on day rollover.

---

## Milestone 5: "All-Powerful" JSON Scene Scripting & Dialogue Engine
**Goal**: Build a fully extensible narrative scripting engine allowing total world, quest, and entity manipulation via JSON.

### Micro-Tasks:
- [ ] **Task 5.1: Universal Condition Evaluation Tree**
  - **Target Files**: `src/quest/conditionNode.h`, `src/quest/conditionNode.cpp`
  - **Details**:
    - Support operators: `LEAF`, `AND`, `OR`, `NOT`.
    - Condition types: `STAT_CHECK`, `HAS_ITEM`, `EQUIPPED_SLOT`, `HAS_TAG`, `HAS_MUTATION`, `QUEST_FLAG`, `IS_PREGNANT`, `TIME_HOUR_BETWEEN`, `GENDER_IS`, `RACE_IS`, `DOMINANCE_BETWEEN`.
  - **Verification**: Complex nested condition trees evaluate boolean logic accurately.

- [ ] **Task 5.2: Expanded Action Effect Suite**
  - **Target Files**: `src/quest/quest.h`, `src/core/game.cpp`
  - **Details**:
    - Implement full action executor for `gameEffect`:
      - `TELEPORT(mapId, x, y)`
      - `GIVE_ITEM(itemId, count)` / `REMOVE_ITEM(itemId, count)` / `TRANSFER_CURRENCY(source, target, amount)`
      - `MODIFY_STAT(statName, delta)`
      - `TRANSFORM_PART(slot, race, covering, color, sizeDelta)`
      - `FILL_FLUID(fluidType, amount)` / `DRAIN_FLUID(fluidType, amount)` / `STRETCH_ORIFICE(orifice, delta)`
      - `IMPREGNATE(motherId, fatherId, litterSize, overrideRace)` / `INDUCE_BIRTH(motherId)`
      - `DISPLACE_CLOTHING(slot, mode)` / `RESTORE_CLOTHING()`
      - `SET_FLAG(flagName, value)` / `CALL_SUB_SCENE(sceneId)` / `RANDOM_BRANCH(weightArray)`
      - `SPAWN_NPC(templateId, x, y)` / `DESPAWN_NPC(npcId)`
  - **Verification**: Executing each script action in a test scene alters the corresponding game subsystem.

- [ ] **Task 5.3: Quest & Scene Database Registry**
  - **Target Files**: `src/quest/questDatabase.h`, `src/quest/questDatabase.cpp`, `data/quests/*.json`
  - **Details**:
    - Recursive directory loading for all `.json` files in `data/quests/`.
    - Scene stack management: `pushScene(sceneId)`, `popScene()` to support subroutines and nested dialogue.
  - **Verification**: Multi-file quest scenes load and transition seamlessly.

---

## Milestone 6: World Map, Navigation & Tile Runtime Mechanics
**Goal**: Implement complete grid map traversal, fog of war, triggers, persistent tile data, and dynamic NPC encounters.

### Micro-Tasks:
- [ ] **Task 6.1: Map Grid & Fog-of-War Discovery**
  - **Target Files**: `src/map/gameMap.h`, `src/map/gameMap.cpp`, `src/map/tile.h`
  - **Details**:
    - Grid traversal with wall/door collision checking.
    - Radius-based line-of-sight fog-of-war updating (`STATE_HIDDEN` -> `STATE_PARTIAL` -> `STATE_REVEALED`).
    - Map warps and coordinate triggers.
  - **Verification**: Moving player reveals surrounding tiles and triggers map transitions on warps.

- [ ] **Task 6.2: Tile Runtime Storage & Unsafe Decay**
  - **Target Files**: `src/map/gameMap.cpp`, `src/map/tile.h`
  - **Details**:
    - Persistent dropped item containers per tile coordinate.
    - Unsafe tile item cleanup when `advanceTime` ticks past item lifespan.
  - **Verification**: Dropping items on safe tiles preserves them; unsafe tiles despawn items over time.

- [ ] **Task 6.3: NPC Encounter Generation**
  - **Target Files**: `src/map/encounterResolver.h`, `src/map/encounterResolver.cpp`, `src/entities/npcGenerator.cpp`
  - **Details**:
    - Roll dynamic encounters based on tile danger level.
    - Generate NPCs using demographic settings (sexuality, gender archetypes, level ranges).
  - **Verification**: Stepping on high-danger tiles triggers appropriate random encounters.

---

## Milestone 7: Combat System: Multi-Party Encounters & Resolution Hub
**Goal**: Build party-based combat, a debug resolution stub, and a comprehensive victory/defeat resolution hub.

### Micro-Tasks:
- [ ] **Task 7.1: Multi-Party Participant Queue & AP Engine**
  - **Target Files**: `src/combat/combatEngine.h`, `src/combat/combatEngine.cpp`
  - **Details**:
    - Handle 1v1 up to 4v4 party combat setups.
    - Round-based Action Point (AP) allocation, action queuing, and turn execution.
  - **Verification**: Multi-character queues resolve actions in AP sequence.

- [ ] **Task 7.2: Combat Debug Simulation Stub**
  - **Target Files**: `src/state/combatState.h`, `src/state/combatState.cpp`
  - **Details**:
    - On entering combat, present a debug action grid displaying all detected combatants.
    - Provide immediate test buttons: `[Simulate Win]`, `[Simulate Defeat]`, `[Simulate Escape]`, `[Simulate Surrender]`.
  - **Verification**: Clicking any simulation button triggers the corresponding combat outcome event.

- [ ] **Task 7.3: Post-Combat Resolution Hub**
  - **Target Files**: `src/state/encounterResolutionState.h`, `src/state/encounterResolutionState.cpp`
  - **Details**:
    - For multi-enemy victories: Display defeated enemies list.
    - Selecting an enemy opens sub-actions: `[Loot Items & Gold]`, `[Strip Clothing]`, `[Interactive Sex]`, `[Subjugate]`, `[Release]`.
    - Completing a sub-action returns to the resolution hub until player chooses `[Leave]`.
    - Single enemy victory routes directly to the options menu without a hub.
  - **Verification**: Looting, stripping, or initiating sex returns to the hub without dropping into exploration prematurely.

- [ ] **Task 7.4: Sexuality-Driven Defeat Resolution**
  - **Target Files**: `src/state/combatState.cpp`
  - **Details**:
    - Check defeating NPC's sexuality and lust.
    - Compatible orientation triggers a narrative defeat/seduction scene; incompatible triggers standard robbery defeat.
  - **Verification**: Defeat branch routes dynamically based on NPC sexuality.

---

## Milestone 8: Dedicated CYOA Interactive Sex Engine (`sexState`)
**Goal**: Implement turn-based CYOA erotic encounters with physical positional constraints, dominance continuum, and fluid transfers.

### Micro-Tasks:
- [ ] **Task 8.1: Sex State Core & Positional Proximity System**
  - **Target Files**: `src/state/sexState.h`, `src/state/sexState.cpp`
  - **Details**:
    - Stances: `MISSIONARY`, `FROM_BEHIND`, `KNEELING`, `STANDING`, `LAP_SITTING`.
    - Positional matrix defining reachable sockets (e.g. Kneeling allows Mouth <-> Groin; Missionary allows Groin <-> Groin & Breasts).
    - Disable actions if interacting body parts are not spatially adjacent.
  - **Verification**: Stance changes dynamically update valid action buttons.

- [ ] **Task 8.2: Dynamic Dominance Continuum & Initiative**
  - **Target Files**: `src/state/sexState.h`, `src/state/sexState.cpp`, `src/entities/entity.h`
  - **Details**:
    - Track `dominance` score (-100 Max Submissive to +100 Max Dominant).
    - When player is submissive: Player turn offers `[Plead / Suggest Action]`, `[Endure]`, `[Beg for Climax]`. Dominant NPC enforces actions and position changes.
    - When player is dominant: Full control over commands and pacing.
  - **Verification**: Actions available to the player reflect their current dominance score.

- [ ] **Task 8.3: Arousal, Orifice Engagement & Fluid Transfer**
  - **Target Files**: `src/state/sexState.cpp`
  - **Details**:
    - Arousal tracking (0-100). Reaching 100 triggers Orgasm Event.
    - Orifice penetration checks: compare organ size vs orifice stretch/lubrication.
    - Climax ejaculates cum/milk into the receiving orifice or body surface, incrementing fluid stores and potential gestation.
  - **Verification**: Orgasm transfers fluids and triggers pregnancy check if applicable.

- [ ] **Task 8.4: Narrative Text Generation & Clothing Reversion**
  - **Target Files**: `src/state/sexState.cpp`
  - **Details**:
    - Generate dynamic text snippets for actions using `characterDescription` and `textParser`.
    - On exiting `sexState`: Automatically restore all clothing displacements to normal worn state.
  - **Verification**: Exiting sex state leaves player clothing fully restored.

---

## Milestone 9: World Simulation & Time-Driven Biological Decay
**Goal**: Unify all biological and world time-advancement hooks under `timeManager`.

### Micro-Tasks:
- [ ] **Task 9.1: Comprehensive Clock & Calendar System**
  - **Target Files**: `src/core/timeManager.h`, `src/core/timeManager.cpp`
  - **Details**:
    - Track minute, hour, day, month, year, day of week.
    - Calculate sunrise, sunset, dawn, day, dusk, night phases.
  - **Verification**: Advancing time increments calendar correctly across month/year boundaries.

- [ ] **Task 9.2: Time Advancement Biological Pipeline**
  - **Target Files**: `src/core/game.cpp`
  - **Details**:
    - On `gameEvent::timeAdvanced`:
      - Advance anatomy mutations (`anatomyComponent::processMutations`).
      - Regenerate milk and cum based on hourly rates.
      - Advance active pregnancies (`gestationComponent::processGestation`).
      - Natural arousal decay and status effect expiration.
  - **Verification**: Fast-forwarding time advances all biological processes.

- [ ] **Task 9.3: Scheduled World Maintenance**
  - **Target Files**: `src/core/game.cpp`
  - **Details**:
    - At 06:00 daily: Trigger merchant restocking and currency refresh.
    - Despawn expired items on unsafe map tiles.
  - **Verification**: Daily restock fires once per day at 06:00.

---

## Milestone 10: Save System Hardening & JSON World Serialization
**Goal**: Build robust, multi-slot, versioned JSON serialization for all game data.

### Micro-Tasks:
- [ ] **Task 10.1: Versioned Save Payload Schema**
  - **Target Files**: `src/save/saveManager.h`, `src/save/saveManager.cpp`
  - **Details**:
    - Schema version `"saveVersion": 1`.
    - Serialize: Player (anatomy, fluids, mutations, gestation, inventory, displacements, stats), all visited map runtime data, active quests/flags, global clock, and settings.
  - **Verification**: Saved JSON can be parsed back into identical game state.

- [ ] **Task 10.2: Atomic File Writing & Multi-Slot Organization**
  - **Target Files**: `src/save/saveManager.cpp`
  - **Details**:
    - Write to `.tmp` file and rename to prevent corruption.
    - Group saves by character name under `data/saves/`.
  - **Verification**: Saves organize cleanly per character and resist partial writes.

- [ ] **Task 10.3: Auto-Save Triggers**
  - **Target Files**: `src/save/saveManager.cpp`, `src/core/game.cpp`
  - **Details**:
    - Auto-save on map transitions, scene exits, and configurable time intervals.
  - **Verification**: Auto-save files rotate up to `maxAutosaves` limit.

---

## Milestone 11: Decoupled UI Frontend & Themeable Presentation
**Goal**: Build a clean, swappable SDL3 UI frontend reading data getters and sending `UICommand` objects.

### Micro-Tasks:
- [ ] **Task 11.1: UI Theme Engine**
  - **Target Files**: `src/ui/theme.h`, `src/ui/theme.cpp`, `data/themes/theme.json`
  - **Details**:
    - Load palettes, borders, and typography configs from `theme.json`.
  - **Verification**: Changing theme JSON recolors the UI instantly.

- [ ] **Task 11.2: Decoupled View Renderer**
  - **Target Files**: `src/ui/uiRenderer.h`, `src/ui/uiRenderer.cpp`
  - **Details**:
    - Render multi-pane layout: Stats/Anatomy (left), Narrative/Dialogue (center), Map/Minimap (right), Action Grid (bottom).
    - View layer queries `game` getters exclusively.
  - **Verification**: UI renders complete game state without executing game rules.

- [ ] **Task 11.3: Dynamic Action Grid & Button Layout**
  - **Target Files**: `src/ui/actionGridManager.h`, `src/ui/actionGridManager.cpp`
  - **Details**:
    - Render paginated button grids translating clicks into `UICommand` packets.
  - **Verification**: Clicking buttons dispatches correct commands to the active game state.
