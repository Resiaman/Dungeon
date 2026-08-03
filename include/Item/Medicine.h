#ifndef MEDICINE_H
#define MEDICINE_H

#include <string>
#include <vector>

#include "Item/Item.h"

class Medicine : public Item {
private:
    int stock;
    int hp_restore;
    ItemType itype;

public:
    Medicine(const int ID , const std::string& n , int stock , int recHp , ItemType itype): 
    Item(ID,n) , stock(stock) , hp_restore(recHp) , itype(itype) {}
    
    //药品使用
    void use(Player& player) override;
    
    //获取药品ID
    int getID() override;
    
    //获取药品类型
    const std::string getType() override {return "Medicine";}

    //显示药品
    void showMedicine() const;

    //药品数量相关
    int getQuantity() const override {return stock;};
    void setQuantity(int q) override {stock = q;};
    bool isStackable() const override {return true;};

    //序列化函数
    nlohmann::json toJson() const override;

    //【f】删除fromJson声明，反序列化由Item::fromJson静态工厂方法处理
    //std::vector<std::unique_ptr<Item>> fromJson(nlohmann::json &j,Player &player) const override;
};

#endif