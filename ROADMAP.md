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
- [x] **Task 1.1: State Machine Controller Architecture**
  * Refactored state handling out of `game.cpp` into concrete `iGameState` controllers (`explorationState`, `inventoryState`, `eventState`).
- [x] **Task 1.2: Centralised Game Event Bus**
  * Implemented `eventBus` publisher/subscriber model for decoupled system communication.
- [x] **Task 1.3: Recursive Compound Condition Evaluator**
  * Implemented AST condition evaluation tree (`conditionNode`) supporting `AND`, `OR`, and `NOT` boolean operations.
- [x] **Task 1.4: Dynamic Scene Parsing & String Interpolation Engine**
  * Implemented `textParser` token replacement for names, stats, and gender pronouns (`{player.he/she}`, `{target.name}`).

### Phase 2: Dynamic Anatomy & Transformation Pipeline
- [x] **Task 2.1: Timed Mutation Queue Engine**
  * Implemented `anatomyComponent::processMutations` hooked directly to `timeManager::advanceTime` via `eventBus`.
- [x] **Task 2.2: Instant Transformation API & Dominant Race Calculator**
  * Built transformation APIs and modal race composition percentage checks in `anatomyComponent`.

### Phase 3: NPC Generation, Tile Persistence & Multi-Slot Saves
- [x] **Task 3.1: JSON Template-Based NPC Generator**
  * Implemented `npcGenerator` to construct randomized entities from `data/npc_templates.json`.
- [x] **Task 3.2: Encounter NPC Tile Persistence**
  * Integrated persistent encounter NPCs into `TileRuntimeData` with save/load JSON serialization.
- [x] **Task 3.3: Multi-Slot Save & Autosave Manager**
  * Implemented `saveManager` supporting named saves, autosave rolling slots, and grouped metadata reads.

### Phase 4: N-vs-M Tactical Combat Backend & Interface
- [x] **Task 4.1: Decoupled Multi-Participant Combat Backend**
  * Build party vectors (`playerParty`, `enemyParty`), turn queue calculation based on initiative, and turn-resolution lifecycle.
- [x] **Task 4.2: Dynamic Combat UI & Combat State Controller**
  * Implement dedicated `CombatState` with party card rendering, action button generation, and target selection loops.
- [ ] **Task 4.3: Quest Overrides & Outcome Handlers for Combat**
  * Hook victory, defeat, surrender, and escape conditions back into `eventState` scenes and quest updates.

### Phase 5: Economy, World Persistence & UI Polish
- [ ] **Task 5.1: Merchant Price Valuation System**
  * Calculate item buying/selling prices dynamically based on item quality, condition, and enchantments.
- [ ] **Task 5.2: Mouse Drag-and-Drop Inventory Overlay**
  * Implement drag-and-drop item movement layered over the grid interface.
- [ ] **Task 5.3: Absolute Tile Safety Persistence Validation**
  * Audit unsafe tile item drops during transitions and direct saves.

---

## 4. Detailed Task Specifications

### [ ] Task 4.1: Decoupled Multi-Participant Combat Backend
* **Goal**: Build an extensible turn-based combat system capable of managing multiple participants on each side without coupling logic to screen render rects.
* **Files Impacted**: `src/common/enums.h`, `src/entities/entity.h/.cpp`
* **New Files**: `src/combat/combatEngine.h`, `src/combat/combatEngine.cpp`
* **Implementation Summary**:
  1. Define `CombatParty` vectors containing `std::shared_ptr<entity>`.
  2. Sort turn order using Speed/Agility initiative checks.
  3. Process damage resolution, status effect ticks, and victory/defeat boolean checks per turn cycle.