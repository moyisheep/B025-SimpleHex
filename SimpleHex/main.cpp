// AncientHexViewer.cpp
// 古风十六进制查看器 - 单文件实现
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <vector>
#include <memory>
#include <algorithm>

// ===================== 古风配色方案 =====================
namespace AncientColors {
    // 主色调 - 青瓷系列
    const wxColor CELADON_LIGHT(224, 240, 233);    // 浅青瓷 - 背景
    const wxColor CELADON_MID(169, 222, 209);      // 中青瓷 - 装饰
    const wxColor CELADON_DARK(86, 179, 163);      // 深青瓷 - 标题

    // 辅助色 - 墨色系列
    const wxColor INK_BLACK(35, 31, 32);           // 浓墨
    const wxColor INK_GRAY(89, 87, 84);            // 淡墨
    const wxColor INK_LIGHT(160, 154, 146);        // 烟墨

    // 点缀色 - 传统中国色
    const wxColor VERMILION(218, 87, 54);          // 朱砂红 - 高亮
    const wxColor SONG_BLUE(88, 154, 202);         // 宋蓝 - 链接
    const wxColor AMBER_GOLD(245, 188, 78);        // 琥珀金 - 强调
    const wxColor LILAC(191, 158, 208);            // 丁香紫 - 边框
    const wxColor BAMBOO_GREEN(135, 188, 97);      // 竹青 - 正常文字

    // 特殊色
    const wxColor RICE_PAPER(253, 251, 247);       // 宣纸白
    const wxColor SILK_YELLOW(255, 249, 227);      // 绢黄
    const wxColor JADE_BLUE(105, 182, 202);        // 青玉
};

// ===================== 古风字体助手 =====================
class AncientFont {
private:
    wxFont primaryFont_;
    wxFont secondaryFont_;
    wxFont decorativeFont_;

public:
    AncientFont() {
        // 尝试使用优雅字体，回退到等宽字体
#ifdef __WXMSW__
        primaryFont_ = wxFont(12, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Consolas"));
        secondaryFont_ = wxFont(11, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        decorativeFont_ = wxFont(14, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#elif __WXOSX__
        primaryFont_ = wxFont(13, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Monaco"));
        secondaryFont_ = wxFont(12, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        decorativeFont_ = wxFont(15, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#else
        primaryFont_ = wxFont(11, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Monospace"));
        secondaryFont_ = wxFont(10, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        decorativeFont_ = wxFont(12, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#endif
    }

    wxFont& Primary() { return primaryFont_; }
    wxFont& Secondary() { return secondaryFont_; }
    wxFont& Decorative() { return decorativeFont_; }

    void SetSize(int size) {
        primaryFont_.SetPointSize(size);
        secondaryFont_.SetPointSize(size - 1);
        decorativeFont_.SetPointSize(size + 2);
    }
};

// ===================== 数据管理器 =====================
class HexData {
private:
    std::vector<uint8_t> data_;
    size_t pageSize_;

public:
    HexData() : pageSize_(65536) {}

    bool LoadFromFile(const wxString& filename) {
        wxFile file(filename);
        if (!file.IsOpened()) return false;

        wxFileOffset size = file.Length();
        if (size <= 0) return false;

        // 限制文件大小
        if (size > 100 * 1024 * 1024) {
            wxMessageBox(wxT("文件过大，建议使用专业工具处理"),
                wxT("提示"), wxOK | wxICON_WARNING);
            return false;
        }

        data_.resize(size);
        return file.Read(data_.data(), size) == size;
    }

    void Clear() { data_.clear(); }
    bool IsEmpty() const { return data_.empty(); }
    size_t Size() const { return data_.size(); }
    const uint8_t* Data() const { return data_.data(); }

    uint8_t GetByte(size_t offset) const {
        return offset < data_.size() ? data_[offset] : 0;
    }

    std::pair<wxString, wxString> GetLine(size_t offset, int bytesPerLine) const {
        wxString hexStr, asciiStr;
        for (int i = 0; i < bytesPerLine; i++) {
            size_t pos = offset + i;
            if (pos >= data_.size()) break;

            uint8_t byte = data_[pos];
            hexStr += wxString::Format(wxT("%02X "), byte);

            if (byte >= 32 && byte <= 126) {
                asciiStr += wxString::Format(wxT("%c"), byte);
            }
            else {
                asciiStr += wxT("·");
            }
        }
        return { hexStr.Trim(), asciiStr };
    }
};

// ===================== 古风装饰绘制器 =====================
class AncientDecoration {
public:
    static void DrawClassicBorder(wxDC& dc, const wxRect& rect) {
        // 绘制双层边框
        dc.SetPen(wxPen(AncientColors::CELADON_DARK, 2));
        dc.DrawRectangle(rect);

        // 内边框
        wxRect innerRect = rect;
        innerRect.Deflate(2);
        dc.SetPen(wxPen(AncientColors::CELADON_MID, 1));
        dc.DrawRectangle(innerRect);

        // 角落装饰
        int cornerSize = 8;
        dc.SetPen(wxPen(AncientColors::VERMILION, 2));

        // 左上角
        dc.DrawLine(rect.x + 5, rect.y, rect.x + cornerSize, rect.y);
        dc.DrawLine(rect.x, rect.y + 5, rect.x, rect.y + cornerSize);

        // 右上角
        dc.DrawLine(rect.x + rect.width - cornerSize, rect.y,
            rect.x + rect.width - 5, rect.y);
        dc.DrawLine(rect.x + rect.width, rect.y + 5,
            rect.x + rect.width, rect.y + cornerSize);

        // 左下角
        dc.DrawLine(rect.x + 5, rect.y + rect.height,
            rect.x + cornerSize, rect.y + rect.height);
        dc.DrawLine(rect.x, rect.y + rect.height - cornerSize,
            rect.x, rect.y + rect.height - 5);

        // 右下角
        dc.DrawLine(rect.x + rect.width - cornerSize, rect.y + rect.height,
            rect.x + rect.width - 5, rect.y + rect.height);
        dc.DrawLine(rect.x + rect.width, rect.y + rect.height - cornerSize,
            rect.x + rect.width, rect.y + rect.height - 5);
    }

    static void DrawScrollBar(wxDC& dc, const wxRect& rect,
        double position, double thumbSize) {
        // 滚动条背景
        wxRect bgRect = rect;
        bgRect.Deflate(1);
        dc.SetPen(wxPen(AncientColors::INK_LIGHT, 1));
        dc.SetBrush(wxBrush(AncientColors::CELADON_LIGHT));
        dc.DrawRoundedRectangle(bgRect, 4);

        // 滚动滑块
        int thumbHeight = rect.height * thumbSize;
        int thumbY = rect.y + (rect.height - thumbHeight) * position;

        wxRect thumbRect(rect.x, thumbY, rect.width, thumbHeight);
        thumbRect.Deflate(2, 4);

        dc.SetPen(wxPen(AncientColors::CELADON_DARK, 1));
        dc.SetBrush(wxBrush(AncientColors::CELADON_MID));
        dc.DrawRoundedRectangle(thumbRect, 6);

        // 滑块内部装饰
        wxRect innerRect = thumbRect;
        innerRect.Deflate(2);
        dc.SetPen(wxPen(AncientColors::LILAC, 1));
        dc.SetBrush(wxBrush(AncientColors::RICE_PAPER, wxBRUSHSTYLE_TRANSPARENT));
        dc.DrawRoundedRectangle(innerRect, 4);
    }

    static void DrawHeader(wxDC& dc, const wxString& text, const wxRect& rect) {
        // 渐变背景
        wxRect gradRect = rect;
        gradRect.height = 40;

        wxColour topColor = AncientColors::CELADON_DARK;
        wxColour bottomColor = AncientColors::CELADON_MID;

        for (int y = 0; y < gradRect.height; y++) {
            double ratio = static_cast<double>(y) / gradRect.height;
            wxColor color = BlendColors(topColor, bottomColor, ratio);
            dc.SetPen(wxPen(color));
            dc.DrawLine(gradRect.x, gradRect.y + y,
                gradRect.x + gradRect.width, gradRect.y + y);
        }

        // 装饰花纹
        dc.SetPen(wxPen(AncientColors::VERMILION, 1));
        for (int i = 0; i < gradRect.width; i += 30) {
            dc.DrawLine(gradRect.x + i, gradRect.y + gradRect.height - 2,
                gradRect.x + i + 15, gradRect.y + gradRect.height - 2);
        }

        // 标题文字
        dc.SetFont(wxFont(14, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
        dc.SetTextForeground(AncientColors::RICE_PAPER);

        wxSize textSize = dc.GetTextExtent(text);
        int textX = gradRect.x + (gradRect.width - textSize.GetWidth()) / 2;
        int textY = gradRect.y + (gradRect.height - textSize.GetHeight()) / 2;

        // 文字阴影
        dc.SetTextForeground(wxColor(0, 0, 0, 100));
        dc.DrawText(text, textX + 1, textY + 1);

        // 主文字
        dc.SetTextForeground(AncientColors::RICE_PAPER);
        dc.DrawText(text, textX, textY);
    }

    static void DrawSeparator(wxDC& dc, int x, int y, int width) {
        // 传统分隔线
        wxPen pen1(AncientColors::CELADON_DARK, 1);
        wxPen pen2(AncientColors::RICE_PAPER, 1);

        for (int i = 0; i < 3; i++) {
            dc.SetPen(i % 2 == 0 ? pen1 : pen2);
            dc.DrawLine(x, y + i, x + width, y + i);
        }
    }

    static wxColor BlendColors(const wxColor& c1, const wxColor& c2, double ratio) {
        int r = c1.Red() * (1 - ratio) + c2.Red() * ratio;
        int g = c1.Green() * (1 - ratio) + c2.Green() * ratio;
        int b = c1.Blue() * (1 - ratio) + c2.Blue() * ratio;
        return wxColor(r, g, b);
    }
};

// ===================== 主视图类 =====================
class AncientHexView : public wxScrolledWindow {
private:
    HexData data_;
    AncientFont fonts_;

    // 显示设置
    int bytesPerLine_;
    int fontSize_;
    int charWidth_;
    int charHeight_;
    int lineHeight_;

    // 选择状态
    size_t selectionStart_;
    size_t selectionEnd_;
    bool hasSelection_;

    // 渲染缓存
    wxBitmap renderCache_;
    bool cacheValid_;

    // 鼠标状态
    wxPoint mousePos_;
    bool dragging_;

    // 装饰元素
    bool showDecorations_;

public:
    AncientHexView(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxBORDER_NONE | wxVSCROLL),
        bytesPerLine_(16),
        fontSize_(11),
        selectionStart_(0),
        selectionEnd_(0),
        hasSelection_(false),
        cacheValid_(false),
        dragging_(false),
        showDecorations_(true) {

        InitStyles();
        SetupEvents();

        SetScrollRate(0, lineHeight_);
        SetDoubleBuffered(true);
    }

    void LoadFile(const wxString& filename) {
        if (data_.LoadFromFile(filename)) {
            cacheValid_ = false;
            hasSelection_ = false;
            UpdateScrollbars();
            Refresh();
        }
    }

    void SetBytesPerLine(int bytes) {
        if (bytes == 8 || bytes == 16 || bytes == 24 || bytes == 32) {
            bytesPerLine_ = bytes;
            cacheValid_ = false;
            UpdateScrollbars();
            Refresh();
        }
    }

    void SetFontSize(int size) {
        if (size >= 8 && size <= 20) {
            fontSize_ = size;
            fonts_.SetSize(size);
            CalculateMetrics();
            cacheValid_ = false;
            UpdateScrollbars();
            Refresh();
        }
    }

private:
    void InitStyles() {
        SetBackgroundColour(AncientColors::RICE_PAPER);
        fonts_.SetSize(fontSize_);
        CalculateMetrics();
    }

    void CalculateMetrics() {
        wxClientDC dc(this);
        dc.SetFont(fonts_.Primary());
        wxSize charSize = dc.GetTextExtent("W");
        charWidth_ = charSize.GetWidth();
        charHeight_ = charSize.GetHeight();
        lineHeight_ = charHeight_ + 8;
    }

    void SetupEvents() {
        Bind(wxEVT_PAINT, &AncientHexView::OnPaint, this);
        Bind(wxEVT_SIZE, &AncientHexView::OnSize, this);
        Bind(wxEVT_MOUSEWHEEL, &AncientHexView::OnMouseWheel, this);
        Bind(wxEVT_LEFT_DOWN, &AncientHexView::OnMouseDown, this);
        Bind(wxEVT_LEFT_UP, &AncientHexView::OnMouseUp, this);
        Bind(wxEVT_MOTION, &AncientHexView::OnMouseMove, this);
        Bind(wxEVT_KEY_DOWN, &AncientHexView::OnKeyDown, this);
    }

    void UpdateScrollbars() {
        if (data_.IsEmpty()) {
            SetScrollbars(0, lineHeight_, 0, 0);
            return;
        }

        int totalLines = (data_.Size() + bytesPerLine_ - 1) / bytesPerLine_;
        int visibleLines = GetClientSize().GetHeight() / lineHeight_;

        SetScrollbars(0, lineHeight_, 0, totalLines, 0, 0, true);
    }

    wxPoint GetTextPosition() const {
        return wxPoint(30, 60); // 留出装饰空间
    }

    wxRect GetContentRect() const {
        wxSize size = GetClientSize();
        return wxRect(10, 50, size.GetWidth() - 40, size.GetHeight() - 80);
    }

    void OnPaint(wxPaintEvent&) {
        wxPaintDC dc(this);
        DoPrepareDC(dc);

        if (!cacheValid_) {
            UpdateRenderCache();
        }

        wxMemoryDC memDC;
        memDC.SelectObject(renderCache_);
        dc.Blit(0, 0, renderCache_.GetWidth(), renderCache_.GetHeight(),
            &memDC, 0, 0);
    }

    void UpdateRenderCache() {
        wxSize size = GetClientSize();
        renderCache_ = wxBitmap(size.GetWidth(), size.GetHeight());

        wxMemoryDC dc(renderCache_);
        RenderBackground(dc, size);

        if (!data_.IsEmpty()) {
            RenderContent(dc);
        }
        else {
            RenderEmptyState(dc, size);
        }

        cacheValid_ = true;
    }

    void RenderBackground(wxMemoryDC& dc, const wxSize& size) {
        // 宣纸纹理背景
        dc.SetBrush(wxBrush(AncientColors::RICE_PAPER));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        // 边缘渐变
        wxRect topRect(0, 0, size.GetWidth(), 50);
        wxRect bottomRect(0, size.GetHeight() - 30, size.GetWidth(), 30);

        for (int y = 0; y < topRect.height; y++) {
            double ratio = static_cast<double>(y) / topRect.height;
            wxColor color = AncientDecoration::BlendColors(
                AncientColors::SILK_YELLOW, AncientColors::RICE_PAPER, ratio);
            dc.SetPen(wxPen(color));
            dc.DrawLine(0, y, size.GetWidth(), y);
        }

        for (int y = 0; y < bottomRect.height; y++) {
            double ratio = static_cast<double>(y) / bottomRect.height;
            wxColor color = AncientDecoration::BlendColors(
                AncientColors::RICE_PAPER, AncientColors::SILK_YELLOW, ratio);
            dc.SetPen(wxPen(color));
            dc.DrawLine(0, bottomRect.y + y, size.GetWidth(), bottomRect.y + y);
        }

        // 绘制标题
        AncientDecoration::DrawHeader(dc, wxT("古风十六进制查看器"),
            wxRect(0, 0, size.GetWidth(), 50));

        // 内容区域边框
        wxRect contentRect = GetContentRect();
        AncientDecoration::DrawClassicBorder(dc, contentRect);

        // 列标题
        wxPoint textPos = GetTextPosition();
        RenderColumnHeaders(dc, textPos);
    }

    void RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos) {
        dc.SetFont(fonts_.Secondary());
        dc.SetTextForeground(AncientColors::CELADON_DARK);

        // 偏移地址标题
        dc.DrawText(wxT("偏移地址"), pos.x, pos.y - 25);

        // 十六进制标题
        int hexX = pos.x + 80;
        dc.DrawText(wxT("十六进制"), hexX, pos.y - 25);

        // ASCII标题
        int asciiX = hexX + bytesPerLine_ * 3 * charWidth_ + 40;
        dc.DrawText(wxT("字符"), asciiX, pos.y - 25);

        // 分隔线
        AncientDecoration::DrawSeparator(dc, pos.x, pos.y - 5,
            GetContentRect().width - 20);
    }

    void RenderContent(wxMemoryDC& dc) {
        wxPoint textPos = GetTextPosition();

        int startLine = GetViewStart().y / lineHeight_;
        int visibleLines = GetClientSize().GetHeight() / lineHeight_ + 2;

        for (int line = 0; line < visibleLines; line++) {
            size_t offset = (startLine + line) * bytesPerLine_;
            if (offset >= data_.Size()) break;

            int y = textPos.y + line * lineHeight_;
            RenderLine(dc, startLine + line, offset, y);
        }

        // 自定义滚动条
        if (showDecorations_) {
            RenderCustomScrollbar(dc);
        }
    }

    void RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y) {
        wxPoint pos = GetTextPosition();

        // 偏移地址
        wxString offsetStr = wxString::Format(wxT("%08X"),
            static_cast<unsigned int>(offset));
        dc.SetFont(fonts_.Primary());
        dc.SetTextForeground(AncientColors::INK_GRAY);
        dc.DrawText(offsetStr, pos.x, y);

        // 十六进制数据
        int hexX = pos.x + 80;
        for (int i = 0; i < bytesPerLine_; i++) {
            size_t index = offset + i;
            if (index >= data_.Size()) break;

            uint8_t byte = data_.GetByte(index);
            wxString byteStr = wxString::Format(wxT("%02X"), byte);

            // 高亮选择
            if (hasSelection_ && index >= selectionStart_ && index <= selectionEnd_) {
                dc.SetPen(wxPen(AncientColors::VERMILION, 1));
                dc.SetBrush(wxBrush(AncientColors::VERMILION.ChangeLightness(180)));
                dc.DrawRectangle(hexX + i * 3 * charWidth_ - 2,
                    y - 1,
                    charWidth_ * 2 + 4,
                    charHeight_ + 2);
            }

            // 颜色编码
            wxColor textColor = GetByteColor(byte);
            dc.SetTextForeground(textColor);

            dc.DrawText(byteStr, hexX + i * 3 * charWidth_, y);

            // 字节间分隔点
            if (i < bytesPerLine_ - 1) {
                dc.SetPen(wxPen(AncientColors::CELADON_MID, 1));
                dc.DrawPoint(hexX + i * 3 * charWidth_ + 2 * charWidth_ + 4, y + charHeight_ / 2);
            }
        }

        // ASCII显示
        int asciiX = hexX + bytesPerLine_ * 3 * charWidth_ + 40;
        auto [hexStr, asciiStr] = data_.GetLine(offset, bytesPerLine_);

        dc.SetFont(fonts_.Primary());
        for (size_t i = 0; i < asciiStr.length(); i++) {
            size_t index = offset + i;

            // 高亮选择
            if (hasSelection_ && index >= selectionStart_ && index <= selectionEnd_) {
                dc.SetPen(wxPen(AncientColors::SONG_BLUE, 1));
                dc.SetBrush(wxBrush(AncientColors::SONG_BLUE.ChangeLightness(180)));
                dc.DrawRectangle(asciiX + i * charWidth_ - 2,
                    y - 1,
                    charWidth_ + 4,
                    charHeight_ + 2);
            }

            wxChar ch = asciiStr[i];
            wxColor color = (ch == wxT('·')) ?
                AncientColors::INK_LIGHT :
                AncientColors::INK_BLACK;

            dc.SetTextForeground(color);
            dc.DrawText(wxString(ch), asciiX + i * charWidth_, y);
        }
    }

    void RenderCustomScrollbar(wxMemoryDC& dc) {
        wxSize size = GetClientSize();
        wxRect scrollRect(size.GetWidth() - 25, 50, 20, size.GetHeight() - 80);

        if (!data_.IsEmpty()) {
            int totalLines = (data_.Size() + bytesPerLine_ - 1) / bytesPerLine_;
            int visibleLines = size.GetHeight() / lineHeight_;

            if (totalLines > visibleLines) {
                double thumbSize = static_cast<double>(visibleLines) / totalLines;
                double position = static_cast<double>(GetViewStart().y) /
                    (totalLines * lineHeight_);

                AncientDecoration::DrawScrollBar(dc, scrollRect, position, thumbSize);
            }
        }
    }

    void RenderEmptyState(wxMemoryDC& dc, const wxSize& size) {
        dc.SetFont(fonts_.Decorative());
        dc.SetTextForeground(AncientColors::INK_LIGHT);

        wxString message = wxT("打开文件以查看十六进制内容");
        wxSize textSize = dc.GetTextExtent(message);

        int x = (size.GetWidth() - textSize.GetWidth()) / 2;
        int y = (size.GetHeight() - textSize.GetHeight()) / 2;

        // 水墨风格文字
        dc.SetTextForeground(wxColor(0, 0, 0, 30));
        dc.DrawText(message, x + 2, y + 2);
        dc.SetTextForeground(AncientColors::INK_GRAY);
        dc.DrawText(message, x, y);

        // 添加传统图案
        dc.SetPen(wxPen(AncientColors::CELADON_MID, 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        int patternSize = 60;
        int patternX = x - patternSize - 20;
        int patternY = y - patternSize / 2;

        // 绘制传统云纹
        DrawCloudPattern(dc, patternX, patternY, patternSize);

        patternX = x + textSize.GetWidth() + 20;
        DrawCloudPattern(dc, patternX, patternY, patternSize);
    }

    void DrawCloudPattern(wxMemoryDC& dc, int x, int y, int size) {
        wxPoint points[] = {
            wxPoint(x, y + size / 2),
            wxPoint(x + size / 4, y),
            wxPoint(x + size / 2, y + size / 4),
            wxPoint(x + 3 * size / 4, y),
            wxPoint(x + size, y + size / 2),
            wxPoint(x + 3 * size / 4, y + size),
            wxPoint(x + size / 2, y + 3 * size / 4),
            wxPoint(x + size / 4, y + size)
        };

        wxColour cloudColor = AncientColors::CELADON_LIGHT;
        for (int i = 0; i < 8; i++) {
            int alpha = 20 + i * 10;
            cloudColor = wxColour(cloudColor.Red(), cloudColor.Green(),
                cloudColor.Blue(), alpha);
            dc.SetPen(wxPen(cloudColor, 2));
            dc.DrawSpline(8, points);

            // 轻微偏移创建层次感
            for (auto& p : points) {
                p.x += 1;
                p.y += 1;
            }
        }
    }

    wxColor GetByteColor(uint8_t byte) const {
        if (byte == 0x00) return AncientColors::INK_LIGHT;
        if (byte == 0xFF) return AncientColors::VERMILION;
        if (byte >= 0x20 && byte <= 0x7E) return AncientColors::BAMBOO_GREEN;
        if (byte < 0x20) return AncientColors::SONG_BLUE;
        return AncientColors::LILAC;
    }

    size_t PosToIndex(const wxPoint& pos) const {
        wxPoint textPos = GetTextPosition();
        int line = (pos.y - textPos.y) / lineHeight_;
        if (line < 0) return 0;

        size_t offset = line * bytesPerLine_;

        int hexX = textPos.x + 80;
        if (pos.x >= hexX && pos.x < hexX + bytesPerLine_ * 3 * charWidth_) {
            int col = (pos.x - hexX) / (3 * charWidth_);
            return offset + col;
        }

        int asciiX = hexX + bytesPerLine_ * 3 * charWidth_ + 40;
        if (pos.x >= asciiX && pos.x < asciiX + bytesPerLine_ * charWidth_) {
            int col = (pos.x - asciiX) / charWidth_;
            return offset + col;
        }

        return offset;
    }

    // 事件处理
    void OnSize(wxSizeEvent&) {
        cacheValid_ = false;
        UpdateScrollbars();
        Refresh();
    }

    void OnMouseWheel(wxMouseEvent& event) {
        int lines = event.GetWheelRotation() / event.GetWheelDelta();
        Scroll(0, GetViewStart().y - lines * 3);
        cacheValid_ = false;
        Refresh();
    }

    void OnMouseDown(wxMouseEvent& event) {
        wxPoint pos = event.GetPosition();
        pos += GetViewStart();

        size_t index = PosToIndex(pos);
        if (index < data_.Size()) {
            selectionStart_ = selectionEnd_ = index;
            hasSelection_ = true;
            cacheValid_ = false;
            Refresh();
        }
        dragging_ = true;
        CaptureMouse();
    }

    void OnMouseUp(wxMouseEvent& event) {
        if (dragging_) {
            dragging_ = false;
            ReleaseMouse();
        }
    }

    void OnMouseMove(wxMouseEvent& event) {
        if (dragging_ && event.Dragging()) {
            wxPoint pos = event.GetPosition();
            pos += GetViewStart();

            size_t index = PosToIndex(pos);
            if (index < data_.Size()) {
                selectionEnd_ = index;
                cacheValid_ = false;
                Refresh();
            }
        }
    }

    void OnKeyDown(wxKeyEvent& event) {
        switch (event.GetKeyCode()) {
        case WXK_UP:
            Scroll(0, GetViewStart().y - lineHeight_);
            break;
        case WXK_DOWN:
            Scroll(0, GetViewStart().y + lineHeight_);
            break;
        case WXK_PAGEUP:
            Scroll(0, GetViewStart().y - GetClientSize().GetHeight());
            break;
        case WXK_PAGEDOWN:
            Scroll(0, GetViewStart().y + GetClientSize().GetHeight());
            break;
        case 'A':
            if (event.ControlDown()) {
                selectionStart_ = 0;
                selectionEnd_ = data_.Size() - 1;
                hasSelection_ = true;
                cacheValid_ = false;
                Refresh();
            }
            break;
        case 'C':
            if (event.ControlDown() && hasSelection_) {
                CopySelection();
            }
            break;
        }
        cacheValid_ = false;
        Refresh();
        event.Skip();
    }

    void CopySelection() {
        if (!hasSelection_ || data_.IsEmpty()) return;

        size_t start = std::min(selectionStart_, selectionEnd_);
        size_t end = std::max(selectionStart_, selectionEnd_);

        wxString hexStr, asciiStr;
        for (size_t i = start; i <= end && i < data_.Size(); i++) {
            hexStr += wxString::Format(wxT("%02X "), data_.GetByte(i));
            uint8_t byte = data_.GetByte(i);
            asciiStr += (byte >= 32 && byte <= 126) ?
                wxString::Format(wxT("%c"), byte) : wxString(wxT("·"));
        }

        wxString result = wxString::Format(
            wxT("偏移: %08X - %08X\n十六进制: %s\nASCII: %s"),
            static_cast<unsigned int>(start),
            static_cast<unsigned int>(end),
            hexStr,
            asciiStr);

        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(result));
            wxTheClipboard->Close();
        }
    }
};

// ===================== 主框架 =====================
class AncientHexFrame : public wxFrame {
private:
    AncientHexView* hexView_;
    wxStatusBar* statusBar_;
    AncientFont fonts_;

public:
    AncientHexFrame(const wxString& title)
        : wxFrame(nullptr, wxID_ANY, title,
            wxDefaultPosition, wxSize(1000, 700)) {

        SetMinSize(wxSize(800, 600));

        // 设置窗口图标
        wxIcon icon;
        icon.CopyFromBitmap(CreateAppIcon());
        SetIcon(icon);

        CreateUI();
        CreateMenu();
        CreateStatusBar();

        Center();
        Show(true);
    }

private:
    void CreateUI() {
        // 主面板
        wxPanel* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(AncientColors::RICE_PAPER);

        // 十六进制视图
        hexView_ = new AncientHexView(panel);
        hexView_->SetBackgroundColour(AncientColors::RICE_PAPER);

        // 工具栏
        CreateToolBar(panel);

        // 布局
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(hexView_, 1, wxEXPAND | wxALL, 10);
        panel->SetSizer(sizer);
    }

    void CreateToolBar(wxWindow* parent) {
        wxToolBar* toolbar = wxFrame::CreateToolBar();
        //wxPanel* toolbar = new wxPanel(parent, wxID_ANY);
        toolbar->SetBackgroundColour(AncientColors::CELADON_LIGHT);

        wxBoxSizer* toolSizer = new wxBoxSizer(wxHORIZONTAL);

        toolSizer->AddSpacer(20);

        // 创建"打开"按钮
        wxButton* btnOpen = new wxButton(toolbar, wxID_ANY, wxT("打开"),
            wxDefaultPosition, wxSize(80, 30));
        btnOpen->SetFont(fonts_.Secondary());
        btnOpen->SetBackgroundColour(AncientColors::CELADON_MID);
        btnOpen->SetForegroundColour(AncientColors::INK_BLACK);
        btnOpen->Bind(wxEVT_BUTTON, &AncientHexFrame::OnOpen, this);
        toolSizer->Add(btnOpen, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

        // 创建"8字节"按钮
        wxButton* btn8Bytes = new wxButton(toolbar, wxID_ANY, wxT("8字节"),
            wxDefaultPosition, wxSize(80, 30));
        btn8Bytes->SetFont(fonts_.Secondary());
        btn8Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
        btn8Bytes->SetForegroundColour(AncientColors::INK_BLACK);
        btn8Bytes->Bind(wxEVT_BUTTON, &AncientHexFrame::OnBytes8, this);
        toolSizer->Add(btn8Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        // 创建"16字节"按钮
        wxButton* btn16Bytes = new wxButton(toolbar, wxID_ANY, wxT("16字节"),
            wxDefaultPosition, wxSize(80, 30));
        btn16Bytes->SetFont(fonts_.Secondary());
        btn16Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
        btn16Bytes->SetForegroundColour(AncientColors::INK_BLACK);
        btn16Bytes->Bind(wxEVT_BUTTON, &AncientHexFrame::OnBytes16, this);
        toolSizer->Add(btn16Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        // 创建"24字节"按钮
        wxButton* btn24Bytes = new wxButton(toolbar, wxID_ANY, wxT("24字节"),
            wxDefaultPosition, wxSize(80, 30));
        btn24Bytes->SetFont(fonts_.Secondary());
        btn24Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
        btn24Bytes->SetForegroundColour(AncientColors::INK_BLACK);
        btn24Bytes->Bind(wxEVT_BUTTON, &AncientHexFrame::OnBytes24, this);
        toolSizer->Add(btn24Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        // 创建"32字节"按钮
        wxButton* btn32Bytes = new wxButton(toolbar, wxID_ANY, wxT("32字节"),
            wxDefaultPosition, wxSize(80, 30));
        btn32Bytes->SetFont(fonts_.Secondary());
        btn32Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
        btn32Bytes->SetForegroundColour(AncientColors::INK_BLACK);
        btn32Bytes->Bind(wxEVT_BUTTON, &AncientHexFrame::OnBytes32, this);
        toolSizer->Add(btn32Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

        // 创建"字体+"按钮
        wxButton* btnFontPlus = new wxButton(toolbar, wxID_ANY, wxT("字体+"),
            wxDefaultPosition, wxSize(80, 30));
        btnFontPlus->SetFont(fonts_.Secondary());
        btnFontPlus->SetBackgroundColour(AncientColors::CELADON_MID);
        btnFontPlus->SetForegroundColour(AncientColors::INK_BLACK);
        btnFontPlus->Bind(wxEVT_BUTTON, &AncientHexFrame::OnFontPlus, this);
        toolSizer->Add(btnFontPlus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        // 创建"字体-"按钮
        wxButton* btnFontMinus = new wxButton(toolbar, wxID_ANY, wxT("字体-"),
            wxDefaultPosition, wxSize(80, 30));
        btnFontMinus->SetFont(fonts_.Secondary());
        btnFontMinus->SetBackgroundColour(AncientColors::CELADON_MID);
        btnFontMinus->SetForegroundColour(AncientColors::INK_BLACK);
        btnFontMinus->Bind(wxEVT_BUTTON, &AncientHexFrame::OnFontMinus, this);
        toolSizer->Add(btnFontMinus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

        // 为所有按钮添加悬停效果
        auto SetupHoverEffect = [](wxButton* btn) {
            btn->Bind(wxEVT_ENTER_WINDOW, [btn](wxMouseEvent&) {
                btn->SetBackgroundColour(AncientColors::CELADON_DARK);
                btn->Refresh();
                });

            btn->Bind(wxEVT_LEAVE_WINDOW, [btn](wxMouseEvent&) {
                btn->SetBackgroundColour(AncientColors::CELADON_MID);
                btn->Refresh();
                });
        };

        SetupHoverEffect(btnOpen);
        SetupHoverEffect(btn8Bytes);
        SetupHoverEffect(btn16Bytes);
        SetupHoverEffect(btn24Bytes);
        SetupHoverEffect(btn32Bytes);
        SetupHoverEffect(btnFontPlus);
        SetupHoverEffect(btnFontMinus);

        toolSizer->AddStretchSpacer();

        // 添加标题
        wxStaticText* title = new wxStaticText(toolbar, wxID_ANY,
            wxT("古风十六进制查看器"));
        title->SetFont(fonts_.Decorative());
        title->SetForegroundColour(AncientColors::CELADON_DARK);
        toolSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

        toolSizer->AddSpacer(20);
        toolbar->SetSizer(toolSizer);
        
    }

    void CreateMenu() {
        wxMenuBar* menuBar = new wxMenuBar();
        menuBar->SetBackgroundColour(AncientColors::CELADON_LIGHT);
        menuBar->SetForegroundColour(AncientColors::INK_BLACK);

        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_OPEN, wxT("打开(&O)...\tCtrl+O"));
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_EXIT, wxT("退出(&X)\tAlt+F4"));

        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(1001, wxT("8字节/行\tCtrl+1"));
        viewMenu->Append(1002, wxT("16字节/行\tCtrl+2"));
        viewMenu->Append(1003, wxT("24字节/行\tCtrl+3"));
        viewMenu->Append(1004, wxT("32字节/行\tCtrl+4"));
        viewMenu->AppendSeparator();
        viewMenu->Append(1005, wxT("增大字体\tCtrl++"));
        viewMenu->Append(1006, wxT("减小字体\tCtrl+-"));

        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, wxT("关于(&A)..."));

        menuBar->Append(fileMenu, wxT("文件(&F)"));
        menuBar->Append(viewMenu, wxT("视图(&V)"));
        menuBar->Append(helpMenu, wxT("帮助(&H)"));

        SetMenuBar(menuBar);

        // 绑定菜单事件
        Bind(wxEVT_MENU, &AncientHexFrame::OnOpen, this, wxID_OPEN);
        Bind(wxEVT_MENU, &AncientHexFrame::OnExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &AncientHexFrame::OnAbout, this, wxID_ABOUT);
        Bind(wxEVT_MENU, &AncientHexFrame::OnBytes8, this, 1001);
        Bind(wxEVT_MENU, &AncientHexFrame::OnBytes16, this, 1002);
        Bind(wxEVT_MENU, &AncientHexFrame::OnBytes24, this, 1003);
        Bind(wxEVT_MENU, &AncientHexFrame::OnBytes32, this, 1004);
        Bind(wxEVT_MENU, &AncientHexFrame::OnFontPlus, this, 1005);
        Bind(wxEVT_MENU, &AncientHexFrame::OnFontMinus, this, 1006);

        // 快捷键
        wxAcceleratorEntry entries[] = {
            {wxACCEL_CTRL, 'O', wxID_OPEN},
            {wxACCEL_CTRL, '1', 1001},
            {wxACCEL_CTRL, '2', 1002},
            {wxACCEL_CTRL, '3', 1003},
            {wxACCEL_CTRL, '4', 1004},
            {wxACCEL_CTRL, '+', 1005},
            {wxACCEL_CTRL, '-', 1006},
        };
        wxAcceleratorTable accel(WXSIZEOF(entries), entries);
        SetAcceleratorTable(accel);
    }

    void CreateStatusBar() {
        statusBar_ = wxFrame::CreateStatusBar(2);
        statusBar_->SetStatusText(wxT("就绪"), 0);
        statusBar_->SetStatusText(wxT("古风典雅 · 十六进制查看器"), 1);

        // 状态栏样式
        statusBar_->SetBackgroundColour(AncientColors::CELADON_LIGHT);
        statusBar_->SetForegroundColour(AncientColors::INK_BLACK);
    }

    wxBitmap CreateAppIcon() {
        wxBitmap icon(32, 32);
        wxMemoryDC dc;
        dc.SelectObject(icon);

        // 绘制古风图标：古书与八卦
        dc.SetBackground(wxBrush(AncientColors::CELADON_DARK));
        dc.Clear();

        dc.SetPen(wxPen(AncientColors::AMBER_GOLD, 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        // 外圆
        dc.DrawCircle(16, 16, 12);

        // 八卦符号简化
        dc.DrawLine(16, 4, 16, 12);   // 阳爻
        dc.DrawLine(12, 20, 20, 20);  // 阴爻

        // 书页装饰
        dc.SetPen(wxPen(AncientColors::VERMILION, 1));
        dc.DrawLine(8, 8, 24, 8);
        dc.DrawLine(8, 24, 24, 24);

        dc.SelectObject(wxNullBitmap);
        return icon;
    }

    void OnOpen(wxCommandEvent&) {
        wxFileDialog dialog(this, wxT("选择文件"),
            wxEmptyString, wxEmptyString,
            wxT("所有文件 (*.*)|*.*"),
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (dialog.ShowModal() == wxID_OK) {
            wxString filename = dialog.GetPath();
            hexView_->LoadFile(filename);

            wxFileName fn(filename);
            SetTitle(wxString::Format(wxT("古风十六进制 - %s"),
                fn.GetFullName()));

            statusBar_->SetStatusText(
                wxString::Format(wxT("已加载: %s"), filename), 0);
        }
    }

    void OnExit(wxCommandEvent&) {
        Close(true);
    }

    void OnAbout(wxCommandEvent&) {
        wxMessageBox(
            wxT("古风十六进制查看器\n\n")
            wxT("版本 1.0\n")
            wxT("设计灵感源自中国传统美学\n")
            wxT("青瓷色调 · 宣纸纹理 · 云纹装饰\n\n")
            wxT("功能特性：\n")
            wxT("• 优雅的古风界面设计\n")
            wxT("• 高性能文件渲染\n")
            wxT("• 支持大文件浏览\n")
            wxT("• 多种视图模式\n")
            wxT("• 跨平台支持"),
            wxT("关于"), wxOK | wxICON_INFORMATION, this);
    }

    void OnBytes8(wxCommandEvent&) { hexView_->SetBytesPerLine(8); }
    void OnBytes16(wxCommandEvent&) { hexView_->SetBytesPerLine(16); }
    void OnBytes24(wxCommandEvent&) { hexView_->SetBytesPerLine(24); }
    void OnBytes32(wxCommandEvent&) { hexView_->SetBytesPerLine(32); }

    void OnFontPlus(wxCommandEvent&) {
        static int size = 11;
        if (size < 20) {
            size++;
            hexView_->SetFontSize(size);
        }
    }

    void OnFontMinus(wxCommandEvent&) {
        static int size = 11;
        if (size > 8) {
            size--;
            hexView_->SetFontSize(size);
        }
    }
};

// ===================== 应用程序 =====================
class AncientHexApp : public wxApp {
public:
    virtual bool OnInit() override {
        // 设置应用程序信息
        SetAppName(wxT("AncientHexViewer"));
        SetAppDisplayName(wxT("古风十六进制查看器"));

        // 设置字体编码
        wxLocale locale;
        if (!locale.Init(wxLANGUAGE_CHINESE_SIMPLIFIED)) {
            locale.Init(wxLANGUAGE_ENGLISH);
        }

        // 创建主窗口
        AncientHexFrame* frame = new AncientHexFrame(wxT("古风十六进制查看器"));
        SetTopWindow(frame);

        return true;
    }
};

wxIMPLEMENT_APP(AncientHexApp);