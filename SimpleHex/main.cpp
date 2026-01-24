#include "main.h"

// ==================== FileData 实现 ====================
FileData::FileData() : filesize(0) {}

bool FileData::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "无法打开文件: " << path << std::endl;
        return false;
    }

    filesize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    data.resize(filesize);
    if (!file.read(reinterpret_cast<char*>(data.data()), filesize)) {
        std::cerr << "读取文件失败: " << path << std::endl;
        data.clear();
        filesize = 0;
        return false;
    }

    filepath = path;
    filename = fs::path(path).filename().string();

    std::cout << "文件加载成功: " << filename
        << " (" << filesize << " 字节)" << std::endl;
    return true;
}

bool FileData::loadFromMemory(const std::vector<uint8_t>& newData) {
    data = newData;
    filesize = data.size();
    filepath = "";
    filename = "内存数据";
    return true;
}

size_t FileData::size() const {
    return filesize;
}

uint8_t FileData::operator[](size_t index) const {
    return index < filesize ? data[index] : 0;
}

std::string FileData::getFilePath() const {
    return filepath;
}

std::string FileData::getFileName() const {
    return filename;
}

std::string FileData::getFileSizeFormatted() const {
    return UIUtils::formatFileSize(filesize);
}

std::vector<uint8_t> FileData::getDataSlice(size_t start, size_t end) const {
    if (start >= filesize || end > filesize || start >= end) {
        return {};
    }
    return std::vector<uint8_t>(data.begin() + start, data.begin() + end);
}

std::string FileData::getHexString(size_t start, size_t length) const {
    std::stringstream ss;
    size_t end = std::min(start + length, filesize);
    for (size_t i = start; i < end; i++) {
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]) << " ";
    }
    return ss.str();
}

std::string FileData::getAsciiString(size_t start, size_t length) const {
    std::string result;
    size_t end = std::min(start + length, filesize);
    for (size_t i = start; i < end; i++) {
        char c = static_cast<char>(data[i]);
        result.push_back((c >= 32 && c <= 126) ? c : '.');
    }
    return result;
}

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

// ==================== HexViewer 实现 ====================
HexViewer::HexViewer()
    : window(nullptr), renderer(nullptr), textRenderer(nullptr),
    scrollOffset(0), totalLines(0), visibleLines(0),
    isDraggingScrollbar(false), scrollbarDragStartY(0), scrollbarStartOffset(0),
    selectionStart(0), selectionEnd(0), isSelecting(false), hasSelection(false),
    hoveredHighlight(nullptr), selectedHighlightIndex(-1),
    isEditingDescription(false), isFileDraggedOver(false),
    mouseX(0), mouseY(0), isMouseOverHex(false), hoveredByte(SIZE_MAX),
    isRunning(true), zoomLevel(1.0f), showAscii(true), showHex(true),
    fps(60.0f) {

    descriptionInputRect = { UI_PADDING, WINDOW_HEIGHT - 50, 300, 35 };
    addButtonRect = { UI_PADDING + 310, WINDOW_HEIGHT - 50, 100, 35 };
    clearButtonRect = { UI_PADDING + 420, WINDOW_HEIGHT - 50, 100, 35 };
    dropHighlightRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
}

HexViewer::~HexViewer() {
    cleanup();
}

bool HexViewer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return false;
    }

    // 设置窗口属性
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    window = SDL_CreateWindow(
        "十六进制阅读器 - Hex Viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        std::cerr << "窗口创建失败: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "渲染器创建失败: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // 设置窗口图标
    const int iconSize = 32;
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    // 创建简单的图标（蓝色的H）
    SDL_Surface* iconSurface = SDL_CreateRGBSurface(
        0, iconSize, iconSize, 32, rmask, gmask, bmask, amask
    );

    if (iconSurface) {
        SDL_FillRect(iconSurface, NULL, SDL_MapRGBA(iconSurface->format, 30, 30, 40, 255));

        // 绘制H字母
        SDL_Rect hRect = { iconSize / 2 - 3, 8, 6, iconSize - 16 };
        SDL_FillRect(iconSurface, &hRect, SDL_MapRGBA(iconSurface->format, 80, 150, 255, 255));

        hRect = { 8, iconSize / 2 - 3, iconSize - 16, 6 };
        SDL_FillRect(iconSurface, &hRect, SDL_MapRGBA(iconSurface->format, 80, 150, 255, 255));

        SDL_SetWindowIcon(window, iconSurface);
        SDL_FreeSurface(iconSurface);
    }

    // 初始化文本渲染器
    textRenderer = new TextRenderer(renderer);
    if (!textRenderer->init()) {
        std::cerr << "文本渲染器初始化失败" << std::endl;
        return false;
    }

    updateVisibleLines();
    lastFrameTime = std::chrono::steady_clock::now();

    // 启用文件拖拽
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    std::cout << "十六进制阅读器初始化成功" << std::endl;
    return true;
}

void HexViewer::updateVisibleLines() {
    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    visibleLines = (windowHeight - HEX_VIEW_Y - UI_PADDING * 2) / LINE_HEIGHT;
    if (fileData.size() > 0) {
        totalLines = (fileData.size() + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
    }
    else {
        totalLines = 0;
    }

    // 限制滚动偏移
    int maxScroll = std::max(0, totalLines - visibleLines);
    scrollOffset = std::min(scrollOffset, maxScroll);
}

bool HexViewer::loadFile(const std::string& path) {
    if (fileData.load(path)) {
        scrollOffset = 0;
        updateVisibleLines();
        clearSelection();
        highlights.clear();
        selectedHighlightIndex = -1;

        std::cout << "文件加载成功: " << path << std::endl;
        return true;
    }

    std::cerr << "文件加载失败: " << path << std::endl;
    return false;
}

void HexViewer::clearSelection() {
    hasSelection = false;
    isSelecting = false;
    selectionStart = 0;
    selectionEnd = 0;
}

void HexViewer::setSelection(size_t start, size_t end) {
    selectionStart = std::min(start, end);
    selectionEnd = std::max(start, end);
    hasSelection = true;
    isSelecting = false;
}

void HexViewer::addCurrentHighlight() {
    if (hasSelection && !descriptionInput.empty()) {
        SDL_Color color = COLOR_HIGHLIGHT;
        color.r = 50 + (rand() % 150);
        color.g = 50 + (rand() % 150);
        color.b = 50 + (rand() % 150);

        highlights.emplace_back(selectionStart, selectionEnd, descriptionInput, color);
        descriptionInput.clear();
        isEditingDescription = false;

        std::cout << "添加标记: " << selectionStart << " - " << selectionEnd
            << ": " << descriptionInput << std::endl;
    }
}

void HexViewer::removeSelectedHighlight() {
    if (selectedHighlightIndex >= 0 && selectedHighlightIndex < static_cast<int>(highlights.size())) {
        highlights.erase(highlights.begin() + selectedHighlightIndex);
        selectedHighlightIndex = -1;
        hoveredHighlight = nullptr;
    }
}

void HexViewer::exportHighlights() {
    // TODO: 实现导出功能
    std::cout << "导出功能待实现" << std::endl;
}

void HexViewer::importHighlights() {
    // TODO: 实现导入功能
    std::cout << "导入功能待实现" << std::endl;
}

void HexViewer::copySelectionToClipboard() {
    if (hasSelection && selectionEnd > selectionStart) {
        size_t size = selectionEnd - selectionStart + 1;
        std::stringstream ss;
        ss << "选中范围: 0x" << std::hex << std::uppercase << selectionStart
            << " - 0x" << selectionEnd << " (" << size << " 字节)\n";

        // 添加十六进制数据
        ss << "Hex: " << fileData.getHexString(selectionStart, std::min(size, (size_t)64)) << "\n";
        // 添加ASCII数据
        ss << "ASCII: " << fileData.getAsciiString(selectionStart, std::min(size, (size_t)64));

        if (SDL_SetClipboardText(ss.str().c_str()) == 0) {
            std::cout << "已复制到剪贴板" << std::endl;
        }
    }
}

void HexViewer::goToOffset() {
    // TODO: 实现跳转功能
    std::cout << "跳转功能待实现" << std::endl;
}

void HexViewer::searchHex() {
    // TODO: 实现十六进制搜索
    std::cout << "十六进制搜索功能待实现" << std::endl;
}

void HexViewer::searchText() {
    // TODO: 实现文本搜索
    std::cout << "文本搜索功能待实现" << std::endl;
}

void HexViewer::updateHoverState() {
    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 重置状态
    isMouseOverHex = false;
    hoveredByte = SIZE_MAX;
    hoveredHighlight = nullptr;

    // 检查是否在十六进制区域
    if (mouseX >= HEX_VIEW_X && mouseX <= windowWidth - SCROLLBAR_WIDTH - UI_PADDING &&
        mouseY >= HEX_VIEW_Y && mouseY <= windowHeight - UI_PADDING * 2 - 50) {

        hoveredByte = getByteIndexAt(mouseX, mouseY);
        if (hoveredByte < fileData.size()) {
            isMouseOverHex = true;

            // 检查是否悬停在标记上
            for (auto& highlight : highlights) {
                if (hoveredByte >= highlight.start && hoveredByte <= highlight.end) {
                    hoveredHighlight = &highlight;
                    break;
                }
            }
        }
    }
}

size_t HexViewer::getByteIndexAt(int x, int y) const {
    if (!isFileLoaded()) return SIZE_MAX;

    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 计算点击位置相对于十六进制区域的坐标
    int relativeX = x - HEX_VIEW_X;
    int relativeY = y - HEX_VIEW_Y;

    // 检查是否在有效区域内
    if (relativeX < 0 || relativeY < 0) return SIZE_MAX;

    // 计算行和列
    int line = relativeY / LINE_HEIGHT;
    int col = -1;

    // 检查点击的是十六进制区域还是ASCII区域
    if (relativeX >= ADDRESS_WIDTH + 10 &&
        relativeX < ADDRESS_WIDTH + 10 + BYTES_PER_LINE * BYTE_WIDTH) {
        col = (relativeX - (ADDRESS_WIDTH + 10)) / BYTE_WIDTH;
    }
    else if (relativeX >= ADDRESS_WIDTH + 10 + BYTES_PER_LINE * BYTE_WIDTH + 20) {
        col = (relativeX - (ADDRESS_WIDTH + 10 + BYTES_PER_LINE * BYTE_WIDTH + 20)) / ASCII_WIDTH;
    }

    if (col < 0 || col >= BYTES_PER_LINE || line >= visibleLines) {
        return SIZE_MAX;
    }

    size_t byteIndex = (scrollOffset + line) * BYTES_PER_LINE + col;
    return byteIndex < fileData.size() ? byteIndex : SIZE_MAX;
}

void HexViewer::handleScrollbarClick(int y) {
    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    int scrollbarHeight = windowHeight - HEX_VIEW_Y - UI_PADDING * 2;
    int scrollbarX = windowWidth - SCROLLBAR_WIDTH - UI_PADDING;

    if (mouseX >= scrollbarX && mouseX <= scrollbarX + SCROLLBAR_WIDTH &&
        y >= HEX_VIEW_Y && y <= HEX_VIEW_Y + scrollbarHeight) {

        isDraggingScrollbar = true;
        scrollbarDragStartY = y;
        scrollbarStartOffset = scrollOffset;
    }
}

void HexViewer::handleHexAreaClick() {
    size_t byteIndex = getByteIndexAt(mouseX, mouseY);
    if (byteIndex < fileData.size()) {
        if (!isSelecting) {
            // 开始新的选择
            selectionStart = byteIndex;
            selectionEnd = byteIndex;
            isSelecting = true;
            hasSelection = true;
        }
        else {
            // 更新选择结束位置
            selectionEnd = byteIndex;
        }
    }
}

void HexViewer::renderStatusBar() {
    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 状态栏背景
    SDL_Rect statusRect = { 0, windowHeight - 40, windowWidth, 40 };
    SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
        COLOR_PANEL.b, COLOR_PANEL.a);
    SDL_RenderFillRect(renderer, &statusRect);

    // 状态栏边框
    SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
        COLOR_BORDER.b, COLOR_BORDER.a);
    SDL_RenderDrawLine(renderer, 0, windowHeight - 40, windowWidth, windowHeight - 40);

    // 状态信息
    std::stringstream statusText;

    if (isFileLoaded()) {
        statusText << "文件: " << fileData.getFileName()
            << " | 大小: " << fileData.getFileSizeFormatted()
            << " | 偏移: 0x" << std::hex << std::uppercase
            << scrollOffset * BYTES_PER_LINE;

        if (hasSelection) {
            size_t selectedSize = selectionEnd - selectionStart + 1;
            statusText << " | 选中: " << selectedSize << " 字节 (0x"
                << std::hex << selectionStart << " - 0x" << selectionEnd << ")";
        }

        if (isMouseOverHex && hoveredByte < fileData.size()) {
            statusText << " | 悬停: 偏移 0x" << std::hex << hoveredByte
                << " | 值: 0x" << std::setw(2) << std::setfill('0')
                << static_cast<int>(fileData[hoveredByte])
                << " | ASCII: '"
                << (fileData[hoveredByte] >= 32 && fileData[hoveredByte] <= 126 ?
                    static_cast<char>(fileData[hoveredByte]) : '.') << "'";
        }
    }
    else {
        statusText << "就绪 - 拖拽文件到窗口或使用文件菜单";
    }

    textRenderer->render(statusText.str(), UI_PADDING, windowHeight - 30, COLOR_TEXT_SECONDARY);
}

void HexViewer::renderSidePanel() {
    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 侧边栏背景
    SDL_Rect sidePanelRect = { windowWidth - 280, HEX_VIEW_Y, 260,
                              windowHeight - HEX_VIEW_Y - UI_PADDING * 2 - 50 };
    SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
        COLOR_PANEL.b, COLOR_PANEL.a);
    SDL_RenderFillRect(renderer, &sidePanelRect);

    // 侧边栏边框
    SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
        COLOR_BORDER.b, COLOR_BORDER.a);
    SDL_RenderDrawRect(renderer, &sidePanelRect);

    // 侧边栏标题
    textRenderer->renderWithShadow("标记面板", windowWidth - 270, HEX_VIEW_Y + 10,
        COLOR_ACCENT_BLUE);

    // 标记数量
    std::string countText = "标记数量: " + std::to_string(highlights.size());
    textRenderer->render(countText, windowWidth - 270, HEX_VIEW_Y + 40, COLOR_TEXT_SECONDARY);

    // 标记列表
    int highlightY = HEX_VIEW_Y + 70;
    int maxHighlights = (sidePanelRect.h - 70) / 30;

    for (size_t i = 0; i < highlights.size() && i < static_cast<size_t>(maxHighlights); i++) {
        const auto& highlight = highlights[i];
        bool isHovered = (hoveredHighlight == &highlight);
        bool isSelected = (static_cast<int>(i) == selectedHighlightIndex);

        // 标记项背景
        SDL_Rect itemRect = { windowWidth - 275, highlightY, 250, 28 };
        SDL_Color bgColor = isSelected ? COLOR_SELECTION :
            (isHovered ? COLOR_BUTTON_HOVER : COLOR_BG);

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &itemRect);

        // 颜色标记
        SDL_Rect colorRect = { windowWidth - 270, highlightY + 4, 20, 20 };
        SDL_SetRenderDrawColor(renderer, highlight.color.r, highlight.color.g,
            highlight.color.b, 200);
        SDL_RenderFillRect(renderer, &colorRect);

        // 标记描述
        std::string desc = highlight.description.empty() ? "(无描述)" : highlight.description;
        if (desc.length() > 20) desc = desc.substr(0, 17) + "...";

        textRenderer->render(desc, windowWidth - 245, highlightY + 6,
            isSelected ? COLOR_TEXT_MAIN : COLOR_TEXT_SECONDARY);

        // 偏移信息
        std::string offsetInfo = "0x" + std::to_string(highlight.start) +
            "-0x" + std::to_string(highlight.end);
        textRenderer->render(offsetInfo, windowWidth - 245, highlightY + 22,
            COLOR_TEXT_SECONDARY, textRenderer->getMonoFont(), true);

        highlightY += 30;
    }

    // 操作按钮
    int buttonY = windowHeight - 120;
    SDL_Rect exportButtonRect = { windowWidth - 270, buttonY, 120, 32 };
    UIUtils::drawButton(renderer, exportButtonRect, "导出标记", textRenderer,
        UIUtils::isPointInRect(mouseX, mouseY, exportButtonRect));

    SDL_Rect importButtonRect = { windowWidth - 140, buttonY, 120, 32 };
    UIUtils::drawButton(renderer, importButtonRect, "导入标记", textRenderer,
        UIUtils::isPointInRect(mouseX, mouseY, importButtonRect));
}

void HexViewer::renderHexGrid() {
    if (!isFileLoaded()) return;

    int windowHeight, windowWidth;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 绘制网格背景
    SDL_Rect gridRect = { HEX_VIEW_X - 10, HEX_VIEW_Y - 5,
                         windowWidth - HEX_VIEW_X - SCROLLBAR_WIDTH - UI_PADDING - 280,
                         visibleLines * LINE_HEIGHT + 10 };

    SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
        COLOR_PANEL.b, COLOR_PANEL.a);
    SDL_RenderFillRect(renderer, &gridRect);

    // 绘制网格边框
    SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
        COLOR_BORDER.b, COLOR_BORDER.a);
    SDL_RenderDrawRect(renderer, &gridRect);

    // 列标题
    textRenderer->renderWithShadow("地址", HEX_VIEW_X, HEX_VIEW_Y - 30, COLOR_ACCENT_PURPLE);
    textRenderer->renderWithShadow("十六进制", HEX_VIEW_X + ADDRESS_WIDTH + 10,
        HEX_VIEW_Y - 30, COLOR_ACCENT_BLUE);
    textRenderer->renderWithShadow("ASCII", HEX_VIEW_X + ADDRESS_WIDTH + 10 +
        BYTES_PER_LINE * BYTE_WIDTH + 20,
        HEX_VIEW_Y - 30, COLOR_ACCENT_GREEN);

    // 计算起始字节
    size_t startByte = static_cast<size_t>(scrollOffset) * BYTES_PER_LINE;
    size_t endByte = std::min(startByte + static_cast<size_t>(visibleLines) * BYTES_PER_LINE,
        fileData.size());

    // 绘制每一行
    for (int line = 0; line < visibleLines; line++) {
        size_t lineStart = startByte + static_cast<size_t>(line) * BYTES_PER_LINE;
        if (lineStart >= endByte) break;

        int yPos = HEX_VIEW_Y + line * LINE_HEIGHT;

        // 交替行背景
        if (line % 2 == 0) {
            SDL_Rect rowRect = { HEX_VIEW_X - 5, yPos,
                               gridRect.w + 5, LINE_HEIGHT };
            SDL_SetRenderDrawColor(renderer, 35, 35, 45, 100);
            SDL_RenderFillRect(renderer, &rowRect);
        }

        // 地址列
        std::stringstream addrStream;
        addrStream << std::hex << std::uppercase << std::setw(8)
            << std::setfill('0') << lineStart;
        textRenderer->render(addrStream.str(), HEX_VIEW_X, yPos + 6,
            COLOR_TEXT_SECONDARY, textRenderer->getMonoFont(), true);

        // 十六进制和ASCII列
        for (int col = 0; col < BYTES_PER_LINE; col++) {
            size_t byteIndex = lineStart + static_cast<size_t>(col);
            if (byteIndex >= endByte) break;

            renderByteCell(HEX_VIEW_X + ADDRESS_WIDTH + 10 + col * BYTE_WIDTH,
                yPos + 6, byteIndex, fileData[byteIndex]);
        }
    }
}

void HexViewer::renderByteCell(int x, int y, size_t byteIndex, uint8_t byte) {
    // 检查是否为悬停状态
    bool isHovered = (hoveredByte == byteIndex);

    // 检查是否在选中范围内
    bool isSelected = hasSelection && byteIndex >= selectionStart && byteIndex <= selectionEnd;

    // 检查是否有标记
    const Highlight* cellHighlight = nullptr;
    for (const auto& highlight : highlights) {
        if (byteIndex >= highlight.start && byteIndex <= highlight.end) {
            cellHighlight = &highlight;
            break;
        }
    }

    // 十六进制显示位置
    int hexX = x;
    int asciiX = HEX_VIEW_X + ADDRESS_WIDTH + 10 + BYTES_PER_LINE * BYTE_WIDTH + 20 +
        ((byteIndex % BYTES_PER_LINE) * ASCII_WIDTH);

    // 绘制标记背景
    if (cellHighlight) {
        SDL_Rect highlightRect = { hexX - 2, y - 6, BYTE_WIDTH, BYTE_HEIGHT };
        SDL_SetRenderDrawColor(renderer, cellHighlight->color.r, cellHighlight->color.g,
            cellHighlight->color.b, 80);
        SDL_RenderFillRect(renderer, &highlightRect);

        // 标记边框
        SDL_SetRenderDrawColor(renderer, cellHighlight->color.r, cellHighlight->color.g,
            cellHighlight->color.b, 180);
        SDL_RenderDrawRect(renderer, &highlightRect);
    }

    // 绘制选中背景
    if (isSelected) {
        SDL_Rect selectRect = { hexX - 2, y - 6, BYTE_WIDTH, BYTE_HEIGHT };
        SDL_SetRenderDrawColor(renderer, COLOR_SELECTION.r, COLOR_SELECTION.g,
            COLOR_SELECTION.b, COLOR_SELECTION.a);
        SDL_RenderFillRect(renderer, &selectRect);
    }

    // 绘制悬停效果
    if (isHovered) {
        SDL_Rect hoverRect = { hexX - 2, y - 6, BYTE_WIDTH, BYTE_HEIGHT };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
        SDL_RenderFillRect(renderer, &hoverRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 80);
        SDL_RenderDrawRect(renderer, &hoverRect);
    }

    // 绘制十六进制值
    std::stringstream hexStream;
    hexStream << std::hex << std::uppercase << std::setw(2)
        << std::setfill('0') << static_cast<int>(byte);

    SDL_Color hexColor = isSelected ? COLOR_TEXT_MAIN :
        (isHovered ? COLOR_ACCENT_BLUE : COLOR_TEXT_SECONDARY);

    if (cellHighlight) {
        hexColor = cellHighlight->color;
        hexColor.a = 255;
    }

    textRenderer->render(hexStream.str(), hexX, y, hexColor, textRenderer->getMonoFont(), true);

    // 绘制ASCII字符
    char asciiChar = (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.';
    SDL_Color asciiColor = isSelected ? COLOR_TEXT_MAIN :
        (isHovered ? COLOR_ACCENT_GREEN : COLOR_TEXT_SECONDARY);

    if (cellHighlight) {
        asciiColor = cellHighlight->color;
        asciiColor.a = 255;
    }

    textRenderer->render(std::string(1, asciiChar), asciiX, y, asciiColor,
        textRenderer->getMonoFont(), true);
}

void HexViewer::render() {
    // 计算FPS
    auto currentTime = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    if (deltaTime > 0) {
        fps = 1000.0f / deltaTime;
    }

    // 清除屏幕
    SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(renderer);

    // 绘制文件拖拽高亮
    if (isFileDraggedOver) {
        SDL_SetRenderDrawColor(renderer, COLOR_DROP_HIGHLIGHT.r, COLOR_DROP_HIGHLIGHT.g,
            COLOR_DROP_HIGHLIGHT.b, COLOR_DROP_HIGHLIGHT.a);
        SDL_RenderFillRect(renderer, &dropHighlightRect);

        std::string dropText = "释放文件以加载";
        SDL_Point textSize = textRenderer->getSize(dropText);
        int textX = (WINDOW_WIDTH - textSize.x) / 2;
        int textY = (WINDOW_HEIGHT - textSize.y) / 2;
        textRenderer->renderWithShadow(dropText, textX, textY, COLOR_TEXT_MAIN);
    }

    // 绘制标题栏
    SDL_Rect headerRect = { 0, 0, WINDOW_WIDTH, 60 };
    SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
        COLOR_PANEL.b, COLOR_PANEL.a);
    SDL_RenderFillRect(renderer, &headerRect);

    // 绘制标题
    std::string title = "十六进制阅读器";
    if (isFileLoaded()) {
        title += " - " + fileData.getFileName() + " (" + fileData.getFileSizeFormatted() + ")";
    }
    textRenderer->renderWithShadow(title, UI_PADDING, 20, COLOR_TEXT_MAIN);

    // 绘制FPS信息
    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
    textRenderer->render(fpsText, WINDOW_WIDTH - 100, 20, COLOR_TEXT_SECONDARY);

    if (isFileLoaded()) {
        // 绘制十六进制视图
        renderHexGrid();

        // 绘制侧边栏
        renderSidePanel();

        // 绘制滚动条
        if (totalLines > visibleLines) {
            int scrollbarHeight = WINDOW_HEIGHT - HEX_VIEW_Y - UI_PADDING * 2 - 50;
            int scrollbarX = WINDOW_WIDTH - SCROLLBAR_WIDTH - UI_PADDING - 280;

            // 滚动条背景
            SDL_Rect scrollbarBg = { scrollbarX, HEX_VIEW_Y, SCROLLBAR_WIDTH, scrollbarHeight };
            SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
                COLOR_BORDER.b, 100);
            SDL_RenderFillRect(renderer, &scrollbarBg);

            // 计算滑块
            float thumbHeightRatio = static_cast<float>(visibleLines) / totalLines;
            int thumbHeight = std::max(40, static_cast<int>(scrollbarHeight * thumbHeightRatio));
            float scrollRatio = static_cast<float>(scrollOffset) / (totalLines - visibleLines);
            int thumbY = HEX_VIEW_Y + static_cast<int>((scrollbarHeight - thumbHeight) * scrollRatio);

            // 滑块
            SDL_Rect thumbRect = { scrollbarX, thumbY, SCROLLBAR_WIDTH, thumbHeight };
            SDL_Color thumbColor = isDraggingScrollbar ? COLOR_SCROLLBAR_HOVER : COLOR_SCROLLBAR;
            SDL_SetRenderDrawColor(renderer, thumbColor.r, thumbColor.g,
                thumbColor.b, thumbColor.a);
            SDL_RenderFillRect(renderer, &thumbRect);

            // 滑块边框
            SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
                COLOR_BORDER.b, 200);
            SDL_RenderDrawRect(renderer, &thumbRect);
        }

        // 绘制操作面板
        SDL_Rect controlPanel = { UI_PADDING, WINDOW_HEIGHT - 100,
                                 WINDOW_WIDTH - UI_PADDING * 2 - 300, 90 };
        SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
            COLOR_PANEL.b, COLOR_PANEL.a);
        SDL_RenderFillRect(renderer, &controlPanel);

        // 操作按钮
        int buttonX = UI_PADDING + 10;
        int buttonY = WINDOW_HEIGHT - 90;

        // 复制按钮
        SDL_Rect copyButtonRect = { buttonX, buttonY, 120, 32 };
        UIUtils::drawButton(renderer, copyButtonRect, "复制选中", textRenderer,
            UIUtils::isPointInRect(mouseX, mouseY, copyButtonRect));

        // 跳转按钮
        SDL_Rect gotoButtonRect = { buttonX + 130, buttonY, 120, 32 };
        UIUtils::drawButton(renderer, gotoButtonRect, "跳转偏移", textRenderer,
            UIUtils::isPointInRect(mouseX, mouseY, gotoButtonRect));

        // 搜索十六进制
        SDL_Rect searchHexButtonRect = { buttonX + 260, buttonY, 140, 32 };
        UIUtils::drawButton(renderer, searchHexButtonRect, "搜索十六进制", textRenderer,
            UIUtils::isPointInRect(mouseX, mouseY, searchHexButtonRect));

        // 搜索文本
        SDL_Rect searchTextButtonRect = { buttonX + 410, buttonY, 120, 32 };
        UIUtils::drawButton(renderer, searchTextButtonRect, "搜索文本", textRenderer,
            UIUtils::isPointInRect(mouseX, mouseY, searchTextButtonRect));

        // 标记输入框
        SDL_Rect inputBgRect = descriptionInputRect;
        inputBgRect.x -= 5;
        inputBgRect.y -= 5;
        inputBgRect.w += 10;
        inputBgRect.h += 10;

        SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g,
            COLOR_PANEL.b, COLOR_PANEL.a);
        SDL_RenderFillRect(renderer, &inputBgRect);

        if (isEditingDescription) {
            SDL_SetRenderDrawColor(renderer, COLOR_ACCENT_BLUE.r, COLOR_ACCENT_BLUE.g,
                COLOR_ACCENT_BLUE.b, 255);
            SDL_RenderDrawRect(renderer, &inputBgRect);
        }
        else {
            SDL_SetRenderDrawColor(renderer, COLOR_BORDER.r, COLOR_BORDER.g,
                COLOR_BORDER.b, 200);
            SDL_RenderDrawRect(renderer, &inputBgRect);
        }

        // 绘制输入文本
        std::string displayText = descriptionInput.empty() ?
            "输入标记描述..." : descriptionInput;
        SDL_Color textColor = descriptionInput.empty() ?
            COLOR_TEXT_SECONDARY : COLOR_TEXT_MAIN;

        textRenderer->render(displayText, descriptionInputRect.x + 10,
            descriptionInputRect.y + 8, textColor);

        // 绘制光标
        if (isEditingDescription) {
            SDL_Point textSize = textRenderer->getSize(displayText);
            SDL_Rect cursorRect = { descriptionInputRect.x + 10 + textSize.x + 2,
                                   descriptionInputRect.y + 8, 2, 20 };

            int time = SDL_GetTicks();
            if ((time / 500) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, COLOR_ACCENT_BLUE.r,
                    COLOR_ACCENT_BLUE.g, COLOR_ACCENT_BLUE.b, 255);
                SDL_RenderFillRect(renderer, &cursorRect);
            }
        }

        // 绘制添加按钮
        bool isAddHovered = UIUtils::isPointInRect(mouseX, mouseY, addButtonRect);
        UIUtils::drawButton(renderer, addButtonRect, "添加标记", textRenderer, isAddHovered);

        // 绘制清除按钮
        bool isClearHovered = UIUtils::isPointInRect(mouseX, mouseY, clearButtonRect);
        UIUtils::drawButton(renderer, clearButtonRect, "清除标记", textRenderer, isClearHovered);
    }

    // 绘制状态栏
    renderStatusBar();

    // 显示渲染
    SDL_RenderPresent(renderer);

    // 控制帧率
    if (fps > 120) {
        SDL_Delay(8); // 大约125FPS的限制
    }
}

void HexViewer::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            updateVisibleLines();

            // 更新UI元素位置
            int windowHeight, windowWidth;
            SDL_GetWindowSize(window, &windowWidth, &windowHeight);

            descriptionInputRect.y = windowHeight - 50;
            addButtonRect.y = windowHeight - 50;
            clearButtonRect.y = windowHeight - 50;
            dropHighlightRect = { 0, 0, windowWidth, windowHeight };
        }
        break;

    case SDL_MOUSEMOTION:
        mouseX = event.motion.x;
        mouseY = event.motion.y;
        updateHoverState();

        // 处理滚动条拖拽
        if (isDraggingScrollbar) {
            int deltaY = mouseY - scrollbarDragStartY;
            int scrollbarHeight = WINDOW_HEIGHT - HEX_VIEW_Y - UI_PADDING * 2 - 50;
            float dragRatio = static_cast<float>(deltaY) / scrollbarHeight;
            int lineDelta = static_cast<int>(dragRatio * totalLines);
            scrollOffset = std::max(0, std::min(totalLines - visibleLines,
                scrollbarStartOffset + lineDelta));
        }
        break;

    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            // 检查是否点击了描述输入框
            if (UIUtils::isPointInRect(mouseX, mouseY, descriptionInputRect)) {
                isEditingDescription = true;
                SDL_StartTextInput();
                break;
            }

            // 检查是否点击了添加按钮
            if (UIUtils::isPointInRect(mouseX, mouseY, addButtonRect)) {
                addCurrentHighlight();
                break;
            }

            // 检查是否点击了清除按钮
            if (UIUtils::isPointInRect(mouseX, mouseY, clearButtonRect)) {
                removeSelectedHighlight();
                break;
            }

            // 检查是否点击了侧边栏的标记项
            if (mouseX >= WINDOW_WIDTH - 275 && mouseX <= WINDOW_WIDTH - 25) {
                int highlightIndex = (mouseY - (HEX_VIEW_Y + 70)) / 30;
                if (highlightIndex >= 0 && highlightIndex < static_cast<int>(highlights.size())) {
                    selectedHighlightIndex = highlightIndex;
                    // 滚动到选中的标记
                    scrollOffset = static_cast<int>(highlights[highlightIndex].start / BYTES_PER_LINE) - visibleLines / 2;
                    scrollOffset = std::max(0, scrollOffset);
                    break;
                }
            }

            // 检查是否点击了滚动条
            if (totalLines > visibleLines) {
                int scrollbarX = WINDOW_WIDTH - SCROLLBAR_WIDTH - UI_PADDING - 280;
                int scrollbarHeight = WINDOW_HEIGHT - HEX_VIEW_Y - UI_PADDING * 2 - 50;

                if (mouseX >= scrollbarX && mouseX <= scrollbarX + SCROLLBAR_WIDTH &&
                    mouseY >= HEX_VIEW_Y && mouseY <= HEX_VIEW_Y + scrollbarHeight) {

                    isDraggingScrollbar = true;
                    scrollbarDragStartY = mouseY;
                    scrollbarStartOffset = scrollOffset;
                    break;
                }
            }

            // 处理十六进制区域点击
            if (isFileLoaded() && mouseX >= HEX_VIEW_X &&
                mouseX <= WINDOW_WIDTH - SCROLLBAR_WIDTH - UI_PADDING - 280 &&
                mouseY >= HEX_VIEW_Y && mouseY <= WINDOW_HEIGHT - UI_PADDING * 2 - 50) {

                handleHexAreaClick();
            }
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            isDraggingScrollbar = false;

            // 如果停止选择，更新选择范围
            if (isSelecting) {
                isSelecting = false;
                if (selectionEnd < selectionStart) {
                    std::swap(selectionStart, selectionEnd);
                }
            }
        }
        break;

    case SDL_MOUSEWHEEL:
        if (isFileLoaded()) {
            scrollOffset -= event.wheel.y * 3;
            int maxScroll = std::max(0, totalLines - visibleLines);
            scrollOffset = std::max(0, std::min(maxScroll, scrollOffset));
        }
        break;

    case SDL_TEXTINPUT:
        if (isEditingDescription) {
            descriptionInput += event.text.text;
        }
        break;

    case SDL_KEYDOWN:
        if (isEditingDescription) {
            switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (!descriptionInput.empty()) {
                    descriptionInput.pop_back();
                }
                break;
            case SDLK_RETURN:
            case SDLK_RETURN2:
                addCurrentHighlight();
                break;
            case SDLK_ESCAPE:
                isEditingDescription = false;
                descriptionInput.clear();
                SDL_StopTextInput();
                break;
            }
        }
        else {
            switch (event.key.keysym.sym) {
            case SDLK_UP:
                scrollOffset = std::max(0, scrollOffset - 1);
                break;
            case SDLK_DOWN:
                scrollOffset = std::min(totalLines - visibleLines, scrollOffset + 1);
                break;
            case SDLK_PAGEUP:
                scrollOffset = std::max(0, scrollOffset - visibleLines);
                break;
            case SDLK_PAGEDOWN:
                scrollOffset = std::min(totalLines - visibleLines, scrollOffset + visibleLines);
                break;
            case SDLK_HOME:
                scrollOffset = 0;
                break;
            case SDLK_END:
                scrollOffset = totalLines - visibleLines;
                break;
            case SDLK_c:
                if (SDL_GetModState() & KMOD_CTRL) {
                    copySelectionToClipboard();
                }
                break;
            case SDLK_g:
                if (SDL_GetModState() & KMOD_CTRL) {
                    goToOffset();
                }
                break;
            case SDLK_f:
                if (SDL_GetModState() & KMOD_CTRL) {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                        searchHex();
                    }
                    else {
                        searchText();
                    }
                }
                break;
            }
        }
        break;

    case SDL_DROPFILE:
        draggedFilePath = event.drop.file;
        if (loadFile(draggedFilePath)) {
            std::cout << "拖拽文件加载成功: " << draggedFilePath << std::endl;
        }
        SDL_free(event.drop.file);
        isFileDraggedOver = false;
        break;

    case SDL_DROPBEGIN:
        isFileDraggedOver = true;
        break;

    case SDL_DROPCOMPLETE:
        isFileDraggedOver = false;
        break;

    case SDL_QUIT:
        isRunning = false;
        break;
    }
}

void HexViewer::run() {
    SDL_Event event;

    std::cout << "十六进制阅读器启动..." << std::endl;
    std::cout << "使用方法:" << std::endl;
    std::cout << "  1. 拖拽文件到窗口" << std::endl;
    std::cout << "  2. 鼠标左键拖拽选择字节" << std::endl;
    std::cout << "  3. 在输入框输入描述后点击'添加标记'" << std::endl;
    std::cout << "  4. 使用 Ctrl+C 复制选中内容" << std::endl;
    std::cout << "  5. 使用 Ctrl+F 搜索文本" << std::endl;
    std::cout << "  6. 使用 Ctrl+Shift+F 搜索十六进制" << std::endl;
    std::cout << "  7. 使用 Ctrl+G 跳转到偏移" << std::endl;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        render();

        // 控制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60FPS
    }
}

void HexViewer::cleanup() {
    if (textRenderer) {
        delete textRenderer;
        textRenderer = nullptr;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
    std::cout << "应用程序已清理" << std::endl;
}

std::string HexViewer::getStatusText() const {
    std::stringstream ss;

    if (isFileLoaded()) {
        ss << "文件: " << fileData.getFileName()
            << " | 大小: " << fileData.getFileSizeFormatted()
            << " | 偏移: 0x" << std::hex << std::uppercase << scrollOffset * BYTES_PER_LINE;

        if (hasSelection) {
            ss << " | 选中: " << (selectionEnd - selectionStart + 1) << " 字节";
        }
    }
    else {
        ss << "等待文件...";
    }

    return ss.str();
}

// 主函数
int main(int argc, char* argv[]) {
    // 设置控制台编码为UTF-8
    system("chcp 65001 > nul");

    HexViewer app;

    if (!app.init()) {
        std::cerr << "应用程序初始化失败" << std::endl;
        return 1;
    }

    // 如果有命令行参数，尝试加载文件
    if (argc > 1) {
        std::string filePath = argv[1];
        if (fs::exists(filePath)) {
            app.loadFile(filePath);
        }
        else {
            std::cerr << "文件不存在: " << filePath << std::endl;
        }
    }

    app.run();

    return 0;
}