#include "ui/fontManager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ui/embeddedFont.h"

fontManager& fontManager::getInstance()
{
    static fontManager instance;
    return instance;
}

fontManager::fontManager()
{
    init();
}

fontManager::~fontManager()
{
    cleanup();
}

bool fontManager::init()
{
    if (m_initialized) return true;

    if (!TTF_Init())
    {
        std::cerr << "[FontManager] Warning: TTF_Init failed (" << SDL_GetError() << "). Will use embedded 8x8 pixel font fallback.\n";
        m_initialized = false;
        return false;
    }

    m_initialized = true;
    return true;
}

void fontManager::cleanup()
{
    clearCache();
    if (m_font)
    {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    if (m_initialized)
    {
        TTF_Quit();
        m_initialized = false;
    }
}

bool fontManager::loadFont(const std::string& fontPath, float pointSize)
{
    clearCache();
    m_currentFontPath = fontPath;
    m_basePointSize = pointSize;

    if (!m_initialized && !init())
    {
        return false;
    }

    if (m_font)
    {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }

    if (fontPath.empty())
    {
        std::cout << "[FontManager] No custom font specified. Using embedded 8x8 pixel font fallback.\n";
        return false;
    }

    float effectiveSize = std::max(8.0f, m_basePointSize * m_currentScale);
    m_font = TTF_OpenFont(fontPath.c_str(), effectiveSize);

    if (!m_font)
    {
        std::cout << "[FontManager] Custom font '" << fontPath << "' not found. Gracefully falling back to embedded 8x8 pixel font.\n";
        return false;
    }

    std::cout << "[FontManager] Loaded custom TTF font: " << fontPath << " (size: " << effectiveSize << "pt)\n";
    return true;
}

void fontManager::setScale(float scale)
{
    if (std::abs(m_currentScale - scale) > 0.05f)
    {
        m_currentScale = scale;
        if (!m_currentFontPath.empty() && m_font)
        {
            loadFont(m_currentFontPath, m_basePointSize);
        }
    }
}

void fontManager::setPointSize(float pointSize)
{
    float clampedSize = std::clamp(pointSize, 10.0f, 36.0f);
    if (std::abs(m_basePointSize - clampedSize) > 0.5f)
    {
        m_basePointSize = clampedSize;
        if (!m_currentFontPath.empty() && m_font)
        {
            loadFont(m_currentFontPath, m_basePointSize);
        }
    }
}

float fontManager::getTextWidth(const std::string& text, float scale)
{
    if (text.empty()) return 0.0f;

    auto it = m_textWidthCache.find(text);
    if (it != m_textWidthCache.end())
    {
        float scaleFactor = (m_currentScale > 0.0f) ? (scale / m_currentScale) : 1.0f;
        return it->second * scaleFactor;
    }

    if (m_font)
    {
        int w = 0, h = 0;
        if (TTF_GetStringSize(m_font, text.c_str(), text.length(), &w, &h))
        {
            m_textWidthCache[text] = static_cast<float>(w);
            float scaleFactor = (m_currentScale > 0.0f) ? (scale / m_currentScale) : 1.0f;
            return static_cast<float>(w) * scaleFactor;
        }
    }
    return text.length() * (8.0f * scale);
}

float fontManager::getLineHeight(float scale)
{
    float scaleFactor = (m_currentScale > 0.0f) ? (scale / m_currentScale) : 1.0f;
    if (m_font)
    {
        return static_cast<float>(TTF_GetFontLineSkip(m_font)) * scaleFactor;
    }
    return 14.0f * scale;
}

void fontManager::drawEmbeddedFallback(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale)
{
    float charWidth = 8.0f * scale;
    float charHeight = 10.0f * scale;
    float curX = x;
    float curY = y;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // Handle multi-byte UTF-8 currency symbol ¤ (0xC2 0xA4)
        if (c == 0xC2 && i + 1 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0xA4)
        {
            c = '$';
            i++;
        }

        if (c == '\n')
        {
            curY += (charHeight + (2.0f * scale));
            curX = x;
            continue;
        }

        if (c >= 32 && c < 128)
        {
            for (int row = 0; row < 8; ++row)
            {
                uint8_t rowBits = FONT_8X8[c][row];
                if (!rowBits) continue;

                for (int col = 0; col < 8; ++col)
                {
                    if (rowBits & (0x80 >> col))
                    {
                        SDL_FRect pixelRect = {
                            curX + (col * scale),
                            curY + (row * scale),
                            std::max(1.0f, scale),
                            std::max(1.0f, scale)
                        };
                        SDL_RenderFillRect(renderer, &pixelRect);
                    }
                }
            }
        }

        curX += charWidth;
    }
}

void fontManager::clearCache()
{
    for (auto& [key, glyph] : m_textCache)
    {
        if (glyph.texture)
        {
            SDL_DestroyTexture(glyph.texture);
            glyph.texture = nullptr;
        }
    }
    m_textCache.clear();
    m_textWidthCache.clear();
}

void fontManager::drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale)
{
    if (!renderer || text.empty()) return;

    if (!m_font)
    {
        drawEmbeddedFallback(renderer, text, x, y, color, scale);
        return;
    }

    // Cache Key: string + color
    std::string cacheKey = text + "_" + std::to_string(color.r) + "_" + std::to_string(color.g) + "_" + std::to_string(color.b) + "_" + std::to_string(color.a);
    auto it = m_textCache.find(cacheKey);

    if (it != m_textCache.end() && it->second.texture)
    {
        float scaleFactor = (m_currentScale > 0.0f) ? (scale / m_currentScale) : 1.0f;
        SDL_FRect dstRect = { x, y, static_cast<float>(it->second.width) * scaleFactor, static_cast<float>(it->second.height) * scaleFactor };
        SDL_RenderTexture(renderer, it->second.texture, nullptr, &dstRect);
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.length(), color);
    if (!surface)
    {
        drawEmbeddedFallback(renderer, text, x, y, color, scale);
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        SDL_DestroySurface(surface);
        drawEmbeddedFallback(renderer, text, x, y, color, scale);
        return;
    }

    int surfW = surface->w;
    int surfH = surface->h;
    SDL_DestroySurface(surface);

    if (m_textCache.size() > 8000)
    {
        clearCache();
    }

    CachedGlyph glyph;
    glyph.texture = texture;
    glyph.width = surfW;
    glyph.height = surfH;
    m_textCache[cacheKey] = glyph;

    float scaleFactor = (m_currentScale > 0.0f) ? (scale / m_currentScale) : 1.0f;
    SDL_FRect dstRect = { x, y, static_cast<float>(surfW) * scaleFactor, static_cast<float>(surfH) * scaleFactor };
    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
}

float fontManager::drawTextWrapped(SDL_Renderer* renderer, const std::string& text, float x, float y, float maxWidth, SDL_Color color, float scale)
{
    if (!renderer || text.empty()) return 0.0f;

    std::istringstream stream(text);
    std::string line;
    float curY = y;
    float lineHeight = getLineHeight(scale);

    while (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        std::string word;
        std::string currentLine = "";

        while (lineStream >> word)
        {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            if (getTextWidth(testLine, scale) > maxWidth && !currentLine.empty())
            {
                drawText(renderer, currentLine, x, curY, color, scale);
                curY += lineHeight;
                currentLine = word;
            }
            else
            {
                currentLine = testLine;
            }
        }

        if (!currentLine.empty())
        {
            drawText(renderer, currentLine, x, curY, color, scale);
            curY += lineHeight;
        }
    }

    return (curY - y);
}

float fontManager::getTextWrappedHeight(const std::string& text, float maxWidth, float scale)
{
    if (text.empty() || maxWidth <= 0.0f) return 0.0f;

    std::istringstream stream(text);
    std::string line;
    float totalH = 0.0f;
    float lineHeight = getLineHeight(scale);

    while (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        std::string word;
        std::string currentLine = "";

        while (lineStream >> word)
        {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            if (getTextWidth(testLine, scale) > maxWidth && !currentLine.empty())
            {
                totalH += lineHeight;
                currentLine = word;
            }
            else
            {
                currentLine = testLine;
            }
        }

        if (!currentLine.empty())
        {
            totalH += lineHeight;
        }
    }

    return totalH;
}
