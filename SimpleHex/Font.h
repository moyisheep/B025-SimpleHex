#pragma once
#include <wx/font.h>

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