#pragma once
#include <wx/app.h>
#include "HexFrame.h"
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