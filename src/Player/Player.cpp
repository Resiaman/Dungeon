#include <iostream>
#include <cmath>
#include <utility>

#include "Player/Player.h"
#include "Item/Medicine.h"
#include "Config/Config.h"
#include "Config/ConfigLoader.h"
#include "Config/UIConfig.h"


    //显示状态
    void Player::showStatus() {
        UIConfig::delay(SHORT_DELAY);
        std::cout << "Level:" << level << std::endl;
        std::cout << "Hp:" << currentHp << std::endl;
        std::cout << "Atk:" << atk.first<<"~"<< atk.second << std::endl;
        std::cout << "Def:" << def << std::endl;
        std::cout << "exp:" << exp << std::endl;
        std::cout << "经验需求:" << expToUp << std::endl;
        UIConfig::delay(LONG_DELAY);
    }

    //显示背包内物品
    void Player::showBag() {
        UIConfig::delay(SHORT_DELAY);
        std::cout << "背包内物品:" << std::endl;
        bag.showItem();
    }

    //攻击
    int Player::attack(){
        return Random::range(atk.first, atk.second);
    }

    //休息回血
    void Player::rest(){
        if(currentHp<hp_UpperLimit){
            int restHp = std::round(hp_UpperLimit*0.3);
            int realRestHp = std::min(restHp,hp_UpperLimit-currentHp);
            currentHp += realRestHp;
            std::cout<<"你恢复了"<<realRestHp<<"点生命值"<<std::endl;
        }else if (currentHp == hp_UpperLimit){
            UIConfig::delay(SHORT_DELAY);
            std::cout<<"状态绝佳，该继续战斗了！"<<std::endl;
        }
    }

    //升级经验值相关
    void Player::levelUp() {
        // [FIX] 等级上限检查：达到 MAXLEVEL 后不再升级（原实现无检查，可无限升级）
        if (level >= maxlevel) {
            UIConfig::delay(SHORT_DELAY);
            std::cout << "你已达到最高等级！" << std::endl;
            return;
        }
        level++;
        expToUp = 10 + level*level;
        std::cout << "你升级了！当前等级:" << level << std::endl;
        hp_UpperLimit+=5+floor(1.5*level);
        currentHp = hp_UpperLimit;
        atk.first+=floor(0.5*level);
        atk.second+=ceil(1.25*level);
        def+=floor(0.95*level);
    }
    void Player::expEnough(int e) {
        exp += e;
        // [FIX] while 条件加 level < maxlevel：达上限后不再扣经验，防止等级封顶后经验被白白扣光
        while (exp >= expToUp && level < maxlevel) {
            exp -= expToUp;
            levelUp();
        }
    }
    void Player::reset(){
        hp_UpperLimit = STARTHP;
        currentHp = hp_UpperLimit;
        atk = {MIN_ATK,MAX_ATK};
        def = STARTDEF;
        level = STARTLEVEL;
        exp = STARTEXP;
        expToUp = EXPTOUP;
        
        bag.clear();
        eqSlot.clear();
        
        // 从 JSON 加载初始物品
        auto initialItems = ConfigLoader::loadInitialItems("config/item.json");
        for (auto& item : initialItems) {
            if(item->getQuantity() > 0)
            {
                bag.addItem(std::move(item));
            }
        }
    }

    //【f】实现玩家数据序列化（完善版）
    nlohmann::json Player::toJson() const {
        nlohmann::json j;
        
        // 保存基础属性
        j["hp_UpperLimit"] = hp_UpperLimit;
        j["currentHp"] = currentHp;
        j["atk_min"] = atk.first;
        j["atk_max"] = atk.second;
        j["def"] = def;
        
        // 保存等级属性
        j["level"] = level;
        j["maxlevel"] = maxlevel;
        j["exp"] = exp;
        j["expToUp"] = expToUp;
        
        // 保存背包物品（调用PlayerBag的toJson）
        j["bag"] = bag.toJson();
        
        // 保存装备栏（PlayerEquipment::toJson 已实现）
        j["eqSlot"] = eqSlot.toJson();
        
        return j;
    }

    //【f】实现玩家数据反序列化（完善版）
    Player Player::fromJson(nlohmann::json &j) {
        Player player;
        
        // 恢复基础属性
        player.hp_UpperLimit = j.value("hp_UpperLimit", STARTHP);
        player.currentHp = j.value("currentHp", player.hp_UpperLimit);
        player.atk.first = j.value("atk_min", MIN_ATK);
        player.atk.second = j.value("atk_max", MAX_ATK);
        player.def = j.value("def", STARTDEF);
        
        // 恢复等级属性
        player.level = j.value("level", STARTLEVEL);
        player.maxlevel = j.value("maxlevel", MAXLEVEL);
        player.exp = j.value("exp", STARTEXP);
        player.expToUp = j.value("expToUp", EXPTOUP);
        
        // 【f】恢复背包数据
        if (j.contains("bag")) {
            player.bag.fromJson(j["bag"]);
        }
        
        // 恢复装备栏（PlayerEquipment::fromJson 已实现）
        if (j.contains("eqSlot")) {
            player.eqSlot.fromJson(j["eqSlot"]);
        }
        
        return player;
    }