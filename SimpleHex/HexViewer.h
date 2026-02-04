#pragma once

#include <vector>
#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "TextRenderer.h"
#include "FileData.h"
#include "UIUtils.h"
#include "ColorConst.h"

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