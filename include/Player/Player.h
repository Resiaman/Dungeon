#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <utility>
#include "Config/Config.h"
#include "Player/PlayerBag.h"
#include "Player/PlayerEquipment.h"

class Medicine;
class Player {
public:
    //基础属性
    int hp_UpperLimit = STARTHP;
    int currentHp = hp_UpperLimit;
    std::pair<int,int> atk = {MIN_ATK,MAX_ATK};
    int def = STARTDEF;

    //等级属性
    int level = STARTLEVEL;
    int maxlevel = MAXLEVEL;
    int exp = STARTEXP;
    int expToUp = EXPTOUP;

    //背包及装备类
    PlayerEquipment eqSlot;
    PlayerBag bag;

    void showStatus();
    void showBag();
    int attack();
    void rest();

    void levelUp();
    void expEnough(int e);

    void reset();

    //【f】新增：玩家数据序列化到JSON
    nlohmann::json toJson() const;
    
    //【f】新增：从JSON加载玩家数据
    static Player fromJson(nlohmann::json &j);

};

#endif