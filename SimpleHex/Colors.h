#pragma once
#include <wx/colour.h>

// ===================== 古风配色方案 =====================
namespace AncientColors {
    // 主色调 - 青瓷系列
    const wxColor CELADON_LIGHT(224, 240, 233);    // 浅青瓷 - 背景


    const wxColor BORDER_INNER(169, 222, 209);      // 中青瓷 - 装饰
    const wxColor BORDER_OUTER(86, 179, 163);      // 深青瓷 - 标题

    // 辅助色 - 墨色系列
    const wxColor INK_BLACK(35, 31, 32);           // 浓墨



    const wxColor BYTE_00(160, 154, 146);        // 烟墨
    const wxColor BYTE_DISPLAY_CHAR(135, 188, 97);      // 竹青 - 正常文字
    const wxColor BYTE_CONTROL_CHAR(88, 154, 202);         // 宋蓝 - 链接
    const wxColor BYTE_FF(218, 87, 54);          // 朱砂红 - 高亮
    const wxColor BYTE_OTHER(191, 158, 208);            // 丁香紫 - 边框

    const wxColor BYTE_TEXT_NONDISPLAY(160, 154, 146);        // 烟墨
    const wxColor BYTE_TEXT_DISPLAY(35, 31, 32);           // 浓墨

    const wxColor HIGHLIGHT_FOREGROUND_HEX(218, 87, 54);          // 朱砂红 - 高亮
    const wxColor HIGHLIGHT_BACKGROUND_TEXT(88, 154, 202);         // 宋蓝 - 链接
    const wxColor HOVER_FOREGROUND(89, 87, 84);            // 淡墨


    const wxColor AMBER_GOLD(245, 188, 78);        // 琥珀金 - 强调



    // 特殊色
    const wxColor RICE_PAPER(253, 251, 247);       // 宣纸白
    const wxColor SILK_YELLOW(255, 249, 227);      // 绢黄
    const wxColor JADE_BLUE(105, 182, 202);        // 青玉
};
