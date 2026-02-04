#pragma once

#include <SDL2/SDL_ttf.h>
#include <string>
#include <map>
#include <iostream>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;
// 文本渲染器
class TextRenderer {
private:
    TTF_Font* fontRegular;
    TTF_Font* fontBold;
    TTF_Font* fontMono;
    SDL_Renderer* renderer;
    std::map<std::pair<std::string, int>, TTF_Font*> fontCache;
    // 纹理缓存结构
    struct TextureCacheEntry {
        SDL_Texture* texture;
        int width;
        int height;
        Uint32 lastUsedTime;
    };

    struct TextureKey {
        std::string text;
        SDL_Color color;
        TTF_Font* font;

        bool operator==(const TextureKey& other) const {
            return text == other.text &&
                color.r == other.color.r &&
                color.g == other.color.g &&
                color.b == other.color.b &&
                color.a == other.color.a &&
                font == other.font;
        }
    };

    struct TextureKeyHash {
        size_t operator()(const TextureKey& k) const {
            size_t h1 = std::hash<std::string>{}(k.text);
            size_t h2 = std::hash<int>{}(k.color.r);
            size_t h3 = std::hash<int>{}(k.color.g);
            size_t h4 = std::hash<int>{}(k.color.b);
            size_t h5 = std::hash<int>{}(k.color.a);
            size_t h6 = std::hash<TTF_Font*>{}(k.font);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
        }
    };

    std::unordered_map<TextureKey, TextureCacheEntry, TextureKeyHash> textureCache;
    const size_t MAX_CACHE_SIZE = 500; // 最大缓存纹理数量
    Uint32 lastCleanupTime = 0;
    const Uint32 CLEANUP_INTERVAL = 10000; // 10秒清理一次

    // 私有方法
    void cleanupCache();
    void addToCache(const std::string& text, SDL_Color color,
        TTF_Font* font, SDL_Texture* texture, int w, int h);
    SDL_Texture* getFromCache(const std::string& text, SDL_Color color, TTF_Font* font);
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
