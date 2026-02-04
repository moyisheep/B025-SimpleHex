#include "TextRenderer.h"

// ==================== TextRenderer 实现 ====================
TextRenderer::TextRenderer(SDL_Renderer* renderer)
    : renderer(renderer), fontRegular(nullptr), fontBold(nullptr), fontMono(nullptr) {}

TextRenderer::~TextRenderer() {
    for (auto& [key, font] : fontCache) {
        if (font) TTF_CloseFont(font);
    }
    fontCache.clear();

    if (fontRegular) TTF_CloseFont(fontRegular);
    if (fontBold) TTF_CloseFont(fontBold);
    if (fontMono) TTF_CloseFont(fontMono);
    TTF_Quit();
}

bool TextRenderer::init() {
    if (TTF_Init() == -1) {
        std::cerr << "TTF初始化失败: " << TTF_GetError() << std::endl;
        return false;
    }

    // 尝试加载不同字体
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",           // 微软雅黑
        "C:\\Windows\\Fonts\\segoeui.ttf",        // Segoe UI
        "C:\\Windows\\Fonts\\arial.ttf",          // Arial
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux
        "/System/Library/Fonts/SFNS.ttf",         // macOS
        nullptr
    };

    const char* monoPaths[] = {
        "C:\\Windows\\Fonts\\consola.ttf",        // Consola
        "C:\\Windows\\Fonts\\lucon.ttf",          // Lucida Console
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        nullptr
    };

    // 加载常规字体
    for (int i = 0; fontPaths[i]; i++) {
        if (fs::exists(fontPaths[i])) {
            fontRegular = TTF_OpenFont(fontPaths[i], 16);
            if (fontRegular) break;
        }
    }

    // 加载等宽字体
    for (int i = 0; monoPaths[i]; i++) {
        if (fs::exists(monoPaths[i])) {
            fontMono = TTF_OpenFont(monoPaths[i], 14);
            if (fontMono) break;
        }
    }

    // 如果没找到等宽字体，使用常规字体
    if (!fontMono && fontRegular) {
        fontMono = fontRegular;
    }

    return fontRegular != nullptr;
}

void TextRenderer::render(const std::string& text, int x, int y, SDL_Color color,
    TTF_Font* font, bool isMono) {
    TTF_Font* useFont = font ? font : (isMono ? fontMono : fontRegular);
    if (!useFont) return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(useFont, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void TextRenderer::renderWithShadow(const std::string& text, int x, int y, SDL_Color color,
    int shadowOffset) {
    // 绘制阴影
    render(text, x + shadowOffset, y + shadowOffset, { 0, 0, 0, 150 });
    // 绘制前景文字
    render(text, x, y, color);
}

SDL_Point TextRenderer::getSize(const std::string& text, TTF_Font* font) {
    SDL_Point size = { 0, 0 };
    TTF_Font* useFont = font ? font : fontRegular;
    if (!useFont) return size;

    TTF_SizeUTF8(useFont, text.c_str(), &size.x, &size.y);
    return size;
}

SDL_Texture* TextRenderer::createTextTexture(const std::string& text, SDL_Color color,
    TTF_Font* font) {
    TTF_Font* useFont = font ? font : fontRegular;
    if (!useFont) return nullptr;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(useFont, text.c_str(), color);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

bool TextRenderer::isAvailable() const {
    return fontRegular != nullptr;
}