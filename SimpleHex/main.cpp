// HexViewer_SDL2.cpp
// 纯SDL2实现的16进制阅读器
// 支持大文件、高亮标记、鼠标悬停信息显示
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <functional>

// ============================================================================
// 配置常量
// ============================================================================
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int BYTES_PER_LINE = 16;
const int HEX_VIEW_WIDTH = 600;
const int ASCII_VIEW_WIDTH = 200;
const int ADDRESS_WIDTH = 100;
const int LINE_HEIGHT = 20;
const int MARGIN = 10;
const int STATUS_BAR_HEIGHT = 30;
const int CONTROL_PANEL_HEIGHT = 80;
const int FONT_SIZE = 14;
const int CACHE_BLOCK_SIZE = 4096;

// ============================================================================
// 颜色定义
// ============================================================================
struct Color {
    uint8_t r, g, b, a;

    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    SDL_Color toSDLColor() const {
        return { r, g, b, a };
    }
};

const Color COLOR_BACKGROUND = { 240, 240, 240, 255 };
const Color COLOR_TEXT = { 0, 0, 0, 255 };
const Color COLOR_SELECTION = { 100, 150, 250, 100 };
const Color COLOR_HIGHLIGHT_BASE = { 255, 255, 0, 100 };
const Color COLOR_ADDRESS = { 100, 100, 200, 255 };
const Color COLOR_STATUS_BAR = { 220, 220, 220, 255 };
const Color COLOR_BUTTON_NORMAL = { 200, 200, 200, 255 };
const Color COLOR_BUTTON_HOVER = { 180, 180, 180, 255 };
const Color COLOR_BUTTON_ACTIVE = { 160, 160, 160, 255 };
const Color COLOR_TEXTBOX_BACKGROUND = { 255, 255, 255, 255 };
const Color COLOR_TEXTBOX_BORDER = { 150, 150, 150, 255 };
const Color COLOR_SCROLLBAR = { 200, 200, 200, 255 };
const Color COLOR_SCROLLBAR_THUMB = { 150, 150, 150, 255 };

// ============================================================================
// 数据结构
// ============================================================================
struct FileBlock {
    size_t offset;
    std::vector<uint8_t> data;

    FileBlock(size_t off = 0) : offset(off) {}

    bool contains(size_t pos) const {
        return pos >= offset && pos < offset + data.size();
    }

    uint8_t getByte(size_t pos) const {
        if (pos >= offset && pos < offset + data.size()) {
            return data[pos - offset];
        }
        return 0;
    }
};

struct Highlight {
    size_t start;
    size_t end;
    Color color;
    std::string description;

    Highlight(size_t s = 0, size_t e = 0,
        const Color& c = COLOR_HIGHLIGHT_BASE,
        const std::string& desc = "")
        : start(s), end(e), color(c), description(desc) {}

    bool contains(size_t pos) const {
        return pos >= start && pos <= end;
    }
};

struct UIState {
    int mouseX = 0;
    int mouseY = 0;
    bool mouseLeftDown = false;
    bool mouseRightDown = false;
    bool needsRedraw = true;

    std::string statusText = "Ready";
    std::string hoverText = "";

    // 滚动
    int scrollOffset = 0;
    int maxScrollOffset = 0;

    // 选择
    size_t selectionStart = 0;
    size_t selectionEnd = 0;
    bool isSelecting = false;

    // 输入状态
    bool typingOffset = false;
    bool typingSearch = false;
    bool typingDescription = false;

    // 当前聚焦的UI元素
    enum FocusType { NONE, OFFSET_BOX, SEARCH_BOX, DESC_BOX, ADD_BUTTON };
    FocusType focus = NONE;
};

// ============================================================================
// 文件处理器类
// ============================================================================
class FileHandler {
private:
    std::string filePath;
    std::ifstream fileStream;
    size_t fileSize = 0;
    std::map<size_t, FileBlock> cache;

public:
    FileHandler() = default;

    ~FileHandler() {
        close();
    }

    bool open(const std::string& path) {
        close();

        try {
            fileStream.open(path, std::ios::binary | std::ios::ate);
            if (!fileStream.is_open()) {
                return false;
            }

            filePath = path;
            fileSize = static_cast<size_t>(fileStream.tellg());
            fileStream.seekg(0, std::ios::beg);

            cache.clear();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void close() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
        filePath.clear();
        fileSize = 0;
        cache.clear();
    }

    bool isOpen() const {
        return fileStream.is_open();
    }

    size_t getFileSize() const {
        return fileSize;
    }

    std::string getFilePath() const {
        return filePath;
    }

    std::vector<uint8_t> readBlock(size_t offset, size_t size) {
        if (!isOpen() || offset >= fileSize) {
            return {};
        }

        // 检查缓存
        size_t blockIndex = offset / CACHE_BLOCK_SIZE;
        size_t blockOffset = blockIndex * CACHE_BLOCK_SIZE;

        auto it = cache.find(blockOffset);
        if (it != cache.end()) {
            return it->second.data;
        }

        // 从文件读取
        size_t readSize = std::min(static_cast<size_t>(CACHE_BLOCK_SIZE), fileSize - blockOffset);
        std::vector<uint8_t> buffer(readSize);

        fileStream.seekg(static_cast<std::streamoff>(blockOffset));
        fileStream.read(reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(readSize));

        // 存入缓存
        FileBlock block(blockOffset);
        block.data = std::move(buffer);
        cache[blockOffset] = std::move(block);

        // 限制缓存大小
        if (cache.size() > 100) {
            cache.erase(cache.begin());
        }

        return cache[blockOffset].data;
    }

    uint8_t getByte(size_t offset) {
        if (!isOpen() || offset >= fileSize) {
            return 0;
        }

        size_t blockOffset = (offset / CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE;

        // 确保块已加载
        if (cache.find(blockOffset) == cache.end()) {
            readBlock(blockOffset, CACHE_BLOCK_SIZE);
        }

        auto it = cache.find(blockOffset);
        if (it != cache.end()) {
            return it->second.getByte(offset);
        }

        return 0;
    }
};

// ============================================================================
// 简单UI组件基类
// ============================================================================
class UIComponent {
protected:
    SDL_Rect rect;
    bool visible = true;
    bool enabled = true;

public:
    UIComponent(int x, int y, int w, int h)
        : rect{ x, y, w, h } {}

    virtual ~UIComponent() = default;

    virtual void render(SDL_Renderer* renderer, TTF_Font* font) = 0;
    virtual void handleEvent(SDL_Event* event, UIState& state) = 0;
    virtual void update(UIState& state) = 0;

    void setPosition(int x, int y) {
        rect.x = x;
        rect.y = y;
    }

    void setSize(int w, int h) {
        rect.w = w;
        rect.h = h;
    }

    void setVisible(bool v) { visible = v; }
    void setEnabled(bool e) { enabled = e; }

    bool isPointInside(int x, int y) const {
        return x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h;
    }

    const SDL_Rect& getRect() const { return rect; }
    bool isVisible() const { return visible; }
    bool isEnabled() const { return enabled; }
};

// ============================================================================
// 按钮类
// ============================================================================
class Button : public UIComponent {
private:
    std::string text;
    std::function<void()> onClick;
    bool hovered = false;
    bool pressed = false;

public:
    Button(const std::string& text, int x, int y, int w, int h)
        : UIComponent(x, y, w, h), text(text) {}

    void setText(const std::string& t) { text = t; }
    void setOnClick(std::function<void()> callback) { onClick = callback; }

    void render(SDL_Renderer* renderer, TTF_Font* font) override {
        if (!visible) return;

        // 绘制背景
        Color bgColor;
        if (!enabled) {
            bgColor = COLOR_BUTTON_NORMAL;
        }
        else if (pressed) {
            bgColor = COLOR_BUTTON_ACTIVE;
        }
        else if (hovered) {
            bgColor = COLOR_BUTTON_HOVER;
        }
        else {
            bgColor = COLOR_BUTTON_NORMAL;
        }

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        // 绘制边框
        SDL_SetRenderDrawColor(renderer, COLOR_TEXTBOX_BORDER.r,
            COLOR_TEXTBOX_BORDER.g, COLOR_TEXTBOX_BORDER.b, 255);
        SDL_RenderDrawRect(renderer, &rect);

        // 绘制文本
        if (font && !text.empty()) {
            SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(),
                COLOR_TEXT.toSDLColor());
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    int texW, texH;
                    SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);

                    SDL_Rect textRect = {
                        rect.x + (rect.w - texW) / 2,
                        rect.y + (rect.h - texH) / 2,
                        texW,
                        texH
                    };
                    SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                    SDL_DestroyTexture(texture);
                }
                SDL_FreeSurface(surface);
            }
        }
    }

    void handleEvent(SDL_Event* event, UIState& state) override {
        if (!visible || !enabled) return;

        switch (event->type) {
        case SDL_MOUSEMOTION:
            hovered = isPointInside(event->motion.x, event->motion.y);
            state.needsRedraw = hovered != (event->motion.x == state.mouseX &&
                event->motion.y == state.mouseY);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                pressed = isPointInside(event->button.x, event->button.y);
                state.needsRedraw = pressed;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT && pressed) {
                if (isPointInside(event->button.x, event->button.y) && onClick) {
                    onClick();
                }
                pressed = false;
                state.needsRedraw = true;
            }
            break;
        }
    }

    void update(UIState& state) override {
        hovered = isPointInside(state.mouseX, state.mouseY);
    }
};

// ============================================================================
// 文本框类
// ============================================================================
class TextBox : public UIComponent {
private:
    std::string text;
    std::string placeholder;
    bool focused = false;
    bool hovered = false;
    Uint32 lastBlink = 0;
    bool cursorVisible = false;
    int cursorPos = 0;
    int scrollX = 0;

public:
    TextBox(const std::string& placeholder, int x, int y, int w, int h)
        : UIComponent(x, y, w, h), placeholder(placeholder) {}

    const std::string& getText() const { return text; }
    void setText(const std::string& t) { text = t; cursorPos = text.length(); }
    bool isFocused() const { return focused; }
    void setFocused(bool f) { focused = f; }

    void render(SDL_Renderer* renderer, TTF_Font* font) override {
        if (!visible) return;

        // 绘制背景
        Color bgColor = focused ? Color(255, 255, 255, 255) : COLOR_TEXTBOX_BACKGROUND;
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        // 绘制边框
        SDL_SetRenderDrawColor(renderer,
            focused ? 100 : COLOR_TEXTBOX_BORDER.r,
            focused ? 150 : COLOR_TEXTBOX_BORDER.g,
            focused ? 250 : COLOR_TEXTBOX_BORDER.b,
            255);
        SDL_RenderDrawRect(renderer, &rect);

        // 绘制文本
        if (font) {
            std::string displayText = text.empty() && !focused ? placeholder : text;
            if (!displayText.empty()) {
                // 计算可见文本
                SDL_Surface* surface = TTF_RenderUTF8_Blended(font, displayText.c_str(),
                    COLOR_TEXT.toSDLColor());
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (texture) {
                        SDL_Rect textRect = {
                            rect.x + 5 - scrollX,
                            rect.y + (rect.h - surface->h) / 2,
                            surface->w,
                            surface->h
                        };

                        // 裁剪
                        SDL_Rect clipRect = rect;
                        clipRect.x += 2;
                        clipRect.y += 2;
                        clipRect.w -= 4;
                        clipRect.h -= 4;

                        SDL_RenderSetClipRect(renderer, &clipRect);
                        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                        SDL_RenderSetClipRect(renderer, nullptr);

                        SDL_DestroyTexture(texture);
                    }
                    SDL_FreeSurface(surface);
                }
            }

            // 绘制光标
            if (focused && cursorVisible) {
                Uint32 now = SDL_GetTicks();
                if (now - lastBlink > 500) {
                    cursorVisible = !cursorVisible;
                    lastBlink = now;
                }

                if (cursorVisible) {
                    // 计算光标位置
                    std::string beforeCursor = text.substr(0, cursorPos);
                    int cursorX = rect.x + 5;

                    if (!beforeCursor.empty()) {
                        int w, h;
                        TTF_SizeText(font, beforeCursor.c_str(), &w, &h);
                        cursorX += w;
                    }

                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDrawLine(renderer, cursorX, rect.y + 5,
                        cursorX, rect.y + rect.h - 5);
                }
            }
        }
    }

    void handleEvent(SDL_Event* event, UIState& state) override {
        if (!visible || !enabled) return;

        switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                focused = isPointInside(event->button.x, event->button.y);
                state.needsRedraw = true;

                if (focused) {
                    // 设置光标位置
                    // 这里简化处理，实际应该计算精确的光标位置
                    cursorPos = text.length();
                }
            }
            break;

        case SDL_TEXTINPUT:
            if (focused) {
                text.insert(cursorPos, event->text.text);
                cursorPos += strlen(event->text.text);
                state.needsRedraw = true;
            }
            break;

        case SDL_KEYDOWN:
            if (focused) {
                switch (event->key.keysym.sym) {
                case SDLK_BACKSPACE:
                    if (!text.empty() && cursorPos > 0) {
                        text.erase(cursorPos - 1, 1);
                        cursorPos--;
                        state.needsRedraw = true;
                    }
                    break;
                case SDLK_DELETE:
                    if (cursorPos < text.length()) {
                        text.erase(cursorPos, 1);
                        state.needsRedraw = true;
                    }
                    break;
                case SDLK_LEFT:
                    if (cursorPos > 0) {
                        cursorPos--;
                        state.needsRedraw = true;
                    }
                    break;
                case SDLK_RIGHT:
                    if (cursorPos < text.length()) {
                        cursorPos++;
                        state.needsRedraw = true;
                    }
                    break;
                case SDLK_HOME:
                    cursorPos = 0;
                    state.needsRedraw = true;
                    break;
                case SDLK_END:
                    cursorPos = text.length();
                    state.needsRedraw = true;
                    break;
                case SDLK_RETURN:
                    // 回车事件由外部处理
                    state.needsRedraw = true;
                    break;
                }
            }
            break;
        }
    }

    void update(UIState& state) override {
        if (focused) {
            hovered = true;
            // 更新光标闪烁
            Uint32 now = SDL_GetTicks();
            if (now - lastBlink > 500) {
                cursorVisible = !cursorVisible;
                lastBlink = now;
                state.needsRedraw = true;
            }
        }
        else {
            hovered = isPointInside(state.mouseX, state.mouseY);
        }
    }
};

// ============================================================================
// 主应用程序类
// ============================================================================
class HexViewerApp {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    FileHandler fileHandler;
    UIState uiState;

    // UI组件
    std::vector<std::unique_ptr<UIComponent>> uiComponents;
    TextBox* offsetBox = nullptr;
    TextBox* searchBox = nullptr;
    TextBox* descriptionBox = nullptr;
    Button* addButton = nullptr;

    // 数据
    std::vector<Highlight> highlights;
    std::vector<std::string> annotationList;
    Color currentHighlightColor = COLOR_HIGHLIGHT_BASE;

    // 文件信息
    std::string currentFile;
    size_t totalLines = 0;

public:
    HexViewerApp() = default;

    ~HexViewerApp() {
        cleanup();
    }

    bool initialize() {
        // 初始化SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return false;
        }

        // 初始化TTF
        if (TTF_Init() < 0) {
            std::cerr << "TTF初始化失败: " << TTF_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        // 创建窗口
        window = SDL_CreateWindow("Hex Viewer - SDL2",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "窗口创建失败: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        // 创建渲染器
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "渲染器创建失败: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        // 加载字体
        font = TTF_OpenFont("Fonts/AlibabaPuHuiTi-3-55-Regular.ttf", FONT_SIZE);
        if (!font) {
     

            std::cerr << "字体加载失败: " << TTF_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return false;
   
        }

        // 初始化UI
        createUI();

        return true;
    }

    void run() {
        bool running = true;
        SDL_Event event;
        Uint32 lastRenderTime = SDL_GetTicks();

        // 启用文本输入
        SDL_StartTextInput();

        while (running) {
            // 处理事件
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    else if (event.key.keysym.sym == SDLK_RETURN) {
                        handleReturnKey();
                    }
                }

                // 更新鼠标位置
                if (event.type == SDL_MOUSEMOTION) {
                    uiState.mouseX = event.motion.x;
                    uiState.mouseY = event.motion.y;
                    uiState.needsRedraw = true;
                }

                // 更新鼠标按钮状态
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        uiState.mouseLeftDown = true;
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        uiState.mouseRightDown = true;
                    }
                }

                if (event.type == SDL_MOUSEBUTTONUP) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        uiState.mouseLeftDown = false;
                        uiState.isSelecting = false;
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        uiState.mouseRightDown = false;
                    }
                }

                // 鼠标滚轮
                if (event.type == SDL_MOUSEWHEEL) {
                    handleMouseWheel(event.wheel.y);
                }

                // 处理UI事件
                for (auto& component : uiComponents) {
                    component->handleEvent(&event, uiState);
                }

                // 处理Hex视图事件
                handleHexViewEvent(&event);
            }

            // 更新状态
            update();

            // 渲染（只在需要时）
            if (uiState.needsRedraw || SDL_GetTicks() - lastRenderTime > 1000 / 60) {
                render();
                lastRenderTime = SDL_GetTicks();
                uiState.needsRedraw = false;
            }

            // 限制帧率
            SDL_Delay(1);
        }

        // 禁用文本输入
        SDL_StopTextInput();
    }

private:
    void createUI() {
        // 创建控制面板组件
        int y = 5;

        // 偏移输入框
        auto offsetLabel = new TextBox("Enter offset (hex)", 120, y, 200, 25);
        offsetLabel->setText("0");
        offsetBox = offsetLabel;
        uiComponents.emplace_back(offsetLabel);

        // 搜索框
        auto searchBoxPtr = new TextBox("Search hex", 350, y, 200, 25);
        searchBox = searchBoxPtr;
        uiComponents.emplace_back(searchBoxPtr);

        y += 35;

        // 描述输入框
        auto descBox = new TextBox("Annotation description", 120, y, 300, 25);
        descriptionBox = descBox;
        uiComponents.emplace_back(descBox);

        // 添加按钮
        auto addBtn = new Button("Add Highlight", 450, y, 150, 25);
        addBtn->setOnClick([this]() { addHighlight(); });
        addButton = addBtn;
        uiComponents.emplace_back(addBtn);
    }

    void handleHexViewEvent(SDL_Event* event) {
        if (!fileHandler.isOpen()) return;

        int hexViewX = MARGIN + ADDRESS_WIDTH;
        int hexViewWidth = HEX_VIEW_WIDTH;
        int hexViewY = CONTROL_PANEL_HEIGHT + 20;
        int hexViewHeight = WINDOW_HEIGHT - CONTROL_PANEL_HEIGHT - STATUS_BAR_HEIGHT - 40;

        switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                int x = event->button.x;
                int y = event->button.y;

                if (x >= hexViewX && x < hexViewX + hexViewWidth &&
                    y >= hexViewY && y < hexViewY + hexViewHeight) {

                    size_t offset = screenToOffset(x, y);
                    if (offset != size_t(-1)) {
                        uiState.selectionStart = offset;
                        uiState.selectionEnd = offset;
                        uiState.isSelecting = true;
                        uiState.needsRedraw = true;
                    }
                }
            }
            break;

        case SDL_MOUSEMOTION:
            if (uiState.isSelecting && uiState.mouseLeftDown) {
                int x = event->motion.x;
                int y = event->motion.y;

                if (x >= hexViewX && x < hexViewX + hexViewWidth &&
                    y >= hexViewY && y < hexViewY + hexViewHeight) {

                    size_t offset = screenToOffset(x, y);
                    if (offset != size_t(-1)) {
                        uiState.selectionEnd = offset;
                        uiState.needsRedraw = true;

                        // 更新悬停信息
                        updateHoverInfo(offset);
                    }
                }
            }
            else {
                // 更新悬停信息
                int x = event->motion.x;
                int y = event->motion.y;

                if (x >= hexViewX && x < hexViewX + hexViewWidth &&
                    y >= hexViewY && y < hexViewY + hexViewHeight) {

                    size_t offset = screenToOffset(x, y);
                    if (offset != size_t(-1)) {
                        updateHoverInfo(offset);
                    }
                }
            }
            break;
        }
    }

    size_t screenToOffset(int x, int y) {
        if (!fileHandler.isOpen()) return size_t(-1);

        int hexViewX = MARGIN + ADDRESS_WIDTH;
        int hexViewY = CONTROL_PANEL_HEIGHT + 20;

        // 转换为相对坐标
        x -= hexViewX;
        y -= hexViewY;

        // 考虑滚动
        y += uiState.scrollOffset;

        // 计算行和列
        int line = y / LINE_HEIGHT;
        int col = x / (HEX_VIEW_WIDTH / BYTES_PER_LINE);

        if (col < 0 || col >= BYTES_PER_LINE) {
            return size_t(-1);
        }

        size_t offset = line * BYTES_PER_LINE + col;
        if (offset < fileHandler.getFileSize()) {
            return offset;
        }

        return size_t(-1);
    }

    void updateHoverInfo(size_t offset) {
        if (!fileHandler.isOpen()) return;

        std::stringstream ss;
        ss << "Offset: 0x" << std::hex << std::uppercase << std::setw(8)
            << std::setfill('0') << offset
            << " (" << std::dec << offset << ")\n";

        uint8_t byte = fileHandler.getByte(offset);
        ss << "Value: 0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte)
            << " (" << std::dec << static_cast<int>(byte) << ")\n";

        // 检查高亮
        for (const auto& highlight : highlights) {
            if (highlight.contains(offset)) {
                ss << "Note: " << highlight.description;
                break;
            }
        }

        uiState.hoverText = ss.str();
        uiState.needsRedraw = true;
    }

    void handleMouseWheel(int delta) {
        uiState.scrollOffset -= delta * LINE_HEIGHT * 3;

        if (uiState.scrollOffset < 0) uiState.scrollOffset = 0;
        if (uiState.scrollOffset > uiState.maxScrollOffset) {
            uiState.scrollOffset = uiState.maxScrollOffset;
        }

        uiState.needsRedraw = true;
    }

    void handleReturnKey() {
        if (offsetBox && offsetBox->isFocused()) {
            // 跳转到偏移
            std::string offsetStr = offsetBox->getText();
            if (!offsetStr.empty()) {
                try {
                    size_t offset = std::stoul(offsetStr, nullptr, 16);
                    gotoOffset(offset);
                }
                catch (...) {
                    uiState.statusText = "Invalid offset";
                    uiState.needsRedraw = true;
                }
            }
        }
    }

    void gotoOffset(size_t offset) {
        if (!fileHandler.isOpen() || offset >= fileHandler.getFileSize()) {
            return;
        }

        // 计算滚动位置
        int line = static_cast<int>(offset / BYTES_PER_LINE);
        uiState.scrollOffset = line * LINE_HEIGHT;

        // 限制滚动范围
        if (uiState.scrollOffset > uiState.maxScrollOffset) {
            uiState.scrollOffset = uiState.maxScrollOffset;
        }

        uiState.needsRedraw = true;

        std::stringstream ss;
        ss << "Jumped to offset: 0x" << std::hex << offset;
        uiState.statusText = ss.str();
    }

    void addHighlight() {
        if (!fileHandler.isOpen()) return;

        size_t start = std::min(uiState.selectionStart, uiState.selectionEnd);
        size_t end = std::max(uiState.selectionStart, uiState.selectionEnd);

        if (start == end) {
            uiState.statusText = "Please select a range first";
            uiState.needsRedraw = true;
            return;
        }

        std::string desc = descriptionBox->getText();
        if (desc.empty()) {
            desc = "Untitled annotation";
        }

        highlights.emplace_back(start, end, currentHighlightColor, desc);

        // 添加到列表
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(8)
            << std::setfill('0') << start << ": " << desc;
        annotationList.push_back(ss.str());

        // 清空输入框
        descriptionBox->setText("");

        uiState.statusText = "Highlight added";
        uiState.needsRedraw = true;
    }

    void update() {
        // 更新UI组件
        for (auto& component : uiComponents) {
            component->update(uiState);
        }

        // 更新最大滚动
        if (fileHandler.isOpen()) {
            totalLines = (fileHandler.getFileSize() + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
            int contentHeight = totalLines * LINE_HEIGHT;
            int viewHeight = WINDOW_HEIGHT - CONTROL_PANEL_HEIGHT - STATUS_BAR_HEIGHT - 40;

            uiState.maxScrollOffset = std::max(0, contentHeight - viewHeight);

            if (uiState.scrollOffset > uiState.maxScrollOffset) {
                uiState.scrollOffset = uiState.maxScrollOffset;
            }
        }
    }

    void render() {
        // 清屏
        SDL_SetRenderDrawColor(renderer,
            COLOR_BACKGROUND.r,
            COLOR_BACKGROUND.g,
            COLOR_BACKGROUND.b,
            COLOR_BACKGROUND.a);
        SDL_RenderClear(renderer);

        // 绘制控制面板背景
        SDL_Rect controlRect = { 0, 0, WINDOW_WIDTH, CONTROL_PANEL_HEIGHT };
        SDL_SetRenderDrawColor(renderer,
            COLOR_STATUS_BAR.r,
            COLOR_STATUS_BAR.g,
            COLOR_STATUS_BAR.b,
            COLOR_STATUS_BAR.a);
        SDL_RenderFillRect(renderer, &controlRect);

        // 绘制状态栏背景
        SDL_Rect statusRect = { 0, WINDOW_HEIGHT - STATUS_BAR_HEIGHT,
                              WINDOW_WIDTH, STATUS_BAR_HEIGHT };
        SDL_SetRenderDrawColor(renderer,
            COLOR_STATUS_BAR.r,
            COLOR_STATUS_BAR.g,
            COLOR_STATUS_BAR.b,
            COLOR_STATUS_BAR.a);
        SDL_RenderFillRect(renderer, &statusRect);

        // 绘制标签
        renderText("Offset:", 10, 10, COLOR_TEXT);
        renderText("Search:", 280, 10, COLOR_TEXT);
        renderText("Description:", 10, 45, COLOR_TEXT);

        // 绘制UI组件
        for (auto& component : uiComponents) {
            component->render(renderer, font);
        }

        // 绘制Hex视图
        if (fileHandler.isOpen()) {
            renderHexView();
            renderAsciiView();
            renderHighlights();
            renderSelection();
            renderInfoPanel();
            renderAnnotationList();
        }
        else {
            renderText("Open a file to begin (not implemented in this example)",
                WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2, COLOR_TEXT);
        }

        // 绘制状态文本
        renderText(uiState.statusText, 10, WINDOW_HEIGHT - 25, COLOR_TEXT);

        // 绘制悬停信息
        if (!uiState.hoverText.empty()) {
            renderText(uiState.hoverText, WINDOW_WIDTH - 400, 100, COLOR_TEXT);
        }

        // 绘制滚动条
        renderScrollBar();

        // 更新屏幕
        SDL_RenderPresent(renderer);
    }

    void renderHexView() {
        int startY = CONTROL_PANEL_HEIGHT + 20;
        int startX = MARGIN + ADDRESS_WIDTH;

        // 计算可见行
        int firstLine = uiState.scrollOffset / LINE_HEIGHT;
        int lastLine = firstLine + (WINDOW_HEIGHT - startY - STATUS_BAR_HEIGHT) / LINE_HEIGHT + 1;

        for (int line = firstLine; line <= lastLine; ++line) {
            size_t lineOffset = line * BYTES_PER_LINE;

            if (lineOffset >= fileHandler.getFileSize()) break;

            // 绘制偏移地址
            std::stringstream offsetText;
            offsetText << std::hex << std::uppercase << std::setw(8)
                << std::setfill('0') << lineOffset;

            renderText(offsetText.str(), MARGIN, startY + (line - firstLine) * LINE_HEIGHT,
                COLOR_ADDRESS);

            // 绘制16进制字节
            for (int col = 0; col < BYTES_PER_LINE; ++col) {
                size_t offset = lineOffset + col;
                if (offset >= fileHandler.getFileSize()) break;

                uint8_t byte = fileHandler.getByte(offset);
                std::stringstream byteText;
                byteText << std::hex << std::uppercase << std::setw(2)
                    << std::setfill('0') << static_cast<int>(byte);

                int x = startX + col * (HEX_VIEW_WIDTH / BYTES_PER_LINE);
                int y = startY + (line - firstLine) * LINE_HEIGHT;

                renderText(byteText.str(), x, y, COLOR_TEXT);
            }
        }
    }

    void renderAsciiView() {
        int startY = CONTROL_PANEL_HEIGHT + 20;
        int startX = MARGIN + ADDRESS_WIDTH + HEX_VIEW_WIDTH + 20;

        // 计算可见行
        int firstLine = uiState.scrollOffset / LINE_HEIGHT;
        int lastLine = firstLine + (WINDOW_HEIGHT - startY - STATUS_BAR_HEIGHT) / LINE_HEIGHT + 1;

        for (int line = firstLine; line <= lastLine; ++line) {
            size_t lineOffset = line * BYTES_PER_LINE;

            if (lineOffset >= fileHandler.getFileSize()) break;

            std::string asciiLine;
            for (int col = 0; col < BYTES_PER_LINE; ++col) {
                size_t offset = lineOffset + col;
                if (offset >= fileHandler.getFileSize()) break;

                uint8_t byte = fileHandler.getByte(offset);
                char c = (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.';
                asciiLine += c;
            }

            int y = startY + (line - firstLine) * LINE_HEIGHT;
            renderText(asciiLine, startX, y, COLOR_TEXT);
        }
    }

    void renderHighlights() {
        for (const auto& highlight : highlights) {
            renderHighlight(highlight);
        }
    }

    void renderHighlight(const Highlight& highlight) {
        int startY = CONTROL_PANEL_HEIGHT + 20;
        int hexStartX = MARGIN + ADDRESS_WIDTH;
        int asciiStartX = MARGIN + ADDRESS_WIDTH + HEX_VIEW_WIDTH + 20;

        // 计算高亮的行范围
        size_t startLine = highlight.start / BYTES_PER_LINE;
        size_t endLine = highlight.end / BYTES_PER_LINE;

        for (size_t line = startLine; line <= endLine; ++line) {
            size_t lineStart = line * BYTES_PER_LINE;
            size_t lineEnd = lineStart + BYTES_PER_LINE - 1;

            // 计算当前行的高亮范围
            size_t highlightStart = std::max(highlight.start, lineStart);
            size_t highlightEnd = std::min(highlight.end, lineEnd);

            if (highlightStart > highlightEnd) continue;

            // 转换为屏幕坐标
            int startCol = static_cast<int>(highlightStart - lineStart);
            int endCol = static_cast<int>(highlightEnd - lineStart);

            // 计算位置
            int y = startY + static_cast<int>(line) * LINE_HEIGHT - uiState.scrollOffset;

            // 绘制16进制区域高亮
            int hexX = hexStartX + startCol * (HEX_VIEW_WIDTH / BYTES_PER_LINE);
            int hexWidth = (endCol - startCol + 1) * (HEX_VIEW_WIDTH / BYTES_PER_LINE);

            SDL_Rect hexRect = { hexX, y, hexWidth, LINE_HEIGHT };
            SDL_SetRenderDrawColor(renderer, highlight.color.r, highlight.color.g,
                highlight.color.b, highlight.color.a);
            SDL_RenderFillRect(renderer, &hexRect);

            // 绘制ASCII区域高亮
            int asciiX = asciiStartX + startCol * (ASCII_VIEW_WIDTH / BYTES_PER_LINE);
            int asciiWidth = (endCol - startCol + 1) * (ASCII_VIEW_WIDTH / BYTES_PER_LINE);

            SDL_Rect asciiRect = { asciiX, y, asciiWidth, LINE_HEIGHT };
            SDL_SetRenderDrawColor(renderer, highlight.color.r, highlight.color.g,
                highlight.color.b, highlight.color.a / 2);
            SDL_RenderFillRect(renderer, &asciiRect);
        }
    }

    void renderSelection() {
        if (uiState.selectionStart == uiState.selectionEnd) return;

        size_t start = std::min(uiState.selectionStart, uiState.selectionEnd);
        size_t end = std::max(uiState.selectionStart, uiState.selectionEnd);

        Highlight tempHighlight(start, end, COLOR_SELECTION, "");
        renderHighlight(tempHighlight);
    }

    void renderInfoPanel() {
        int panelWidth = 300;
        int panelX = WINDOW_WIDTH - panelWidth - MARGIN;
        int panelY = CONTROL_PANEL_HEIGHT + 20;
        int panelHeight = 150;

        // 绘制面板背景
        SDL_Rect panelRect = { panelX, panelY, panelWidth, panelHeight };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderFillRect(renderer, &panelRect);

        // 绘制边框
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &panelRect);

        // 绘制标题
        renderText("File Info", panelX + 10, panelY + 10, COLOR_TEXT);

        if (fileHandler.isOpen()) {
            std::stringstream ss;
            ss << "File: " << currentFile << "\n"
                << "Size: " << fileHandler.getFileSize() << " bytes\n"
                << "Highlights: " << highlights.size() << " annotations";

            renderText(ss.str(), panelX + 10, panelY + 30, COLOR_TEXT);
        }
    }

    void renderAnnotationList() {
        int listWidth = 300;
        int listX = WINDOW_WIDTH - listWidth - MARGIN;
        int listY = CONTROL_PANEL_HEIGHT + 200;
        int listHeight = WINDOW_HEIGHT - listY - STATUS_BAR_HEIGHT - 20;

        // 绘制列表背景
        SDL_Rect listRect = { listX, listY, listWidth, listHeight };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderFillRect(renderer, &listRect);

        // 绘制边框
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &listRect);

        // 绘制标题
        renderText("Annotations", listX + 10, listY + 10, COLOR_TEXT);

        // 绘制列表项
        int y = listY + 30;
        for (size_t i = 0; i < annotationList.size() && i < 20; ++i) {
            renderText(annotationList[i], listX + 10, y, COLOR_TEXT);
            y += 20;
        }
    }

    void renderScrollBar() {
        if (!fileHandler.isOpen() || totalLines == 0) return;

        int scrollbarWidth = 20;
        int scrollbarX = WINDOW_WIDTH - scrollbarWidth - MARGIN;
        int scrollbarY = CONTROL_PANEL_HEIGHT + 20;
        int scrollbarHeight = WINDOW_HEIGHT - scrollbarY - STATUS_BAR_HEIGHT - 20;

        // 绘制滚动条背景
        SDL_Rect scrollbarRect = { scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight };
        SDL_SetRenderDrawColor(renderer,
            COLOR_SCROLLBAR.r,
            COLOR_SCROLLBAR.g,
            COLOR_SCROLLBAR.b,
            COLOR_SCROLLBAR.a);
        SDL_RenderFillRect(renderer, &scrollbarRect);

        // 绘制滑块
        float scrollRatio = static_cast<float>(uiState.scrollOffset) /
            static_cast<float>(uiState.maxScrollOffset);
        scrollRatio = std::max(0.0f, std::min(1.0f, scrollRatio));

        int thumbHeight = std::max(20,
            static_cast<int>(scrollbarHeight * scrollbarHeight /
                (totalLines * LINE_HEIGHT)));
        int thumbY = scrollbarY + static_cast<int>((scrollbarHeight - thumbHeight) * scrollRatio);

        SDL_Rect thumbRect = { scrollbarX, thumbY, scrollbarWidth, thumbHeight };
        SDL_SetRenderDrawColor(renderer,
            COLOR_SCROLLBAR_THUMB.r,
            COLOR_SCROLLBAR_THUMB.g,
            COLOR_SCROLLBAR_THUMB.b,
            COLOR_SCROLLBAR_THUMB.a);
        SDL_RenderFillRect(renderer, &thumbRect);
    }

    void renderText(const std::string& text, int x, int y, const Color& color) {
        if (!font || text.empty()) return;

        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color.toSDLColor());
        if (!surface) return;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dstRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
            SDL_DestroyTexture(texture);
        }

        SDL_FreeSurface(surface);
    }

    void cleanup() {
        // 清理UI组件
        uiComponents.clear();

        // 清理SDL资源
        if (font) {
            TTF_CloseFont(font);
            font = nullptr;
        }

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        TTF_Quit();
        SDL_Quit();
    }
};

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "Hex Viewer - SDL2 Implementation" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "- 16进制查看器" << std::endl;
    std::cout << "- 支持大文件加载" << std::endl;
    std::cout << "- 鼠标选择和高亮" << std::endl;
    std::cout << "- 添加注释和描述" << std::endl;
    std::cout << "- 鼠标悬停显示信息" << std::endl;
    std::cout << "- 高性能渲染" << std::endl;
    std::cout << "==================================" << std::endl;

    HexViewerApp app;

    if (!app.initialize()) {
        std::cerr << "应用程序初始化失败!" << std::endl;
        return 1;
    }

    std::cout << "应用程序启动成功!" << std::endl;
    std::cout << "按ESC退出" << std::endl;
   
    app.run();

    return 0;
}