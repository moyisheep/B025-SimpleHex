#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <SDL2\SDL_render.h>
#include "TextRenderer.h"
#include "ColorConst.h"



// 添加一个辅助函数来处理时间格式化
inline std::tm safe_localtime(const std::time_t& time_t) {
#ifdef _WIN32
    std::tm tm_info;
    localtime_s(&tm_info, &time_t);
    return tm_info;
#else
    std::tm tm_info;
    localtime_r(&time_t, &tm_info);
    return tm_info;
#endif
}



// UI工具类
class UIUtils {
public:
    static void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);
    static void drawButton(SDL_Renderer* renderer, SDL_Rect rect, const std::string& text,
        TextRenderer* textRenderer, bool isHovered = false);
    static void drawProgressBar(SDL_Renderer* renderer, SDL_Rect rect, float progress);
    static SDL_Color blendColors(SDL_Color c1, SDL_Color c2, float alpha);
    static bool isPointInRect(int x, int y, const SDL_Rect& rect);
    static SDL_Rect getCenteredRect(int parentWidth, int parentHeight, int width, int height);
    static std::string formatFileSize(size_t bytes);
    static std::string formatTime(std::chrono::system_clock::time_point time);
};
