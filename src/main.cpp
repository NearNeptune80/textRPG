#include <iostream>
#include <filesystem>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "core/game.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/loadGameState.h"
#include "state/characterCreationState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/transformationState.h"
#include "save/saveManager.h"
#include "ui/theme.h"
#include "ui/uiRenderer.h"

int main(int argc, char* argv[])
{
    // Check for --screenshot <state_name> [output_path]
    std::string screenshotState = "";
    std::string screenshotOut = "";
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc)
        {
            screenshotState = argv[i + 1];
            if (i + 2 < argc && argv[i + 2][0] != '-')
            {
                screenshotOut = argv[i + 2];
            }
            break;
        }
    }

    // 1. Initialise SDL Video Subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "[SDL] Failed to initialise video: " << SDL_GetError() << "\n";
        return -1;
    }

    const int windowWidth = 1280;
    const int windowHeight = 720;

    SDL_WindowFlags winFlags = SDL_WINDOW_RESIZABLE;
    if (!screenshotState.empty())
    {
        winFlags = SDL_WINDOW_HIDDEN;
    }

    // 2. Create Window and Renderer Context
    SDL_Window* window = SDL_CreateWindow("TextRPG Engine", windowWidth, windowHeight, winFlags);
    if (!window)
    {
        std::cerr << "[SDL] Failed to create window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        std::cerr << "[SDL] Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);

    // 3. Initialise UI Theme (Attempt Dark Fantasy default, fallback to theme.json)
    if (!Theme::loadFromFile("data/themes/theme_dark_fantasy.json"))
    {
        Theme::loadFromFile("data/theme.json");
    }

    // 4. Initialise Headless Engine
    game engine;
    engine.init();

    // 5. Initialise UI Presentation Renderer
    uiRenderer view;

    // Handle Screenshot Mode
    if (!screenshotState.empty())
    {
        if (screenshotState == "main_menu" || screenshotState == "menu")
        {
            engine.changeState(std::make_unique<mainMenuState>());
        }
        else if (screenshotState == "options" || screenshotState == "options_general")
        {
            engine.changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>()));
        }
        else if (screenshotState == "options_keybinds" || screenshotState == "keybinds")
        {
            auto opt = std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>());
            opt->isKeybindsOpen = true;
            engine.changeState(std::move(opt));
        }
        else if (screenshotState.starts_with("options_content"))
        {
            auto opt = std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<mainMenuState>());
            int catIdx = 0;
            if (screenshotState.length() > 16) catIdx = screenshotState[16] - '0';
            if (catIdx >= 0 && catIdx <= 8) opt->contentCategory = static_cast<ContentOptionsCategory>(catIdx);
            engine.changeState(std::move(opt));
        }
        else if (screenshotState == "save_load")
        {
            engine.changeState(std::make_unique<loadGameState>(SaveMenuMode::SAVE_AND_LOAD, std::make_unique<mainMenuState>()));
        }
        else if (screenshotState == "cc_salon")
        {
            auto cc = std::make_unique<characterCreationState>(EditorConfig::hairSalonPreset(), 0);
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "cc_tattoo")
        {
            auto cc = std::make_unique<characterCreationState>(EditorConfig::tattooPiercingPreset(), 0);
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "cc_transform" || screenshotState.starts_with("cc_transform_step"))
        {
            int tStep = 0;
            if (screenshotState == "cc_transform") tStep = 5; // Appendages tab
            else if (screenshotState.length() > 17) tStep = screenshotState[17] - '0';
            auto cc = std::make_unique<characterCreationState>(EditorConfig::fullTransformationPreset(), tStep);
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "character_creation" || screenshotState == "cc" || screenshotState == "cc_wardrobe" || screenshotState.starts_with("cc_step") || screenshotState.starts_with("character_creation_step"))
        {
            auto cc = std::make_unique<characterCreationState>(0);
            if (screenshotState == "cc_step1" || screenshotState == "character_creation_step1") cc->step = 1;
            else if (screenshotState == "cc_step2" || screenshotState == "character_creation_step2") cc->step = 2;
            else if (screenshotState == "cc_step3" || screenshotState == "character_creation_step3" || screenshotState == "cc_wardrobe") cc->step = 3;
            else if (screenshotState == "cc_step4" || screenshotState == "character_creation_step4") cc->step = 4;
            else if (screenshotState == "cc_step5" || screenshotState == "character_creation_step5") cc->step = 5;
            else cc->step = 0;
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "transformation" || screenshotState.starts_with("tf_") || screenshotState == "transformation_inspect")
        {
            if (!engine.getPlayer())
            {
                auto p = std::make_shared<entity>("hero_tf", "Rudy");
                p->genderArchetype = GenderArchetype::MALE;
                engine.playerEntity = p;
            }

            TransformationTab tab = TransformationTab::CORE;
            if (screenshotState == "tf_eyes") tab = TransformationTab::EYES;
            else if (screenshotState == "tf_hair") tab = TransformationTab::HAIR;
            else if (screenshotState == "tf_head" || screenshotState == "tf_face") tab = TransformationTab::HEAD_FACE;
            else if (screenshotState == "tf_ass") tab = TransformationTab::ASS_HIPS;
            else if (screenshotState == "tf_breasts") tab = TransformationTab::BREASTS;
            else if (screenshotState == "tf_vagina") tab = TransformationTab::VAGINA;
            else if (screenshotState == "tf_penis") tab = TransformationTab::PENIS;
            else if (screenshotState == "tf_crotch") tab = TransformationTab::CROTCH_BOOBS;
            else if (screenshotState == "tf_appendages") tab = TransformationTab::APPENDAGES;
            else if (screenshotState == "tf_inspect" || screenshotState == "transformation_inspect") tab = TransformationTab::INSPECT_PRESETS;

            auto tfState = std::make_unique<transformationState>(tab);
            tfState->resetToHuman(&engine);
            engine.changeState(std::move(tfState));
        }
        else if (screenshotState.starts_with("load_"))
        {
            std::string saveFile = screenshotState.substr(5);
            saveManager::loadFromFile(&engine, saveFile);
            engine.changeState(std::make_unique<explorationState>());
        }
        else if (screenshotState.starts_with("exploration"))
        {
            if (!engine.getPlayer())
            {
                auto p = std::make_shared<entity>("hero_player", "Aria Vesper");
                p->genderArchetype = GenderArchetype::FEMALE;
                p->stats.setBaseStat("health", 100.0f);
                p->stats.setBaseStat("max_health", 100.0f);
                p->stats.setBaseStat("mana", 80.0f);
                p->stats.setBaseStat("max_mana", 80.0f);
                p->stats.setBaseStat("lust", 15.0f);
                p->stats.setBaseStat("arousal", 10.0f);
                p->stats.setBaseStat("currency", 250.0f);
                engine.playerEntity = p;
            }
            if (screenshotState == "exploration_companion" || screenshotState == "exploration_status")
            {
                auto lilaya = std::make_shared<entity>("companion_lilaya", "Lilaya");
                lilaya->stats.level = 5;
                lilaya->stats.setBaseStat("health", 120.0f);
                lilaya->stats.setBaseStat("max_health", 120.0f);
                lilaya->stats.setBaseStat("lust", 25.0f);
                engine.addCompanion(lilaya);
            }
            if (screenshotState == "exploration_status")
            {
                if (engine.getPlayer())
                {
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_str", "Strength Buff", "Increases physical damage by 15%", 10, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_arc", "Arcane Focus", "Mana regen increased by 20%", 10, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "debuff_pois", "Poisoned", "Taking 5 nature damage per turn", 5, true });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_haste", "Haste", "Action speed doubled", 3, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_shield", "Arcane Ward", "Absorbs up to 50 damage", 8, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "debuff_lust", "Aphrodisiac", "Arousal increases over time", 6, true });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_regen", "Regeneration", "Restores 10 HP every turn", 12, false });
                }
            }
            engine.loadMap("house_01", 1, 1);
            engine.changeState(std::make_unique<explorationState>());
        }
        else if (screenshotState.starts_with("inventory"))
        {
            if (!engine.getPlayer())
            {
                auto p = std::make_shared<entity>("hero_player", "Aria Vesper");
                p->genderArchetype = GenderArchetype::FEMALE;
                p->stats.setBaseStat("health", 100.0f);
                p->stats.setBaseStat("max_health", 100.0f);
                p->stats.setBaseStat("mana", 80.0f);
                p->stats.setBaseStat("max_mana", 80.0f);
                p->stats.setBaseStat("currency", 5000.0f);
                
                auto pot = std::make_shared<item>();
                pot->id = "item_potion_health";
                pot->name = "Health Potion";
                pot->description = "Restores 50 HP with revitalizing arcane herbs.";
                pot->baseValue = 50;
                pot->isConsumable = true;
                pot->count = 3;
                p->inventory.addItem(pot);

                auto blouse = std::make_shared<item>();
                blouse->id = "item_shirt_cotton";
                blouse->name = "Cotton Blouse";
                blouse->description = "A comfortable everyday buttoned cotton blouse.";
                blouse->baseValue = 120;
                blouse->isEquippable = true;
                blouse->targetSlot = equipSlot::TORSO_UNDER;
                blouse->supportedDisplacements[DisplacementMode::UNBUTTON] = { bodySlot::TORSO, bodySlot::BREASTS };
                blouse->supportedDisplacements[DisplacementMode::LIFT_UP] = { bodySlot::STOMACH, bodySlot::BREASTS };
                p->inventory.addItem(blouse);

                auto skirt = std::make_shared<item>();
                skirt->id = "item_skirt_pleated";
                skirt->name = "Pleated Skirt";
                skirt->description = "A stylish pleated skirt that rests neatly on the hips.";
                skirt->baseValue = 150;
                skirt->isEquippable = true;
                skirt->targetSlot = equipSlot::LEGS_OUTER;
                skirt->supportedDisplacements[DisplacementMode::LIFT_UP] = { bodySlot::GROIN, bodySlot::ASS };
                p->inventory.addItem(skirt);

                auto dagger = std::make_shared<item>();
                dagger->id = "item_dagger_iron";
                dagger->name = "Iron Dagger";
                dagger->description = "A sharp utility dagger for close combat.";
                dagger->baseValue = 200;
                dagger->isEquippable = true;
                dagger->targetSlot = equipSlot::WEAPON_MAIN;
                p->inventory.addItem(dagger);

                p->inventory.equipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = blouse;
                p->inventory.equipped[static_cast<size_t>(equipSlot::LEGS_OUTER)] = skirt;

                engine.playerEntity = p;
            }
            engine.changeState(std::make_unique<inventoryState>());
            if (screenshotState == "inventory_slot")
            {
                engine.selectedEquipmentSlot = equipSlot::TORSO_UNDER;
                engine.selectedInventoryIndex = -1;
            }
            else if (screenshotState == "inventory_item")
            {
                engine.selectedInventorySide = 0;
                engine.selectedInventoryIndex = 1;
                engine.selectedEquipmentSlot = equipSlot::NONE;
            }
            else if (screenshotState == "inventory_companion" || screenshotState == "inventory_status")
            {
                auto lilaya = std::make_shared<entity>("companion_lilaya", "Lilaya");
                lilaya->stats.level = 5;
                lilaya->stats.setBaseStat("health", 120.0f);
                lilaya->stats.setBaseStat("max_health", 120.0f);
                lilaya->stats.setBaseStat("lust", 25.0f);
                engine.addCompanion(lilaya);
                engine.selectedEquipmentSlot = equipSlot::TORSO_UNDER;

                if (screenshotState == "inventory_status" && engine.getPlayer())
                {
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_str", "Strength Buff", "Increases physical damage by 15%", 10, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_arc", "Arcane Focus", "Mana regen increased by 20%", 10, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "debuff_pois", "Poisoned", "Taking 5 nature damage per turn", 5, true });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_haste", "Haste", "Action speed doubled", 3, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_shield", "Arcane Ward", "Absorbs up to 50 damage", 8, false });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "debuff_lust", "Aphrodisiac", "Arousal increases over time", 6, true });
                    engine.getPlayer()->addStatusEffect(StatusEffect{ "buff_regen", "Regeneration", "Restores 10 HP every turn", 12, false });
                }
            }
        }

        engine.refreshActionGrid();

        // Render multiple frames to stabilize fonts/layout
        for (int f = 0; f < 3; ++f)
        {
            engine.update(0.016f);
            view.render(renderer, &engine);
        }

        if (screenshotOut.empty())
        {
            std::filesystem::create_directories("/home/jackd/.gemini/antigravity/brain/0ae033e5-0614-491c-8eb5-fc151a7bb89a/screenshots");
            screenshotOut = "/home/jackd/.gemini/antigravity/brain/0ae033e5-0614-491c-8eb5-fc151a7bb89a/screenshots/" + screenshotState + ".png";
        }

        SDL_Surface* surface = SDL_RenderReadPixels(renderer, NULL);
        if (surface)
        {
            IMG_SavePNG(surface, screenshotOut.c_str());
            SDL_DestroySurface(surface);
            std::cout << "[Screenshot] Successfully captured " << screenshotState << " -> " << screenshotOut << "\n";
        }
        else
        {
            std::cerr << "[Screenshot] Failed to capture surface: " << SDL_GetError() << "\n";
        }

        engine.clean();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    uint64_t lastTime = SDL_GetTicks();

    // 6. Main Decoupled Loop
    while (engine.isRunning)
    {
        uint64_t currentTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Step A: Input Processing
        engine.handleEvents();

        // Step B: Headless Simulation Update
        engine.update(deltaTime);

        // Step C: Isolated UI View Render
        view.render(renderer, &engine);
    }

    // 7. Cleanup & Exit
    engine.clean();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}