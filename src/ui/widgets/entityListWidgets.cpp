#include "ui/widgets/entityListWidgets.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"

namespace EntityListWidgets
{
    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 1: Zone & Environment Status Card
        // ==========================================
        float card1H = 44.0f * uiScale;
        SDL_FRect card1Rect = { padX, curY, availableW, card1H };
        UIWidget::drawPanel(renderer, card1Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        std::string locName = "Sanctuary Manor F1";
        if (const gameMap* m = gameContext->getActiveMap())
        {
            if (!m->getName().empty() && m->getName() != "District Map") locName = m->getName();
        }

        UIWidget::drawText(renderer, locName, innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);
        std::string safeTag = "[ Safe ]";
        float safeW = UIWidget::getTextWidth(safeTag, uiScale * 0.68f);
        UIWidget::drawText(renderer, safeTag, innerX + cW - safeW, curY + (5.0f * uiScale), Theme::colors.companion, uiScale * 0.68f);

        UIWidget::drawText(renderer, "Sanctuary interior", innerX, curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.66f);

        TooltipManager::setHoverTooltip(card1Rect, mousePos, locName,
                                        "Current environment location and zone safety rating.", safeTag);

        curY += card1H + (8.0f * uiScale);

        // ==========================================
        // CARD 2: Characters Present Card
        // ==========================================
        bool hasNpc = false;
        std::shared_ptr<entity> npcShared = nullptr;
        entity* npc = nullptr;
        if (gameContext->map)
        {
            auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            if (!tileData.namedNPCs.empty())
            {
                hasNpc = true;
                npcShared = tileData.namedNPCs.front();
                npc = npcShared.get();
            }
            else if (tileData.persistentNPC && tileData.persistentNPC != tileData.ambushState.npc)
            {
                hasNpc = true;
                npcShared = tileData.persistentNPC;
                npc = npcShared.get();
            }
        }

        float card2H = hasNpc ? (64.0f * uiScale) : (48.0f * uiScale);
        SDL_FRect card2Rect = { padX, curY, availableW, card2H };
        UIWidget::drawPanel(renderer, card2Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "CHARACTERS PRESENT", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        if (hasNpc && npc)
        {
            float npcY = curY + (22.0f * uiScale);
            UIWidget::drawText(renderer, npc->name, innerX, npcY, Theme::colors.textGold, uiScale * 0.78f);
            std::string raceStr = npc->anatomy.getRacialTitle().empty() ? "Demon" : npc->anatomy.getRacialTitle();
            UIWidget::drawText(renderer, raceStr, innerX, npcY + (14.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.66f);

            // Action buttons: [ Talk ] [ View ]
            float btnW = 38.0f * uiScale;
            float btnH = 20.0f * uiScale;
            SDL_FRect talkBtn = { innerX + cW - (btnW * 2.0f) - (4.0f * uiScale), npcY + (2.0f * uiScale), btnW, btnH };
            bool tHov = (mousePos.x >= talkBtn.x && mousePos.x <= talkBtn.x + talkBtn.w &&
                         mousePos.y >= talkBtn.y && mousePos.y <= talkBtn.y + talkBtn.h);
            UIWidget::drawButton(renderer, talkBtn, "Talk", tHov, true, false, uiScale * 0.66f);
            TooltipManager::setHoverTooltip(talkBtn, mousePos, "Talk to " + npc->name, "Initiate dialogue conversation with this character.", "Dialogue");

            if (tHov && clicked)
            {
                gameContext->activeTargetNPC = npcShared;
                gameContext->activeTargetMode = TargetMode::DIALOGUE;
                gameContext->input.consumeMouseClick();
            }

            SDL_FRect inspBtn = { innerX + cW - btnW, npcY + (2.0f * uiScale), btnW, btnH };
            bool iHov = (mousePos.x >= inspBtn.x && mousePos.x <= inspBtn.x + inspBtn.w &&
                         mousePos.y >= inspBtn.y && mousePos.y <= inspBtn.y + inspBtn.h);
            UIWidget::drawButton(renderer, inspBtn, "View", iHov, true, false, uiScale * 0.66f);
            TooltipManager::setHoverTooltip(inspBtn, mousePos, "Inspect " + npc->name, "View character paperdoll, equipment, and appearance.", "Inspector");

            if (iHov && clicked)
            {
                gameContext->activeTargetNPC = npcShared;
                gameContext->activeTargetMode = TargetMode::DIALOGUE;
                gameContext->input.consumeMouseClick();
            }
        }
        else
        {
            UIWidget::drawText(renderer, "No characters here.", innerX, curY + (22.0f * uiScale), Theme::colors.textMuted, uiScale * 0.68f);
        }

        curY += card2H + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        auto ground = gameContext->getTileInventoryStacked();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 3: Items on Ground Card
        // ==========================================
        float card3H = ground.empty() ? (48.0f * uiScale) : ((28.0f * uiScale) + (ground.size() * (26.0f * uiScale)));
        card3H = std::min(card3H, 130.0f * uiScale);

        SDL_FRect card3Rect = { padX, curY, availableW, card3H };
        UIWidget::drawPanel(renderer, card3Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "ITEMS PRESENT", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        if (ground.empty())
        {
            UIWidget::drawText(renderer, "No items dropped.", innerX, curY + (22.0f * uiScale), Theme::colors.textMuted, uiScale * 0.68f);
        }
        else
        {
            float itemY = curY + (22.0f * uiScale);
            for (size_t i = 0; i < ground.size() && i < 3; ++i)
            {
                if (ground[i].itemPtr)
                {
                    std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                    UIWidget::drawText(renderer, line, innerX, itemY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.70f);

                    float pickBtnW = 38.0f * uiScale;
                    float pickBtnH = 18.0f * uiScale;
                    SDL_FRect pickBtn = { innerX + cW - pickBtnW, itemY + (2.0f * uiScale), pickBtnW, pickBtnH };
                    bool pHov = (mousePos.x >= pickBtn.x && mousePos.x <= pickBtn.x + pickBtn.w &&
                                 mousePos.y >= pickBtn.y && mousePos.y <= pickBtn.y + pickBtn.h);
                    UIWidget::drawButton(renderer, pickBtn, "Take", pHov, true, false, uiScale * 0.64f);

                    std::string sub = std::format("Ground Loot • Value: {} ¤", ground[i].itemPtr->baseValue);
                    std::string hk = std::format("x{}", ground[i].totalCount);
                    TooltipManager::setHoverTooltip(pickBtn, mousePos, "Take " + ground[i].itemPtr->name,
                                                    std::format("Transfer 1x {} from the ground into your backpack.", ground[i].itemPtr->name),
                                                    sub, hk);

                    if (pHov && clicked)
                    {
                        gameContext->handlePickupAction(static_cast<int>(i), 1);
                        gameContext->input.consumeMouseClick();
                        break;
                    }

                    itemY += (24.0f * uiScale);
                }
            }
        }

        curY += card3H + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (5.0f * uiScale);
        float availableW = innerW - (10.0f * uiScale);
        float innerPad = 5.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 4: Activity & Event Log Card
        // ==========================================
        float card4H = 145.0f * uiScale;
        SDL_FRect logBox = { padX, curY, availableW, card4H };
        UIWidget::drawPanel(renderer, logBox, Theme::colors.bgSlot, Theme::colors.borderNormal);

        const auto& allLogs = gameContext->getEventLog();
        int totalLogs = static_cast<int>(allLogs.size());
        int maxVisible = 6;
        int maxOffset = std::max(0, totalLogs - maxVisible);

        static int s_eventLogScrollOffset = 0;
        static size_t s_lastLogCount = 0;
        if (allLogs.size() > s_lastLogCount)
        {
            s_eventLogScrollOffset = 0; // Automatically snap to newest entries on top
            s_lastLogCount = allLogs.size();
        }
        s_eventLogScrollOffset = std::clamp(s_eventLogScrollOffset, 0, maxOffset);

        bool logBoxHovered = (mousePos.x >= logBox.x && mousePos.x <= logBox.x + logBox.w &&
                              mousePos.y >= logBox.y && mousePos.y <= logBox.y + logBox.h);

        // Mouse wheel scrolling
        if (logBoxHovered && maxOffset > 0)
        {
            float wheelY = gameContext->input.getMouseWheelY();
            if (wheelY != 0.0f)
            {
                int step = static_cast<int>(std::round(wheelY));
                if (step == 0) step = (wheelY > 0.0f) ? 1 : -1;
                s_eventLogScrollOffset = std::clamp(s_eventLogScrollOffset - step, 0, maxOffset);
                gameContext->input.consumeMouseWheel();
            }
        }

        // Header with title and scroll arrow buttons
        UIWidget::drawText(renderer, "ACTIVITY & EVENT LOG", innerX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.74f);

        if (totalLogs > maxVisible)
        {
            float btnW = 16.0f * uiScale;
            float btnH = 14.0f * uiScale;
            float btnGap = 2.0f * uiScale;
            float btnY = curY + (4.0f * uiScale);
            float btnDownX = padX + availableW - innerPad - btnW;
            float btnUpX = btnDownX - btnW - btnGap;

            SDL_FRect upRect = { btnUpX, btnY, btnW, btnH };
            bool upHov = (mousePos.x >= upRect.x && mousePos.x <= upRect.x + upRect.w &&
                          mousePos.y >= upRect.y && mousePos.y <= upRect.y + upRect.h);
            bool canUp = (s_eventLogScrollOffset > 0);
            if (upHov && clicked && canUp)
            {
                s_eventLogScrollOffset = std::max(0, s_eventLogScrollOffset - 1);
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, upRect, "^", upHov, canUp, false, uiScale * 0.60f);

            SDL_FRect downRect = { btnDownX, btnY, btnW, btnH };
            bool downHov = (mousePos.x >= downRect.x && mousePos.x <= downRect.x + downRect.w &&
                            mousePos.y >= downRect.y && mousePos.y <= downRect.y + downRect.h);
            bool canDown = (s_eventLogScrollOffset < maxOffset);
            if (downHov && clicked && canDown)
            {
                s_eventLogScrollOffset = std::min(maxOffset, s_eventLogScrollOffset + 1);
                gameContext->input.consumeMouseClick();
            }
            UIWidget::drawButton(renderer, downRect, "v", downHov, canDown, false, uiScale * 0.60f);
        }

        float logCurY = curY + (22.0f * uiScale);
        float scrollbarW = (totalLogs > maxVisible) ? (12.0f * uiScale) : 0.0f;
        cW = availableW - (innerPad * 2.0f) - scrollbarW;

        if (allLogs.empty())
        {
            UIWidget::drawText(renderer, "No recent events recorded.", innerX, logCurY, Theme::colors.textMuted, uiScale * 0.65f);
        }
        else
        {
            // Render entries with MOST RECENT ON TOP (reverse chronological order)
            // When s_eventLogScrollOffset == 0, row 0 is allLogs[N - 1] (newest)
            for (int r = 0; r < maxVisible; ++r)
            {
                int entryIdx = (totalLogs - 1) - (s_eventLogScrollOffset + r);
                if (entryIdx < 0 || entryIdx >= totalLogs) break;

                const auto& entry = allLogs[entryIdx];
                SDL_Color tagColor = { entry.color.r, entry.color.g, entry.color.b, entry.color.a };
                UIWidget::drawText(renderer, entry.tag, innerX, logCurY, tagColor, uiScale * 0.65f);
                float tagW = UIWidget::getTextWidth(entry.tag, uiScale * 0.65f);

                float maxTextW = cW - tagW - (6.0f * uiScale);
                std::string displayText = entry.text;
                while (!displayText.empty() && UIWidget::getTextWidth(displayText, uiScale * 0.65f) > maxTextW)
                {
                    displayText.pop_back();
                }
                if (displayText.size() < entry.text.size() && displayText.size() > 3)
                {
                    displayText.replace(displayText.size() - 2, 2, "..");
                }

                UIWidget::drawText(renderer, displayText, innerX + tagW + (4.0f * uiScale), logCurY, Theme::colors.textPrimary, uiScale * 0.65f);

                SDL_FRect rowRect = { innerX, logCurY, cW, 16.0f * uiScale };
                TooltipManager::setHoverTooltip(rowRect, mousePos, entry.tag + (!entry.timeStr.empty() ? (" (" + entry.timeStr + ")") : ""), entry.text, "Event Log");

                logCurY += (18.0f * uiScale);
            }

            // Draw scrollbar track and thumb with full mouse click jumping and drag support
            if (totalLogs > maxVisible)
            {
                float barW = 5.0f * uiScale;
                float trackX = padX + availableW - innerPad - barW;
                float trackY = curY + (22.0f * uiScale);
                float trackH = card4H - (26.0f * uiScale);

                float thumbH = std::max(16.0f * uiScale, trackH * (static_cast<float>(maxVisible) / static_cast<float>(totalLogs)));
                float travelH = trackH - thumbH;
                float ratio = (maxOffset > 0 && travelH > 0.0f) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                float thumbY = trackY + ratio * travelH;

                static bool s_isDraggingLogScroll = false;
                static float s_dragStartMouseY = 0.0f;
                static int s_dragStartOffset = 0;

                SDL_FRect trackHitRect = { trackX - (4.0f * uiScale), trackY, barW + (8.0f * uiScale), trackH };
                bool trackHovered = (mousePos.x >= trackHitRect.x && mousePos.x <= trackHitRect.x + trackHitRect.w &&
                                     mousePos.y >= trackHitRect.y && mousePos.y <= trackHitRect.y + trackHitRect.h);

                bool isMouseDown = gameContext->input.isLeftMouseDown();
                if (clicked && trackHovered)
                {
                    s_isDraggingLogScroll = true;
                    s_dragStartMouseY = mousePos.y;
                    s_dragStartOffset = s_eventLogScrollOffset;

                    // If clicked directly on track outside thumb, jump immediately
                    bool onThumb = (mousePos.y >= thumbY && mousePos.y <= thumbY + thumbH);
                    if (!onThumb && travelH > 0.0f)
                    {
                        float clickRatio = std::clamp((mousePos.y - trackY - (thumbH / 2.0f)) / travelH, 0.0f, 1.0f);
                        s_eventLogScrollOffset = std::clamp(static_cast<int>(std::round(clickRatio * maxOffset)), 0, maxOffset);
                        s_dragStartOffset = s_eventLogScrollOffset;
                        ratio = (maxOffset > 0) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                        thumbY = trackY + ratio * travelH;
                    }
                    gameContext->input.consumeMouseClick();
                }

                if (s_isDraggingLogScroll)
                {
                    if (isMouseDown)
                    {
                        float deltaY = mousePos.y - s_dragStartMouseY;
                        if (travelH > 0.0f)
                        {
                            float offsetDelta = (deltaY / travelH) * maxOffset;
                            s_eventLogScrollOffset = std::clamp(s_dragStartOffset + static_cast<int>(std::round(offsetDelta)), 0, maxOffset);
                            ratio = (maxOffset > 0) ? (static_cast<float>(s_eventLogScrollOffset) / static_cast<float>(maxOffset)) : 0.0f;
                            thumbY = trackY + ratio * travelH;
                        }
                    }
                    else
                    {
                        s_isDraggingLogScroll = false;
                    }
                }

                SDL_FRect trackRect = { trackX, trackY, barW, trackH };
                UIWidget::drawPanel(renderer, trackRect, Theme::colors.bgDark, Theme::colors.borderNormal);

                SDL_FRect thumbRect = { trackX, thumbY, barW, thumbH };
                SDL_Color thumbCol = (s_isDraggingLogScroll || trackHovered) ? Theme::colors.textGold : Theme::colors.borderSelected;
                UIWidget::drawPanel(renderer, thumbRect, thumbCol, thumbCol);
            }
        }

        curY += card4H + (8.0f * uiScale);
        return (curY - startY);
    }
}
