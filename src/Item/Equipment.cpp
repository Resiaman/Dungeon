#include "Item/Equipment.h"
#include "Player/Player.h"
#include <iostream>
#include <utility>
#include <vector>

void Equipment::showEquipment() const{
    std::cout << "Name: " << name << std::endl
             << "ID: " << ID << std::endl
             << "Attack: " << atk.first << "~" << atk.second << std::endl
             << "Defense: " << def << std::endl;
}

void Equipment::dropEquipment(PlayerBag& bag){
    bag.removeByID(getID(),1);
}


void Equipment::use(Player& player) {
    player.bag.equipItem(getID(), player);
}

nlohmann::json Equipment::toJson() const{
    return nlohmann::json{
        {"name",name},
        {"ID",ID},
        {"ItemType","equipment"},
        {"EquipmentType",enumToString(etype)},
        {"atk_min",atk.first},
        {"atk_max",atk.second},
        {"def",def}
    };
}

