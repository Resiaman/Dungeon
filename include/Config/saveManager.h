#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <string>
#include "Player/Player.h"

// 负责存档和读档操作
class SaveManager {
public:
    // 保存游戏到文件，返回 true 表示成功
    // [V5.0] 默认路径改为相对路径（原为 D:/AAA-Work/MyFirstGame/... 的遗留硬编码）
    static bool saveGame(const Player& player, const std::string& filepath = "save/savegame.json");
    
    // 从文件加载游戏到 player，返回 true 表示成功
    static bool loadGame(Player& player, const std::string& filepath = "save/savegame.json");
};

#endif
