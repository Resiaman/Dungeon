#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <string>
#include "Player/Player.h"

// 负责存档和读档操作
class SaveManager {
public:
    // 保存游戏到文件，返回 true 表示成功
    static bool saveGame(const Player& player, const std::string& filepath = "D:/AAA-Work/MyFirstGame/save/savegame.json");
    
    // 从文件加载游戏到 player，返回 true 表示成功
    static bool loadGame(Player& player, const std::string& filepath = "D:/AAA-Work/MyFirstGame/save/savegame.json");
};

#endif
