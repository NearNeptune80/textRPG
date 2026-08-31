#pragma once

#include <string>
#include <vector>
#include <memory>
#include "state/iGameState.h"

enum class TransformationTab
{
    CORE = 0,
    EYES,
    HAIR,
    HEAD_FACE,
    ASS_HIPS,
    BREASTS,
    VAGINA,
    PENIS,
    CROTCH_BOOBS,
    APPENDAGES,
    INSPECT_PRESETS
};

inline const char* transformationTabToString(TransformationTab tab)
{
    switch (tab)
    {
        case TransformationTab::CORE:            return "Core";
        case TransformationTab::EYES:            return "Eyes";
        case TransformationTab::HAIR:            return "Hair";
        case TransformationTab::HEAD_FACE:       return "Head & Face";
        case TransformationTab::ASS_HIPS:        return "Ass & Hips";
        case TransformationTab::BREASTS:         return "Breasts";
        case TransformationTab::VAGINA:          return "Vagina";
        case TransformationTab::PENIS:           return "Penis";
        case TransformationTab::CROTCH_BOOBS:    return "Crotch-Boobs";
        case TransformationTab::APPENDAGES:      return "Appendages";
        case TransformationTab::INSPECT_PRESETS: return "Inspect & Presets";
        default:                                 return "Core";
    }
}

/**
 * Headless state controller for Full Body Transformations and Anatomy Customization.
 */
class transformationState : public iGameState
{
public:
    TransformationTab currentTab = TransformationTab::CORE;
    std::string presetInputName = "Demon_Form";
    std::string statusMessage = "";

    explicit transformationState(TransformationTab initialTab = TransformationTab::CORE);
    ~transformationState() override = default;

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void setTab(TransformationTab tab, game* gameContext);
    void savePreset(game* gameContext, const std::string& name);
    void loadPreset(game* gameContext, const std::string& name);
    void deletePreset(const std::string& name);
    const std::vector<std::string>& getPresetNames() const { return m_presetNames; }
    void refreshPresetNames();

    void resetToHuman(game* gameContext);
    void randomizeForm(game* gameContext);

private:
    std::vector<std::string> m_presetNames;
};
