#ifndef PLAYEREQUIPMENT_H
#define PLAYEREQUIPMENT_H 
#include <memory>
#include <vector>
#include <array>

#include "Item/Item.h"
#include "Item/Equipment.h"


class PlayerBag;
class Player;
class PlayerEquipment {
    public:
    //装备栏
    std::array<std::unique_ptr<Equipment>,(int)EquipmentType::count> Equipslot;

    //获取对应装备栏是否为空
    bool isEmpty(EquipmentType etype) const;
    bool isEmpty(std::string etype) const;

    //获取对应装备栏序号    
    int getEquipIndex(EquipmentType etype) const;
    int getEquipIndex(std::string etype) const;

    //装备类型向下转型
    std::unique_ptr<Equipment> changeEuipType(std::unique_ptr<Item> item);

    //穿戴相关
    void wear(Player& player,std::unique_ptr<Equipment>&& equipment);//此处是背包中的装备
    void takeOff(Player& player,std::unique_ptr<Equipment>&& equipment);//此处是装备栏中的装备
    void changeEuip(Player& player,std::unique_ptr<Equipment>&& BagEquipment,std::unique_ptr<Equipment>&& SlotEquipment);//此处需要背包和装备栏中的装备

    //玩家属性增幅
    void playerGain(const Equipment& equipment,Player& player);
    void takeOffGain(const Equipment& equipment,Player& player);

    //查看装备栏
    void showEquipment() const;

    //获取装备栏的大小
    int getSize() const;

    //清空装备栏
    void clear();

    //装备栏序列化
    nlohmann::json toJson() const;
    void fromJson(nlohmann::json &j);

};




#endif