#ifndef MONSTER_H
#define MONSTER_H

#include <string>
#include <utility>
#include <vector>

class Player;
class PlayerBag;

// [FIX_J] 掉落条目：itemId 关联物品配置，weight 为权重
struct DropEntry {
    int itemId;
    int weight;
};

class Monster {
public:
    std::string name;
    int hp;
    std::pair<int,int> atk;
    int killExp;
    std::pair<int,int> levelRange;
    std::vector<DropEntry> drops;  // [FIX_J] 掉落表替代 hardcode
    
    Monster(std::string n, int h, std::pair<int,int> a, int exp,
            std::pair<int,int> b, std::vector<DropEntry> d = {})
        : name(n), hp(h), atk(a), killExp(exp), levelRange(b), drops(std::move(d)) {}
    
    void takeDamage(int d);
    void attack(Player &player);
    bool isAlive() const;
    void dropItem(PlayerBag& bag);
    
};

#endif
