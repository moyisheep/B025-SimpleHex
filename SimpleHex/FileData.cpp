#include "FileData.h"


// ==================== FileData 实现 ====================
FileData::FileData() : filesize(0) {}

bool FileData::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "无法打开文件: " << path << std::endl;
        return false;
    }

    filesize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    data.resize(filesize);
    if (!file.read(reinterpret_cast<char*>(data.data()), filesize)) {
        std::cerr << "读取文件失败: " << path << std::endl;
        data.clear();
        filesize = 0;
        return false;
    }

    filepath = path;
    filename = fs::path(path).filename().string();

    std::cout << "文件加载成功: " << filename
        << " (" << filesize << " 字节)" << std::endl;
    return true;
}

bool FileData::loadFromMemory(const std::vector<uint8_t>& newData) {
    data = newData;
    filesize = data.size();
    filepath = "";
    filename = "内存数据";
    return true;
}

size_t FileData::size() const {
    return filesize;
}

uint8_t FileData::operator[](size_t index) const {
    return index < filesize ? data[index] : 0;
}

std::string FileData::getFilePath() const {
    return filepath;
}

std::string FileData::getFileName() const {
    return filename;
}



std::vector<uint8_t> FileData::getDataSlice(size_t start, size_t end) const {
    if (start >= filesize || end > filesize || start >= end) {
        return {};
    }
    return std::vector<uint8_t>(data.begin() + start, data.begin() + end);
}

std::string FileData::getHexString(size_t start, size_t length) const {
    std::stringstream ss;
    size_t end = std::min(start + length, filesize);
    for (size_t i = start; i < end; i++) {
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]) << " ";
    }
    return ss.str();
}

std::string FileData::getAsciiString(size_t start, size_t length) const {
    std::string result;
    size_t end = std::min(start + length, filesize);
    for (size_t i = start; i < end; i++) {
        char c = static_cast<char>(data[i]);
        result.push_back((c >= 32 && c <= 126) ? c : '.');
    }
    return result;
}


