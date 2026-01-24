#ifndef MAIN_H
#define MAIN_H

// 解决中文乱码问题
#pragma execution_character_set("utf-8")

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// 常量定义
const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;
const int HEX_VIEW_X = 60;
const int HEX_VIEW_Y = 100;
const int BYTES_PER_LINE = 16;
const int BYTE_WIDTH = 28;
const int BYTE_HEIGHT = 28;
const int ASCII_WIDTH = 16;
const int ADDRESS_WIDTH = 100;
const int ADDRESS_HEIGHT = 28;
const int LINE_HEIGHT = 28;
const int SCROLLBAR_WIDTH = 16;
const int UI_PADDING = 15;

// 专业暗色主题（护眼）
const SDL_Color COLOR_BG = { 18, 18, 24, 255 };           // 深灰背景
const SDL_Color COLOR_PANEL = { 28, 28, 36, 255 };       // 面板背景
const SDL_Color COLOR_BORDER = { 60, 60, 80, 255 };      // 边框色
const SDL_Color COLOR_TEXT_MAIN = { 220, 220, 220, 255 };// 主文字
const SDL_Color COLOR_TEXT_SECONDARY = { 160, 160, 180, 255 };// 次要文字
const SDL_Color COLOR_ACCENT_BLUE = { 80, 150, 255, 255 };// 强调蓝色
const SDL_Color COLOR_ACCENT_GREEN = { 100, 220, 140, 255 };// 绿色
const SDL_Color COLOR_ACCENT_PURPLE = { 180, 120, 255, 255 };// 紫色
const SDL_Color COLOR_SELECTION = { 40, 100, 180, 120 }; // 选中背景
const SDL_Color COLOR_HIGHLIGHT = { 255, 200, 60, 80 };  // 标记高亮
const SDL_Color COLOR_HIGHLIGHT_BORDER = { 255, 180, 40, 180 }; // 标记边框
const SDL_Color COLOR_SCROLLBAR = { 70, 70, 90, 200 };   // 滚动条
const SDL_Color COLOR_SCROLLBAR_HOVER = { 90, 90, 120, 220 };// 滚动条悬停
const SDL_Color COLOR_BUTTON = { 50, 100, 200, 180 };    // 按钮色
const SDL_Color COLOR_BUTTON_HOVER = { 70, 130, 230, 220 };// 按钮悬停
const SDL_Color COLOR_DROP_HIGHLIGHT = { 40, 100, 200, 80 };// 拖拽高亮


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

// 标记结构体
struct Highlight {
    size_t start;
    size_t end;
    std::string description;
    SDL_Color color;
    std::string time;

    Highlight(size_t s, size_t e, const std::string& desc, SDL_Color col)
        : start(s), end(e), description(desc), color(col) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_info = safe_localtime(time_t);
        std::stringstream ss;
        ss << std::put_time(&tm_info, "%H:%M:%S");
        time = ss.str();
    }
};

// 文件数据类
class FileData {
private:
    std::vector<uint8_t> data;
    std::string filepath;
    std::string filename;
    size_t filesize;

public:
    FileData();
    bool load(const std::string& path);
    bool loadFromMemory(const std::vector<uint8_t>& newData);
    size_t size() const;
    uint8_t operator[](size_t index) const;
    std::string getFilePath() const;
    std::string getFileName() const;
    std::string getFileSizeFormatted() const;
    std::vector<uint8_t> getDataSlice(size_t start, size_t end) const;
    std::string getHexString(size_t start, size_t length) const;
    std::string getAsciiString(size_t start, size_t length) const;
};

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

// 主应用类
class HexViewer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    FileData fileData;

    // UI状态
    int scrollOffset;
    int totalLines;
    int visibleLines;
    bool isDraggingScrollbar;
    int scrollbarDragStartY;
    int scrollbarStartOffset;

    // 选择状态
    size_t selectionStart;
    size_t selectionEnd;
    bool isSelecting;
    bool hasSelection;

    // 标记管理
    std::vector<Highlight> highlights;
    Highlight* hoveredHighlight;
    int selectedHighlightIndex;

    // 输入状态
    std::string descriptionInput;
    bool isEditingDescription;
    SDL_Rect descriptionInputRect;
    SDL_Rect addButtonRect;
    SDL_Rect clearButtonRect;

    // 拖拽
    bool isFileDraggedOver;
    std::string draggedFilePath;
    SDL_Rect dropHighlightRect;

    // 鼠标状态
    int mouseX, mouseY;
    bool isMouseOverHex;
    size_t hoveredByte;

    // 窗口状态
    bool isRunning;
    float zoomLevel;
    bool showAscii;
    bool showHex;

    // 性能
    std::chrono::steady_clock::time_point lastFrameTime;
    float fps;

    // 私有方法
    void updateVisibleLines();
    void renderStatusBar();
    void renderSidePanel();
    void renderHexGrid();
    void renderByteCell(int x, int y, size_t byteIndex, uint8_t byte);
    void updateHoverState();
    size_t getByteIndexAt(int x, int y) const;
    void handleScrollbarClick(int y);
    void handleHexAreaClick();
    void addCurrentHighlight();
    void removeSelectedHighlight();
    void exportHighlights();
    void importHighlights();
    void copySelectionToClipboard();
    void goToOffset();
    void searchHex();
    void searchText();

public:
    HexViewer();
    ~HexViewer();

    bool init();
    bool loadFile(const std::string& path);
    void clearSelection();
    void setSelection(size_t start, size_t end);
    void run();
    void render();
    void handleEvent(SDL_Event& event);
    void cleanup();

    // Getter/Setter
    bool isFileLoaded() const { return fileData.size() > 0; }
    std::string getStatusText() const;
};

#endif // MAIN_H