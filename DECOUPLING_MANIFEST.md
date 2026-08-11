# DECOUPLING_MANIFEST.md

## 1. Primary Objective & Core Vision
The goal of this refactoring pass is **absolute Model-View separation** (headless engine architecture).

The game engine must be decoupled from all rendering, window context, layout math, and screen-space collision handling. Once completed:
* The core engine runs as a pure headless simulation (managing rules, stats, time, inventory, maps, and state transitions).
* The UI layer acts as an exchangeable presentation wrapper that reads engine snapshots and dispatches abstract commands (`UICommand`) back to the engine.
* You can completely destroy, replace, or redesign `uiRenderer` and the UI visual layout twenty times over without ever breaking engine logic or modifying core code.

---

## 2. Mandatory Architectural Constraints & Coding Rules

### Rule 1: Zero Graphics in Core Logic
* No `SDL_Renderer*`, `SDL_Window*`, or SDL display presentation calls anywhere in `src/core/`, `src/state/`, `src/input/`, `src/entities/`, or `src/combat/`.
* No `render()` member functions on `game` or `iGameState`.
* Render loop execution and window management live **exclusively** in `main.cpp` and `src/ui/`.

### Rule 2: Naming & File Conventions
* **Extensions**: Strictly `.h` and `.cpp` (never `.hpp`).
* **Casing**: Strictly `camelCase` for all file names, class names, structs, and variables (e.g., `inputHandler.h`, `stateManager.cpp`, `combatState.h`, `iGameState.h`).
* **Spelling Standard**: British English conventions across standard comments and descriptions (`colour`, `initialise`, `serialise`, `behaviour`).

### Rule 3: Input & UI Decoupling
* `inputHandler` processes hardware event polling and maintains logical key/mouse states (`keyAction`). It **must not** contain widget collision logic (`UIGridHelper::contains`), screen bounds, or layout calculations.
* Screen button clicks, pixel hitboxes, and pagination calculations belong **strictly** in `src/ui/`.
* UI interactions trigger engine actions by calling engine methods or sending `UICommand` objects.

---

## 3. System Architecture & Responsibility Matrix

+-------------------------------------------------------------------------+
|                              VIEW LAYER                                 |
|               (src/ui/ & main.cpp render loop)                          |
|  - Manages SDL_Window, SDL_Renderer, UI layout geometry & pixel bounds. |
|  - Queries Engine data snapshots to render pixels on screen.            |
|  - Converts screen clicks to UICommands / engine method calls.          |
+-------------------------------------------------------------------------+
|                 ^
Dispatches          |                 | Inspects
UICommands /        |                 | State
Key Actions         v                 | Snapshots
+-------------------------------------------------------------------------+
|                           HEADLESS ENGINE                               |
|              (src/core/, src/state/, src/entities/, etc.)               |
|  - Pure simulation logic: state machine, turn queues, time, items.      |
|  - Zero knowledge of pixels, renderers, fonts, or UI button positions.  |
+-------------------------------------------------------------------------+

### File Map & Domain Ownership

| Directory | Core Purpose | Rendering Allowed? |
| :--- | :--- | :---: |
| `src/core/` | Engine controller, event bus, time, text parsing | **NO** |
| `src/state/` | Headless state machine controllers (`iGameState` implementations) | **NO** |
| `src/input/` | Raw hardware input mapping (`inputHandler`) | **NO** |
| `src/entities/` | Anatomy, stats, mutations, status effects, NPC templates | **NO** |
| `src/combat/` | Combat engine backend, participant queues, damage logic | **NO** |
| `src/map/` | Map grid, tile runtime data, warps, triggers | **NO** |
| `src/items/` | Inventory data, item databases, merchant pricing math | **NO** |
| `src/quest/` | Condition evaluation trees (`conditionNode`), quest database | **NO** |
| `src/save/` | JSON serialization and multi-slot save manager | **NO** |
| `src/ui/` | **VIEW ONLY**: Renderers, layout geometry, widget drawing | **YES** |
| `src/main.cpp` | Entry point, window/renderer initialization, main loop | **YES** |

---

## 4. Master Refactoring Checklist

### Phase 1: Engine Cleansing (`src/core/game.h` & `game.cpp`)
- [ ] Remove `SDL_Window*` and `SDL_Renderer*` from `game.h`.
- [ ] Remove `game::render()` and clear call from `game.cpp`.
- [ ] Remove UI presentation states (`actionGridPage`, `currentInventoryPage`, `currentRightInventoryPage`, `descriptionScrollY`) from `game.h`.
- [ ] Expose pure data-query methods for the UI to inspect engine state safely.

### Phase 2: State Controller Refactoring (`src/state/`)
- [ ] Remove `virtual void render(...)` from `iGameState.h`.
- [ ] Clean `explorationState.h/.cpp`: pure movement, time advancement, quick-saves.
- [ ] Clean `inventoryState.h/.cpp`: pure selection state tracking and item manipulation.
- [ ] Clean `eventState.h/.cpp`: scene choices and narrative flow.
- [ ] Clean `combatState.h/.cpp`: turn lifecycle and combat resolution.

### Phase 3: Hardware Input Decoupling (`src/input/`)
- [ ] Remove `SDL_RenderCoordinatesFromWindow`, layout bounds, and `UIGridHelper` click checks from `inputHandler.cpp`.
- [ ] Map physical keys to logical actions (`keyAction::moveUp`, `keyAction::toggleInventory`, `keyAction::confirm`).
- [ ] Store raw window mouse position without needing an `SDL_Renderer*`.

### Phase 4: UI View Layer Isolation & Window Ownership (`src/ui/` & `main.cpp`)
- [ ] Initialize `SDL_Window` and `SDL_Renderer` strictly inside `main.cpp`.
- [ ] Ensure `main.cpp` owns the main loop:
    1. `game.handleEvents()` (or UI event processing).
    2. `game.update(deltaTime)`.
    3. `uiRenderer.draw(game)` -> `SDL_RenderPresent`.
- [ ] Move grid click hitboxes and action button layout math into `src/ui/`.

---

## 5. Memory Recovery Protocol (If Model Reset Occurs)
If context is lost or a fresh session begins:
1. Load `DECOUPLING_MANIFEST.md` (this file).
2. Inspect the **Master Refactoring Checklist** above to see which phase and files are currently pending.
3. Review `src/core/game.h`, `src/state/iGameState.h`, and `src/input/inputHandler.h` to confirm the present state of the codebase.
4. Continue refactoring directly from the uncompleted tasks in Phase 1.
