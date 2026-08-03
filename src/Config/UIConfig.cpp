#include <iostream>
#include <limits>

#include "Config/UIConfig.h"

namespace UIConfig
{
    int checkNumberInput(int f , int e)
    {
        int num;
        int attempts = 0;
        constexpr int MAX_ATTEMPTS = 50;
        while(!(std::cin >> num) || num < f || num > e){
            attempts++;
            if (attempts >= MAX_ATTEMPTS) {
                std::cout << "输入错误次数过多，返回默认值" << std::endl;
                return f;
            }
            std::cout << "非法输入，请重新输入：" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');//清除缓存
        }
        return num;
    }

    void delay(int ms){
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
} // namespace UIConfig

