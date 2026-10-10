# Technical Codebase Audit: Stubs, Hardcoding & Unwired State

## Executive Summary
This audit inspects all C++ header (`.h`) and source (`.cpp`) files across the codebase to locate:
1. **Stubs and Placeholders**: Functions returning dummy literals (`return true;`, `return 0;`, `return 0.0f;`), empty method bodies (`{}`), or placeholder graphics/handlers.
2. **Hardcoding Violations**: Gameplay data (item stats/names, NPC dialogues, room connections, enemy templates, quest flags, fetishes) defined directly in C++ rather than loaded via JSON deserializers.
3. **Unwired State**: Systems, components, methods, or events that are instantiated/declared but never receive updates, ticks, or subscriptions from the main game loop.

### Prioritization Structure
- **Tier 1 (Must Fix Now)**: Any instances where gameplay data (items, dialogue, stats, rooms) is hardcoded in C++ instead of loaded from JSON, and any stubs that cause active gameplay loops or mechanics to silently fail.
- **Tier 2 (Postpone / Benign Stubs)**: Any placeholder functions or stubs for secondary features we are not actively testing right now (e.g., sound channels, weather effects, visual polish hooks, harmless empty lifecycle callbacks on modal states).

---

## Tier 1 (Must Fix Now)

### 1.1 Core Engine (16 items)

- [x] `src/state/explorationState.cpp:30`: `explorationState::update(game*, float)` — Empty method body `{}`. World simulation, tile passage, and continuous timers are never ticked during exploration. `[Missing Logic]`
- [x] `src/save/saveManager.cpp:88`: `catch (...) {}` — Empty exception block swallows save game serialization errors silently. `[Missing Logic]`
- [x] `src/core/game.cpp:152-154`: Hardcoded mock initialization log entries (`"[ZONE] Sanctuary Manor F1"`, `"[QUEST] Sanctuary Research"`, `"[INFO] Systems active"`). `[Hardcoded Mock Data]`
- [x] `src/state/characterCreationState.cpp:928-939`: Hardcoded starter quest IDs and starting stages (`"root_delivery"`, `"cottage_investigation"`, `"patrol_duty"`). `[Hardcoded Mock Data]`
- [x] `src/state/characterCreationState.cpp:941-945`: Hardcoded spawn map (`"overworld"`, 1, 1) and calendar time (month, day 29, 21:47). `[Hardcoded Mock Data]`
- [x] `src/state/editorConfig.h:120-310`: `EditorConfig::Catalogs` defines massive hardcoded C++ string vectors for races, body sizes, foot types, leg configurations, body hair, horns, wetness, cups, etc. instead of loading from `data/catalogs/character_creation.json`. `[Hardcoded Mock Data]`
- [x] `src/state/phoneAppsState.cpp:12-20`: Hardcoded static list of 28 fetishes in constructor instead of loading from `data/catalogs/fetishes.json`. `[Hardcoded Mock Data]`
- [x] `src/settings/gameSettings.h:320-325`: Hardcoded fetish preference names and default integer ratings embedded in C++ struct definition. `[Hardcoded Mock Data]`
- [x] `src/map/gameMap.cpp:138, 146, 161`: Fallback enemy template IDs hardcoded as `"tpl_alley_bandit"` and `"tpl_rogue_mage"` when danger level > 0 without explicit map encounter configuration. `[Hardcoded Mock Data]`
- [x] `src/entities/entity.cpp:815-824`: `canTransform()` hardcodes trait strings (`"shapeshifter"`, `"metamorph"`, `"changeling"`, `"transform"`, `"can_transform"`) and race strings (`"slime"`, `"demon"`, `"succubus"`, `"incubus"`, `"kitsune"`, `"doppelganger"`, `"changeling"`). `[Hardcoded Mock Data]`
- [x] `src/entities/npcGenerator.cpp:145-250`: Procedural anatomical body part IDs, names, fluid baselines, and orifice capacities hardcoded directly in C++. `[Hardcoded Mock Data]`
- [x] `src/save/saveManager.cpp:445-520`: `grantStarterTestKit()` hardcodes item deliveries, counts, and manually instantiates mock items (`item_infused_predator`, `item_infused_primal`) in C++. `[Hardcoded Mock Data]`
- [x] `src/main.cpp:240-350, 520-565, 620-660`: Hundreds of lines of hardcoded mock entities, stats, inventory items, and scene setups embedded in the game executable's CLI entry point. `[Hardcoded Mock Data]`
- [x] `src/entities/entity.cpp:59`: `entity::updateStatusEffectsOnTurn()` decrements turn-based durations and prunes expired status effects, but is never called anywhere in the codebase. `[Unwired State]`
- [x] `src/map/encounterResolver.cpp:11`: `encounterResolver::shouldTriggerEncounter(dangerLevel, phase, playerStealth)` calculates ambush chance based on time of day and player stealth, but is never invoked; `game.cpp:movePlayer` bypasses it completely. `[Unwired State]`
- [x] `src/core/game.cpp:312-335`: Time passage listener (`gameEvent::timeAdvanced`) only processes gestation for `Player` and `activeTargetNPC`; party companions and persistent map NPCs never have their pregnancy/gestation updated. `[Unwired State]`

### 1.2 Combat (11 items)

- [x] `src/combat/weaponSkillDatabase.cpp:23`: `node.baseMagnitude = 25.0f;` — Hardcoded arbitrary literal magnitude for all weapon skill action conversions. `[Missing Logic / Stub]`
- [x] `src/combat/combatEngine.cpp:235-246`: Companion attacks hardcoded in C++ (`"companion_strike_" + cElem`, names: `"Flame Burst"`, `"Aqua Splash"`, `"Rock Toss"`, `"Gale Gust"`, stamina cost 15.0f, damage calculation) rather than loaded from JSON. `[Hardcoded Mock Data]`
- [x] `src/combat/combatEngine.cpp:267-277`: Enemy basic strike hardcoded in C++ (`"action_basic_strike"`, `"Strike"`, staminaCost 20.0f, physical damage equal to physique) rather than loaded from JSON. `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:216-248`: Defensive maneuvers hardcoded in C++ (`"DEFENSE_GUARD"`, `"DEFENSE_DODGE"`, `"DEFENSE_PARRY"`) with hardcoded stamina costs and flags. `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:249-270`: Seduction maneuvers hardcoded in C++ (`"SEDUCE_TEASE"`, `"SEDUCE_FLIRT"`, `"SEDUCE_EXPOSE"`) with hardcoded lust damage values (+15, +25, +35). `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:271-310`: Companion attack and physical strikes hardcoded in C++ (`"COMPANION_ATTACK"`, `"STRIKE"`, `"HEAVY_STRIKE"`, `"DISARM"`). `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:311-380`: Spells and restorative items hardcoded in C++ (`"SPELL_DART"`, `"SPELL_FIREBALL"`, `"SPELL_SHIELD"`, `"SPELL_CLEANSE"`, `"SPELL_BLINK"`, `"ITEM_POTION"`) despite the existence of `data/spells.json`. `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:480-530`: Erotic and standard defeat scenes constructed via inline C++ string templates and choices (`"scene_combat_defeat"`, `"Recover and Continue"`). `[Hardcoded Mock Data]`
- [x] `src/state/combatState.cpp:555-565`: `handleClearQueue` manually constructs a hardcoded item (`item_health_potion`) via C++ `std::make_shared<item>()` when refunding potion actions. `[Hardcoded Mock Data]`
- [x] `src/combat/combatEngine.cpp`: `statusEffects` is never referenced or evaluated during combat damage calculation or turn execution; combatants' status effects are neither triggered nor updated. `[Unwired State]`
- [x] `src/combat/combatEngine.cpp`: Turn resolution does not call `entity::updateStatusEffectsOnTurn()`; turn-based buffs and debuffs never decay during combat rounds. `[Unwired State]`

### 1.3 Inventory (10 items)

- [x] `src/items/item.h:134, 165-167`: Substring searches on item IDs (`"key"`, `"gem"`, `"ore"`, `"stone"`, `"essence"`, `"material"`) hardcoded for category classification fallback. `[Hardcoded Mock Data]`
- [x] `src/items/merchantValuation.cpp:63-67`: Hardcoded restock items (`"item_canis_root"` and `"item_linen_shirt"`) directly added to merchant inventory. `[Hardcoded Mock Data]`
- [x] `src/items/enchantingEngine.cpp:86-180`: `composeItemName()` hardcodes extensive name affixes and suffixes directly in C++ (`"of Colossus Might"`, `"of Predator's Instinct"`, `"of Eldritch Clarity"`, `"of Primal Surge"`, etc.). `[Hardcoded Mock Data]`
- [x] `src/items/enchantingEngine.cpp:297-302`: `craftInfusedItem()` instantiates a hardcoded fallback item (`"infused_tonic"`, `"Infused Tonic"`) in C++ when `baseItem` is null. `[Hardcoded Mock Data]`
- [x] `src/items/enchantmentAspects.cpp:12-50`: `s_focusDefs` contains 25 hardcoded `AspectDefinition` records (IDs, display names, descriptions, prefixes, rarities) directly in C++. `[Hardcoded Mock Data]`
- [x] `src/items/enchantmentAspects.cpp:51-240`: `s_propDefs` contains over 100 hardcoded `AspectDefinition` records for body/attribute properties instead of loading from `data/enchantments/aspects.json`. `[Hardcoded Mock Data]`
- [x] `src/items/enchantmentAspects.cpp:280-315`: `getPropertyDefinition()` directly queries the hardcoded `s_propDefs` map rather than querying `EnchantmentRegistry`. `[Hardcoded Mock Data]`
- [x] `src/items/enchantmentAspects.cpp:337-470, 480-1550`: Massive hardcoded fallback vectors for compatible focuses, properties, and equipment slot restrictions in C++. `[Hardcoded Mock Data]`
- [x] `src/state/shopState.cpp:33-44`: Default merchant is hardcoded to named character `"marcus"` if no merchant entity is passed. `[Hardcoded Mock Data]`
- [x] `src/items/item.h:11` / `src/items/itemDatabase.cpp:30-120`: `item.h` declares `infusionEffects`, but `itemDatabase::from_json` never deserializes `infusionEffects` from JSON; pre-infused equipment cannot be created via data files. `[Unwired State]`

### 1.4 Dialogue (16 items)

- [x] `src/state/sexState.cpp:565`: `catch (...) {}` empty exception handler when loading `data/scenes/sex_actions.json`. `[Missing Logic]`
- [x] `src/quest/questDatabase.cpp:360-408`: Complete scene `debug_content_test_scene` with body text and 5 `dialogueChoice` options hardcoded directly in C++. `[Hardcoded Mock Data]`
- [x] `src/quest/questDatabase.cpp:410-425`: Complete narrative scene `scene_enslavement_success` with speaker, body text, and choice hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/map/encounterResolver.cpp:50-80`: Ambush encounter scene (`buildEncounterScene`) body text and 4 choices (`"Fight"`, `"Bribe (25¤)"`, `"Evade / Flee"`, `"Surrender"`) hardcoded directly in C++. `[Hardcoded Mock Data]`
- [x] `src/map/encounterResolver.cpp:83-97`: Encounter victory flavor texts (`selectFlavorText`) hardcoded for forest, house, and generic environments directly in C++. `[Hardcoded Mock Data]`
- [x] `src/map/encounterResolver.cpp:99-130`: Post-combat victory scene (`buildVictoryScene`) with choices (`"Leave / Spare"`, `"Permanently Remove"`, `"Loot"`, `"Interrogate"`) hardcoded directly in C++. `[Hardcoded Mock Data]`
- [x] `src/core/game.cpp:1490-1515`: Content filter choice warning dialogue scene (`"warning_choice_"`) with prompt text and `"Proceed"` / `"Cancel"` choices constructed via C++ strings. `[Hardcoded Mock Data]`
- [x] `src/core/game.cpp:1945-1965`: Content filter scene warning dialogue (`"warning_"`) with prompt text and `"Proceed to Scene"` / `"Skip Scene"` choices constructed via C++ strings. `[Hardcoded Mock Data]`
- [x] `src/core/game.cpp:2385-2398`: Fallback transformation dialogue scene (`"Bodily Sensation"`, body text, and `"Continue"` choice) hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/state/encounterResolutionState.cpp:331-335`: NPC conversation narrative log (`"You speak with {}. They look back at you with wary curiosity..."`) hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/state/sexState.cpp:308-320`: Solitary and partner encounter intro narrative strings hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/state/sexState.cpp:570-584`: Fallback action `partner_kiss` ("Passionate Kiss") hardcoded in C++ if JSON is missing. `[Hardcoded Mock Data]`
- [x] `src/state/sexState.cpp:2135-2225`: Plead, endure, beg climax, and struggle result narrative texts hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/state/sexState.cpp:2235-2300`: Orgasm, ejaculation, and conception narrative generation strings hardcoded in C++. `[Hardcoded Mock Data]`
- [x] `src/entities/namedCharacter.cpp:185`: `nc->defaultSceneId = cJson.value("dialogueSceneId", "");` loads `defaultSceneId`, but character conversation interactions in `actionGridManager.cpp:1790` and exploration bypass NPC dialogue trees unless a specific quest trigger is active. `[Unwired State]`
- [x] `src/entities/perkDatabase.h:19`: `std::vector<std::string> flags;` stores dialogue/scene unlock flags, but these flags are never evaluated during dialogue choice condition checks in `game::checkSingleCondition`. `[Unwired State]`

### 1.5 UI Adapter (7 items)

- [ ] `src/ui/actionGridManager.cpp:1515-1525`: Hardcoded list of combat spells (`SPELL_DART`, `SPELL_FIREBALL`, `SPELL_SHIELD`, `SPELL_CLEANSE`, `SPELL_BLINK`) with names, MP costs, and tooltips hardcoded directly in UI grid builder. `[Hardcoded Mock Data]`
- [ ] `src/ui/actionGridManager.cpp:1545-1620`: Hardcoded combat maneuver buttons (`DEFENSE_GUARD`, `DEFENSE_DODGE`, `DEFENSE_PARRY`, `SEDUCE_TEASE`, `SEDUCE_FLIRT`, `SEDUCE_EXPOSE`) with costs and descriptions embedded in UI code. `[Hardcoded Mock Data]`
- [ ] `src/ui/actionGridManager.cpp:1741-1743`: Hardcoded map warp label overrides (`"house_01"` -> `"Enter Cottage"`, `"overworld"` -> `"Enter Town"`). `[Hardcoded Mock Data]`
- [ ] `src/ui/views/gameplayViews.cpp:722-752`: Hardcoded list of all 28 fetish names and description strings directly inside gameplay view rendering. `[Hardcoded Mock Data]`
- [ ] `src/ui/views/gameplayViews.cpp:3442-3444`: Hardcoded map warp door descriptions (`"house_01"` -> `"the Cozy Cottage"`, `"overworld"` -> `"the Town District avenues"`). `[Hardcoded Mock Data]`
- [ ] `src/ui/views/optionsView.cpp:744-758`: Hardcoded static array of 39 fetishes and sensitivity tags in options view rendering. `[Hardcoded Mock Data]`
- [ ] `src/ui/views/transformationView.cpp:589`: `static const std::vector<std::string> crotchTypes = { "None", "Bovine Udders", "Caprine Udders", "Feline Crotch-Boobs", "Canine Teats" };` defined directly in view. `[Hardcoded Mock Data]`

---

## Tier 2 (Postpone / Benign Stubs)

### 2.1 Core Engine (25 items)

- [ ] `src/state/explorationState.cpp:13`: `explorationState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/explorationState.cpp:28`: `explorationState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/characterCreationState.cpp:164`: `characterCreationState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/characterCreationState.cpp:168`: `characterCreationState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/encounterResolutionState.cpp:29`: `encounterResolutionState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/encounterResolutionState.cpp:107`: `encounterResolutionState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/eventState.cpp:8`: `eventState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/eventState.cpp:18`: `eventState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/eventState.cpp:20`: `eventState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/loadGameState.cpp:16`: `loadGameState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/loadGameState.cpp:87`: `loadGameState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/loadGameState.cpp:89`: `loadGameState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/mainMenuState.cpp:12`: `mainMenuState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/mainMenuState.cpp:22`: `mainMenuState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/mainMenuState.cpp:24`: `mainMenuState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/optionsState.cpp:15`: `optionsState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/optionsState.cpp:37`: `optionsState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/phoneAppsState.cpp:54`: `phoneAppsState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/phoneAppsState.cpp:56`: `phoneAppsState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/transformationState.cpp:61`: `transformationState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/transformationState.cpp:63`: `transformationState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/mainMenuState.cpp:42-47`: Hardcoded fallback quicksave file names (`"Hero_QuickSave.json"`, `"QuickSave.json"`, `"Hero_Initial.json"`). `[Hardcoded Mock Data]`
- [ ] `src/core/eventBus.h:13`: `gameEvent::entityMutated` is declared in the enum, but is never published or subscribed anywhere across the engine. `[Unwired State]`
- [ ] `src/core/eventBus.h:16`: `gameEvent::itemUsed` is declared in the enum, but is never published or subscribed anywhere across the engine. `[Unwired State]`
- [ ] `src/core/eventBus.h:17`: `gameEvent::mapEntered` is published in `game.cpp:475`, but has zero subscribers across the entire engine. `[Unwired State]`

### 2.2 Combat (1 items)

- [ ] `src/state/combatState.cpp:55`: `CombatState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`

### 2.3 Inventory (9 items)

- [ ] `src/state/inventoryState.cpp:15`: `inventoryState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/inventoryState.cpp:28`: `inventoryState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/inventoryState.cpp:30`: `inventoryState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/enchantingState.cpp:52`: `enchantingState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/enchantingState.cpp:64`: `enchantingState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/shopState.cpp:20`: `shopState::initialise(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/shopState.cpp:55`: `shopState::onExit(game*)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/state/shopState.cpp:57`: `shopState::update(game*, float)` — Empty method body `{}`. `[Missing Logic]`
- [ ] `src/items/inventory.cpp`: Item condition / durability is represented in valuation logic (`merchantValuation.cpp`), but is never decremented during gameplay, combat, or wear. `[Unwired State]`

### 2.4 Dialogue (3 items)

- [ ] `src/quest/questDatabase.cpp:423-427`: Generic error scene hardcoded as fallback when a requested scene ID is missing (`speakerName = "System"`, `bodyText = "Error: Scene " + id + " not found."`). `[Missing Logic / Stub]`
- [ ] `src/state/sexState.cpp:349`: `sexState::update` contains `(void)gameContext;` empty stub. `[Missing Logic]`
- [ ] `src/state/sexState.cpp:55-150`: Over 100 hardcoded string replacement rules for verb and pronoun conjugation embedded directly in C++. `[Hardcoded Mock Data]`

### 2.5 UI Adapter (5 items)

- [ ] `src/ui/widgets/radarWidget.cpp:293`: `renderWidgetOptionsToolbar(...)` returns dummy literal `return 0.0f;`. `[Missing Logic / Stub]`
- [ ] `src/ui/actionGridManager.cpp:381`: `"Select Item to Trade"` button binds an empty dummy lambda callback `[gameContext]() {}`. `[Missing Logic / Stub]`
- [ ] `src/ui/widgets/paperdollWidgets.cpp:520`: Inner subtle frame and centered `"No Image"` text rendered as placeholder when item texture is null. `[Missing Logic / Stub]`
- [ ] `src/ui/widgets/radarWidget.cpp:293`: `renderWidgetOptionsToolbar` is unreferenced in layout rendering; options toolbar never displays widgets. `[Unwired State]`
- [ ] `src/ui/theme.cpp:136-146`: Theme loader hardcodes theme mappings for `theme_dark_fantasy`, `theme_cyber_neon`, `theme_parchment` directly, bypassing directory discovery for custom user themes in `data/themes/`. `[Unwired State]`
