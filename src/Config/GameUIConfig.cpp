#include "Config/GameUIConfig.h"

#include <iostream>
#include <vector>
#include <memory>

#include "Item/Item.h"
#include "Player/Player.h"
#include "Config/UIConfig.h"

void GameUIConfig::Game_PlayerEqSlotUI(Player &player,int slotUseChoice){
    if (player.eqSlot.Equipslot[slotUseChoice]) {
        std::cout<<"当前选中："<< player.eqSlot.Equipslot[slotUseChoice]->getName()<<std::endl; 
        UIConfig::delay(SHORT_DELAY);
    }

    std::cout<<"你可以：\n [1.穿戴装备]\n [2.卸下装备]\n [3.更换装备]\n"<<std::endl;
    int slotEquip = UIConfig::checkNumberInput(1,3);
    switch (slotEquip)
    {
        case 1:
            {
                // [FIX] 用显式映射代替 static_cast 依赖枚举顺序
                constexpr EquipmentType SLOT_TYPE[] = {EquipmentType::armor, EquipmentType::weapon};
                EquipmentType eqType = SLOT_TYPE[slotUseChoice];
                std::cout<<"你拥有："<<std::endl;
                std::vector<int> idMappiing = player.bag.showItem(eqType);
                if (idMappiing.empty()) {
                    break;
                }
                std::cout<<"选择穿戴（输入"<< idMappiing.size()+1 <<"取消）：";
                int cancel = idMappiing.size()+1;
                int rawChoice = UIConfig::checkNumberInput(1,cancel);
                if(rawChoice == cancel) {std::cout<<"取消装备"<<std::endl;break;}
                int wearChoice = rawChoice - 1;
                player.bag.useItem(wearChoice,idMappiing,player);              
            }
            break;

        case 2:
            player.eqSlot.takeOff(player,move(player.eqSlot.Equipslot[slotUseChoice]));
            // Equipslot 已在 takeOff 中被 move 走，无需重复 reset
            break;

        case 3:
            {
                if (!player.eqSlot.Equipslot[slotUseChoice]) {
                    std::cout << "该槽位为空，无法更换" << std::endl;
                    UIConfig::delay(SHORT_DELAY);
                    break;
                }
                // [FIX] 用显式映射代替 static_cast 依赖枚举顺序
                constexpr EquipmentType SLOT_TYPE[] = {EquipmentType::armor, EquipmentType::weapon};
                EquipmentType eqType = SLOT_TYPE[slotUseChoice];
                std::cout<<"你拥有：\n"<<std::endl;
                std::vector<int> idMappiing = player.bag.showItem(eqType);
                if (idMappiing.empty()) {
                    break;
                }
                std::cout<<"选择替换（输入"<< idMappiing.size()+1 <<"取消）：";
                int cancel = idMappiing.size()+1;
                // [FIX] 先检查 cancel 再减 1
                int rawChoice = UIConfig::checkNumberInput(1,cancel);
                if(rawChoice == cancel) {std::cout<<"取消替换"<<std::endl;break;}
                int wearChoice = rawChoice - 1;
                int bagIndex = idMappiing[wearChoice];
                player.eqSlot.changeEuip(player,move(player.bag.getEquipment(bagIndex)),move(player.eqSlot.Equipslot[slotUseChoice]));
                // [FIX] 之前此处多了一个 .reset()，导致 changeEuip 刚填入的装备被销毁，现已删除
            }
            break;
    }
}