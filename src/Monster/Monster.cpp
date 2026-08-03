#include <iostream>
#include "Player/Player.h"
#include "Player/PlayerBag.h"
#include "Monster/Monster.h"
#include "Item/Medicine.h"
#include "Config/Config.h"
#include "Config/ConfigLoader.h"
#include "Config/UIConfig.h"

    //受到伤害
    void Monster::takeDamage(int d) {
        std::cout << "你对" << name << "造成 " << d << "点伤害！" << std::endl;
        hp -= d;
        if (hp < 0) hp = 0;
    }
    //攻击计算
    void Monster::attack(Player &player){
        //计算怪物的伤害
        int dmg = Random::range(atk.first, atk.second) - player.def;
        if(dmg<=0){
            UIConfig::delay(SHORT_DELAY);
            std::cout<<"你免疫了"<<name<<"的攻击"<<std::endl;
        }else{
            UIConfig::delay(SHORT_DELAY);
            player.currentHp -= dmg;
            std::cout << name << "攻击造成 " << dmg << "点伤害！" << std::endl;
        }
    }
    bool Monster::isAlive() const { return hp > 0; }

    // [FIX_J] 从怪物自身的 drops 表按权重随机掉落
    // [V5.0] Boss 掉落 3 次 roll（专属武器高权重保证必掉），普通怪 1 次
    void Monster::dropItem(PlayerBag &bag) {
        if (drops.empty()) return;

        int rollCount = isBoss ? 3 : 1;
        for (int r = 0; r < rollCount; ++r) {
            // 计算总权重
            int totalWeight = 0;
            for (const auto& d : drops) {
                totalWeight += d.weight;
            }
            if (totalWeight <= 0) return;

            int roll = Random::range(1, totalWeight);
            int cumulative = 0;
            for (const auto& d : drops) {
                cumulative += d.weight;
                if (roll <= cumulative) {
                    auto item = ConfigLoader::createItemById(d.itemId, 1);
                    if (item) {
                        if (isBoss) {
                            std::cout << "[BOSS掉落] 你获得了一份" << item->name << std::endl;
                        } else {
                            std::cout << "你获得了一份" << item->name << std::endl;
                        }
                        bag.addItem(std::move(item));
                    }
                    break;
                }
            }
        }
    }