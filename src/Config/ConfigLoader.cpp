#include "Config/ConfigLoader.h"
#include <json.hpp>
#include <fstream>
#include <iostream>
#include "Item/Medicine.h"
#include "Item/Equipment.h"
#include "Config/Config.h"

using json = nlohmann::json;

// [FIX_J] 静态成员初始化
std::vector<Monster> ConfigLoader::s_monsters;
bool ConfigLoader::s_monstersLoaded = false;
std::unordered_map<int, ConfigLoader::ItemFactoryData> ConfigLoader::s_itemFactoryData;
bool ConfigLoader::s_itemsLoaded = false;

// [FIX_J] 带缓存的怪物加载
const std::vector<Monster>& ConfigLoader::loadMonsters(const std::string& filepath) {
    if (s_monstersLoaded) {
        return s_monsters;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "无法打开怪物配置文件: " << filepath << std::endl;
        return s_monsters;
    }

    json data;
    file >> data;
    s_monsters.clear();

    for (const auto& item : data["monsters"]) {
        // [FIX_J] 字段存在性校验
        auto check = [&](const std::string& key) {
            if (!item.contains(key)) {
                std::cerr << "[FIX_J] Warning: monster 缺少字段 '" << key << "'" << std::endl;
                return false;
            }
            return true;
        };

        if (!check("name") || !check("hp") || !check("atk") ||
            !check("exp") || !check("minLevel") || !check("maxLevel")) {
            continue;
        }

        std::string name = item["name"];
        int hp = item["hp"];
        int atk_min = item["atk"][0];
        int atk_max = item["atk"][1];
        int exp = item["exp"];
        int minLevel = item["minLevel"];
        int maxLevel = item["maxLevel"];
        // [V5.0] 可选 Boss 标志，缺省为普通怪
        bool isBoss = item.value("isBoss", false);

        // [FIX_J] 解析掉落表
        std::vector<DropEntry> drops;
        if (item.contains("drops")) {
            for (const auto& d : item["drops"]) {
                if (d.contains("itemId") && d.contains("weight")) {
                    drops.push_back({d["itemId"], d["weight"]});
                } else {
                    std::cerr << "[FIX_J] Warning: " << name << " 的 drops 条目缺少 itemId 或 weight" << std::endl;
                }
            }
        }

        s_monsters.emplace_back(name, hp, std::make_pair(atk_min, atk_max),
                                exp, std::make_pair(minLevel, maxLevel), std::move(drops), isBoss);
    }

    std::cout << "已加载 " << s_monsters.size() << " 个怪物" << std::endl;
    s_monstersLoaded = true;
    return s_monsters;
}

// [FIX_J] 带缓存的物品加载（首次解析 JSON，后续从缓存构造新实例）
std::vector<std::unique_ptr<Item>> ConfigLoader::loadInitialItems(const std::string& filepath) {
    // 首次加载：解析 JSON 填充工厂数据
    if (!s_itemsLoaded) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "无法打开物品配置文件: " << filepath << std::endl;
            return {};
        }

        json data;
        file >> data;
        s_itemFactoryData.clear();

        // [FIX_J] 解析 medicines 顶级 key
        if (data.contains("medicines")) {
            for (const auto& item : data["medicines"]) {
                if (!item.contains("id") || !item.contains("name") || !item.contains("hp_restore")) {
                    std::cerr << "[FIX_J] Warning: medicines 条目缺少必要字段" << std::endl;
                    continue;
                }
                int id = item["id"];
                ItemFactoryData fd;
                fd.type = "medicine";
                fd.name = item["name"];
                fd.hp_restore = item["hp_restore"];
                fd.stackable = item.value("stackable", true);
                fd.baseQuantity = item.value("baseQuantity", 1);
                s_itemFactoryData[id] = std::move(fd);
            }
        }

        // [FIX_J] 解析 equipments 顶级 key
        if (data.contains("equipments")) {
            for (const auto& item : data["equipments"]) {
                if (!item.contains("id") || !item.contains("name") || !item.contains("type_2")) {
                    std::cerr << "[FIX_J] Warning: equipments 条目缺少必要字段" << std::endl;
                    continue;
                }
                int id = item["id"];
                ItemFactoryData fd;
                fd.type = "equipment";
                fd.name = item["name"];
                fd.type_2 = item["type_2"];
                fd.atk = {item["atk"][0], item["atk"][1]};
                fd.def = item.value("def", 0);
                fd.stackable = item.value("stackable", false);
                fd.baseQuantity = item.value("baseQuantity", 1);
                s_itemFactoryData[id] = std::move(fd);
            }
        }

        s_itemsLoaded = true;
        std::cout << "已加载 " << s_itemFactoryData.size() << " 个物品模板" << std::endl;
    }

    // [FIX_J] 从缓存构造新实例（首次/后续共用）
    std::vector<std::unique_ptr<Item>> result;
    for (const auto& [id, fd] : s_itemFactoryData) {
        if (fd.baseQuantity > 0) {
            auto item = createItemById(id, fd.baseQuantity);
            if (item) result.push_back(std::move(item));
        }
    }
    return result;
}

// [FIX_J] 根据 id 创建物品实例
std::unique_ptr<Item> ConfigLoader::createItemById(int id, int quantity) {
    auto it = s_itemFactoryData.find(id);
    if (it == s_itemFactoryData.end()) {
        std::cerr << "[FIX_J] 警告: 未找到 itemId=" << id << " 的物品模板" << std::endl;
        return nullptr;
    }

    const auto& data = it->second;
    if (data.type == "medicine") {
        ItemType itype = Item::stringToItEnum("medicine");
        return std::make_unique<Medicine>(id, data.name, quantity, data.hp_restore, itype);
    } else if (data.type == "equipment") {
        ItemType itype = Item::stringToItEnum("equipment");
        EquipmentType etype = Item::stringToEqEnum(data.type_2);
        return std::make_unique<Equipment>(id, data.name, data.atk, data.def,
                                           quantity, itype, etype);
    }
    return nullptr;
}
