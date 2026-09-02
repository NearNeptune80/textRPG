# MASTER ROADMAP: textRPG Engine & UI Completion

> **Core Philosophy**:
> 1. **Data-Driven Layouts (ZERO HARDCODING)**: All panel dimensions, split ratios, column sizes, widget assignments, and screen geometries MUST be defined in `data/layouts/*.json` (e.g. `default_layout.json`), never hardcoded in C++ source files.
> 2. **Systems & UI First, Content Last**: Content will only be developed once the engine and UI are fully complete, optimized, responsive, and visually verified.
> 3. **Screen-by-Screen Perfection**: Perfect one UI screen at a time starting from Character Creation, thoroughly testing functionality and responsiveness as we go.
> 4. **Holistic Integration Testing**: Validate the entire game loop seamlessly using dedicated test JSON fixtures for quests, items, maps, and NPCs.
> 5. **Iterative Multi-Pass Optimization Cycles**: Multiple refactor & optimization passes (Pass A -> Test -> Pass B -> Test -> Pass C -> Test) to trim token bloat, eliminate duplication, and maximize efficiency.
> 6. **Full Content Production**: Once the engine and interface are rock-solid, go all in on content authoring.

---

## Phase 1: Screen-by-Screen UI & Systems Perfection

We will work through each screen one by one from start to finish. For each screen, ensure:
- Layout configuration is driven by `data/layouts/*.json`.
- Visual layout, contrast, spacing, borders, fonts, and responsive scaling are perfected.
- All interactive controls, tooltips, and underlying gameplay systems work 100%.
- Verified interactively in-game and covered by automated tests.

---

### Screen 1: Character Creation Screen (`characterCreationState` / `characterCreationView`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Ensure all panel bounds, banner heights, column widths, and scroll areas are configured via `default_layout.json` rather than hardcoded in C++.
- [ ] **Visual Polish & Ergonomics**:
  - Header banner, tab navigation bar, character summary preview bar.
  - Reusable card widgets (`drawPillCard`, `drawTogglePillCard`, `drawColorSwatchCard`, `drawStepperCard`, `drawToggleCard`).
  - High-contrast selection highlights, consistent padding, and crisp font scaling across resolutions.
- [ ] **Gameplay Systems & State**:
  - Full attribute and demographic initialization (Gender, Femininity, Orientation, Height, Body Build, Hair, Eyes, Cosmetics, Piercings).
  - Vitals and maximum bounds initialization (`health = 100`, `max_health = 100`, `mana = 100`, `max_mana = 100`, `max_lust = 100`, `max_arousal = 100`, `currency = 5000`, `arcaneEssence = 20`).
  - Dynamic wardrobe choices and initial equipment dressing.
- [ ] **Verification**:
  - Interactive walkthrough test from New Game -> Step 1 to Step 5 -> Finalize Character into Exploration.
  - Automated test validation in `engineTests.cpp`.

---

### Screen 2: Overworld Exploration & Map Navigation Screen (`explorationState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Configure left sidebar (character card + radar dpad), center column (narrative story + action grid), right sidebar (present characters, ground items, activity log).
- [ ] **Visual Polish & Ergonomics**:
  - Centered 5x5 / 7x7 radar map grid with clear player marker, discovered terrain, obstacles, and fog-of-war.
  - Time & calendar widget display (day, month, hour, phase: Day/Night/Dawn/Dusk).
  - Location header and environmental danger indicators.
- [ ] **Gameplay Systems**:
  - Cardinal movement (North, South, East, West) with wall/obstacle collision checks.
  - Tile dropped items decay and pickup/inspection.
  - Dynamic tile encounter triggers and transition to combat/dialogue.
- [ ] **Verification**:
  - Live map exploration test; automated movement test.

---

### Screen 3: Inventory & Equipment Screen (`inventoryState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Dual-column container split (backpack left, ground/container right).
  - Outside page selectors, 6x6 paperdoll equipment matrix, item details inspector.
- [ ] **Visual Polish & Ergonomics**:
  - Square grid slots with distinct borders and item rarity/type colors.
  - Comprehensive tooltips for all equipped slots, inventory items, and action buttons.
- [ ] **Gameplay Systems**:
  - Equip / unequip gear with real-time stat modifiers updating the character card.
  - Usable consumable items (potions restore vitals and decrement stack counts).
  - Clothing partial displacement engine (Unbutton, Pull Down, Lift Up, Expose).
  - Drop 1, Drop All, Take 1, Take All operations.
- [ ] **Verification**:
  - Interactive inventory management test; verify stat modifications and displacement states.

---

### Screen 4: Post-Combat Resolution & Looting Screen (`encounterResolutionState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Dedicated two-column container looting view (Defeated enemy inventory on right, player backpack on left).
  - Victory summary card displaying gained EXP, currency, and enemy condition.
- [ ] **Visual Polish & Ergonomics**:
  - High-visibility loot transfer grid with "Take 1", "Take All", and "Loot Everything" buttons.
- [ ] **Gameplay Systems**:
  - Direct transition from combat victory into looting container.
  - Transfer of enemy equipped items, carried drops, and gold into player inventory.
  - Experience distribution and leveling calculations.
  - Seamless exit back to exploration state.
- [ ] **Verification**:
  - Combat victory -> Looting session -> Return to world.

---

### Screen 5: Merchant & Barter Screen (`shopState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Three-pane / dual-column layout: Merchant catalog, player inventory, transaction cart.
- [ ] **Visual Polish & Ergonomics**:
  - Item price tags with currency symbol (`¤`), rarity borders, and item comparison tooltips.
  - Merchant gold reserve readout.
- [ ] **Gameplay Systems**:
  - Buy, Sell 1, Sell Stack operations with merchant affinity price scaling.
  - Daily inventory restock and merchant wallet replenishment.
- [ ] **Verification**:
  - Buying and selling transactions verified against player and merchant gold.

---

### Screen 6: Dialogue, Quest & CYOA Story Scene Screen (`eventState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Speaker identification header, avatar portrait, wrapped story body pane, action command grid.
- [ ] **Visual Polish & Ergonomics**:
  - Text interpolation tags (`[Player.Name]`, `[NPC.Name]`, pronouns).
  - Disabled choices styled with greyed out appearance and data-driven requirement tooltips.
- [ ] **Gameplay Systems**:
  - Universal condition evaluation (items, flags, stats, time, gender).
  - Branching choice dispatch, quest stage advancement, rewards.
- [ ] **Verification**:
  - Multi-branch quest conversation execution.

---

### Screen 7: Turn-Based Tactical Combat Screen (`combatState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Top turn order queue, left player card, center tactical arena, right enemy vitals card, bottom combat action grid.
- [ ] **Visual Polish & Ergonomics**:
  - High-contrast action queue, damage floaters/log entries, health/mana/lust gauge animations.
- [ ] **Gameplay Systems**:
  - AP-based combat actions (Physical Attack, Magic, Abilities, Consumables, Flee).
  - Multi-party turn processing and enemy AI target selection.
  - Defeat resolution: physical defeat, erotic seduction/surrender, capture.
- [ ] **Verification**:
  - Full combat match from start to finish.

---

### Screen 8: Interactive Sex & Intimacy Encounter Screen (`sexState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Dual arousal & lust progress bars, positional status, intimacy action grid.
- [ ] **Visual Polish & Ergonomics**:
  - Orifice status indicators, fluid transfer readouts, dominance continuum bar.
- [ ] **Gameplay Systems**:
  - Position changes, arousal/lust accumulation, orgasms, fluid transfers.
- [ ] **Verification**:
  - Full intimate encounter playthrough.

---

### Screen 9: Body Inspection & Paperdoll Anatomy Screen
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Comprehensive anatomical inspection panels for all body parts and sockets.
- [ ] **Visual Polish & Ergonomics**:
  - Procedural descriptive prose generator reflecting active transformations.
- [ ] **Gameplay Systems**:
  - Real-time measurements, cup sizes, fluids, and mutations.
- [ ] **Verification**:
  - Verify prose accuracy for varied body configurations.

---

### Screen 10: Transformation Chamber Screen (`transformationState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - 8-tab body modification suite driven by layout JSON.
- [ ] **Visual Polish & Ergonomics**:
  - Shared card widgets, swatch pickers, and live character preview.
- [ ] **Gameplay Systems**:
  - Real-time racial mutations, hybrid traits, preset save/load.
- [ ] **Verification**:
  - Apply transformations, save preset, and restore.

---

### Screen 11: Save & Load Game Management Screen (`loadGameState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Character-grouped save list with metadata previews.
- [ ] **Visual Polish & Ergonomics**:
  - Sorting toggles (Name vs Timestamp), confirmation modals for delete/overwrite.
- [ ] **Gameplay Systems**:
  - Atomic JSON save/load with version migration.
- [ ] **Verification**:
  - Save, reload, and verify world/player fidelity.

---

### Screen 12: Options & Settings Screen (`optionsState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - 9-category content settings, demographics, keybindings, and themes.
- [x] Shared toggles and demographic components refactored.
- [ ] **Verification**:
  - Setting changes persist across restarts.

---

### Screen 13: Main Menu & Title Screen (`mainMenuState`)
- [ ] **Data-Driven Layout (`data/layouts/default_layout.json`)**:
  - Title banner, hero portrait, quick-start actions.
- [ ] **Verification**:
  - Navigation routing to New Game, Continue, Settings, and Load.

---

## Phase 2: Comprehensive End-to-End System Testing

Look at the game holistically and test all interconnected systems using dedicated test JSON fixtures.

### 2.1 Test JSON Suite Construction
- [ ] Create `data/test_fixtures/test_quests.json` (multi-branch quests with rewards and conditions).
- [ ] Create `data/test_fixtures/test_items.json` (diverse equipment with stat modifiers and consumables).
- [ ] Create `data/test_fixtures/test_maps.json` (multi-tile overworld with dungeons and safe zones).
- [ ] Create `data/test_fixtures/test_npcs.json` (friendly merchants, hostile bandits, companions).

### 2.2 Full Gameplay Loop Integration Test
- [ ] **Cycle 1**: New Game -> Custom Character -> Spawn into World -> Move on Map.
- [ ] **Cycle 2**: Discover Tile -> Talk to NPC -> Accept Quest -> Purchase Equipment from Merchant.
- [ ] **Cycle 3**: Equip Armor (verify stat boosts) -> Encounter Enemy -> Combat -> Defeat Enemy.
- [ ] **Cycle 4**: Loot Enemy Drops -> Return to Town -> Turn in Quest -> Consume Health Potion.
- [ ] **Cycle 5**: Save Game -> Quit to Main Menu -> Continue Game -> Verify exact world and player restitution.

---

## Phase 3: Iterative Optimization & Refactoring Cycles

Execute multiple focused passes across the codebase to eliminate repetition, streamline architecture, reduce token footprint, and enhance performance.

### Pass 1: UI View Deduplication & Layout Engine Optimization
- Consolidate layout math and drawing routines in `src/ui/views/gameplayViews.cpp`.
- Centralize grid calculations and panel backgrounds across gameplay screens.
- *Verification*: Run automated test suite; verify zero visual regressions via screenshots.

### Pass 2: Engine Text Generation & Interpolation Streamlining
- Refactor repetitive descriptive branching in `src/core/characterDescription.cpp`.
- Optimize string allocations in `textParser.cpp` using `std::string_view`.
- *Verification*: Run automated test suite; verify character inspection prose matches expected output.

### Pass 3: State & Action Grid Consolidation
- Unify repetitive action button dispatch and pagination across all state classes.
- Ensure common commands (hotkeys, escape, submenus) share centralized routing.
- *Verification*: Run automated test suite; test interactive keybinding responsiveness.

### Pass 4: Memory, Include & Build Optimization
- Audit header dependencies; replace unnecessary `#include` with forward declarations.
- Optimize compile times and eliminate any heap allocations in inner render loops.
- *Verification*: Clean compile with `-j1` and test suite execution.

---

## Phase 4: Full Content Production

Once the engine, UI, and systems are rock-solid, fully optimized, and thoroughly tested, transition to complete content authoring:
- Expansive world maps (Towns, Wilderness, Dungeons, Ruins).
- Full main storyline quests and deep companion sidequests.
- Complete item database (Weapons, Armor sets, Relics, Consumables).
- Rich enemy and NPC rosters with unique dialogue, abilities, and transformation paths.
