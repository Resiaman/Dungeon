#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <stdexcept>
#include "json.hpp"

enum class ItemType
{
    Medicine,
    Equipment
};

enum class EquipmentType{
    armor,
    weapon,
    count
};

class Player;
class Item{
public:
    int ID;
    std::string name;
    Item(const int ID , const std::string& n): ID(ID),name(n){};

    //基类物品使用定义
    virtual void use(Player& player) = 0;
    
    //基类信息获取
    virtual int getID() = 0;
    virtual const std::string getType() = 0;

    //数量堆叠相关
    virtual int getQuantity() const {return 1;};
    virtual void setQuantity(int n) {};
    virtual bool isStackable() const {return false;}

    //序列化
    virtual nlohmann::json toJson() const;

    //【f】反序列化改为静态工厂方法（虚函数无法正确多态，应通过ItemType分发）
    //virtual std::vector<std::unique_ptr<Item>> fromJson(nlohmann::json &j,Player &player) const;
    static std::unique_ptr<Item> fromJson(nlohmann::json &j);

    virtual ~Item() = default;

    //枚举类型转换
    static std::string enumToString(ItemType it);
    static std::string enumToString(EquipmentType et);
    static ItemType stringToItEnum(const std::string& s);
    static EquipmentType stringToEqEnum(const std::string& s);

};

#endif