#pragma once

#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include "Font.h"
#include "HexData.h"


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
    AncientHexView(wxWindow* parent);
       
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
