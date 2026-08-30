#include "ui/widgets/statusWidgets.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include <format>
#include <algorithm>

namespace StatusWidgets
{
    float renderWidgetCharOverview(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* p = gameContext->getPlayer();
        if (!p) return 0.0f;

        float lineH = 18.0f * uiScale;
        float startY = curY;
        float padX = curX + (10.0f * uiScale);

        UIWidget::drawText(renderer, std::format("Name: {}", p->name), padX, curY, Theme::colors.textPrimary, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Title: {}", p->anatomy.getRacialTitle()), padX, curY, Theme::colors.textAccent, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Gender: {}", genderArchetypeToString(p->anatomy.getGenderArchetype())), padX, curY, Theme::colors.textSecondary, uiScale); curY += (lineH + 4.0f * uiScale);

        return (curY - startY);
    }

    float renderWidgetVitals(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* p = gameContext->getPlayer();
        if (!p) return 0.0f;

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float barW = innerW - (20.0f * uiScale);
        float barH = 18.0f * uiScale;

        float hp = p->getStat("health");
        UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, hp, 100.0f, Theme::colors.health, Theme::colors.bgDark, std::format("HP: {:.0f}/100", hp), uiScale); curY += (barH + 6.0f * uiScale);

        float mana = p->getStat("mana");
        UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, mana, 50.0f, Theme::colors.mana, Theme::colors.bgDark, std::format("Mana: {:.0f}/50", mana), uiScale); curY += (barH + 6.0f * uiScale);

        float lust = p->getStat("lust");
        UIWidget::drawProgressBar(renderer, { padX, curY, barW, barH }, lust, 100.0f, Theme::colors.lust, Theme::colors.bgDark, std::format("Lust: {:.0f}/100", lust), uiScale); curY += (barH + 8.0f * uiScale);

        return (curY - startY);
    }

    float renderWidgetAttributes(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* p = gameContext->getPlayer();
        if (!p) return 0.0f;

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float lineH = 18.0f * uiScale;

        UIWidget::drawText(renderer, "ATTRIBUTES", padX, curY, Theme::colors.textGold, uiScale); curY += (lineH + 2.0f * uiScale);
        UIWidget::drawText(renderer, std::format("Physique:   {:.0f}", p->getStat("physique")), padX, curY, Theme::colors.physique, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Agility:    {:.0f}", p->getStat("agility")), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Arcane:     {:.0f}", p->getStat("arcane")), padX, curY, Theme::colors.arcane, uiScale); curY += lineH;
        UIWidget::drawText(renderer, std::format("Corruption: {:.0f}", p->getStat("corruption")), padX, curY, Theme::colors.corruption, uiScale); curY += (lineH + 8.0f * uiScale);

        return (curY - startY);
    }

    float renderWidgetAnatomy(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        entity* p = gameContext->getPlayer();
        if (!p) return 0.0f;

        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float lineH = 18.0f * uiScale;

        UIWidget::drawText(renderer, "ANATOMY & FLUIDS", padX, curY, Theme::colors.textGold, uiScale); curY += (lineH + 2.0f * uiScale);
        UIWidget::drawText(renderer, std::format("Height: {:.2f}m", p->anatomy.heightMeters), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;

        if (const bodyPart* b = p->anatomy.getPart(bodySlot::BREASTS))
        {
            UIWidget::drawText(renderer, std::format("Breasts: {}-Cup ({:.0f}/{:.0f}ml milk)", bodyPart::getCupSizeName(b->cupSize), b->currentFluidMl, b->maxFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
        }
        if (const bodyPart* g = p->anatomy.getPart(bodySlot::GROIN))
        {
            if (p->anatomy.hasPenis())
            {
                UIWidget::drawText(renderer, std::format("Penis: {:.1f}cm x {:.1f}cm ({:.0f}/{:.0f}ml cum)", g->length, g->diameter, g->currentFluidMl, g->maxFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
            }
            if (p->anatomy.hasVagina())
            {
                UIWidget::drawText(renderer, std::format("Vagina: Capacity {:.0f}ml ({:.0f}ml stored)", g->maxFluidMl, g->currentFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;
            }
        }
        if (const bodyPart* a = p->anatomy.getPart(bodySlot::ASS))
        {
            UIWidget::drawText(renderer, std::format("Anus: Capacity {:.0f}ml ({:.0f}ml stored)", a->maxFluidMl, a->currentFluidMl), padX, curY, Theme::colors.textSecondary, uiScale); curY += (lineH + 8.0f * uiScale);
        }

        return (curY - startY);
    }

    float renderWidgetTarget(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float lineH = 16.0f * uiScale;

        UIWidget::drawText(renderer, "PROXIMITY TARGET", padX, curY, Theme::colors.textGold, uiScale); curY += (18.0f * uiScale);

        if (entity* npc = gameContext->getActiveTargetNPC())
        {
            UIWidget::drawText(renderer, std::format("Name: {}", npc->name), padX, curY, Theme::colors.textPrimary, uiScale); curY += lineH;
            UIWidget::drawText(renderer, std::format("Level: {} | {}", npc->stats.level, npc->anatomy.getDominantRace()), padX, curY, Theme::colors.textSecondary, uiScale); curY += lineH;

            float hp = npc->getStat("health");
            UIWidget::drawProgressBar(renderer, { padX, curY, innerW - (20.0f * uiScale), 16.0f * uiScale }, hp, 100.0f, Theme::colors.enemy, Theme::colors.bgDark, std::format("HP: {:.0f}", hp), uiScale);
            curY += (20.0f * uiScale);
        }
        else
        {
            UIWidget::drawText(renderer, "No active target.", padX, curY, Theme::colors.textMuted, uiScale); curY += lineH;
        }

        return (curY - startY);
    }
}
