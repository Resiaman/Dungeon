    #include "Item/Medicine.h"
    #include "Player/Player.h"
    #include "Config/UIConfig.h"
    #include <iostream>
    #include <vector>

    void Medicine::use(Player& player){
        if(getQuantity()>0 && player.currentHp< player.hp_UpperLimit){
            player.currentHp+=hp_restore;
            if(player.currentHp>player.hp_UpperLimit){
                player.currentHp=player.hp_UpperLimit;
            }
            std::cout<<"使用"<<name<<"恢复"<<hp_restore<<"点生命值"<< std::endl;
            //库存减少
            stock--;
            if(stock!=0){
                std::cout<<"剩余"<<stock<<"个"<<name<< std::endl;
            }else{
                std::cout<<"已使用完"<<name<<std::endl;
                player.bag.removeItem();
            }
        }else{
            UIConfig::delay(SHORT_DELAY);
            std::cout<<"\n你的生命值已满"<<std::endl;
            UIConfig::delay(SHORT_DELAY);
        }
    }


    void Medicine::showMedicine() const{
        std::cout << name << "有" << stock << "个" <<std::endl;
    }

    int Medicine::getID(){
        return ID;
    }

    nlohmann::json Medicine::toJson() const{
        return nlohmann::json{
            {"name",name},
            {"ID",ID},
            {"ItemType","medicine"},
            {"stock",getQuantity()},
            {"restore",hp_restore}
        };
    }

    //【f】删除旧的虚函数fromJson实现，现在由Item::fromJson静态工厂方法统一处理