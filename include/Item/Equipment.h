#ifndef EQUIPMENT_H    // [FIX] 添加头文件保护
// 原因：之前缺 guards，多文件包含时会重复定义 class Equipment 导致编译错误
// 同时切断了 Equipment.h → Player.h → PlayerEquipment.h → Equipment.h 的循环包含
#define EQUIPMENT_H

#include <utility>
#include <vector>
#include <memory>
#include "Item/Item.h"    // [FIX] 大小写统一（原来是 item.h）
// 原因：Windows 忽略大小写，Linux/Mac 区分，统一为 Item/Item.h 保证跨平台编译通过

class Player;		// [FIX] 前向声明代替 #include "Player/Player.h"
// 原因：Equipment.h 只用到 Player& 做参数类型，前向声明就够
// 如果 include Player.h 会导致 Player.h→PlayerEquipment.h→Equipment.h→Player.h 循环包含
class PlayerBag;
class Equipment : public Item {
private:
    std::pair<int,int> atk;
    int def;
    ItemType itype;//父类型
    EquipmentType etype;//装备子类型

public:
    Equipment(const int ID , const std::string& name , std::pair<int,int> a , int def, int /*baseQuantity*/, ItemType itype, EquipmentType etype): 
    Item(ID,name) , atk(a) ,def(def), itype(itype), etype(etype) {};
    
    //获取装备ID
    int getID() override {return ID;}
        
    //获取装备类型
    const std::string getType() override {return Item::enumToString(etype);};
    const std::string getItType() {return Item::enumToString(itype);};
    
    //直接在背包中穿戴装备
    void use(Player& player) override;

    //查看装备属性
    void showEquipment() const;

    //获取装备属性
    std::pair<int,int> getAttack() const {return atk;}
    int getDef() const {return def;}
    std::string getName() const {return name;}


    //丢弃装备
    void dropEquipment(PlayerBag& bag);

    // 获取装备类型枚举（供背包判断使用）
    EquipmentType getEquipmentType() const {return etype;}

    //装备不可堆叠
    bool isStackable() const override {return false;};
    
    //序列化函数
    nlohmann::json toJson() const override;

    //【f】删除fromJson声明，反序列化由Item::fromJson静态工厂方法处理
    //std::vector<std::unique_ptr<Item>> fromJson(nlohmann::json &j,Player &player) const override;
};

#endif