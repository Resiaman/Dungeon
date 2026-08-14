#include "Config/ConfigLoader.h"
#include <json.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include "Item/Medicine.h"
#include "Item/Equipment.h"
#include "Config/Config.h"

using json = nlohmann::json;

namespace {

bool validAtkShape(const json& atk) {
    return atk.is_array() && atk.size() >= 2 &&
           atk[0].is_number_integer() && atk[1].is_number_integer() &&
           atk[0].get<int>() <= atk[1].get<int>();
}

}  // namespace

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
    try {
        file >> data;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[P0-1] 怪物配置解析失败: " << e.what() << std::endl;
        return s_monsters;
    } catch (const std::exception& e) {
        std::cerr << "[P0-1] 怪物配置解析失败: " << e.what() << std::endl;
        return s_monsters;
    }

    s_monsters.clear();
    if (!data.is_object() || !data.contains("monsters") || !data["monsters"].is_array()) {
        std::cerr << "[P0-1] Warning: 怪物配置缺少 monsters 数组" << std::endl;
        s_monstersLoaded = true;
        return s_monsters;
    }

    size_t monsterIndex = 0;
    for (const auto& item : data["monsters"]) {
        ++monsterIndex;
        try {
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
            if (!item["atk"].is_array() || !validAtkShape(item["atk"])) {
                std::cerr << "[P0-1] Warning: 怪物 " << name << " 的 atk 形状非法，已跳过" << std::endl;
                continue;
            }

            int hp = item["hp"];
            int atk_min = item["atk"][0];
            int atk_max = item["atk"][1];
            int exp = item["exp"];
            int minLevel = item["minLevel"];
            int maxLevel = item["maxLevel"];
            bool isBoss = item.value("isBoss", false);

            std::vector<DropEntry> drops;
            if (item.contains("drops")) {
                if (!item["drops"].is_array()) {
                    std::cerr << "[P0-1] Warning: " << name << " 的 drops 形状非法" << std::endl;
                } else {
                    for (const auto& d : item["drops"]) {
                        if (!d.is_object() || !d.contains("itemId") || !d.contains("weight") ||
                            !d["itemId"].is_number_integer() || !d["weight"].is_number_integer() ||
                            d["weight"].get<int>() <= 0) {
                            std::cerr << "[P0-1] Warning: " << name << " 的 drops 条目 weight 非法" << std::endl;
                            continue;
                        }
                        drops.push_back({d["itemId"].get<int>(), d["weight"].get<int>()});
                    }
                }
            }

            s_monsters.emplace_back(name, hp, std::make_pair(atk_min, atk_max),
                                    exp, std::make_pair(minLevel, maxLevel), std::move(drops), isBoss);
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[P0-1] Warning: 跳过怪物（第 " << monsterIndex << " 条），原因: "
                      << e.what() << std::endl;
            continue;
        } catch (const std::exception& e) {
            std::cerr << "[P0-1] Warning: 跳过怪物（第 " << monsterIndex << " 条），原因: "
                      << e.what() << std::endl;
            continue;
        }
    }

    std::cout << "已加载 " << s_monsters.size() << " 个怪物" << std::endl;
    s_monstersLoaded = true;
    return s_monsters;
}

// [FIX_J] 带缓存的物品加载
std::vector<std::unique_ptr<Item>> ConfigLoader::loadInitialItems(const std::string& filepath) {
    if (!s_itemsLoaded) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "无法打开物品配置文件: " << filepath << std::endl;
            return {};
        }

        json data;
        try {
            file >> data;
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[P0-1] 物品配置解析失败: " << e.what() << std::endl;
            return {};
        } catch (const std::exception& e) {
            std::cerr << "[P0-1] 物品配置解析失败: " << e.what() << std::endl;
            return {};
        }

        s_itemFactoryData.clear();
        if (!data.is_object()) {
            std::cerr << "[P0-1] Warning: 物品配置不是 JSON 对象" << std::endl;
            s_itemsLoaded = true;
            return {};
        }

        if (data.contains("medicines")) {
            if (!data["medicines"].is_array()) {
                std::cerr << "[P0-1] Warning: medicines 不是数组" << std::endl;
            } else {
                for (size_t i = 0; i < data["medicines"].size(); ++i) {
                    try {
                        const auto& item = data["medicines"][i];
                        if (!item.is_object() || !item.contains("id") || !item.contains("name") ||
                            !item.contains("hp_restore") || !item["id"].is_number_integer() ||
                            !item["hp_restore"].is_number_integer()) {
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
                    } catch (const nlohmann::json::exception& e) {
                        std::cerr << "[P0-1] Warning: 跳过药品条目，原因: " << e.what() << std::endl;
                        continue;
                    } catch (const std::exception& e) {
                        std::cerr << "[P0-1] Warning: 跳过药品条目，原因: " << e.what() << std::endl;
                        continue;
                    }
                }
            }
        }

        if (data.contains("equipments")) {
            if (!data["equipments"].is_array()) {
                std::cerr << "[P0-1] Warning: equipments 不是数组" << std::endl;
            } else {
                for (size_t i = 0; i < data["equipments"].size(); ++i) {
                    try {
                        const auto& item = data["equipments"][i];
                        if (!item.is_object() || !item.contains("id") || !item.contains("name") ||
                            !item.contains("type_2") || !item["id"].is_number_integer()) {
                            std::cerr << "[FIX_J] Warning: equipments 条目缺少必要字段" << std::endl;
                            continue;
                        }
                        int id = item["id"];
                        std::string name = item["name"];
                        std::string t2 = item["type_2"];
                        if (t2 != "weapon" && t2 != "armor") {
                            std::cerr << "[P0-1] Warning: 装备 " << name << " 的 type_2='"
                                      << t2 << "' 非法，已跳过" << std::endl;
                            continue;
                        }
                        if (!item.contains("atk") || !validAtkShape(item["atk"])) {
                            std::cerr << "[P0-1] Warning: 装备 " << name << " 的 atk 形状非法，已跳过" << std::endl;
                            continue;
                        }
                        ItemFactoryData fd;
                        fd.type = "equipment";
                        fd.name = name;
                        fd.type_2 = t2;
                        fd.atk = {item["atk"][0], item["atk"][1]};
                        fd.def = item.value("def", 0);
                        fd.stackable = item.value("stackable", false);
                        fd.baseQuantity = item.value("baseQuantity", 1);
                        s_itemFactoryData[id] = std::move(fd);
                    } catch (const nlohmann::json::exception& e) {
                        std::cerr << "[P0-1] Warning: 跳过装备条目，原因: " << e.what() << std::endl;
                        continue;
                    } catch (const std::exception& e) {
                        std::cerr << "[P0-1] Warning: 跳过装备条目，原因: " << e.what() << std::endl;
                        continue;
                    }
                }
            }
        }

        s_itemsLoaded = true;
        std::cout << "已加载 " << s_itemFactoryData.size() << " 个物品模板" << std::endl;
    }

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
        if (etype == EquipmentType::count) {
            std::cerr << "[P0-1] Warning: 装备类型非法，itemId=" << id << std::endl;
            return nullptr;
        }
        return std::make_unique<Equipment>(id, data.name, data.atk, data.def,
                                           quantity, itype, etype);
    }
    return nullptr;
}
