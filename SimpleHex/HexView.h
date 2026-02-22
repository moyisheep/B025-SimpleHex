#pragma once

#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include "Font.h"
#include "HexData.h"


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

    // 选择状态
    size_t m_selectionStart;
    size_t m_selectionEnd;
    bool m_hasSelection;

    // 渲染缓存
    wxBitmap m_renderCache;
    bool m_isCacheValid;

    // 鼠标状态
    wxPoint m_mousePos;
    bool m_isDragging;

    int m_titleHeaderHeight;
    int m_columnHeaderHeight;
    int m_columnAddressHeight;
    int m_padding;

public:
    HexView(wxWindow* parent);
       
    void LoadFile(const wxString& filename); 

    void SetBytesPerLine(int bytes); 

    void SetFontSize(int size);

private:
    void InitStyles(); 
    void CalculateMetrics(); 

    void SetupEvents(); 

    void UpdateScrollbars(); 

    wxPoint GetTextPosition() const;

    wxRect GetContentRect() const; 

    void OnPaint(wxPaintEvent&); 

    void UpdateRenderCache();
    void RenderBackground(wxMemoryDC& dc, const wxSize& size); 

    void RenderColumnAddress(wxMemoryDC& dc, const wxPoint& pos);

    void RenderColumnHeaders(wxMemoryDC& dc, const wxPoint& pos);

    void RenderContent(wxMemoryDC& dc); 

    void RenderLine(wxMemoryDC& dc, int lineNum, size_t offset, int y); 
    void RenderCustomScrollbar(wxMemoryDC& dc); 

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
