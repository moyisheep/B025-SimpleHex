#include "HexView.h"
#include "Colors.h"
#include "Decoration.h"
#include <wx/clipbrd.h>

HexView::HexView(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxBORDER_NONE | wxVSCROLL),
    m_bytesPerLine(16),
    m_fontSize(11),
    m_hoverIndex(0),
    m_isCacheValid(false),
    m_isDragging(false),
    m_titleHeaderHeight(50)
{

    InitStyles();
    SetupEvents();

    SetScrollRate(0, m_lineHeight);
    SetDoubleBuffered(true);
}

void HexView::LoadFile(const wxString& filename)
{
    if (m_data.LoadFromFile(filename)) {
        m_isCacheValid = false;
        m_selection.Clear();
        UpdateScrollbars();
        Refresh();
    }
}

void HexView::SetBytesPerLine(int bytes)
{
    if (bytes == 8 || bytes == 16 || bytes == 24 || bytes == 32) {
        m_bytesPerLine = bytes;
        m_isCacheValid = false;
        UpdateScrollbars();
        Refresh();
    }
}

void HexView::SetFontSize(int size)
{
    if (size >= 8 && size <= 20) {
        m_fontSize = size;
        m_fonts.SetSize(size);
        CalculateMetrics();
        m_isCacheValid = false;
        UpdateScrollbars();
        Refresh();
    }
}

void HexView::InitStyles()
{
    SetBackgroundColour(AncientColors::RICE_PAPER);
    m_fonts.SetSize(m_fontSize);
    CalculateMetrics();
}

void HexView::CalculateMetrics()
{
    wxClientDC dc(this);
    dc.SetFont(m_fonts.Primary());
    wxSize charSize = dc.GetTextExtent("W");
    m_charWidth = charSize.GetWidth();
    m_charHeight = charSize.GetHeight();

    m_padding = m_charWidth;
    m_lineHeight = m_charHeight + 8;
    m_columnAddressHeight = m_charHeight + m_padding;


    dc.SetFont(m_fonts.Secondary());
    wxSize secondaryCharSize = dc.GetTextExtent("W");
    m_columnHeaderHeight = secondaryCharSize.GetHeight() + m_padding;


}

void HexView::SetupEvents()
{
    Bind(wxEVT_PAINT, &HexView::OnPaint, this);
    Bind(wxEVT_SIZE, &HexView::OnSize, this);
    Bind(wxEVT_MOUSEWHEEL, &HexView::OnMouseWheel, this);
    Bind(wxEVT_LEFT_DOWN, &HexView::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &HexView::OnMouseUp, this);
    Bind(wxEVT_MOTION, &HexView::OnMouseMove, this);
    Bind(wxEVT_KEY_DOWN, &HexView::OnKeyDown, this);
}

void HexView::UpdateScrollbars()
{
    if (m_data.IsEmpty()) {
        SetScrollbars(0, m_lineHeight, 0, 0);
        return;
    }

    int totalLines = (m_data.Size() + m_bytesPerLine - 1) / m_bytesPerLine;
    int visibleLines = GetClientSize().GetHeight() / m_lineHeight;

    SetScrollbars(0, m_lineHeight, 0, totalLines, 0, 0, true);
}

wxPoint HexView::GetTextPosition() const
{
    return wxPoint(30, m_titleHeaderHeight + 
        m_columnHeaderHeight + 
        m_columnAddressHeight ); // 留出装饰空间
}

wxRect HexView::GetContentRect() const
{
    wxSize size = GetClientSize();
    return wxRect(10, m_titleHeaderHeight, size.GetWidth() - 40, size.GetHeight() - 60);
}

wxRect HexView::GetIndexRect() const
{
    return wxRect();
}

void HexView::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    DoPrepareDC(dc);

    if (!m_isCacheValid) {
        UpdateRenderCache();
    }

    wxMemoryDC memDC;
    memDC.SelectObject(m_renderCache);
    dc.Blit(0, 0, m_renderCache.GetWidth(), m_renderCache.GetHeight(),
        &memDC, 0, 0);
}

void HexView::UpdateRenderCache()
{
    wxSize size = GetClientSize();
    m_renderCache = wxBitmap(size.GetWidth(), size.GetHeight());

    wxMemoryDC dc(m_renderCache);
    RenderBackground(dc, size);

    if (!m_data.IsEmpty()) {
        RenderContent(dc);
    }
    else {
        RenderEmptyState(dc, size);
    }

    m_isCacheValid = true;
}

void HexView::RenderBackground(wxMemoryDC& dc, const wxSize& size)
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
        wxRect(0, 0, size.GetWidth(), m_titleHeaderHeight));

    // 内容区域边框
    wxRect contentRect = GetContentRect();
    AncientDecoration::DrawClassicBorder(dc, contentRect);

    if(!m_data.IsEmpty())
    {

    // 列标题
    wxPoint textPos = GetTextPosition();
    RenderColumnHeaders(dc, wxPoint(textPos.x, textPos.y - m_columnHeaderHeight - m_columnAddressHeight));

    // 列地址
    RenderColumnAddress(dc, wxPoint(textPos.x, textPos.y - m_columnHeaderHeight));
    }
}


void HexView::RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos)
{


    dc.SetFont(m_fonts.Secondary());
    dc.SetTextForeground(AncientColors::CELADON_DARK);


    // 偏移地址标题
    dc.DrawText(wxT("偏移地址"), pos.x, pos.y + m_padding/2);

    // 十六进制标题
    int hexX = pos.x + 80;
    dc.DrawText(wxT("十六进制"), hexX, pos.y + m_padding / 2);

    // ASCII标题
    int asciiX = hexX + m_bytesPerLine * 3 * m_charWidth + 40;
    dc.DrawText(wxT("字符"), asciiX, pos.y + m_padding / 2);

    // 分隔线
    AncientDecoration::DrawSeparator(dc, pos.x, pos.y + m_columnHeaderHeight,
        GetContentRect().width - 20);
}
void HexView::RenderColumnAddress(wxMemoryDC& dc, const wxPoint& pos)
{

    dc.SetFont(m_fonts.Primary());
    dc.SetTextForeground(AncientColors::INK_GRAY);


    // 十六进制数据
    int hexX = pos.x + 80;
    for (int i = 0; i < m_bytesPerLine; i++) {

        wxString byteStr = wxString::Format(wxT("%02X"), i);


        dc.DrawText(byteStr, hexX + i * 3 * m_charWidth, pos.y + m_padding / 3);

        // 字节间分隔点
        if (i < m_bytesPerLine - 1) {
            dc.SetPen(wxPen(AncientColors::CELADON_MID, 1));
            dc.DrawPoint(hexX + 
                i * 3 * m_charWidth + 
                2 * m_charWidth + 4, 
                pos.y + m_charHeight / 2 + m_padding / 2);
        }
    }
}
void HexView::RenderContent(wxMemoryDC& dc)
{
    wxPoint textPos = GetTextPosition();

    int startLine = GetViewStart().y / m_lineHeight;
    int visibleLines = GetClientSize().GetHeight() / m_lineHeight + 2;

    for (int line = 0; line < visibleLines; line++) {
        size_t offset = (startLine + line) * m_bytesPerLine;
        if (offset >= m_data.Size()) break;

        int y = textPos.y + line * m_lineHeight ;
        RenderLine(dc, startLine + line, offset, y);
    }


}

void HexView::RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y)
{
    wxPoint pos = GetTextPosition();

    // 偏移地址
    wxString offsetStr = wxString::Format(wxT("%08X"),
        static_cast<unsigned int>(offset));
    dc.SetFont(m_fonts.Primary());
    dc.SetTextForeground(AncientColors::INK_GRAY);
    dc.DrawText(offsetStr, pos.x, y);

    // 十六进制数据
    int hexX = pos.x + 80;

    for (int i = 0; i < m_bytesPerLine; i++) {
        size_t index = offset + i;
        if (index >= m_data.Size()) break;

        uint8_t byte = m_data.GetByte(index);
        wxString byteStr = wxString::Format(wxT("%02X"), byte);

        // 高亮选择
        if (m_selection.HasSelection(index)) {
            dc.SetPen(wxPen(AncientColors::VERMILION, 1));
            dc.SetBrush(wxBrush(AncientColors::VERMILION.ChangeLightness(180)));
            dc.DrawRectangle(hexX + i * 3 * m_charWidth - 2,
                y - 1,
                m_charWidth * 2 + 4,
                m_charHeight + 2);
        }

        // 鼠标悬浮
        if(m_hoverIndex == index)
        {
            dc.SetPen(wxPen(AncientColors::INK_GRAY, 1));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(hexX + i * 3 * m_charWidth - 2,
                y - 1,
                m_charWidth * 2 + 4,
                m_charHeight + 2);
        }

        // 颜色编码
        wxColor textColor = GetByteColor(byte);
        dc.SetTextForeground(textColor);

        dc.DrawText(byteStr, hexX + i * 3 * m_charWidth, y);

        // 字节间分隔点
        if (i < m_bytesPerLine - 1) {
            dc.SetPen(wxPen(AncientColors::CELADON_MID, 1));
            dc.DrawPoint(hexX + i * 3 * m_charWidth + 2 * m_charWidth + 4, y + m_charHeight / 2);
        }
    }

    // ASCII显示
    int asciiX = hexX + m_bytesPerLine * 3 * m_charWidth + 40;
    auto [hexStr, asciiStr] = m_data.GetLine(offset, m_bytesPerLine);

    dc.SetFont(m_fonts.Primary());
    for (size_t i = 0; i < asciiStr.length(); i++) {
        size_t index = offset + i;

        // 高亮选择
        if (m_selection.HasSelection(index)) {
            dc.SetPen(wxPen(AncientColors::SONG_BLUE, 1));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(AncientColors::SONG_BLUE.ChangeLightness(180)));
            dc.DrawRectangle(asciiX + i * m_charWidth ,
                y ,
                m_charWidth ,
                m_charHeight );
        }

        // 鼠标悬浮
        if (m_hoverIndex == index)
        {
            dc.SetPen(wxPen(AncientColors::INK_GRAY, 1));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(asciiX + i * m_charWidth,
                y,
                m_charWidth,
                m_charHeight);
        }

        wxChar ch = asciiStr[i];
        wxColor color = (ch == wxT('·')) ?
            AncientColors::INK_LIGHT :
            AncientColors::INK_BLACK;

        dc.SetTextForeground(color);
        dc.DrawText(wxString(ch), asciiX + i * m_charWidth, y);
    }
}



void HexView::RenderEmptyState(wxMemoryDC& dc, const wxSize& size)
{
    dc.SetFont(m_fonts.Decorative());
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

void HexView::DrawCloudPattern(wxMemoryDC& dc, int x, int y, int size)
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

wxColor HexView::GetByteColor(uint8_t byte) const
{
    if (byte == 0x00) return AncientColors::INK_LIGHT;
    if (byte == 0xFF) return AncientColors::VERMILION;
    if (byte >= 0x20 && byte <= 0x7E) return AncientColors::BAMBOO_GREEN;
    if (byte < 0x20) return AncientColors::SONG_BLUE;
    return AncientColors::LILAC;
}

size_t HexView::PosToIndex(const wxPoint& pos) const
{
    wxPoint textPos = GetTextPosition();
    int line = (pos.y - textPos.y) / m_lineHeight;
    if (line < 0) return 0;

    size_t offset = line * m_bytesPerLine;

    int hexX = textPos.x + 80;
    if (pos.x >= hexX && pos.x < hexX + m_bytesPerLine * 3 * m_charWidth) {
        int col = (pos.x - hexX) / (3 * m_charWidth);
        return offset + col;
    }

    int asciiX = hexX + m_bytesPerLine * 3 * m_charWidth + 40;
    if (pos.x >= asciiX && pos.x < asciiX + m_bytesPerLine * m_charWidth) {
        int col = (pos.x - asciiX) / m_charWidth;
        return offset + col;
    }

    return offset;
}

void HexView::OnSize(wxSizeEvent&)
{
    m_isCacheValid = false;
    UpdateScrollbars();
    Refresh();
}

void HexView::OnMouseWheel(wxMouseEvent& event)
{
    int lines = event.GetWheelRotation() / event.GetWheelDelta();
    Scroll(0, GetViewStart().y - lines * 3);
    m_isCacheValid = false;
    Refresh();
}

void HexView::OnMouseDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    pos += GetViewStart();

    size_t index = PosToIndex(pos);
    if (index < m_data.Size()) {
        m_selection.OnMouseDown(index);
        m_isCacheValid = false;
        Refresh();
    }
    m_isDragging = true;
    CaptureMouse();
}

void HexView::OnMouseUp(wxMouseEvent& event)
{
    if (m_isDragging) {
        m_isDragging = false;
        ReleaseMouse();
    }
}

void HexView::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    pos += GetViewStart();
    size_t index = PosToIndex(pos);

    if (m_isDragging && event.Dragging()) {
        if (index < m_data.Size()) {
            m_selection.OnMouseMove(index);
            m_isCacheValid = false;
            Refresh();
        }
    }
    if ( index != m_hoverIndex)
    {
        m_hoverIndex = index;
        if(m_hoverIndex)
        {
            m_isCacheValid = false;
            Refresh();
        }
    }

}

void HexView::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_UP:
        Scroll(0, GetViewStart().y - m_lineHeight);
        break;
    case WXK_DOWN:
        Scroll(0, GetViewStart().y + m_lineHeight);
        break;
    case WXK_PAGEUP:
        Scroll(0, GetViewStart().y - GetClientSize().GetHeight());
        break;
    case WXK_PAGEDOWN:
        Scroll(0, GetViewStart().y + GetClientSize().GetHeight());
        break;
    case 'A':
        if (event.ControlDown()) {
            m_selection.Set(0, m_data.Size() - 1);
            m_isCacheValid = false;
            Refresh();
        }
        break;
    case 'C':
        if (event.ControlDown() && m_selection.HasSelection()) {
            CopySelection();
        }
        break;
    }
    m_isCacheValid = false;
    Refresh();
    event.Skip();
}

void HexView::CopySelection()
{
    if (!m_selection.HasSelection() || m_data.IsEmpty()) return;

    size_t start = 0, end = 0;
    m_selection.Get(start, end);
    wxString hexStr, asciiStr;
    for (size_t i = start; i <= end && i < m_data.Size(); i++) {
        hexStr += wxString::Format(wxT("%02X "), m_data.GetByte(i));
        uint8_t byte = m_data.GetByte(i);
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

