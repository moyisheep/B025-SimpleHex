#include "UIUtils.h"

// ==================== UIUtils 实现 ====================
void UIUtils::drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // 绘制四个角的圆弧
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                // 左上角
                SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + dy);
                // 右上角
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - radius + dx, rect.y + dy);
                // 左下角
                SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + rect.h - radius + dy);
                // 右下角
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - radius + dx, rect.y + rect.h - radius + dy);
            }
        }
    }

    // 绘制填充矩形
    SDL_Rect fillRect = {
        rect.x, rect.y + radius,
        rect.w, rect.h - radius * 2
    };
    SDL_RenderFillRect(renderer, &fillRect);

    fillRect = {
        rect.x + radius, rect.y,
        rect.w - radius * 2, rect.h
    };
    SDL_RenderFillRect(renderer, &fillRect);
}

void UIUtils::drawButton(SDL_Renderer* renderer, SDL_Rect rect, const std::string& text,
    TextRenderer* textRenderer, bool isHovered) {
    SDL_Color bgColor = isHovered ? COLOR_BUTTON_HOVER : COLOR_BUTTON;
    SDL_Color borderColor = isHovered ? COLOR_ACCENT_BLUE : COLOR_BORDER;

    // 绘制背景
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &rect);

    // 绘制边框
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &rect);

    // 计算文字居中位置
    SDL_Point textSize = textRenderer->getSize(text);
    int textX = rect.x + (rect.w - textSize.x) / 2;
    int textY = rect.y + (rect.h - textSize.y) / 2;

    // 绘制文字
    textRenderer->renderWithShadow(text, textX, textY, COLOR_TEXT_MAIN);
}

void UIUtils::drawProgressBar(SDL_Renderer* renderer, SDL_Rect rect, float progress) {
    // 绘制背景
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 200);
    SDL_RenderFillRect(renderer, &rect);

    // 绘制进度条
    SDL_Rect progressRect = rect;
    progressRect.w = static_cast<int>(rect.w * progress);
    SDL_SetRenderDrawColor(renderer, COLOR_ACCENT_BLUE.r, COLOR_ACCENT_BLUE.g,
        COLOR_ACCENT_BLUE.b, 180);
    SDL_RenderFillRect(renderer, &progressRect);

    // 绘制边框
    SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
        COLOR_BORDER.b, 200);
    SDL_RenderDrawRect(renderer, &rect);
}

SDL_Color UIUtils::blendColors(SDL_Color c1, SDL_Color c2, float alpha) {
    return {
        static_cast<Uint8>(c1.r * (1 - alpha) + c2.r * alpha),
        static_cast<Uint8>(c1.g * (1 - alpha) + c2.g * alpha),
        static_cast<Uint8>(c1.b * (1 - alpha) + c2.b * alpha),
        static_cast<Uint8>(c1.a * (1 - alpha) + c2.a * alpha)
    };
}

bool UIUtils::isPointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x <= rect.x + rect.w &&
        y >= rect.y && y <= rect.y + rect.h;
}

SDL_Rect UIUtils::getCenteredRect(int parentWidth, int parentHeight, int width, int height) {
    return {
        (parentWidth - width) / 2,
        (parentHeight - height) / 2,
        width,
        height
    };
}

std::string UIUtils::formatFileSize(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

std::string UIUtils::formatTime(std::chrono::system_clock::time_point time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm_info = safe_localtime(time_t);
    std::stringstream ss;
    ss << std::put_time(&tm_info, "%H:%M:%S");
    return ss.str();
}
