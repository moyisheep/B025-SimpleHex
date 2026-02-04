#include "TextRenderer.h"

// ==================== TextRenderer 实现 ====================
TextRenderer::TextRenderer(SDL_Renderer* renderer)
    : renderer(renderer), fontRegular(nullptr), fontBold(nullptr), fontMono(nullptr) {}

TextRenderer::~TextRenderer() {
    // 清理纹理缓存
    for (auto& [key, entry] : textureCache) {
        SDL_DestroyTexture(entry.texture);
    }
    textureCache.clear();

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

// 核心优化：修改render函数
void TextRenderer::render(const std::string& text, int x, int y, SDL_Color color,
    TTF_Font* font, bool isMono) {
    TTF_Font* useFont = font ? font : (isMono ? fontMono : fontRegular);
    if (!useFont) return;

    // 定期清理缓存
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastCleanupTime > CLEANUP_INTERVAL) {
        cleanupCache();
        lastCleanupTime = currentTime;
    }

    // 尝试从缓存获取纹理
    SDL_Texture* texture = getFromCache(text, color, useFont);
    int w = 0, h = 0;

    if (texture) {
        // 从缓存命中，获取尺寸
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    }
    else {
        // 缓存未命中，创建新纹理
        SDL_Surface* surface = TTF_RenderUTF8_Blended(useFont, text.c_str(), color);
        if (!surface) return;

        texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }

        w = surface->w;
        h = surface->h;

        // 添加到缓存
        addToCache(text, color, useFont, texture, w, h);

        SDL_FreeSurface(surface);
    }

    SDL_Rect dstRect = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
}

void TextRenderer::cleanupCache() {
    Uint32 currentTime = SDL_GetTicks();

    // 只清理超过60秒未使用的纹理
    auto it = textureCache.begin();
    while (it != textureCache.end()) {
        if (currentTime - it->second.lastUsedTime > 60000) { // 60秒
            SDL_DestroyTexture(it->second.texture);
            it = textureCache.erase(it);
        }
        else {
            ++it;
        }
    }
}

// 修改addToCache，添加前检查缓存大小
void TextRenderer::addToCache(const std::string& text, SDL_Color color,
    TTF_Font* font, SDL_Texture* texture, int w, int h) {
    // 添加前如果缓存已满，进行一次清理
    if (textureCache.size() >= MAX_CACHE_SIZE) {
        cleanupCache();
    }

    TextureKey key{ text, color, font };
    textureCache[key] = { texture, w, h, SDL_GetTicks() };
}

SDL_Texture* TextRenderer::getFromCache(const std::string& text, SDL_Color color, TTF_Font* font) {
    TextureKey key{ text, color, font };
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        it->second.lastUsedTime = SDL_GetTicks(); // 更新使用时间
        return it->second.texture;
    }
    return nullptr;
}

// 优化createTextTexture，也使用缓存
SDL_Texture* TextRenderer::createTextTexture(const std::string& text, SDL_Color color,
    TTF_Font* font) {
    TTF_Font* useFont = font ? font : fontRegular;
    if (!useFont) return nullptr;

    // 尝试从缓存获取
    SDL_Texture* texture = getFromCache(text, color, useFont);
    if (texture) return texture;

    // 创建新纹理
    SDL_Surface* surface = TTF_RenderUTF8_Blended(useFont, text.c_str(), color);
    if (!surface) return nullptr;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        addToCache(text, color, useFont, texture, surface->w, surface->h);
    }

    SDL_FreeSurface(surface);
    return texture;
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



bool TextRenderer::isAvailable() const {
    return fontRegular != nullptr;
}