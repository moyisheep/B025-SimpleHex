#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;


// 文件数据类
class FileData {
private:
    std::vector<uint8_t> data;
    std::string filepath;
    std::string filename;
    size_t filesize;

public:
    FileData();
    bool load(const std::string& path);
    bool loadFromMemory(const std::vector<uint8_t>& newData);
    size_t size() const;
    uint8_t operator[](size_t index) const;
    std::string getFilePath() const;
    std::string getFileName() const;
    std::vector<uint8_t> getDataSlice(size_t start, size_t end) const;
    std::string getHexString(size_t start, size_t length) const;
    std::string getAsciiString(size_t start, size_t length) const;
};