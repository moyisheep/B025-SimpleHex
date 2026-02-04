#pragma once

#include <SDL2/SDL_ttf.h>
#include <string>
#include <map>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
// 文本渲染器
class TextRenderer {
private:
    TTF_Font* fontRegular;
    TTF_Font* fontBold;
    TTF_Font* fontMono;
    SDL_Renderer* renderer;
    std::map<std::pair<std::string, int>, TTF_Font*> fontCache;

public:
    TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer();

    bool init();
    void render(const std::string& text, int x, int y, SDL_Color color,
        TTF_Font* font = nullptr, bool isMono = false);
    void renderWithShadow(const std::string& text, int x, int y, SDL_Color color,
        int shadowOffset = 1);
    SDL_Point getSize(const std::string& text, TTF_Font* font = nullptr);
    SDL_Texture* createTextTexture(const std::string& text, SDL_Color color,
        TTF_Font* font = nullptr);
    bool isAvailable() const;

    TTF_Font* getRegularFont() const { return fontRegular; }
    TTF_Font* getMonoFont() const { return fontMono; }
};
