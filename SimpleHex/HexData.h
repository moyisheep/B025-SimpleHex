#pragma once

#include <wx/string.h>
#include <wx/file.h>
#include <wx/msgdlg.h>
#include <vector>

// ===================== 数据管理器 =====================
class HexData {
private:
    std::vector<uint8_t> data_;
    size_t pageSize_;

public:
    HexData() : pageSize_(65536) {}

    bool LoadFromFile(const wxString& filename); 

    void Clear() { data_.clear(); }
    bool IsEmpty() const { return data_.empty(); }
    size_t Size() const { return data_.size(); }
    const uint8_t* Data() const { return data_.data(); }

    uint8_t GetByte(size_t offset) const; 

    std::pair<wxString, wxString> GetLine(size_t offset, int bytesPerLine) const;
};