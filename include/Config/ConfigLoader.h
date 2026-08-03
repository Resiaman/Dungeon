#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <vector>
#include <unordered_map>
#include "Monster/Monster.h"
#include "Item/Item.h"
#include <memory>

class ConfigLoader {
public:
    // 加载所有怪物数据（带缓存，首次解析后复用）
    static const std::vector<Monster>& loadMonsters(const std::string& filepath);
    
    // 加载初始物品（带缓存）
    static std::vector<std::unique_ptr<Item>> loadInitialItems(const std::string& filepath);

    // [FIX_J] 根据 itemId 创建物品实例（供 dropItem 使用，需先调用 loadInitialItems）
    static std::unique_ptr<Item> createItemById(int id, int quantity = 1);

private:
    // [FIX_J] 缓存容器
    static std::vector<Monster> s_monsters;
    static bool s_monstersLoaded;
    
    // [FIX_J] 按 id 索引的物品工厂数据
    struct ItemFactoryData {
        std::string type;    // "medicine" / "equipment"
        std::string name;
        int hp_restore = 0;
        std::pair<int,int> atk = {0, 0};
        int def = 0;
        std::string type_2;  // 装备子类型
        bool stackable = false;
        int baseQuantity = 0;
    };
    static std::unordered_map<int, ItemFactoryData> s_itemFactoryData;
    static bool s_itemsLoaded;
};

#endif
