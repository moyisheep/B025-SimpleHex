#include "main.h"




// 主函数
int main(int argc, char* argv[]) {
    // 设置控制台编码为UTF-8
    system("chcp 65001 > nul");

    HexViewer app;

    if (!app.init()) {
        std::cerr << "应用程序初始化失败" << std::endl;
        return 1;
    }

    // 如果有命令行参数，尝试加载文件
    if (argc > 1) {
        std::string filePath = argv[1];
        if (fs::exists(filePath)) {
            app.loadFile(filePath);
        }
        else {
            std::cerr << "文件不存在: " << filePath << std::endl;
        }
    }

    app.run();

    return 0;
}