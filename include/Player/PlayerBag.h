#ifndef PLAYERBAG_H
#define PLAYERBAG_H 
//未适配CMake
#include <memory>
#include <vector>

#include "Item/Item.h"
#include "Item/Equipment.h"
#include "Item/Medicine.h"


class Player;
class PlayerBag {
    std::vector<std::unique_ptr<Item>> bag;
    
    public:
        //背包添加、使用物品
        void addItem(std::unique_ptr<Item> item);
        void useItem(size_t index,std::vector<int> v,Player& player);
        void useItem(size_t index,Player &player);

        //根据父标签分类显示 并根据传入类型显示对应物品的映射数组
        std::vector<int> showItem(ItemType it_type) const;
        //根据子标签分类显示 并根据传入类型显示对应物品的映射数组
        std::vector<int> showItem(EquipmentType eq_type) const;

        //背包总览
        void showItem() const;
        
        //获取所有可见物品（quantity>0）的向量下标映射
        std::vector<int> getVisibleIndices() const;

        //背包容量
        size_t size() const {return bag.size();};
        
        //当物品使用完后清除物品
        void removeItem();
        
        //清空背包
        void clear();
        
        // 从背包中装备物品（根据物品ID）
        // [FIX] 新增：供 Equipment::use(Player&) 调用
        bool equipItem(int id, Player& player);

        //取得背包中装备的指针
        std::unique_ptr<Equipment> getEquipment(size_t index);

        //测试专用
        bool removeByID(int ID,int quantity);
        bool addItemByID(int ID,int quantity);

        //【f】新增：背包序列化到JSON
        nlohmann::json toJson() const;
        
        //【f】新增：从JSON加载背包数据
        void fromJson(nlohmann::json &j);
};



#endif