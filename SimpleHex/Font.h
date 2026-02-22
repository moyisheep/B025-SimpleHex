#pragma once
#include <wx/font.h>

// ===================== 古风字体助手 =====================
class AncientFont {
private:
    wxFont m_primaryFont;
    wxFont m_secondaryFont;
    wxFont m_decorativeFont;

public:
    AncientFont() {
        // 尝试使用优雅字体，回退到等宽字体
#ifdef __WXMSW__
        m_primaryFont = wxFont(12, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Consolas"));
        m_secondaryFont = wxFont(11, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_decorativeFont = wxFont(14, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#elif __WXOSX__
        m_primaryFont = wxFont(13, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Monaco"));
        m_secondaryFont = wxFont(12, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_decorativeFont = wxFont(15, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#else
        m_primaryFont = wxFont(11, wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
            false, wxT("Monospace"));
        m_secondaryFont = wxFont(10, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_decorativeFont = wxFont(12, wxFONTFAMILY_DECORATIVE,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
#endif
    }

    wxFont& Primary() { return m_primaryFont; }
    wxFont& Secondary() { return m_secondaryFont; }
    wxFont& Decorative() { return m_decorativeFont; }

    void SetSize(int size) {
        m_primaryFont.SetPointSize(size);
        m_secondaryFont.SetPointSize(size - 1);
        m_decorativeFont.SetPointSize(size + 2);
    }
};