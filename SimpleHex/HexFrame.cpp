#include "HexFrame.h"

HexFrame::HexFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title,
        wxDefaultPosition, wxSize(1000, 700)) {

    SetMinSize(wxSize(800, 600));

    // 设置窗口图标
    wxIcon icon;
    icon.CopyFromBitmap(CreateAppIcon());
    SetIcon(icon);

    CreateUI();
    CreateMenu();
    CreateStatusBar();

    Center();
    Show(true);

    Bind(wxEVT_MOTION, &HexFrame::OnMouseMove, this);
}

void HexFrame::CreateUI()
{
    // 主面板
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(AncientColors::RICE_PAPER);

    // 十六进制视图
    m_hexView = new HexView(panel);
    m_hexView->SetBackgroundColour(AncientColors::RICE_PAPER);

    // 工具栏
    CreateToolBar(panel);

    // 布局
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_hexView, 1, wxEXPAND | wxALL);
    panel->SetSizer(sizer);
}

void HexFrame::CreateToolBar(wxWindow* parent)
{
    wxToolBar* toolbar = wxFrame::CreateToolBar();
    //wxPanel* toolbar = new wxPanel(parent, wxID_ANY);
    toolbar->SetBackgroundColour(AncientColors::CELADON_LIGHT);

    wxBoxSizer* toolSizer = new wxBoxSizer(wxHORIZONTAL);

    toolSizer->AddSpacer(20);

    // 创建"打开"按钮
    wxButton* btnOpen = new wxButton(toolbar, wxID_ANY, wxT("打开"),
        wxDefaultPosition, wxSize(80, 30));
    btnOpen->SetFont(m_fonts.Secondary());
    btnOpen->SetBackgroundColour(AncientColors::CELADON_MID);
    btnOpen->SetForegroundColour(AncientColors::INK_BLACK);
    btnOpen->Bind(wxEVT_BUTTON, &HexFrame::OnOpen, this);
    toolSizer->Add(btnOpen, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

   
    // 创建"8字节"按钮
    wxButton* btn8Bytes = new wxButton(toolbar, wxID_ANY, wxT("8字节"),
        wxDefaultPosition, wxSize(80, 30));
    btn8Bytes->SetFont(m_fonts.Secondary());
    btn8Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
    btn8Bytes->SetForegroundColour(AncientColors::INK_BLACK);
    btn8Bytes->Bind(wxEVT_BUTTON, &HexFrame::OnBytes8, this);
    toolSizer->Add(btn8Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // 创建"16字节"按钮
    wxButton* btn16Bytes = new wxButton(toolbar, wxID_ANY, wxT("16字节"),
        wxDefaultPosition, wxSize(80, 30));
    btn16Bytes->SetFont(m_fonts.Secondary());
    btn16Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
    btn16Bytes->SetForegroundColour(AncientColors::INK_BLACK);
    btn16Bytes->Bind(wxEVT_BUTTON, &HexFrame::OnBytes16, this);
    toolSizer->Add(btn16Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // 创建"24字节"按钮
    wxButton* btn24Bytes = new wxButton(toolbar, wxID_ANY, wxT("24字节"),
        wxDefaultPosition, wxSize(80, 30));
    btn24Bytes->SetFont(m_fonts.Secondary());
    btn24Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
    btn24Bytes->SetForegroundColour(AncientColors::INK_BLACK);
    btn24Bytes->Bind(wxEVT_BUTTON, &HexFrame::OnBytes24, this);
    toolSizer->Add(btn24Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // 创建"32字节"按钮
    wxButton* btn32Bytes = new wxButton(toolbar, wxID_ANY, wxT("32字节"),
        wxDefaultPosition, wxSize(80, 30));
    btn32Bytes->SetFont(m_fonts.Secondary());
    btn32Bytes->SetBackgroundColour(AncientColors::CELADON_MID);
    btn32Bytes->SetForegroundColour(AncientColors::INK_BLACK);
    btn32Bytes->Bind(wxEVT_BUTTON, &HexFrame::OnBytes32, this);
    toolSizer->Add(btn32Bytes, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    // 创建"字体+"按钮
    wxButton* btnFontPlus = new wxButton(toolbar, wxID_ANY, wxT("字体+"),
        wxDefaultPosition, wxSize(80, 30));
    btnFontPlus->SetFont(m_fonts.Secondary());
    btnFontPlus->SetBackgroundColour(AncientColors::CELADON_MID);
    btnFontPlus->SetForegroundColour(AncientColors::INK_BLACK);
    btnFontPlus->Bind(wxEVT_BUTTON, &HexFrame::OnFontPlus, this);
    toolSizer->Add(btnFontPlus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // 创建"字体-"按钮
    wxButton* btnFontMinus = new wxButton(toolbar, wxID_ANY, wxT("字体-"),
        wxDefaultPosition, wxSize(80, 30));
    btnFontMinus->SetFont(m_fonts.Secondary());
    btnFontMinus->SetBackgroundColour(AncientColors::CELADON_MID);
    btnFontMinus->SetForegroundColour(AncientColors::INK_BLACK);
    btnFontMinus->Bind(wxEVT_BUTTON, &HexFrame::OnFontMinus, this);
    toolSizer->Add(btnFontMinus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    // 为所有按钮添加悬停效果
    auto SetupHoverEffect = [](wxButton* btn) {
        btn->Bind(wxEVT_ENTER_WINDOW, [btn](wxMouseEvent&) {
            btn->SetBackgroundColour(AncientColors::CELADON_DARK);
            btn->Refresh();
            });

        btn->Bind(wxEVT_LEAVE_WINDOW, [btn](wxMouseEvent&) {
            btn->SetBackgroundColour(AncientColors::CELADON_MID);
            btn->Refresh();
            });
    };

    SetupHoverEffect(btnOpen);
    SetupHoverEffect(btn8Bytes);
    SetupHoverEffect(btn16Bytes);
    SetupHoverEffect(btn24Bytes);
    SetupHoverEffect(btn32Bytes);
    SetupHoverEffect(btnFontPlus);
    SetupHoverEffect(btnFontMinus);

    toolSizer->AddStretchSpacer();

    // 添加标题
    wxStaticText* title = new wxStaticText(toolbar, wxID_ANY,
        wxT("古风十六进制查看器"));
    title->SetFont(m_fonts.Decorative());
    title->SetForegroundColour(AncientColors::CELADON_DARK);
    toolSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    toolSizer->AddSpacer(20);
    toolbar->SetSizer(toolSizer);

}

void HexFrame::CreateMenu()
{
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->SetBackgroundColour(AncientColors::CELADON_LIGHT);
    menuBar->SetForegroundColour(AncientColors::INK_BLACK);

    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_OPEN, wxT("打开(&O)...\tCtrl+O"));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, wxT("退出(&X)\tAlt+F4"));

    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(1001, wxT("8字节/行\tCtrl+1"));
    viewMenu->Append(1002, wxT("16字节/行\tCtrl+2"));
    viewMenu->Append(1003, wxT("24字节/行\tCtrl+3"));
    viewMenu->Append(1004, wxT("32字节/行\tCtrl+4"));
    viewMenu->AppendSeparator();
    viewMenu->Append(1005, wxT("增大字体\tCtrl++"));
    viewMenu->Append(1006, wxT("减小字体\tCtrl+-"));

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, wxT("关于(&A)..."));

    menuBar->Append(fileMenu, wxT("文件(&F)"));
    menuBar->Append(viewMenu, wxT("视图(&V)"));
    menuBar->Append(helpMenu, wxT("帮助(&H)"));

    SetMenuBar(menuBar);

    // 绑定菜单事件
    Bind(wxEVT_MENU, &HexFrame::OnOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &HexFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &HexFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &HexFrame::OnBytes8, this, 1001);
    Bind(wxEVT_MENU, &HexFrame::OnBytes16, this, 1002);
    Bind(wxEVT_MENU, &HexFrame::OnBytes24, this, 1003);
    Bind(wxEVT_MENU, &HexFrame::OnBytes32, this, 1004);
    Bind(wxEVT_MENU, &HexFrame::OnFontPlus, this, 1005);
    Bind(wxEVT_MENU, &HexFrame::OnFontMinus, this, 1006);



    // 快捷键
    wxAcceleratorEntry entries[] = {
        {wxACCEL_CTRL, 'O', wxID_OPEN},
        {wxACCEL_CTRL, '1', 1001},
        {wxACCEL_CTRL, '2', 1002},
        {wxACCEL_CTRL, '3', 1003},
        {wxACCEL_CTRL, '4', 1004},
        {wxACCEL_CTRL, '+', 1005},
        {wxACCEL_CTRL, '-', 1006},
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}

void HexFrame::CreateStatusBar()
{
    m_statusBar = wxFrame::CreateStatusBar(2);
    m_statusBar->SetStatusText(wxT("就绪"), 0);
    m_statusBar->SetStatusText(wxT("古风典雅 · 十六进制查看器"), 1);

    // 状态栏样式
    m_statusBar->SetBackgroundColour(AncientColors::CELADON_LIGHT);
    m_statusBar->SetForegroundColour(AncientColors::INK_BLACK);
}

wxBitmap HexFrame::CreateAppIcon()
{
    wxBitmap icon(32, 32);
    wxMemoryDC dc;
    dc.SelectObject(icon);

    // 绘制古风图标：古书与八卦
    dc.SetBackground(wxBrush(AncientColors::CELADON_DARK));
    dc.Clear();

    dc.SetPen(wxPen(AncientColors::AMBER_GOLD, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    // 外圆
    dc.DrawCircle(16, 16, 12);

    // 八卦符号简化
    dc.DrawLine(16, 4, 16, 12);   // 阳爻
    dc.DrawLine(12, 20, 20, 20);  // 阴爻

    // 书页装饰
    dc.SetPen(wxPen(AncientColors::VERMILION, 1));
    dc.DrawLine(8, 8, 24, 8);
    dc.DrawLine(8, 24, 24, 24);

    dc.SelectObject(wxNullBitmap);
    return icon;
}

void HexFrame::OnOpen(wxCommandEvent&)
{
    wxFileDialog dialog(this, wxT("选择文件"),
        wxEmptyString, wxEmptyString,
        wxT("所有文件 (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK) {
        wxString filename = dialog.GetPath();
        m_hexView->LoadFile(filename);

        wxFileName fn(filename);
        SetTitle(wxString::Format(wxT("古风十六进制 - %s"),
            fn.GetFullName()));

        m_statusBar->SetStatusText(
            wxString::Format(wxT("已加载: %s"), filename), 0);
    }
}

void HexFrame::OnExit(wxCommandEvent&)
{
    Close(true);
}

void HexFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(
        wxT("古风十六进制查看器\n\n")
        wxT("版本 1.0\n")
        wxT("设计灵感源自中国传统美学\n")
        wxT("青瓷色调 · 宣纸纹理 · 云纹装饰\n\n")
        wxT("功能特性：\n")
        wxT("• 优雅的古风界面设计\n")
        wxT("• 高性能文件渲染\n")
        wxT("• 支持大文件浏览\n")
        wxT("• 多种视图模式\n")
        wxT("• 跨平台支持"),
        wxT("关于"), wxOK | wxICON_INFORMATION, this);
}

void HexFrame::OnBytes8(wxCommandEvent&)
{
    m_hexView->SetBytesPerLine(8);
}

void HexFrame::OnFontPlus(wxCommandEvent&)
{
    int size = m_hexView->GetFontSize() + 1;
    m_hexView->SetFontSize(size);
}

void HexFrame::OnFontMinus(wxCommandEvent&)
{
    int size = m_hexView->GetFontSize() - 1;
    m_hexView->SetFontSize(size);
}

void HexFrame::OnBytes16(wxCommandEvent&)
{
    m_hexView->SetBytesPerLine(16);
}

void HexFrame::OnBytes24(wxCommandEvent&)
{
    m_hexView->SetBytesPerLine(24);
}

void HexFrame::OnBytes32(wxCommandEvent&)
{
    m_hexView->SetBytesPerLine(32);
}


void HexFrame::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();

    
}