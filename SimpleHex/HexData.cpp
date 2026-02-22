#include "HexData.h"

std::pair<wxString, wxString> HexData::GetLine(size_t offset, int bytesPerLine)  const
{
    wxString hexStr, asciiStr;
    for (int i = 0; i < bytesPerLine; i++) {
        size_t pos = offset + i;
        if (pos >= data_.size()) break;

        uint8_t byte = data_[pos];
        hexStr += wxString::Format(wxT("%02X "), byte);

        if (byte >= 32 && byte <= 126) {
            asciiStr += wxString::Format(wxT("%c"), byte);
        }
        else {
            asciiStr += wxT("·");
        }
    }
    return { hexStr.Trim(), asciiStr };
}

uint8_t HexData::GetByte(size_t offset) const
{
    return offset < data_.size() ? data_[offset] : 0;
}

bool HexData::LoadFromFile(const wxString& filename)
{
    wxFile file(filename);
    if (!file.IsOpened()) return false;

    wxFileOffset size = file.Length();
    if (size <= 0) return false;

    // 限制文件大小
    if (size > 100 * 1024 * 1024) {
        wxMessageBox(wxT("文件过大，建议使用专业工具处理"),
            wxT("提示"), wxOK | wxICON_WARNING);
        return false;
    }

    data_.resize(size);
    return file.Read(data_.data(), size) == size;
}
