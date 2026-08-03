#include "Item/Item.h"
#include "Item/Equipment.h"
#include "Item/Medicine.h"

// 基类 toJson：提供默认实现（子类各自覆盖）
nlohmann::json Item::toJson() const {
    return nlohmann::json{
        {"ID", ID},
        {"name", name}
    };
}

//【f】实现静态工厂方法，根据JSON中的ItemType字段分发到具体类的反序列化
std::unique_ptr<Item> Item::fromJson(nlohmann::json &j) {
    if (!j.contains("ItemType")) {
        throw std::runtime_error("JSON缺少ItemType字段");
    }
    
    std::string type = j.at("ItemType");
    if (type == "medicine") {
        // 从Medicine类反序列化
        std::string name = j.at("name");
        int ID = j.at("ID");
        int stock = j.at("stock");
        int hp_restore = j.at("restore");
        ItemType itype = stringToItEnum("medicine");
        return std::make_unique<Medicine>(ID, name, stock, hp_restore, itype);
    } else if (type == "equipment") {
        // 从Equipment类反序列化
        std::string name = j.at("name");
        int ID = j.at("ID");
        EquipmentType etype = stringToEqEnum(j.at("EquipmentType"));
        int atk_min = j.at("atk_min");
        int atk_max = j.at("atk_max");
        std::pair<int,int> atk = std::make_pair(atk_min, atk_max);
        int def = j.at("def");
        ItemType itype = stringToItEnum("equipment");
        int quantity = 1; // 装备不可堆叠
        return std::make_unique<Equipment>(ID, name, atk, def, quantity, itype, etype);
    }
    
    throw std::runtime_error("未知的物品类型: " + type);
}

std::string Item::enumToString(ItemType it){
        switch(it){
            case ItemType::Medicine: return "Medicine";
            case ItemType::Equipment: return "Equipment";
        }
        return "Unknown Item Type";
    };

    std::string Item::enumToString(EquipmentType et){
        switch(et){
            case EquipmentType::armor: return "armor";
            case EquipmentType::weapon: return "weapon";
        }
        return "Unknown Equipment Type";
    }

    ItemType Item::stringToItEnum(const std::string& s){
        if(s == "medicine") return ItemType::Medicine;
        if(s == "equipment") return ItemType::Equipment;
        throw std::runtime_error("Unknown equipment type: " + s);
    }

    EquipmentType Item::stringToEqEnum(const std::string& s){
        if(s == "weapon") return EquipmentType::weapon;
        if(s == "armor") return EquipmentType::armor;
        throw std::runtime_error("Unknown equipment type: " + s);
    }