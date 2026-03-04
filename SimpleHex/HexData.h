// HexData.h
#pragma once

#include <wx/string.h>
#include <wx/file.h>
#include <wx/msgdlg.h>
#include <vector>
#include <memory>
#include <ctime>

struct CacheBlock
{
    time_t lastAccessTime;
    size_t offset;
    std::vector<uint8_t> data;
};

class HexData {
private:
    size_t m_pageSize;
    size_t m_pageCount;
    std::vector<CacheBlock> m_cacheBlocks;
    std::unique_ptr<wxFile> m_file;
    size_t m_size;

    // 私有辅助方法
    bool HitCache(size_t offset) ;
    uint8_t GetByteFromCache(size_t offset) ;
    bool UpdateCache(size_t offset);
    time_t GetTime() const;

public:
    HexData();

    // 公共接口 - 这些不能改
    bool LoadFromFile(const wxString& filename);
    void Clear();
    bool IsEmpty() const;
    size_t Size() const;
    uint8_t GetByte(size_t offset) ;
    std::pair<wxString, wxString> GetLine(size_t offset, int bytesPerLine) ;
};