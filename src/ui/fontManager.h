#pragma once

#include <string>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

/**
 * Font Manager with Automatic Bitmap Fallback.
 * Loads and renders TrueType vector fonts (TTF/OTF) specified by themes/skins.
 * Automatically falls back to the embedded 8x8 pixel font if the font file is missing,
 * corrupted, or running in an environment without external assets.
 */
class fontManager
{
public:
    static fontManager& getInstance();

    bool init();
    void cleanup();

    bool loadFont(const std::string& fontPath, float pointSize = 14.0f);
    void setScale(float scale);
    void setPointSize(float pointSize);

    void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale = 1.0f);
    float drawTextWrapped(SDL_Renderer* renderer, const std::string& text, float x, float y, float maxWidth, SDL_Color color, float scale = 1.0f);

    float getTextWidth(const std::string& text, float scale = 1.0f);
    float getLineHeight(float scale = 1.0f);

    bool hasLoadedFont() const { return m_font != nullptr; }
    const std::string& getLoadedFontPath() const { return m_currentFontPath; }
    float getPointSize() const { return m_basePointSize; }

private:
    fontManager();
    ~fontManager();

    fontManager(const fontManager&) = delete;
    fontManager& operator=(const fontManager&) = delete;

    bool m_initialized = false;
    TTF_Font* m_font = nullptr;
    std::string m_currentFontPath;
    float m_basePointSize = 14.0f;
    float m_currentScale = 1.0f;

    void drawEmbeddedFallback(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale);
};
