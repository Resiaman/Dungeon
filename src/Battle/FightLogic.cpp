#include <algorithm>
#include <iostream>
#include <vector>

#include "Config/Config.h"
#include "Config/UIConfig.h"
#include "Battle/FightLogic.h"
#include "Monster/Monster.h"
#include "Player/Player.h"
#include "Player/PlayerBag.h"

namespace fightLogic{
    //单局战斗循环
    void currentFight(Player &player, Monster &m, bool &ReadyToFight){
        while (player.currentHp > 0 && m.isAlive()) {
        // 玩家回合
            UIConfig::delay(SHORT_DELAY);
            std::cout << "你的HP:" << player.currentHp << std::endl << m.name<<"的HP:" << m.hp ;
            UIConfig::delay(SHORT_DELAY);

            std::cout << "\n选择你的行为：\n [1.攻击]\n [2.逃跑]\n [3.使用药水]\n";
            int fightChoice = UIConfig::checkNumberInput(1,3);
            if (fightChoice == 1) {
                int playerDam = player.attack();
                m.takeDamage(playerDam);
            }
            else if (fightChoice == 2) {
                UIConfig::delay(SHORT_DELAY);
                int escape = Random::range(1, 100);
                if(escape <= 50){
                    std::cout << "你逃跑了..." << std::endl;
                    ReadyToFight = false;
                    break;
                }else{
                    std::cout << "逃跑失败！" << std::endl;
                } 
            }
            else if(fightChoice == 3){
                UIConfig::delay(SHORT_DELAY);
                std::cout << "请选择你要使用的药水：" << std::endl;
                std::vector<int> medicineIdMapping = player.bag.showItem(ItemType::Medicine);//创建了一个映射表保证用户输入与真实药水索引相对应
                int cancelChoice = medicineIdMapping.size()+1;
                std::cout << cancelChoice << ".取消使用" << std::endl;
                int use_choice = UIConfig::checkNumberInput(1,cancelChoice);
                if(use_choice != cancelChoice){
                    player.bag.useItem(use_choice-1,medicineIdMapping,player);
                }else{
                    continue;
                }
            }
            
            // 怪物反击（只在怪物存活时）
            if (m.isAlive()) {
                m.attack(player);
            }

            //敌对目标死亡
            if (!m.isAlive()) {
                UIConfig::delay(SHORT_DELAY);
                std::cout << "你击败了 " << m.name << "!" << std::endl;
                UIConfig::delay(SHORT_DELAY);
                std::cout << "你获得了 " << m.killExp << " 经验！" << std::endl;
                player.expEnough(m.killExp);
                UIConfig::delay(SHORT_DELAY);
                m.dropItem(player.bag);
                UIConfig::delay(SHORT_DELAY);
                std::cout << "继续战斗吗？ [1.继续战斗] [2.稍作休息]" << std::endl;
                int continueFight = UIConfig::checkNumberInput(1,2);
                if (continueFight == 1) {
                    continue;
                } else {
                    ReadyToFight = false;
                    break;
                }
            }
                                

            //玩家战败
            if (player.currentHp <= 0) {
                UIConfig::delay(SHORT_DELAY);
                std::cout << "你被击败了..." << std::endl;
                std::cout << "是否重新开始游戏？ [1.是] [2.否]" << std::endl;
                int restart=UIConfig::checkNumberInput(1,2);
                if (restart == 1) {
                    player.bag.clear();
                    player.reset();
                    std::cout << "重新开始游戏..." << std::endl;
                    ReadyToFight = false;
                    break;
                } else {
                    std::cout << "游戏结束..." << std::endl;
                }
                //若在战斗中选择逃跑则返回到地牢门口
                if (!ReadyToFight)
                {
                    break;
                }
            }
        }
    }

    //无尽战斗循环
    void unlimitedFight(Player &player, std::vector<Monster> &monsters, bool &ReadyToFight){
        while(player.currentHp > 0 && ReadyToFight){
            // [V5.0] 普通怪池与 Boss 池分开筛选（Boss 有独立刷新概率）
            std::vector<Monster> availableMons;
            std::vector<Monster> availableBoss;
            copy_if(monsters.begin(),monsters.end(),std::back_inserter(availableMons),[&player](const Monster &m)
                {if(!m.isBoss && player.level >= m.levelRange.first && m.levelRange.second+3>=player.level){return true;}else{return false;}});
            copy_if(monsters.begin(),monsters.end(),std::back_inserter(availableBoss),[&player](const Monster &m)
                {if(m.isBoss && player.level >= m.levelRange.first && m.levelRange.second+3>=player.level){return true;}else{return false;}});

            Monster m;
            bool isBossFight = false;
            // Boss 战：Boss 池非空时 10% 概率遭遇（普通怪为主，Boss 是惊喜）
            if (!availableBoss.empty() && Random::range(1, 100) <= 10) {
                m = availableBoss[Random::range(0, static_cast<int>(availableBoss.size()) - 1)];
                isBossFight = true;
            } else if (!availableMons.empty()) {
                m = availableMons[Random::range(0, static_cast<int>(availableMons.size()) - 1)];
            } else {
                // [FIX] 防止 availableMons 为空导致下标越界崩溃
                // 原因：当没有怪物满足等级条件时 availableMons.size()=0
                // uniform_int_distribution(0, -1) 是未定义行为，访问 availableMons[0] 更是越界
                // 需要在访问前检查，为空时退出战斗循环
                std::cout << "当前等级没有可挑战的怪物！" << std::endl;
                ReadyToFight = false;
                break;
            }
            
            // 怪物等级筛选条件说明：
            // 条件1 player.level >= m.levelRange.first  → 玩家等级不低于怪物最低等级，防止遇到太强的怪
            // 条件2 m.levelRange.second + 3 >= player.level → 怪物最高等级+3 不低于玩家等级，
            //        可打窗口 = [minLevel, maxLevel+3]，随等级提升低级怪自然退场，且高级怪不会断档
            UIConfig::delay(SHORT_DELAY);
            if (isBossFight) {
                std::cout << "⚠️ 你遭遇了 BOSS：" << m.name << "！" << std::endl;
            } else {
                std::cout << "你遇到了" << m.name << "！" << std::endl;
            }
            //单局战斗循环
            currentFight(player,m,ReadyToFight);
        }
    }

}
