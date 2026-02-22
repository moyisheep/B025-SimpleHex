#pragma once

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

#include "HexView.h"
#include "Font.h"
#include "Colors.h"

// ===================== 主框架 =====================
class AncientHexFrame : public wxFrame {
private:
    AncientHexView* hexView_;
    wxStatusBar* statusBar_;
    AncientFont fonts_;

public:
    AncientHexFrame(const wxString& title);
       

private:
    void CreateUI(); 

    void CreateToolBar(wxWindow* parent); 

    void CreateMenu(); 

    void CreateStatusBar(); 
    wxBitmap CreateAppIcon(); 

    void OnOpen(wxCommandEvent&); 
    void OnExit(wxCommandEvent&); 

    void OnAbout(wxCommandEvent&);

    void OnBytes8(wxCommandEvent&); 
    void OnBytes16(wxCommandEvent&); 
    void OnBytes24(wxCommandEvent&);
    void OnBytes32(wxCommandEvent&); 

    void OnFontPlus(wxCommandEvent&);

    void OnFontMinus(wxCommandEvent&); 
};