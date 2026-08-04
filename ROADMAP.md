# textRPG — Master Blueprint & Implementation Roadmap

## 1. Project Standards & Collaboration Protocol

### Codebase Conventions
* **Language Standard**: C++26 (`CMAKE_CXX_STANDARD 26`).
* **Spelling Standard**: British English conventions across all symbols, variables, functions, comments, and JSON data keys (`colour`, `initialise`, `serialise`, `behaviour`, `customise`, `jewellery`).
* **Memory & Performance Goal**: Zero heap allocations in hot loop code paths; contiguous array storage for enums; data-driven JSON extensibility.

### 2-Step Collaboration Protocol
1. **Architectural Evaluation**: Before writing code, evaluate design, memory layout, system boundaries, and impacted files. Discuss without ego—refactoring bad patterns is always preferred over preserving flawed legacy code.
2. **Execution**: Write clean, modern C++26 code once both parties agree on the structure.

---

## 2. Core Architectural Principles

* **State Machine Pattern**: Replace conditional UI and input branches in `game.cpp` with dedicated `IGameState` controllers (`ExplorationState`, `InventoryState`, `CombatState`, `EventState`).
* **Decoupled Event Bus**: Systems publish and subscribe to light game events rather than calling concrete engine functions directly.
* **Timed Mutation Engine**: Anatomical transformations support instant changes (potions/menus) and scheduled growth over time (enchantments/potions ticking during time advance).
* **Dynamic Scene Interpolation Engine**: Dialogue text handles dynamic token expansion (`{player.name}`, `{player.he/she}`, `{target.chest_desc}`) based on runtime entity traits.
* **Template-Driven Persistent NPCs**: Random encounters generate NPCs from JSON templates and anchor them to `TileRuntimeData` so they persist across tile visits and saves.
* **N-vs-M Combat Backend**: Combat logic manages party arrays (`playerParty`, `enemyParty`) independently of UI screen bounds, allowing participant limits to scale without engine rewrites.
* **Compound Condition Engine**: Quests and dialogue support AST boolean evaluation trees (`AND`, `OR`, `NOT`) for complex game state checks.
* **Multi-Slot Save & Autosave System**: Full serialisation supporting unlimited custom save slots and automated background saves on key triggers.
* **Strict World Persistence**: Safe tile item drops persist indefinitely; unsafe tile drops expire on exit but are accurately preserved if saved directly while on the tile.

---

## 3. Master Phase & Task Tracker

### Phase 1: Architecture Refactoring & Core Engine
- [ ] **Task 1.1: State Machine Controller Architecture**
    * Refactor state handling out of `game.cpp` into concrete `IGameState` objects.
- [ ] **Task 1.2: Centralised Game Event Bus**
    * Implement an event publisher/subscriber model for decoupled system communication.
- [ ] **Task 1.3: Recursive Compound Condition Evaluator**
    * Replace flat condition vectors with tree-based boolean evaluation (`ConditionNode`).
- [ ] **Task 1.4: Dynamic Scene Parsing & String Interpolation Engine**
    * Implement tag replacement for names, pronouns, anatomical descriptors, and dynamic scene terms.

### Phase 2: Dynamic Anatomy & Transformation Pipeline
- [ ] **Task 2.1: Timed Mutation Queue Engine**
    * Implement scheduled anatomical growth ticks hooked into `timeManager::advanceTime`.
- [ ] **Task 2.2: Instant Transformation API & Dominant Race Calculator**
    * Build potion/menu transformation methods and modal body race percentage checks.

### Phase 3: NPC Generation, Tile Persistence & Multi-Slot Saves
- [ ] **Task 3.1: JSON Template-Based NPC Generator**
    * Load NPC templates from JSON to generate varied entities with randomized traits/equipment.
- [ ] **Task 3.2: Encounter NPC Tile Persistence**
    * Ensure generated encounter NPCs remain anchored to their overworld tile runtime data until resolved.
- [ ] **Task 3.3: Multi-Slot Save & Autosave Manager**
    * Expand `saveManager` to support unlimited custom save files, slot metadata, and automatic save triggers.

### Phase 4: N-vs-M Tactical Combat Backend & Interface
- [ ] **Task 4.1: Decoupled Multi-Participant Combat Backend**
    * Build party vectors, initiative speed sorting, and turn step resolution loops.
- [ ] **Task 4.2: Dynamic Combat UI & Action Generation**
    * Render participant cards dynamically and generate combat actions from items/anatomy.
- [ ] **Task 4.3: Quest Overrides for Combat Resolution**
    * Connect custom victory, defeat, escape, and surrender outcome handlers to JSON quest files.

### Phase 5: Economy, World Persistence & UI Polish
- [ ] **Task 5.1: Merchant Price Valuation System**
    * Calculate item buying/selling prices dynamically based on condition and enchantments.
- [ ] **Task 5.2: Mouse Drag-and-Drop Inventory Overlay**
    * Implement drag-and-drop item movement layered over the existing grid interface.
- [ ] **Task 5.3: Absolute Tile Safety Persistence Validation**
    * Ensure unsafe tile items are properly serialized on save and cleared on transition.

---

## 4. Detailed Task Specifications

### [ ] Task 1.1: State Machine Controller Architecture
* **Goal**: Extract UI state logic, event loops, and input handlers from `game.cpp` and `inputHandler.cpp` into distinct state controllers.
* **Files Impacted**: `src/core/game.h`, `src/core/game.cpp`, `src/input/inputHandler.cpp`
* **New Files**: `src/state/IGameState.h`, `src/state/ExplorationState.h/.cpp`, `src/state/InventoryState.h/.cpp`, `src/state/EventState.h/.cpp`
* **Implementation Summary**:
    1. Define `IGameState` interface (`handleInput`, `update`, `render`, `onEnter`, `onExit`).
    2. Implement state objects holding reference to shared `game` context.
    3. Replace `switch (currentState)` statements with `currentGameState->render()`.

### [ ] Task 1.4: Dynamic Scene Parsing & String Interpolation Engine
* **Goal**: Replace dialogue placeholders with dynamic text based on current entity stats, pronouns, and anatomy.
* **Files Impacted**: `src/core/game.cpp`, `src/quest/quest.h`
* **New Files**: `src/core/textParser.h`, `src/core/textParser.cpp`
* **Implementation Summary**:
    1. Build a string parsing function that replaces tokens like `{player.name}`, `{player.he/she}`, or `{target.race}`.
    2. Add helper functions for grammatical agreement (e.g., `a/an` selection, singular/plural verbs).

### [ ] Task 3.1: JSON Template-Based NPC Generator
* **Goal**: Procedurally construct NPCs from templates in `data/npc_templates.json`.
* **Files Impacted**: `src/entities/entity.h`, `src/core/game.cpp`
* **New Files**: `src/entities/npcGenerator.h`, `src/entities/npcGenerator.cpp`
* **Implementation Summary**:
    1. Define `NPCTemplate` structure with stat ranges, body part probability tables, and equipment lists.
    2. Instantiate customized `entity` pointers on demand during encounters.

### [ ] Task 3.3: Multi-Slot Save & Autosave Manager
* **Goal**: Support unlimited save slots, save file management, and automated background saving.
* **Files Impacted**: `src/save/saveManager.h`, `src/save/saveManager.cpp`, `src/core/game.cpp`
* **Implementation Summary**:
    1. Implement a directory scanner for `data/saves/` returning save metadata (timestamp, play time, player level).
    2. Implement an `autosave()` trigger method called on map changes or major story choices.