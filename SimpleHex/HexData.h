#pragma once

#include <wx/string.h>
#include <wx/file.h>
#include <wx/msgdlg.h>
#include <vector>

// ===================== 数据管理器 =====================
class HexData {
private:
    std::vector<uint8_t> m_data;
    size_t m_pageSize;

public:
    HexData() : m_pageSize(65536) {}

    bool LoadFromFile(const wxString& filename); 

    void Clear() { m_data.clear(); }
    bool IsEmpty() const { return m_data.empty(); }
    size_t Size() const { return m_data.size(); }
    const uint8_t* Data() const { return m_data.data(); }

    uint8_t GetByte(size_t offset) const; 

    std::pair<wxString, wxString> GetLine(size_t offset, int bytesPerLine) const;
};