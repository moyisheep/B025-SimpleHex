#pragma once

#include <wx/dc.h>
#include <wx/gdicmn.h>



// ===================== 古风装饰绘制器 =====================
class AncientDecoration {
public:
    static void DrawClassicBorder(wxDC& dc, const wxRect& rect); 

    static void DrawScrollBar(wxDC& dc, const wxRect& rect,
        double position, double thumbSize);

    static void DrawHeader(wxDC& dc, const wxString& text, const wxRect& rect); 

    static void DrawSeparator(wxDC& dc, int x, int y, int width); 

    static wxColor BlendColors(const wxColor& c1, const wxColor& c2, double ratio); 
};
