#include "HexView.h"
#include "Colors.h"
#include <wx/clipbrd.h>
#include "Debug.h"
HexView::HexView(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxBORDER_NONE | wxVSCROLL),
    m_bytesPerLine(16),
    m_fontSize(11),
    m_hoverIndex(0),
    m_isCacheValid(false),
    m_isDragging(false),
    m_titleHeaderHeight(50), 
    m_scrollDelta(0), 
    m_totalLines(0)
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
        m_scrollDelta = 0;
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
        CalculateMetrics();
        UpdateScrollbars();
        Refresh();
    }
}
int HexView::GetFontSize() const
{
    return m_fontSize;
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
    m_rowAddressWidth = m_charWidth * 8 + m_padding * 2;
    m_rowHexWidth = m_bytesPerLine * m_charWidth * 3 + m_padding * 2;

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
    // 添加滚动事件
    Bind(wxEVT_SCROLLWIN_TOP, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_BOTTOM, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_LINEUP, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_LINEDOWN, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_PAGEUP, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_PAGEDOWN, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_THUMBTRACK, &HexView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &HexView::OnScroll, this);
}

void HexView::OnScroll(wxScrollWinEvent& event)
{
    event.Skip();  // 让父类处理滚动
    
    m_scrollDelta = event.GetPosition();
    // 滚动后需要刷新显示
    m_isCacheValid = false;
    Refresh();
}
void HexView::UpdateScrollbars()
{

    if (m_data.IsEmpty()) {
        SetScrollbars(0, m_lineHeight, 0, 0);
        return;
    }

    m_totalLines = std::ceill((m_data.Size() ) / m_bytesPerLine);
    //int visibleLines = GetClientSize().GetHeight() / m_lineHeight;

    SetScrollbars(0, m_lineHeight, 0, m_totalLines, 0, m_scrollDelta);
}

wxPoint HexView::GetTextPosition() const
{
    return wxPoint(30, m_titleHeaderHeight + 
        m_columnHeaderHeight + 
        m_columnAddressHeight ); // 留出装饰空间
}

wxString HexView::GetHoverAddress() const
{
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << m_hoverIndex << "\n";
    return ss.str();
}
wxRect HexView::GetContentRect() const
{
    wxSize size = GetClientSize();
    return wxRect(m_padding, m_titleHeaderHeight, size.GetWidth() - m_padding*2, size.GetHeight() - m_titleHeaderHeight - m_padding);
}



void HexView::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    //DoPrepareDC(dc);

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
        wxColor color = BlendColors(
            AncientColors::SILK_YELLOW, AncientColors::RICE_PAPER, ratio);
        dc.SetPen(wxPen(color));
        dc.DrawLine(0, y, size.GetWidth(), y);
    }

    for (int y = 0; y < bottomRect.height; y++) {
        double ratio = static_cast<double>(y) / bottomRect.height;
        wxColor color = BlendColors(
            AncientColors::RICE_PAPER, AncientColors::SILK_YELLOW, ratio);
        dc.SetPen(wxPen(color));
        dc.DrawLine(0, bottomRect.y + y, size.GetWidth(), bottomRect.y + y);
    }

    // 绘制标题
    RenderTitleHeader(dc, wxT("古风十六进制查看器"),
        wxRect(0, 0, size.GetWidth(), m_titleHeaderHeight));

    

    

    if(!m_data.IsEmpty())
    {
        // 列标题
        wxPoint textPos = GetTextPosition();
        RenderColumnHeaders(dc, wxPoint(textPos.x, textPos.y - m_columnHeaderHeight - m_columnAddressHeight));

        // 列地址
        RenderColumnAddress(dc, wxPoint(textPos.x, textPos.y - m_columnHeaderHeight));
    }
    else
    {
        // 内容区域边框
        RenderBorder(dc, GetContentRect());
        //RenderBorder(dc, wxRect(m_padding, m_titleHeaderHeight, size.GetWidth() - m_padding * 2, size.GetHeight() - m_titleHeaderHeight - m_padding));
    }
}
wxColor HexView::BlendColors(const wxColor& c1, const wxColor& c2, double ratio)
{
    int r = c1.Red() * (1 - ratio) + c2.Red() * ratio;
    int g = c1.Green() * (1 - ratio) + c2.Green() * ratio;
    int b = c1.Blue() * (1 - ratio) + c2.Blue() * ratio;
    return wxColor(r, g, b);
}
void HexView::RenderTitleHeader(wxDC& dc, const wxString& text, const wxRect& rect)
{
    // 渐变背景
    wxRect gradRect = rect;
    gradRect.height = 40;

    wxColour topColor = AncientColors::BORDER_OUTER;
    wxColour bottomColor = AncientColors::BORDER_INNER;

    for (int y = 0; y < gradRect.height; y++) {
        double ratio = static_cast<double>(y) / gradRect.height;
        wxColor color = BlendColors(topColor, bottomColor, ratio);
        dc.SetPen(wxPen(color));
        dc.DrawLine(gradRect.x, gradRect.y + y,
            gradRect.x + gradRect.width, gradRect.y + y);
    }

    // 装饰花纹
    dc.SetPen(wxPen(AncientColors::HIGHLIGHT_FOREGROUND_HEX, 1));
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
void HexView::RenderBorder(wxDC& dc, const wxRect& rect)
{
    // 绘制双层边框
    dc.SetPen(wxPen(AncientColors::BORDER_OUTER, 2));
    dc.DrawRectangle(rect);

    // 内边框
    wxRect innerRect = rect;
    innerRect.Deflate(2);
    dc.SetPen(wxPen(AncientColors::BORDER_INNER, 1));
    dc.DrawRectangle(innerRect);

    // 角落装饰
    int cornerSize = 8;
    dc.SetPen(wxPen(AncientColors::HIGHLIGHT_FOREGROUND_HEX, 2));

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
void HexView::RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos)
{


    dc.SetFont(m_fonts.Secondary());
    dc.SetTextForeground(AncientColors::BORDER_OUTER);


    // 偏移地址标题
    dc.DrawText(wxT("偏移地址"), pos.x, pos.y + m_padding/2);

    // 十六进制标题
    int hexX = pos.x + m_rowAddressWidth;
    dc.DrawText(wxT("十六进制"), hexX, pos.y + m_padding / 2);

    // ASCII标题
    int asciiX = hexX + m_rowHexWidth;
    dc.DrawText(wxT("字符"), asciiX, pos.y + m_padding / 2);

    // 分隔线
    RenderSeparator(dc, pos.x, pos.y + m_columnHeaderHeight,
        GetContentRect().width - 20);
}

void HexView::RenderSeparator(wxDC& dc, int x, int y, int width)
{
    // 传统分隔线
    wxPen pen1(AncientColors::BORDER_OUTER, 1);
    wxPen pen2(AncientColors::RICE_PAPER, 1);

    for (int i = 0; i < 3; i++) {
        dc.SetPen(i % 2 == 0 ? pen1 : pen2);
        dc.DrawLine(x, y + i, x + width, y + i);
    }
}
void HexView::RenderColumnAddress(wxMemoryDC& dc, const wxPoint& pos)
{

    dc.SetFont(m_fonts.Primary());
    dc.SetTextForeground(AncientColors::HOVER_FOREGROUND);


    // 十六进制数据
    int hexX = pos.x + m_rowAddressWidth;
    for (int i = 0; i < m_bytesPerLine; i++) {

        wxString byteStr = wxString::Format(wxT("%02X"), i);


        dc.DrawText(byteStr, hexX + i * 3 * m_charWidth, pos.y + m_padding / 3);

        // 字节间分隔点
        if (i < m_bytesPerLine - 1) {
            dc.SetPen(wxPen(AncientColors::BORDER_INNER, 1));
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


    int startLine = m_scrollDelta;
    //int startLine = GetViewStart().y / m_lineHeight;
    int visibleLines = GetClientSize().GetHeight() / m_lineHeight ;

    for (int line = 0; line < visibleLines; line++) {
        size_t offset = (startLine + line) * m_bytesPerLine;
        if (offset >= m_data.Size()) break;

        int y = textPos.y + line * m_lineHeight ;
        RenderLine(dc, startLine + line, offset, y);
    }


}

void HexView::RenderAddress(wxMemoryDC& dc, size_t offset, int x, int y)
{
    wxString offsetStr = wxString::Format(wxT("%08X"),
        static_cast<unsigned int>(offset));
    dc.SetFont(m_fonts.Primary());
    dc.SetTextForeground(AncientColors::HOVER_FOREGROUND);
    dc.DrawText(offsetStr, x, y);
}

void HexView::RenderLineHex(wxMemoryDC& dc, size_t offset, int x, int y)
{
    for (int i = 0; i < m_bytesPerLine; i++) {
        size_t index = offset + i;
        if (index >= m_data.Size()) break;

        uint8_t byte = m_data.GetByte(index);
        wxString byteStr = wxString::Format(wxT("%02X"), byte);

        // 高亮选择
        if (m_selection.HasSelection(index)) {
            dc.SetPen(wxPen(AncientColors::HIGHLIGHT_FOREGROUND_HEX, 1));
            dc.SetBrush(wxBrush(AncientColors::HIGHLIGHT_FOREGROUND_HEX.ChangeLightness(180)));
            dc.DrawRectangle(x + i * 3 * m_charWidth - 2,
                y - 1,
                m_charWidth * 2 + 4,
                m_charHeight + 2);
        }

        // 鼠标悬浮
        if (m_hoverIndex == index)
        {
            dc.SetPen(wxPen(AncientColors::HOVER_FOREGROUND, 1));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(x + i * 3 * m_charWidth - 2,
                y - 1,
                m_charWidth * 2 + 4,
                m_charHeight + 2);
        }

        // 颜色编码
        wxColor textColor = GetByteColor(byte);
        dc.SetTextForeground(textColor);

        dc.DrawText(byteStr, x + i * 3 * m_charWidth, y);

        // 字节间分隔点
        if (i < m_bytesPerLine - 1) {
            dc.SetPen(wxPen(AncientColors::BORDER_INNER, 1));
            dc.DrawPoint(x + i * 3 * m_charWidth + 2 * m_charWidth + 4, y + m_charHeight / 2);
        }
    }
}

void HexView::RenderLineHexText(wxMemoryDC& dc, size_t offset, int x, int y)
{
    auto [hexStr, asciiStr] = m_data.GetLine(offset, m_bytesPerLine);

    dc.SetFont(m_fonts.Primary());
    for (size_t i = 0; i < asciiStr.length(); i++) {
        size_t index = offset + i;

        // 高亮选择
        if (m_selection.HasSelection(index)) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(AncientColors::HIGHLIGHT_BACKGROUND_TEXT.ChangeLightness(180)));
            dc.DrawRectangle(x + i * m_charWidth,
                y,
                m_charWidth,
                m_charHeight);
        }

        // 鼠标悬浮
        if (m_hoverIndex == index)
        {
            dc.SetPen(wxPen(AncientColors::HOVER_FOREGROUND, 1));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(x + i * m_charWidth,
                y,
                m_charWidth,
                m_charHeight);
        }

        wxChar ch = asciiStr[i];
        wxColor color = (ch == wxT('·')) ?
            AncientColors::BYTE_TEXT_NONDISPLAY :
            AncientColors::BYTE_TEXT_DISPLAY;

        dc.SetTextForeground(color);
        dc.DrawText(wxString(ch), x + i * m_charWidth, y);
    }
}

void HexView::RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y)
{
    wxPoint pos = GetTextPosition();

    // 偏移地址
    RenderAddress(dc, offset, pos.x, y);

    // 十六进制数据
    int hexX = pos.x + m_rowAddressWidth;

    RenderLineHex(dc, offset, hexX, y);

    // ASCII显示
    int asciiX = hexX + m_rowHexWidth;
    
    RenderLineHexText(dc, offset, asciiX, y);
}



void HexView::RenderEmptyState(wxMemoryDC& dc, const wxSize& size)
{
    dc.SetFont(m_fonts.Decorative());
    dc.SetTextForeground(AncientColors::BYTE_00);

    wxString message = wxT("打开文件以查看十六进制内容");
    wxSize textSize = dc.GetTextExtent(message);

    int x = (size.GetWidth() - textSize.GetWidth()) / 2;
    int y = (size.GetHeight() - textSize.GetHeight()) / 2;

    // 水墨风格文字
    dc.SetTextForeground(wxColor(0, 0, 0, 30));
    dc.DrawText(message, x + 2, y + 2);
    dc.SetTextForeground(AncientColors::HOVER_FOREGROUND);
    dc.DrawText(message, x, y);

    // 添加传统图案
    dc.SetPen(wxPen(AncientColors::BORDER_INNER, 2));
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
    if (byte == 0x00) return AncientColors::BYTE_00;
    if (byte == 0xFF) return AncientColors::BYTE_FF;
    if (byte >= 0x20 && byte <= 0x7E) return AncientColors::BYTE_DISPLAY_CHAR;
    if (byte < 0x20) return AncientColors::BYTE_CONTROL_CHAR;
    return AncientColors::BYTE_OTHER;
}

size_t HexView::PosToIndex(const wxPoint& pos) const
{
    wxPoint textPos = GetTextPosition();
    int line = (pos.y - textPos.y) / m_lineHeight;
    if (line < 0) return 0;

    size_t offset = (m_scrollDelta + line) * m_bytesPerLine;

    int hexX = textPos.x + m_rowAddressWidth;
    if (pos.x >= hexX && pos.x < hexX + m_bytesPerLine * 3 * m_charWidth) {
        int col = (pos.x - hexX) / (3 * m_charWidth);
        return offset + col;
    }

    int asciiX = hexX + m_rowHexWidth;
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
    if (m_data.IsEmpty()) { return; }
    int lines = event.GetWheelRotation() / event.GetWheelDelta();
    m_scrollDelta = std::clamp(static_cast<int>(m_scrollDelta) - lines, 0, static_cast<int>(m_totalLines));
  
 
    m_isCacheValid = false;
    UpdateScrollbars();
    Refresh();
}

void HexView::OnMouseDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();


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

        m_isCacheValid = false;
        Refresh();
        
        
    }
    
    event.Skip();
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

