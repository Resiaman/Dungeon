#include "Config/saveManager.h"
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <direct.h>  // _mkdir
#else
#include <sys/stat.h>  // mkdir
#endif

// 创建目录（跨平台）
static bool createDirIfNeeded(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return true;  // 当前目录
    
    std::string dir = path.substr(0, pos);
#ifdef _WIN32
    return _mkdir(dir.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool SaveManager::saveGame(const Player& player, const std::string& filepath) {
    // 确保目录存在
    if (!createDirIfNeeded(filepath)) {
        std::cerr << "无法创建存档目录!" << std::endl;
        return false;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "无法打开存档文件: " << filepath << std::endl;
        return false;
    }

    nlohmann::json j = player.toJson();
    file << j.dump(4);
    file.close();

    std::cout << "游戏已保存至 " << filepath << std::endl;
    return true;
}

bool SaveManager::loadGame(Player& player, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "未找到存档文件: " << filepath << std::endl;
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
        player = Player::fromJson(j);
    } catch (const std::exception& e) {
        std::cerr << "读取存档失败: " << e.what() << std::endl;
        return false;
    }
    file.close();

    std::cout << "游戏已加载！" << std::endl;
    return true;
}
