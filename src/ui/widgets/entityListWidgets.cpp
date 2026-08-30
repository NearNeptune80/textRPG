#include "ui/widgets/entityListWidgets.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/characterCreationState.h"
#include <format>
#include <vector>
#include <string_view>

namespace EntityListWidgets
{
    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float availableW = innerW - (20.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        if (inPrologue)
        {
            UIWidget::drawText(renderer, "Land", padX + (availableW * 0.35f), curY, Theme::colors.textGold, uiScale * 1.05f);
            curY += (18.0f * uiScale);
            UIWidget::drawText(renderer, "Safe", padX + (availableW * 0.35f), curY, Theme::colors.companion, uiScale * 0.9f);
            curY += (20.0f * uiScale);

            UIWidget::drawText(renderer, "Characters Present", padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += (18.0f * uiScale);

            SDL_FRect cardRect = { padX, curY, availableW, 24.0f * uiScale };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
            UIWidget::drawText(renderer, "✋ Few people", padX + (6.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.85f);
            curY += (28.0f * uiScale);
            return (curY - startY);
        }

        UIWidget::drawText(renderer, "Land", padX + (availableW * 0.35f), curY, Theme::colors.textGold, uiScale * 1.05f);
        curY += (18.0f * uiScale);
        UIWidget::drawText(renderer, "Safe", padX + (availableW * 0.35f), curY, Theme::colors.companion, uiScale * 0.9f);
        curY += (20.0f * uiScale);

        UIWidget::drawText(renderer, "Characters Present", padX + (10.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += (18.0f * uiScale);

        bool hasNpc = false;
        if (gameContext->map)
        {
            auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            if (tileData.persistentNPC)
            {
                hasNpc = true;
                SDL_FRect cardRect = { padX, curY, availableW, 24.0f * uiScale };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, std::format("👤 {}", tileData.persistentNPC->name), padX + (6.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.lust, uiScale * 0.85f);
                curY += (28.0f * uiScale);
            }
        }

        if (!hasNpc)
        {
            UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
            curY += (16.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float availableW = innerW - (20.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        if (inPrologue)
        {
            UIWidget::drawText(renderer, "Items Present", padX + (16.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += (18.0f * uiScale);
            UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
            curY += (16.0f * uiScale);
            return (curY - startY);
        }

        UIWidget::drawText(renderer, "Items Present", padX + (16.0f * uiScale), curY, Theme::colors.textPrimary, uiScale * 0.88f);
        curY += (18.0f * uiScale);

        auto ground = gameContext->getTileInventoryStacked();
        if (ground.empty())
        {
            UIWidget::drawText(renderer, "None...", padX + (availableW * 0.35f), curY, Theme::colors.textMuted, uiScale * 0.85f);
            curY += (16.0f * uiScale);
        }
        else
        {
            for (size_t i = 0; i < ground.size() && i < 6; ++i)
            {
                if (ground[i].itemPtr)
                {
                    std::string line = std::format("{}x {}", ground[i].totalCount, ground[i].itemPtr->name);
                    UIWidget::drawText(renderer, line, padX, curY, Theme::colors.textAccent, uiScale * 0.85f);
                    curY += (15.0f * uiScale);
                }
            }
        }

        return (curY - startY);
    }

    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float availableW = innerW - (20.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        if (inPrologue)
        {
            UIWidget::drawText(renderer, "Event Log", padX + (availableW * 0.3f), curY, Theme::colors.textPrimary, uiScale * 0.88f);
            curY += (18.0f * uiScale);

            static constexpr std::string_view startingEquipLog[] = {
                "Equipped: Silver masculine watch",
                "Equipped: Silver ring",
                "Equipped: Black men's shoes",
                "Equipped: Black socks",
                "Equipped: Black trousers",
                "Equipped: White short-sleeved shirt",
                "Equipped: Black boxer shorts"
            };

            for (const auto& entry : startingEquipLog)
            {
                float descH = UIWidget::drawTextWrapped(renderer, std::string(entry), padX, curY, availableW, Theme::colors.textSecondary, uiScale * 0.8f);
                curY += descH + (6.0f * uiScale);
            }
            return (curY - startY);
        }

        UIWidget::drawText(renderer, "EVENT LOG", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);

        static const std::vector<std::pair<std::string, SDL_Color>> logEntries = {
            { "Entered: Lilaya's Home F1", Theme::colors.friendly },
            { "Discovered: Lilaya's Home F1", Theme::colors.textGold },
            { "Encyclopedia: Cat-morph", Theme::colors.textAccent },
            { "Encyclopedia: Half-demon", Theme::colors.arcane },
            { "Equipped: opaque demonstone", SDL_Color{ 100, 180, 255, 255 } },
            { "Encyclopedia: Opaque demonstone", Theme::colors.textGold },
            { "Gained: ¤ 5,000", Theme::colors.friendly },
            { "New Task: Lilaya's Tests", SDL_Color{ 100, 220, 255, 255 } }
        };

        for (const auto& entry : logEntries)
        {
            UIWidget::drawText(renderer, entry.first, padX, curY, entry.second, uiScale * 0.8f);
            curY += (14.0f * uiScale);
        }

        return (curY - startY);
    }
}
