#include <iostream>
#include <vector>
#include <string>

#include "Player/Player.h"
#include "Monster/Monster.h"
#include "Item/Medicine.h"
#include "Config/Config.h"
#include "Config/UIConfig.h"
#include "Config/ConfigLoader.h"
#include "Config/GameUIConfig.h"
#include "Config/saveManager.h"
#include "Battle/FightLogic.h"

int main(){
    system("chcp 65001 > nul");
    Player player;
    player.reset();
    std::vector<Monster> monsters = ConfigLoader::loadMonsters("config/monster.json");
    if (monsters.empty()) {
        std::cerr << "怪物配置为空，无法继续" << std::endl;
        return 1;
    }

    std::cout << "Welcome to Dongeon!" << std::endl;
    UIConfig::delay(SHORT_DELAY);
    std::cout << " [1.开始游戏]\n [2.加载存档]\n [3.退出游戏]\n";
    int start_choice = UIConfig::checkNumberInput(1,3);
    //菜单
    switch (start_choice) {
        case 1:
            UIConfig::delay(SHORT_DELAY);
            std::cout << "开始游戏！" << std::endl;
            break;

        case 2:
            UIConfig::delay(SHORT_DELAY);
            if (SaveManager::loadGame(player, "save/savegame.json")) {
                std::cout << "继续冒险！" << std::endl;
            } else {
                std::cout << "开始新游戏！" << std::endl;
            }
            break;

        case 3:
            std::cout << "退出游戏..." << std::endl;
            return 0;
    }

    while(player.currentHp > 0) {
        bool ReadyToFight = true;
        UIConfig::delay(SHORT_DELAY);
        std::cout << "现在你准备：\n [1.战斗]\n [2.休息]\n [3.查看状态]\n [4.查看背包]\n [5.查看装备]\n [6.保存游戏]\n [7.退出游戏]\n";
        int choice = UIConfig::checkNumberInput(1,7);
        switch (choice)
        {
            case 1:
                UIConfig::delay(SHORT_DELAY);
                std::cout<<"进入战斗！\n";
                fightLogic::unlimitedFight(player, monsters, ReadyToFight);
                break;
            
            case 2:
                player.rest();
                break;
            
            case 3:
                player.showStatus();
                break;

            case 4://背包界面现在应该能够停留并在其中实现使用物品的操作
                while(1){
                    player.bag.showItem();
                    auto visibleIndices = player.bag.getVisibleIndices();
                    if (visibleIndices.empty()) {
                        std::cout<<"背包空了，按0返回"<<std::endl;
                        UIConfig::checkNumberInput(0,0);
                        break;
                    }
                    std::cout<<"输入物品序号以查看/使用物品(输入数字0以退出)："<<std::endl;
                    int bagUseChoice = UIConfig::checkNumberInput(0, static_cast<int>(visibleIndices.size()));
                    if(bagUseChoice==0){
                        break;
                    }
                    player.bag.useItem(visibleIndices[bagUseChoice-1], player);
                    //std::cout<<"currant bag size:"<<player.bag.size()<<std::endl;
                }
                break;

            case 5://装备界面同背包界面效果
                {
                    player.eqSlot.showEquipment();
                    std::cout<<"输入物品序号以查看/使用物品(输入数字0以退出)："<<std::endl;
                    // [FIX] 用户输入 0~2, 不做减一，switch case 直接对应
                    int slotUseChoice = UIConfig::checkNumberInput(0, player.eqSlot.getSize());
                    if (slotUseChoice == 0) {
                        break;
                    }
                    // [FIX] 合并重复 case，两个槽位走的同一逻辑
                    GameUIConfig::Game_PlayerEqSlotUI(player, slotUseChoice - 1);
                }
                break;

            case 6:
                SaveManager::saveGame(player, "save/savegame.json");
                UIConfig::delay(SHORT_DELAY);
                break;

            case 7:
                std::cout << "退出游戏..." << std::endl;
                return 0;
        }
    }
    
    return 0;
}
