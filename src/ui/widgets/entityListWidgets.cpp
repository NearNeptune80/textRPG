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
        float innerPad = 8.0f * uiScale;
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

        std::string locName = "Lilaya's Home F1";
        if (const gameMap* m = gameContext->getActiveMap())
        {
            if (!m->getName().empty() && m->getName() != "District Map") locName = m->getName();
        }

        UIWidget::drawText(renderer, locName, innerX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        std::string safeTag = "[ Safe Sanctuary ]";
        float safeW = UIWidget::getTextWidth(safeTag, uiScale * 0.74f);
        UIWidget::drawText(renderer, safeTag, innerX + cW - safeW, curY + (6.0f * uiScale), Theme::colors.companion, uiScale * 0.74f);

        UIWidget::drawText(renderer, "Sanctuary interior • Quiet atmosphere", innerX, curY + (24.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

        curY += card1H + (10.0f * uiScale);

        // ==========================================
        // CARD 2: Characters Present Card
        // ==========================================
        bool hasNpc = false;
        entity* npc = nullptr;
        if (gameContext->map)
        {
            auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            if (tileData.persistentNPC)
            {
                hasNpc = true;
                npc = tileData.persistentNPC.get();
            }
        }

        float card2H = hasNpc ? (68.0f * uiScale) : (52.0f * uiScale);
        SDL_FRect card2Rect = { padX, curY, availableW, card2H };
        UIWidget::drawPanel(renderer, card2Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "CHARACTERS PRESENT", innerX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

        if (hasNpc && npc)
        {
            float npcY = curY + (24.0f * uiScale);
            UIWidget::drawText(renderer, npc->name, innerX, npcY, Theme::colors.textGold, uiScale * 0.85f);
            std::string raceStr = npc->anatomy.getRacialTitle().empty() ? "Demon" : npc->anatomy.getRacialTitle();
            UIWidget::drawText(renderer, raceStr, innerX, npcY + (16.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.70f);

            // Action buttons: [ Talk ] [ View ]
            float btnW = 46.0f * uiScale;
            float btnH = 22.0f * uiScale;
            SDL_FRect talkBtn = { innerX + cW - (btnW * 2.0f) - (6.0f * uiScale), npcY + (2.0f * uiScale), btnW, btnH };
            bool tHov = (mousePos.x >= talkBtn.x && mousePos.x <= talkBtn.x + talkBtn.w &&
                         mousePos.y >= talkBtn.y && mousePos.y <= talkBtn.y + talkBtn.h);
            UIWidget::drawButton(renderer, talkBtn, "Talk", tHov, true, false, uiScale * 0.72f);
            if (tHov && clicked)
            {
                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                gameContext->activeTargetNPC = tileData.persistentNPC;
                gameContext->activeTargetMode = TargetMode::DIALOGUE;
                gameContext->input.consumeMouseClick();
            }

            SDL_FRect inspBtn = { innerX + cW - btnW, npcY + (2.0f * uiScale), btnW, btnH };
            bool iHov = (mousePos.x >= inspBtn.x && mousePos.x <= inspBtn.x + inspBtn.w &&
                         mousePos.y >= inspBtn.y && mousePos.y <= inspBtn.y + inspBtn.h);
            UIWidget::drawButton(renderer, inspBtn, "View", iHov, true, false, uiScale * 0.72f);
            if (iHov && clicked)
            {
                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                gameContext->activeTargetNPC = tileData.persistentNPC;
                gameContext->activeTargetMode = TargetMode::DIALOGUE;
                gameContext->input.consumeMouseClick();
            }
        }
        else
        {
            UIWidget::drawText(renderer, "No other characters present in this area.", innerX, curY + (26.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
        }

        curY += card2H + (10.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);
        float innerPad = 8.0f * uiScale;
        float innerX = padX + innerPad;
        float cW = availableW - (innerPad * 2.0f);

        auto ground = gameContext->getTileInventoryStacked();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // ==========================================
        // CARD 3: Items on Ground Card
        // ==========================================
        float card3H = ground.empty() ? (52.0f * uiScale) : ((32.0f * uiScale) + (ground.size() * (32.0f * uiScale)));
        card3H = std::min(card3H, 140.0f * uiScale);

        SDL_FRect card3Rect = { padX, curY, availableW, card3H };
        UIWidget::drawPanel(renderer, card3Rect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "ITEMS PRESENT", innerX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

        if (ground.empty())
        {
            UIWidget::drawText(renderer, "No items dropped on the floor.", innerX, curY + (26.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
        }
        else
        {
            float itemY = curY + (24.0f * uiScale);
            for (size_t i = 0; i < ground.size() && i < 3; ++i)
            {
                if (ground[i].itemPtr)
                {
                    std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                    UIWidget::drawText(renderer, line, innerX, itemY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.76f);

                    float pickBtnW = 48.0f * uiScale;
                    float pickBtnH = 20.0f * uiScale;
                    SDL_FRect pickBtn = { innerX + cW - pickBtnW, itemY + (2.0f * uiScale), pickBtnW, pickBtnH };
                    bool pHov = (mousePos.x >= pickBtn.x && mousePos.x <= pickBtn.x + pickBtn.w &&
                                 mousePos.y >= pickBtn.y && mousePos.y <= pickBtn.y + pickBtn.h);
                    UIWidget::drawButton(renderer, pickBtn, "Take", pHov, true, false, uiScale * 0.68f);

                    if (pHov && clicked)
                    {
                        gameContext->handlePickupAction(static_cast<int>(i), 1);
                        gameContext->input.consumeMouseClick();
                        break;
                    }

                    itemY += (28.0f * uiScale);
                }
            }
        }

        curY += card3H + (10.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);
        float innerPad = 8.0f * uiScale;
        float innerX = padX + innerPad;

        // ==========================================
        // CARD 4: Activity & Event Log Card
        // ==========================================
        float card4H = 150.0f * uiScale;
        SDL_FRect logBox = { padX, curY, availableW, card4H };
        UIWidget::drawPanel(renderer, logBox, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, "ACTIVITY & EVENT LOG", innerX, curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.78f);

        float logCurY = curY + (24.0f * uiScale);

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
            UIWidget::drawText(renderer, entry.tag, innerX, logCurY, entry.tagColor, uiScale * 0.70f);
            float tagW = UIWidget::getTextWidth(entry.tag, uiScale * 0.70f);
            UIWidget::drawText(renderer, entry.text, innerX + tagW + (6.0f * uiScale), logCurY, Theme::colors.textPrimary, uiScale * 0.70f);
            logCurY += (18.0f * uiScale);
        }

        curY += card4H + (10.0f * uiScale);
        return (curY - startY);
    }
}
