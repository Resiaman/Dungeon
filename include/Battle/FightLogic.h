#ifndef FIGHTLOGIC_H
#define FIGHTLOGIC_H


#include "Player/Player.h"
#include "Monster/Monster.h"
#include "Item/Medicine.h"
#include <vector>

namespace fightLogic{

    void unlimitedFight(Player &player, std::vector<Monster> &monsters, bool &ReadyToFight);
    void currentFight(Player &player, Monster &m, bool& ReadyToFight);

};


#endif