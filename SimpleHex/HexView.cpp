#include "HexView.h"
#include "Colors.h"
#include "Decoration.h"
#include <wx/clipbrd.h>

AncientHexView::AncientHexView(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxBORDER_NONE | wxVSCROLL),
    bytesPerLine_(16),
    fontSize_(11),
    selectionStart_(0),
    selectionEnd_(0),
    hasSelection_(false),
    cacheValid_(false),
    dragging_(false),
    showDecorations_(true)
{

    InitStyles();
    SetupEvents();

    SetScrollRate(0, lineHeight_);
    SetDoubleBuffered(true);
}

void AncientHexView::LoadFile(const wxString& filename)
{
    if (data_.LoadFromFile(filename)) {
        cacheValid_ = false;
        hasSelection_ = false;
        UpdateScrollbars();
        Refresh();
    }
}

void AncientHexView::SetBytesPerLine(int bytes)
{
    if (bytes == 8 || bytes == 16 || bytes == 24 || bytes == 32) {
        bytesPerLine_ = bytes;
        cacheValid_ = false;
        UpdateScrollbars();
        Refresh();
    }
}

void AncientHexView::SetFontSize(int size)
{
    if (size >= 8 && size <= 20) {
        fontSize_ = size;
        fonts_.SetSize(size);
        CalculateMetrics();
        cacheValid_ = false;
        UpdateScrollbars();
        Refresh();
    }
}

void AncientHexView::InitStyles()
{
    SetBackgroundColour(AncientColors::RICE_PAPER);
    fonts_.SetSize(fontSize_);
    CalculateMetrics();
}

void AncientHexView::CalculateMetrics()
{
    wxClientDC dc(this);
    dc.SetFont(fonts_.Primary());
    wxSize charSize = dc.GetTextExtent("W");
    charWidth_ = charSize.GetWidth();
    charHeight_ = charSize.GetHeight();
    lineHeight_ = charHeight_ + 8;
}

void AncientHexView::SetupEvents()
{
    Bind(wxEVT_PAINT, &AncientHexView::OnPaint, this);
    Bind(wxEVT_SIZE, &AncientHexView::OnSize, this);
    Bind(wxEVT_MOUSEWHEEL, &AncientHexView::OnMouseWheel, this);
    Bind(wxEVT_LEFT_DOWN, &AncientHexView::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &AncientHexView::OnMouseUp, this);
    Bind(wxEVT_MOTION, &AncientHexView::OnMouseMove, this);
    Bind(wxEVT_KEY_DOWN, &AncientHexView::OnKeyDown, this);
}

void AncientHexView::UpdateScrollbars()
{
    if (data_.IsEmpty()) {
        SetScrollbars(0, lineHeight_, 0, 0);
        return;
    }

    int totalLines = (data_.Size() + bytesPerLine_ - 1) / bytesPerLine_;
    int visibleLines = GetClientSize().GetHeight() / lineHeight_;

    SetScrollbars(0, lineHeight_, 0, totalLines, 0, 0, true);
}

wxPoint AncientHexView::GetTextPosition() const
{
    return wxPoint(30, 60); // 留出装饰空间
}

wxRect AncientHexView::GetContentRect() const
{
    wxSize size = GetClientSize();
    return wxRect(10, 50, size.GetWidth() - 40, size.GetHeight() - 80);
}

void AncientHexView::OnPaint(wxPaintEvent&)
{
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

void AncientHexView::UpdateRenderCache()
{
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

void AncientHexView::RenderBackground(wxMemoryDC& dc, const wxSize& size)
{
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

void AncientHexView::RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos)
{
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

void AncientHexView::RenderContent(wxMemoryDC& dc)
{
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

void AncientHexView::RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y)
{
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

void AncientHexView::RenderCustomScrollbar(wxMemoryDC& dc)
{
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

void AncientHexView::RenderEmptyState(wxMemoryDC& dc, const wxSize& size)
{
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

void AncientHexView::DrawCloudPattern(wxMemoryDC& dc, int x, int y, int size)
{
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

wxColor AncientHexView::GetByteColor(uint8_t byte) const
{
    if (byte == 0x00) return AncientColors::INK_LIGHT;
    if (byte == 0xFF) return AncientColors::VERMILION;
    if (byte >= 0x20 && byte <= 0x7E) return AncientColors::BAMBOO_GREEN;
    if (byte < 0x20) return AncientColors::SONG_BLUE;
    return AncientColors::LILAC;
}

size_t AncientHexView::PosToIndex(const wxPoint& pos) const
{
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

void AncientHexView::OnSize(wxSizeEvent&)
{
    cacheValid_ = false;
    UpdateScrollbars();
    Refresh();
}

void AncientHexView::OnMouseWheel(wxMouseEvent& event)
{
    int lines = event.GetWheelRotation() / event.GetWheelDelta();
    Scroll(0, GetViewStart().y - lines * 3);
    cacheValid_ = false;
    Refresh();
}

void AncientHexView::OnMouseDown(wxMouseEvent& event)
{
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

void AncientHexView::OnMouseUp(wxMouseEvent& event)
{
    if (dragging_) {
        dragging_ = false;
        ReleaseMouse();
    }
}

void AncientHexView::OnMouseMove(wxMouseEvent& event)
{
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

void AncientHexView::OnKeyDown(wxKeyEvent& event)
{
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

void AncientHexView::CopySelection()
{
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

