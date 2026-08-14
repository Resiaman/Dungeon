#include "Player/PlayerBag.h"
#include "Player/Player.h"
#include "Item/Item.h"
#include "Config/UIConfig.h"
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>

void PlayerBag::addItem(std::unique_ptr<Item> item) {
    if (!item->isStackable()) {
        bag.push_back(std::move(item));
        return;
    }

    for (auto& it : bag) {
        if (it->getID() == item->getID() && it->isStackable()) {
            it->setQuantity(it->getQuantity() + item->getQuantity());
            return;
        }
    }

    bag.push_back(std::move(item));
}

void PlayerBag::useItem(size_t index, std::vector<int> realIdMapping, Player& player) {
    if (index >= realIdMapping.size()) {
        std::cout << "Invalid index" << std::endl;
        return;
    }

    int item = realIdMapping[index];
    if (item < 0 || static_cast<size_t>(item) >= bag.size() || !bag[item]) {
        std::cerr << "[P0-3] Warning: 背包映射索引非法 item=" << item << std::endl;
        return;
    }

    bag[item]->use(player);
}

void PlayerBag::useItem(size_t index, Player& player) {
    if (index >= bag.size()) {
        std::cerr << "[P0-3] Warning: 背包索引越界 index=" << index
                  << " size=" << bag.size() << std::endl;
        return;
    }
    if (!bag[index]) {
        return;
    }

    bag[index]->use(player);
}

std::vector<int> PlayerBag::showItem(ItemType it_type) const {
    std::vector<int> idMapping;
    if (bag.empty()) {
        std::cout << "Bag is empty" << std::endl;
        return idMapping;
    }

    int index = 1;
    for (size_t i = 0; i < bag.size(); i++) {
        if (bag[i]->getType() == Item::enumToString(it_type) && bag[i]->getQuantity() != 0) {
            std::cout << index++ << "." << bag[i]->name;
            std::cout << " x" << bag[i]->getQuantity();
            idMapping.push_back(i);
            std::cout << std::endl;
        }
    }
    return idMapping;
}

std::vector<int> PlayerBag::showItem(EquipmentType eq_type) const {
    std::vector<int> idMapping;
    if (bag.empty()) {
        std::cout << "Bag is empty" << std::endl;
        return idMapping;
    }

    int index = 1;
    for (size_t i = 0; i < bag.size(); i++) {
        if (bag[i]->getType() == Item::enumToString(eq_type) && bag[i]->getQuantity() != 0) {
            std::cout << index++ << "." << bag[i]->name;
            std::cout << " x" << bag[i]->getQuantity();
            idMapping.push_back(i);
            std::cout << std::endl;
        }
    }
    return idMapping;
}

void PlayerBag::showItem() const {
    if (bag.empty()) {
        std::cout << "Bag is empty" << std::endl;
        return;
    }

    int index = 1;
    for (size_t i = 0; i < bag.size(); i++) {
        if (bag[i]->getQuantity() != 0) {
            std::cout << index++ << "." << bag[i]->name;
            std::cout << " x" << bag[i]->getQuantity();
            std::cout << std::endl;
        }
    }
}

std::vector<int> PlayerBag::getVisibleIndices() const {
    std::vector<int> indices;
    for (size_t i = 0; i < bag.size(); ++i) {
        if (bag[i]->getQuantity() != 0) {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}

void PlayerBag::clear() {
    bag.clear();
}

void PlayerBag::removeItem() {
    bag.erase(
        std::remove_if(bag.begin(), bag.end(),
                       [](const auto& item) { return item->getQuantity() <= 0; }),
        bag.end());
}

std::unique_ptr<Equipment> PlayerBag::getEquipment(size_t index) {
    if (index >= bag.size()) return nullptr;
    Equipment* rawPtr = dynamic_cast<Equipment*>(bag[index].get());
    if (!rawPtr) return nullptr;
    auto result = std::unique_ptr<Equipment>(rawPtr);
    bag[index].release();
    bag.erase(bag.begin() + index);
    return result;
}

bool PlayerBag::equipItem(int id, Player& player) {
    for (size_t i = 0; i < bag.size(); i++) {
        if (bag[i]->getID() == id) {
            Equipment* eqPtr = dynamic_cast<Equipment*>(bag[i].get());
            if (!eqPtr) {
                std::cerr << "[P0-3] Warning: 物品 id=" << id
                          << " 不是装备，无法穿戴" << std::endl;
                return false;
            }

            auto eq = player.eqSlot.changeEuipType(std::move(bag[i]));
            bag.erase(bag.begin() + i);
            if (!eq) {
                return false;
            }
            player.eqSlot.wear(player, std::move(eq));
            return true;
        }
    }
    return false;
}

bool PlayerBag::removeByID(int ID, int quantity) {
    for (auto it = bag.begin(); it != bag.end(); it++) {
        if ((*it)->getID() == ID) {
            int current = (*it)->getQuantity();
            if (current > quantity) {
                (*it)->setQuantity(current - quantity);
                return true;
            } else {
                std::cout << "Remove " << (*it)->name << " successfully";
                bag.erase(it);
                return true;
            }
        }
    }
    return false;
}

bool PlayerBag::addItemByID(int ID, int quantity) {
    for (auto& item : bag) {
        if (item->getID() == ID && item->isStackable()) {
            item->setQuantity(item->getQuantity() + quantity);
            std::cout << "Add " << item->name << " successfully";
            return true;
        }
    }
    return false;
}

nlohmann::json PlayerBag::toJson() const {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& item : bag) {
        if (item && item->getQuantity() > 0) {
            j.push_back(item->toJson());
        }
    }
    return j;
}

void PlayerBag::fromJson(nlohmann::json& j) {
    bag.clear();
    if (!j.is_array()) {
        return;
    }

    for (auto& itemJson : j) {
        try {
            auto item = Item::fromJson(itemJson);
            if (item && item->getQuantity() > 0) {
                bag.push_back(std::move(item));
            }
        } catch (const std::exception& e) {
            std::cerr << "[f] 警告: 加载物品失败: " << e.what() << std::endl;
        }
    }
}
