#include <vector>
#include <memory>
#include "Player/PlayerEquipment.h"
#include "Item/Equipment.h"
#include "Config/UIConfig.h"
#include "Player/Player.h"

//将装备类型和slot对应的装备栏位进行比较并返回对应的slot是否为空
bool PlayerEquipment::isEmpty(EquipmentType etype) const{
    size_t index = static_cast<size_t>(etype);
    //因为智能指针重载了operator[]，可以将指向对象内容与nullptr进行比较，从而实现判断是否为空。
    return Equipslot[index] == nullptr;
}

//将装备类型和slot对应的装备栏位进行比较并返回对应的slot是否为空
bool PlayerEquipment::isEmpty(std::string etype) const{
    for (int i = 0; i < static_cast<int>(EquipmentType::count); ++i) {
        EquipmentType type = static_cast<EquipmentType>(i);
        if(Equipment::enumToString(type) == etype){
            return Equipslot[i] == nullptr;
        }
    }
    return false;
}

int PlayerEquipment::getEquipIndex(EquipmentType etype) const{
    size_t index = static_cast<size_t>(etype);
    return index;
}

int PlayerEquipment::getEquipIndex(std::string etype) const{ 
    for (int i = 0; i < static_cast<int>(EquipmentType::count); ++i) {
        EquipmentType type = static_cast<EquipmentType>(i);
        if(Equipment::enumToString(type) == etype){
            return i;
        }
    }
    return -1;//异常处理还没做
}

std::unique_ptr<Equipment> PlayerEquipment::changeEuipType(std::unique_ptr<Item> item){
   if(auto eqPtr = dynamic_cast<Equipment*>(item.get())){
        item.release();
        return std::unique_ptr<Equipment>(eqPtr);
   }
   return nullptr;
}

void PlayerEquipment::wear(Player& player,std::unique_ptr<Equipment>&& BagEquipment){//调用时需要手动将背包中的空格子删除
    int slotIndex=getEquipIndex(BagEquipment->getType());
    if(isEmpty(BagEquipment->getType())){
        UIConfig::delay(SHORT_DELAY);
        std::cout<<"成功装备 "<<BagEquipment->getName()<<std::endl;
        Equipslot[slotIndex] = std::move(BagEquipment);
        playerGain(*Equipslot[slotIndex],player);
        UIConfig::delay(SHORT_DELAY);
    }else{
        std::unique_ptr<Equipment> SlotEquipment=std::move(Equipslot[slotIndex]);
        changeEuip(player,std::move(BagEquipment),move(SlotEquipment));
    }
}

void PlayerEquipment::takeOff(Player& player,std::unique_ptr<Equipment>&& SlotEquipment){
    // [FIX] 防御性检查：防止空指针解引用崩溃
    // 原因：takeOffGain(*SlotEquipment) 会解引用指针，如果传入了 nullptr 则段错误
    // 虽然目前调用处不会传空指针，但作为公共接口应有防御性检查
    if (!SlotEquipment) {
        std::cout << "没有可卸下的装备" << std::endl;
        UIConfig::delay(SHORT_DELAY);
        return;
    }
    takeOffGain(*SlotEquipment,player);
    std::cout<<SlotEquipment->getName()<<"已卸下"<<std::endl;
    UIConfig::delay(SHORT_DELAY);
    player.bag.addItem(std::move(SlotEquipment));
}

void PlayerEquipment::changeEuip(Player& player,std::unique_ptr<Equipment>&& BagEquipment,std::unique_ptr<Equipment>&& SlotEquipment){
    std::cout << "该位置已有装备，是否更换？\n[1.确定 2.取消]" << std::endl;
    int changeChoice = UIConfig::checkNumberInput(1,2);
    switch (changeChoice) {
        case 1:
            {
                int slotIndex = getEquipIndex(SlotEquipment->getType());
                takeOffGain(*SlotEquipment,player);
                std::swap(BagEquipment,SlotEquipment);
                playerGain(*SlotEquipment,player);
                PlayerEquipment::Equipslot[slotIndex] = move(SlotEquipment);
                player.bag.addItem(move(BagEquipment));
                break;
            }
        case 2: {
            int slotIndex = getEquipIndex(SlotEquipment->getType());
            Equipslot[slotIndex] = std::move(SlotEquipment);
            player.bag.addItem(std::move(BagEquipment));
            break;
        }
    }
    
}

void PlayerEquipment::playerGain(const Equipment& equipment,Player& player){
    player.atk.first += equipment.getAttack().first;
    player.atk.second += equipment.getAttack().second;
    player.def += equipment.getDef();
}

void PlayerEquipment::takeOffGain(const Equipment& equipment,Player& player){ 
    player.atk.first -= equipment.getAttack().first;
    player.atk.second -= equipment.getAttack().second;
    player.def -= equipment.getDef();
}

void PlayerEquipment::showEquipment() const{ 
    for(int i = 0 ; i < (int)EquipmentType::count ; ++i){
        if(!isEmpty((EquipmentType)i)){
            std::cout << "Slot " << i+1 << ": " << Equipslot[i]->getType() << " " << Equipslot[i]->getName() << std::endl;
        }else{
            std::cout << "Slot " << i+1 << ": Empty" << std::endl;
        }
    }
}

int PlayerEquipment::getSize() const {
    return Equipslot.size();
}

void PlayerEquipment::clear() {
    for(auto &eqPtr : Equipslot){
        eqPtr.reset();
    }
}

nlohmann::json PlayerEquipment::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (int i = 0; i < (int)EquipmentType::count; ++i) {
        if (Equipslot[i]) {
            arr.push_back(Equipslot[i]->toJson());
        } else {
            arr.push_back(nullptr);
        }
    }
    return arr;
}

void PlayerEquipment::fromJson(nlohmann::json& j) {
    for (int i = 0; i < (int)EquipmentType::count; ++i) {
        if (i >= (int)j.size() || j[i].is_null()) {
            Equipslot[i].reset();
        } else {
            auto ptr = Item::fromJson(j[i]);
            Equipment* eq = dynamic_cast<Equipment*>(ptr.get());
            if (eq) {
                ptr.release();
                Equipslot[i] = std::unique_ptr<Equipment>(eq);
            } else {
                Equipslot[i].reset();
            }
        }
    }
}
