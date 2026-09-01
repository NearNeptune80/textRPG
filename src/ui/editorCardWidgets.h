#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <functional>
#include <format>
#include <algorithm>
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "core/game.h"

namespace EditorCardWidgets
{
    struct ColorOption
    {
        std::string id;
        std::string name;
        SDL_Color color;
    };

    // Shared Standard Palettes
    const std::vector<ColorOption>& getSkinTones();
    const std::vector<ColorOption>& getHairColors();
    const std::vector<ColorOption>& getEyeColors();
    const std::vector<ColorOption>& getMakeupColors();
    const std::vector<ColorOption>& getTransformationColors();

    // Reusable UI Card Renderers
    float drawPillCard(SDL_Renderer* renderer,
                       game* gameContext,
                       const SDL_FRect& panelRect,
                       float padX,
                       float curY,
                       float availableW,
                       std::string_view title,
                       std::string_view description,
                       const std::vector<std::string>& options,
                       const std::string& currentSelected,
                       const std::function<void(const std::string&)>& onSelect,
                       float uiScale,
                       int cols = 5);

    float drawTogglePillCard(SDL_Renderer* renderer,
                             game* gameContext,
                             const SDL_FRect& panelRect,
                             float padX,
                             float curY,
                             float availableW,
                             std::string_view title,
                             std::string_view description,
                             const std::vector<std::string>& options,
                             const std::set<std::string>& activeItems,
                             const std::function<void(const std::string&, bool)>& onToggle,
                             float uiScale,
                             int cols = 5);

    float drawTogglePillCard(SDL_Renderer* renderer,
                             game* gameContext,
                             const SDL_FRect& panelRect,
                             float padX,
                             float curY,
                             float availableW,
                             std::string_view title,
                             std::string_view description,
                             const std::vector<std::string>& options,
                             const std::vector<std::string>& activeItems,
                             const std::function<void(const std::string&, bool)>& onToggle,
                             float uiScale,
                             int cols = 4);

    float drawColorSwatchCard(SDL_Renderer* renderer,
                              game* gameContext,
                              const SDL_FRect& panelRect,
                              float padX,
                              float curY,
                              float availableW,
                              std::string_view title,
                              std::string_view description,
                              const std::vector<ColorOption>& options,
                              const std::string& currentSelected,
                              const std::function<void(const std::string&)>& onSelect,
                              float uiScale);

    float drawCycleStepperCard(SDL_Renderer* renderer,
                               game* gameContext,
                               const SDL_FRect& panelRect,
                               float padX,
                               float curY,
                               float availableW,
                               std::string_view title,
                               std::string_view description,
                               const std::vector<std::string>& options,
                               const std::string& currentSelected,
                               const std::function<void(int)>& onCycle,
                               float uiScale);

    float drawToggleCard(SDL_Renderer* renderer,
                         game* gameContext,
                         const SDL_FRect& panelRect,
                         float padX,
                         float curY,
                         float availableW,
                         std::string_view title,
                         std::string_view description,
                         bool currentState,
                         const std::function<void(bool)>& onToggle,
                         float uiScale,
                         std::string_view trueLabel = "Yes",
                         std::string_view falseLabel = "No");

    template <typename T = float>
    float drawStepperCard(SDL_Renderer* renderer,
                          game* gameContext,
                          const SDL_FRect& panelRect,
                          float padX,
                          float curY,
                          float availableW,
                          std::string_view title,
                          std::string_view description,
                          std::string_view displayVal,
                          const std::function<void(T)>& onStep,
                          float uiScale,
                          T stepSmall = 1,
                          T stepMed = 0,
                          T stepLarge = 0)
    {
        float startY = curY;
        float innerPad = 12.0f * uiScale;
        float cardH = (description.empty() ? 48.0f : 58.0f) * uiScale;

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        // Title & Description
        UIWidget::drawText(renderer, title, padX + innerPad, curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
        if (!description.empty())
        {
            UIWidget::drawText(renderer, description, padX + innerPad, curY + (28.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);
        }

        bool hasMultiTier = (stepMed > 0);
        float btnW = (hasMultiTier ? 28.0f : 34.0f) * uiScale;
        float btnGap = 2.0f * uiScale;
        float valW = (hasMultiTier ? 120.0f : 140.0f) * uiScale;
        float h = 26.0f * uiScale;

        int btnCountPerSide = (stepLarge > 0) ? 3 : ((stepMed > 0) ? 2 : 1);
        float sideBtnsW = (btnCountPerSide * btnW) + ((btnCountPerSide - 1) * btnGap);
        float controlsTotalW = (sideBtnsW * 2.0f) + valW + (8.0f * uiScale);
        float curX = padX + availableW - innerPad - controlsTotalW;
        float controlY = curY + ((cardH - h) / 2.0f);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        auto drawBtnHelper = [&](float x, const char* label, T delta) {
            SDL_FRect bRect = { x, controlY, btnW, h };
            bool inPanel = (mousePos.y >= panelRect.y && mousePos.y <= panelRect.y + panelRect.h &&
                            controlY + h >= panelRect.y && controlY <= panelRect.y + panelRect.h);
            bool hov = inPanel && (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                                   mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);
            UIWidget::drawButton(renderer, bRect, label, hov, true, false, uiScale * 0.76f);
            if (hov && clicked)
            {
                onStep(delta);
                gameContext->input.consumeMouseClick();
            }
        };

        // Left Decrease Buttons
        if (btnCountPerSide == 3)
        {
            drawBtnHelper(curX, "<<<", -stepLarge);
            curX += btnW + btnGap;
        }
        if (btnCountPerSide >= 2)
        {
            drawBtnHelper(curX, "<<", -stepMed);
            curX += btnW + btnGap;
        }
        drawBtnHelper(curX, "<", -stepSmall);
        curX += btnW + (4.0f * uiScale);

        // Center Value Display
        SDL_FRect valRect = { curX, controlY, valW, h };
        UIWidget::drawPanel(renderer, valRect, Theme::colors.bgHeader, Theme::colors.borderNormal);
        float textW = UIWidget::getTextWidth(displayVal, uiScale * 0.82f);
        UIWidget::drawText(renderer, displayVal, curX + ((valW - textW) / 2.0f), controlY + (5.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
        curX += valW + (4.0f * uiScale);

        // Right Increase Buttons
        drawBtnHelper(curX, ">", stepSmall);
        curX += btnW + btnGap;
        if (btnCountPerSide >= 2)
        {
            drawBtnHelper(curX, ">>", stepMed);
            curX += btnW + btnGap;
        }
        if (btnCountPerSide == 3)
        {
            drawBtnHelper(curX, ">>>", stepLarge);
        }

        curY += cardH + (12.0f * uiScale);
        return curY - startY;
    }
}
