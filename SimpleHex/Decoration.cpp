#include "Decoration.h"
#include "Colors.h"

void AncientDecoration::DrawClassicBorder(wxDC& dc, const wxRect& rect)
{
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

void AncientDecoration::DrawScrollBar(wxDC& dc, const wxRect& rect, double position, double thumbSize)
{
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

void AncientDecoration::DrawHeader(wxDC& dc, const wxString& text, const wxRect& rect)
{
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

void AncientDecoration::DrawSeparator(wxDC& dc, int x, int y, int width)
{
    // 传统分隔线
    wxPen pen1(AncientColors::CELADON_DARK, 1);
    wxPen pen2(AncientColors::RICE_PAPER, 1);

    for (int i = 0; i < 3; i++) {
        dc.SetPen(i % 2 == 0 ? pen1 : pen2);
        dc.DrawLine(x, y + i, x + width, y + i);
    }
}

wxColor AncientDecoration::BlendColors(const wxColor& c1, const wxColor& c2, double ratio)
{
    int r = c1.Red() * (1 - ratio) + c2.Red() * ratio;
    int g = c1.Green() * (1 - ratio) + c2.Green() * ratio;
    int b = c1.Blue() * (1 - ratio) + c2.Blue() * ratio;
    return wxColor(r, g, b);
}
