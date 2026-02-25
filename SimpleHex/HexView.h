#pragma once

#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include "Font.h"
#include "HexData.h"
#include "Selection.h"

// ===================== 主视图类 =====================
class HexView : public wxScrolledWindow {
private:
    HexData m_data;
    AncientFont m_fonts;

    // 显示设置
    int m_bytesPerLine;
    int m_fontSize;
    int m_charWidth;
    int m_charHeight;
    int m_lineHeight;
    int m_rowAddressWidth;
    int m_rowHexWidth;


    Selection m_selection;

    // 渲染缓存
    wxBitmap m_renderCache;
    bool m_isCacheValid;

    // 鼠标状态
    size_t m_hoverIndex;
    bool m_isDragging;

    int m_titleHeaderHeight;
    int m_columnHeaderHeight;
    int m_columnAddressHeight;
    int m_padding;

    size_t m_scrollDelta;
    size_t m_totalLines;
public:
    HexView(wxWindow* parent);
       
    void LoadFile(const wxString& filename); 

    void SetBytesPerLine(int bytes);
    int GetFontSize() const;


    void SetFontSize(int size);

private:
    void InitStyles(); 
    void CalculateMetrics(); 

    void SetupEvents(); 

    void OnScroll(wxScrollWinEvent& event);

    void UpdateScrollbars();

    wxPoint GetTextPosition() const;

    wxRect GetContentRect() const; 

    wxRect GetIndexRect() const;

    void OnPaint(wxPaintEvent&); 

    //void UpdateBackgoundLayer();
    //void UpdateSelection();
    //void UpdateHoverLayer();
    //void UpdateMarkerLayer();
    //void UpdateContentLayer();
    void UpdateRenderCache();
    void RenderBackground(wxMemoryDC& dc, const wxSize& size);
    wxColor BlendColors(const wxColor& c1, const wxColor& c2, double ratio);

    void RenderTitleHeader(wxDC& dc, const wxString& text, const wxRect& rect);


    void RenderSeparator(wxDC& dc, int x, int y, int width);

    void RenderColumnAddress(wxMemoryDC& dc, const wxPoint& pos);

    void RenderBorder(wxDC& dc, const wxRect& rect);

    void RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos);

    void RenderContent(wxMemoryDC& dc);


    void RenderAddress(wxMemoryDC& dc, size_t offset, int x, int y);

    void RenderLineHex(wxMemoryDC& dc, size_t offset, int x, int y);

    void RenderLineHexText(wxMemoryDC& dc, size_t offset, int x, int y);

    void RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y);


    void RenderEmptyState(wxMemoryDC& dc, const wxSize& size);

    void DrawCloudPattern(wxMemoryDC& dc, int x, int y, int size); 
    wxColor GetByteColor(uint8_t byte) const; 

    size_t PosToIndex(const wxPoint& pos) const; 

    // 事件处理
    void OnSize(wxSizeEvent&); 

    void OnMouseWheel(wxMouseEvent& event); 

    void OnMouseDown(wxMouseEvent& event);

    void OnMouseUp(wxMouseEvent& event);

    void OnMouseMove(wxMouseEvent& event); 

    void OnKeyDown(wxKeyEvent& event); 
    void CopySelection(); 
};
