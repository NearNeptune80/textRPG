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

namespace EntityListWidgets
{
    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        // 1. Zone & Danger Status Card
        float zoneH = 26.0f * uiScale;
        SDL_FRect zoneRect = { padX, curY, availableW, zoneH };
        std::string locName = "Lilaya's Home F1";
        if (const gameMap* m = gameContext->getActiveMap())
        {
            if (!m->getName().empty() && m->getName() != "District Map") locName = m->getName();
        }
        UIWidget::drawHeader(renderer, zoneRect, locName + " [Safe]", Theme::colors.bgHeader, Theme::colors.companion, uiScale * 0.82f);
        curY += zoneH + (8.0f * uiScale);

        // 2. Characters Present Header
        UIWidget::drawText(renderer, "CHARACTERS PRESENT", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
        curY += (18.0f * uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        bool hasNpc = false;
        if (gameContext->map)
        {
            auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            if (tileData.persistentNPC)
            {
                hasNpc = true;
                entity* npc = tileData.persistentNPC.get();

                float cardH = 44.0f * uiScale;
                SDL_FRect cardRect = { padX, curY, availableW, cardH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, npc->name, padX + (8.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
                std::string raceStr = npc->anatomy.getRacialTitle().empty() ? "Demon" : npc->anatomy.getRacialTitle();
                UIWidget::drawText(renderer, raceStr, padX + (8.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.72f);

                // Action buttons: [ Talk ] [ Inspect ]
                float btnW = 46.0f * uiScale;
                float btnH = 22.0f * uiScale;
                SDL_FRect talkBtn = { padX + availableW - (btnW * 2.0f) - (8.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };
                bool tHov = (mousePos.x >= talkBtn.x && mousePos.x <= talkBtn.x + talkBtn.w &&
                             mousePos.y >= talkBtn.y && mousePos.y <= talkBtn.y + talkBtn.h);
                UIWidget::drawButton(renderer, talkBtn, "Talk", tHov, true, false, uiScale * 0.72f);
                if (tHov && clicked)
                {
                    gameContext->activeTargetNPC = tileData.persistentNPC;
                    gameContext->activeTargetMode = TargetMode::DIALOGUE;
                    gameContext->input.consumeMouseClick();
                }

                SDL_FRect inspBtn = { padX + availableW - btnW - (4.0f * uiScale), curY + (11.0f * uiScale), btnW, btnH };
                bool iHov = (mousePos.x >= inspBtn.x && mousePos.x <= inspBtn.x + inspBtn.w &&
                             mousePos.y >= inspBtn.y && mousePos.y <= inspBtn.y + inspBtn.h);
                UIWidget::drawButton(renderer, inspBtn, "View", iHov, true, false, uiScale * 0.72f);
                if (iHov && clicked)
                {
                    gameContext->activeTargetNPC = tileData.persistentNPC;
                    gameContext->activeTargetMode = TargetMode::DIALOGUE;
                    gameContext->input.consumeMouseClick();
                }

                curY += cardH + (8.0f * uiScale);
            }
        }

        if (!hasNpc)
        {
            SDL_FRect emptyCard = { padX, curY, availableW, 28.0f * uiScale };
            UIWidget::drawPanel(renderer, emptyCard, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "No other characters present.", padX + (8.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
            curY += emptyCard.h + (8.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        UIWidget::drawText(renderer, "ITEMS PRESENT", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
        curY += (18.0f * uiScale);

        auto ground = gameContext->getTileInventoryStacked();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        if (ground.empty())
        {
            SDL_FRect emptyCard = { padX, curY, availableW, 28.0f * uiScale };
            UIWidget::drawPanel(renderer, emptyCard, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "No items on the ground.", padX + (8.0f * uiScale), curY + (7.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
            curY += emptyCard.h + (8.0f * uiScale);
        }
        else
        {
            for (size_t i = 0; i < ground.size() && i < 4; ++i)
            {
                if (ground[i].itemPtr)
                {
                    float cardH = 34.0f * uiScale;
                    SDL_FRect iBox = { padX, curY, availableW, cardH };
                    UIWidget::drawPanel(renderer, iBox, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                    UIWidget::drawText(renderer, line, padX + (8.0f * uiScale), curY + (9.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

                    float pickBtnW = 48.0f * uiScale;
                    float pickBtnH = 20.0f * uiScale;
                    SDL_FRect pickBtn = { padX + availableW - pickBtnW - (6.0f * uiScale), curY + (7.0f * uiScale), pickBtnW, pickBtnH };
                    bool pHov = (mousePos.x >= pickBtn.x && mousePos.x <= pickBtn.x + pickBtn.w &&
                                 mousePos.y >= pickBtn.y && mousePos.y <= pickBtn.y + pickBtn.h);
                    UIWidget::drawButton(renderer, pickBtn, "Pick Up", pHov, true, false, uiScale * 0.68f);

                    if (pHov && clicked)
                    {
                        gameContext->handlePickupAction(static_cast<int>(i), 1);
                        gameContext->input.consumeMouseClick();
                        break;
                    }

                    curY += cardH + (6.0f * uiScale);
                }
            }
        }

        return (curY - startY);
    }

    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        UIWidget::drawText(renderer, "ACTIVITY & EVENT LOG", padX, curY, Theme::colors.textGold, uiScale * 0.85f);
        curY += (18.0f * uiScale);

        SDL_FRect logBox = { padX, curY, availableW, 140.0f * uiScale };
        UIWidget::drawPanel(renderer, logBox, Theme::colors.bgDark, Theme::colors.borderButton);

        float logCurY = curY + (6.0f * uiScale);
        float innerLogX = padX + (8.0f * uiScale);

        struct LogEntry {
            std::string tag;
            std::string text;
            SDL_Color tagColor;
        };

        static const std::vector<LogEntry> logEntries = {
            { "[ZONE]", "Entered Lilaya's Home F1", Theme::colors.companion },
            { "[TASK]", "Discovered: Lilaya's Tests", Theme::colors.textGold },
            { "[LORE]", "Encyclopedia: Demon Morph", Theme::colors.arcane },
            { "[ITEM]", "Equipped: Opaque Demonstone", Theme::colors.health },
            { "[GOLD]", "Gained: 5,000 ¤", Theme::colors.currency },
            { "[INFO]", "All systems operational", Theme::colors.textSecondary }
        };

        for (const auto& entry : logEntries)
        {
            UIWidget::drawText(renderer, entry.tag, innerLogX, logCurY, entry.tagColor, uiScale * 0.72f);
            float tagW = UIWidget::getTextWidth(entry.tag, uiScale * 0.72f);
            UIWidget::drawText(renderer, entry.text, innerLogX + tagW + (6.0f * uiScale), logCurY, Theme::colors.textPrimary, uiScale * 0.72f);
            logCurY += (18.0f * uiScale);
        }

        curY += logBox.h + (8.0f * uiScale);
        return (curY - startY);
    }
}
