// HexData.cpp
#include "HexData.h"
#include <algorithm>
#include <chrono>

HexData::HexData()
    : m_pageSize(16384),
    m_pageCount(16),
    m_file(nullptr),
    m_size(0)
{
}

bool HexData::LoadFromFile(const wxString& filename)
{
    Clear(); // 先清理现有数据

    m_file = std::make_unique<wxFile>(filename);
    if (!m_file || !m_file->IsOpened()) {
        return false;
    }

    m_size = m_file->Length();


    return true;
}

void HexData::Clear()
{
    m_cacheBlocks.clear();
    m_file.reset();
    m_size = 0;
}

bool HexData::IsEmpty() const
{
    return !m_file || m_size <= 0;
}

size_t HexData::Size() const
{
    if (!m_file) {
        return 0;
    }
    return m_size;
} 

bool HexData::HitCache(size_t offset) 
{
    if (!IsEmpty() && offset <= m_size) 
    {
        size_t pageNumber = offset / m_pageSize;
        size_t pageOffset = pageNumber * m_pageSize;

        for (auto& block : m_cacheBlocks) {
            if (block.offset == pageOffset) {
                block.lastAccessTime = GetTime();
                return true;
            }
        }
    }


    return false;
}

uint8_t HexData::GetByteFromCache(size_t offset) 
{
    if (offset <= m_size) 
    {
        size_t pageNumber = offset / m_pageSize;
        size_t pageOffset = pageNumber * m_pageSize;
        size_t pageIndex = offset - pageOffset; // 在块内的偏移

        for (auto& block : m_cacheBlocks) {
            if (block.offset == pageOffset && pageIndex < block.data.size())
            {
                return block.data[pageIndex];
            }
        }
    }


    return 0; // 不应该到这里
}

bool HexData::UpdateCache(size_t offset)
{
    if (!m_file || offset >= m_size) {
        return false;
    }

    // 计算块信息
    size_t pageNumber = offset / m_pageSize;
    size_t pageOffset = pageNumber * m_pageSize;
    size_t bytesToRead = std::min(m_pageSize, m_size - pageOffset);

    for (auto& block : m_cacheBlocks) {
        if (block.offset == pageOffset) {
            // 更新访问时间
            block.lastAccessTime = GetTime();
            return true; // 块已在缓存中
        }
    }

    // 读取数据到缓存
    std::vector<uint8_t> data(bytesToRead);
    if(pageOffset != 0)
    {
        if (!m_file->Seek(pageOffset)) {
            return false;
        }

    }


    ssize_t bytesRead = m_file->Read(data.data(), bytesToRead);
    if (bytesRead != static_cast<ssize_t>(bytesToRead)) {
        return false;
    }

    // 创建新的缓存块
    CacheBlock newBlock;
    newBlock.offset = pageOffset;
    newBlock.data = std::move(data);
    newBlock.lastAccessTime = GetTime();

    // 检查缓存是否已满
    if (m_cacheBlocks.size() >= m_pageCount) {
        // 使用LRU算法找到最久未访问的块
        auto oldest = std::min_element(m_cacheBlocks.begin(), m_cacheBlocks.end(),
            [](const CacheBlock& a, const CacheBlock& b) {
                return a.lastAccessTime < b.lastAccessTime;
            });

        if (oldest != m_cacheBlocks.end()) {
            *oldest = std::move(newBlock);
            return true;
        }
    }

    // 缓存未满，直接添加
    m_cacheBlocks.push_back(std::move(newBlock));
    return true;
}

uint8_t HexData::GetByte(size_t offset) 
{
    if (offset >= m_size) {
        return 0;
    }

    // 检查缓存
    if (!HitCache(offset)) {
        // 需要更新缓存（使用const_cast，因为实际需要修改缓存）
        const_cast<HexData*>(this)->UpdateCache(offset);
    }

    return GetByteFromCache(offset);
}

std::pair<wxString, wxString> HexData::GetLine(size_t offset, int bytesPerLine) 
{
    wxString hexStr, asciiStr;

    for (int i = 0; i < bytesPerLine; i++) {
        size_t pos = offset + i;
        if (pos >= m_size) {
            // 填充剩余空格
            hexStr += wxT("   ");
            asciiStr += wxT(" ");
            continue;
        }

        uint8_t byte = GetByte(pos);
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

// 私有辅助函数
time_t HexData::GetTime() const
{
    return std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
}