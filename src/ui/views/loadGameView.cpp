#include "ui/views/loadGameView.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "save/saveManager.h"
#include "state/loadGameState.h"
#include "state/explorationState.h"
#include "entities/entity.h"
#include <format>
#include <vector>
#include <string>
#include <algorithm>

namespace LoadGameView
{
    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        loadGameState* loadState = dynamic_cast<loadGameState*>(gameContext->getActiveState());
        float startY = curY;
        float centerX = rect.x + (rect.w / 2.0f);
        float padX = rect.x + (24.0f * uiScale);
        float availableW = rect.w - (48.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        bool isSaveMode = (loadState && loadState->getMode() == SaveMenuMode::SAVE_AND_LOAD);
        entity* player = gameContext->getPlayer();
        std::string activeCharName = (player && !player->name.empty()) ? player->name : "Hero";

        // 1. Centered Header Card
        float cardW = std::min(availableW, 400.0f * uiScale);
        float cardH = 34.0f * uiScale;
        UIWidget::drawCenteredHeaderCard(renderer, centerX, curY, cardW, cardH, "Save game files", Theme::colors.textPrimary, uiScale);
        curY += cardH + (18.0f * uiScale);

        // 2. Centered "Please Note:"
        std::string noteTitle = "Please Note:";
        float noteTitleW = noteTitle.size() * (7.5f * uiScale);
        UIWidget::drawText(renderer, noteTitle, centerX - (noteTitleW / 2.0f), curY, Theme::colors.textPrimary, uiScale * 0.95f);
        curY += (22.0f * uiScale);

        float textW = std::min(availableW, 680.0f * uiScale);
        float textX = centerX - (textW / 2.0f);

        static const char* notes[] = {
            "1. Only standard characters (letters and numbers) will work for save file names.",
            "2. The 'AutoSave' file is automatically overwritten every time you move between maps.",
            "3. The 'QuickSave' file is automatically overwritten every time you quick save (binding is F5).",
            "4. You cannot save during scenes which restrict your movement, including combat and sex."
        };

        for (int i = 0; i < 4; ++i)
        {
            UIWidget::drawText(renderer, notes[i], textX, curY, Theme::colors.textSecondary, uiScale * 0.86f);
            curY += (18.0f * uiScale);
        }
        curY += (14.0f * uiScale);

        // Confirmation Modals (Overwrite / Delete)
        if (loadState && !loadState->pendingOverwriteSaveName.empty())
        {
            SDL_FRect modalRect = { padX, curY, availableW, 60.0f * uiScale };
            UIWidget::drawPanel(renderer, modalRect, Theme::colors.bgHeader, Theme::colors.enemy);

            std::string warnMsg = std::format("! Overwrite Save: '{}' already exists for {}. Overwrite this file?", loadState->pendingOverwriteSaveName, activeCharName);
            UIWidget::drawText(renderer, warnMsg, padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

            float btnW = 120.0f * uiScale;
            float btnH = 24.0f * uiScale;
            SDL_FRect yesBtnRect = { padX + availableW - (btnW * 2.0f) - (20.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };
            SDL_FRect cancelBtnRect = { padX + availableW - btnW - (10.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };

            bool yesHovered = (mousePos.x >= yesBtnRect.x && mousePos.x <= yesBtnRect.x + yesBtnRect.w &&
                               mousePos.y >= yesBtnRect.y && mousePos.y <= yesBtnRect.y + yesBtnRect.h);
            bool cancelHovered = (mousePos.x >= cancelBtnRect.x && mousePos.x <= cancelBtnRect.x + cancelBtnRect.w &&
                                  mousePos.y >= cancelBtnRect.y && mousePos.y <= cancelBtnRect.y + cancelBtnRect.h);

            UIWidget::drawButton(renderer, yesBtnRect, "YES, OVERWRITE", yesHovered, true, false, uiScale * 0.75f);
            UIWidget::drawButton(renderer, cancelBtnRect, "CANCEL", cancelHovered, true, false, uiScale * 0.75f);

            if (yesHovered && clicked)
            {
                saveManager::saveNamedGame(gameContext, loadState->pendingOverwriteSaveName);
                loadState->pendingOverwriteSaveName = "";
                gameContext->input.consumeMouseClick();
            }
            else if (cancelHovered && clicked)
            {
                loadState->pendingOverwriteSaveName = "";
                gameContext->input.consumeMouseClick();
            }

            curY += modalRect.h + (14.0f * uiScale);
        }

        if (loadState && !loadState->pendingDeleteFileName.empty())
        {
            SDL_FRect modalRect = { padX, curY, availableW, 60.0f * uiScale };
            UIWidget::drawPanel(renderer, modalRect, Theme::colors.bgHeader, Theme::colors.enemy);

            std::string warnMsg = std::format("! Delete Save: Are you sure you want to permanently delete '{}'?", loadState->pendingDeleteFileName);
            UIWidget::drawText(renderer, warnMsg, padX + (12.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.enemy, uiScale * 0.85f);

            float btnW = 120.0f * uiScale;
            float btnH = 24.0f * uiScale;
            SDL_FRect yesBtnRect = { padX + availableW - (btnW * 2.0f) - (20.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };
            SDL_FRect cancelBtnRect = { padX + availableW - btnW - (10.0f * uiScale), curY + (28.0f * uiScale), btnW, btnH };

            bool yesHovered = (mousePos.x >= yesBtnRect.x && mousePos.x <= yesBtnRect.x + yesBtnRect.w &&
                               mousePos.y >= yesBtnRect.y && mousePos.y <= yesBtnRect.y + yesBtnRect.h);
            bool cancelHovered = (mousePos.x >= cancelBtnRect.x && mousePos.x <= cancelBtnRect.x + cancelBtnRect.w &&
                                  mousePos.y >= cancelBtnRect.y && mousePos.y <= cancelBtnRect.y + cancelBtnRect.h);

            UIWidget::drawButton(renderer, yesBtnRect, "YES, DELETE", yesHovered, true, false, uiScale * 0.75f);
            UIWidget::drawButton(renderer, cancelBtnRect, "CANCEL", cancelHovered, true, false, uiScale * 0.75f);

            if (yesHovered && clicked)
            {
                saveManager::deleteSave(loadState->pendingDeleteFileName);
                loadState->pendingDeleteFileName = "";
                gameContext->input.consumeMouseClick();
            }
            else if (cancelHovered && clicked)
            {
                loadState->pendingDeleteFileName = "";
                gameContext->input.consumeMouseClick();
            }

            curY += modalRect.h + (14.0f * uiScale);
        }

        // New Save Input Row (when in Save mode or Player entity active)
        if (isSaveMode || player != nullptr)
        {
            float inputRowH = 32.0f * uiScale;
            SDL_FRect inputRowRect = { padX, curY, availableW, inputRowH };
            UIWidget::drawPanel(renderer, inputRowRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            UIWidget::drawText(renderer, "New save name:", padX + (10.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.88f);

            float saveBtnW = 100.0f * uiScale;
            float btnH = 24.0f * uiScale;
            SDL_FRect saveBtnRect = { padX + availableW - saveBtnW - (6.0f * uiScale), curY + (4.0f * uiScale), saveBtnW, btnH };
            bool saveHovered = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                                mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

            float nameBoxX = padX + (140.0f * uiScale);
            float nameBoxW = availableW - (150.0f * uiScale) - saveBtnW - (12.0f * uiScale);
            SDL_FRect nameBoxRect = { nameBoxX, curY + (4.0f * uiScale), nameBoxW, btnH };
            bool boxHovered = (mousePos.x >= nameBoxRect.x && mousePos.x <= nameBoxRect.x + nameBoxRect.w &&
                               mousePos.y >= nameBoxRect.y && mousePos.y <= nameBoxRect.y + nameBoxRect.h);

            SDL_Color boxBorder = (loadState && loadState->isEditingSaveName) ? Theme::colors.textGold : (boxHovered ? Theme::colors.borderSelected : Theme::colors.borderButton);
            UIWidget::drawPanel(renderer, nameBoxRect, Theme::colors.bgPanel, boxBorder);

            std::string dispInput = loadState ? loadState->newSaveNameInput : "Manual_Save";
            if (loadState && loadState->isEditingSaveName) dispInput += "_";
            UIWidget::drawText(renderer, dispInput, nameBoxX + (8.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);

            if (boxHovered && clicked && loadState)
            {
                loadState->isEditingSaveName = true;
                gameContext->input.consumeMouseClick();
            }

            UIWidget::drawButton(renderer, saveBtnRect, "Save Game", saveHovered, true, false, uiScale * 0.78f);
            if (saveHovered && clicked && loadState)
            {
                saveManager::saveNamedGame(gameContext, loadState->newSaveNameInput);
                loadState->isEditingSaveName = false;
                gameContext->input.consumeMouseClick();
            }

            curY += inputRowH + (12.0f * uiScale);
        }

        // 3. Table Column Headers: Time | Name | Save | Load | Delete
        float tableX = padX;
        float timeColW = 160.0f * uiScale;
        float actionsColX = padX + availableW - (190.0f * uiScale);

        UIWidget::drawText(renderer, "Time", tableX + (10.0f * uiScale), curY, Theme::colors.textSecondary, uiScale * 0.9f);
        UIWidget::drawText(renderer, "Name", tableX + timeColW, curY, Theme::colors.textSecondary, uiScale * 0.9f);
        UIWidget::drawText(renderer, "Save | Load | Delete", actionsColX, curY, Theme::colors.textSecondary, uiScale * 0.9f);
        curY += (24.0f * uiScale);

        // 4. Character Groups & Saves
        auto characterGroups = saveManager::getSavesGroupedByCharacter();

        if (loadState && loadState->sortMode == 1)
        {
            // Alphabetical sort
            std::sort(characterGroups.begin(), characterGroups.end(), [](const CharacterSaveGroup& a, const CharacterSaveGroup& b) {
                return a.characterName < b.characterName;
            });
            for (auto& grp : characterGroups)
            {
                std::sort(grp.saves.begin(), grp.saves.end(), [](const SaveMetaData& a, const SaveMetaData& b) {
                    return a.saveName < b.saveName;
                });
            }
        }

        if (characterGroups.empty())
        {
            UIWidget::drawText(renderer, "No save game files found.", textX, curY, Theme::colors.textMuted, uiScale * 0.88f);
            curY += (24.0f * uiScale);
        }
        else
        {
            for (const auto& group : characterGroups)
            {
                bool isCollapsed = loadState ? loadState->isCharacterCollapsed(group.characterName) : false;

                // Character Header Accordion
                SDL_FRect groupHeader = { padX, curY, availableW, 26.0f * uiScale };
                bool groupHeaderHovered = (mousePos.x >= groupHeader.x && mousePos.x <= groupHeader.x + groupHeader.w &&
                                           mousePos.y >= groupHeader.y && mousePos.y <= groupHeader.y + groupHeader.h);

                UIWidget::drawPanel(renderer, groupHeader, Theme::colors.bgHeader, groupHeaderHovered ? Theme::colors.borderSelected : Theme::colors.borderButton);

                std::string arrow = isCollapsed ? "[ + ]" : "[ - ]";
                std::string headerText = std::format("{} Character: {} ({} saves)", arrow, group.characterName, group.saves.size());
                UIWidget::drawText(renderer, headerText, padX + (10.0f * uiScale), curY + (5.0f * uiScale), groupHeaderHovered ? Theme::colors.textPrimary : Theme::colors.textGold, uiScale * 0.85f);

                if (groupHeaderHovered && clicked && loadState)
                {
                    loadState->toggleCharacterCollapsed(group.characterName);
                    gameContext->input.consumeMouseClick();
                }

                curY += groupHeader.h + (6.0f * uiScale);

                // Expanded saves
                if (!isCollapsed)
                {
                    for (const auto& save : group.saves)
                    {
                        SDL_FRect itemRect = { padX, curY, availableW, 28.0f * uiScale };
                        UIWidget::drawPanel(renderer, itemRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                        // Time column
                        std::string timeStr = save.timestamp.empty() ? "2026-08-29" : save.timestamp;
                        UIWidget::drawText(renderer, timeStr, padX + (8.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.8f);

                        // Name column
                        std::string displayName = save.saveName.empty() ? save.fileName : save.saveName;
                        UIWidget::drawText(renderer, displayName, padX + timeColW, curY + (5.0f * uiScale), save.isAutosave ? Theme::colors.arcane : Theme::colors.textPrimary, uiScale * 0.82f);

                        // Action buttons: Save | Load | Delete
                        float rightOffset = 6.0f * uiScale;

                        // Delete button
                        float delBtnW = 55.0f * uiScale;
                        float btnH = 20.0f * uiScale;
                        SDL_FRect delBtnRect = { padX + availableW - rightOffset - delBtnW, curY + (4.0f * uiScale), delBtnW, btnH };
                        bool delHovered = (mousePos.x >= delBtnRect.x && mousePos.x <= delBtnRect.x + delBtnRect.w &&
                                           mousePos.y >= delBtnRect.y && mousePos.y <= delBtnRect.y + delBtnRect.h);

                        UIWidget::drawButton(renderer, delBtnRect, "Delete", delHovered, true, false, uiScale * 0.72f);
                        if (delHovered && clicked && loadState)
                        {
                            if (loadState->confirmationsEnabled)
                            {
                                loadState->pendingDeleteFileName = save.fileName;
                            }
                            else
                            {
                                saveManager::deleteSave(save.fileName);
                            }
                            gameContext->input.consumeMouseClick();
                        }
                        rightOffset += delBtnW + (6.0f * uiScale);

                        // Load button
                        float loadBtnW = 55.0f * uiScale;
                        SDL_FRect loadBtnRect = { padX + availableW - rightOffset - loadBtnW, curY + (4.0f * uiScale), loadBtnW, btnH };
                        bool loadHovered = (mousePos.x >= loadBtnRect.x && mousePos.x <= loadBtnRect.x + loadBtnRect.w &&
                                            mousePos.y >= loadBtnRect.y && mousePos.y <= loadBtnRect.y + loadBtnRect.h);

                        UIWidget::drawButton(renderer, loadBtnRect, "Load", loadHovered, true, false, uiScale * 0.72f);
                        if (loadHovered && clicked)
                        {
                            std::string fileToLoad = save.fileName;
                            gameContext->input.consumeMouseClick();
                            if (saveManager::loadFromFile(gameContext, fileToLoad))
                            {
                                gameContext->changeState(std::make_unique<explorationState>());
                                return (curY - startY);
                            }
                        }
                        rightOffset += loadBtnW + (6.0f * uiScale);

                        // Save button
                        if ((isSaveMode || player) && group.characterName == activeCharName && !save.isAutosave)
                        {
                            float saveBtnW = 55.0f * uiScale;
                            SDL_FRect saveBtnRect = { padX + availableW - rightOffset - saveBtnW, curY + (4.0f * uiScale), saveBtnW, btnH };
                            bool saveHovered = (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                                                mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

                            UIWidget::drawButton(renderer, saveBtnRect, "Save", saveHovered, true, false, uiScale * 0.72f);
                            if (saveHovered && clicked && loadState)
                            {
                                if (loadState->confirmationsEnabled)
                                {
                                    loadState->pendingOverwriteSaveName = save.saveName.empty() ? save.fileName : save.saveName;
                                }
                                else
                                {
                                    saveManager::saveNamedGame(gameContext, save.saveName.empty() ? save.fileName : save.saveName);
                                }
                                gameContext->input.consumeMouseClick();
                            }
                        }

                        curY += itemRect.h + (4.0f * uiScale);
                    }
                }

                curY += (8.0f * uiScale);
            }
        }

        return (curY - startY);
    }
}
