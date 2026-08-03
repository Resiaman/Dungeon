#ifndef CONFIG_H
#define CONFIG_H

// 引入随机数库
#include <random>
//引入输入输出库及验证
#include <iostream>
#include <limits>
#include <memory>
#include "json.hpp"


// 随机数生成器v1.0
    // random_device rd;   -- 此处由于VS原因可能导致运行后rd返回同一个值（实现原因）
    // mt19937 gen(rd());//此处的gen可以作为shuffle的第三个参数
    //uniform_int_distribution<> dis(0, 100); --dis生成100以内的随机数
    
 //随机数生成器v2.0
extern std::mt19937 gen;

// [FIX] 封装随机数，统一调用方式，避免各处直接操作 gen
namespace Random {
    int range(int min, int max);
}
//time_since_epoch()返回从1970年1月1日0时0分0秒到现在的毫秒数,.count()将时间值转为int

constexpr int SHORT_DELAY = 500;
constexpr int LONG_DELAY = 1000;

constexpr int STARTHP = 25;    // [V5.0] 20→25：新手期容错（攻击浮动下限时也打得过森林怪）
constexpr int STARTDEF = 0;
constexpr int STARTLEVEL = 0;
constexpr int STARTEXP = 0;

constexpr int MAXLEVEL = 60;
constexpr int EXPTOUP = 15;   // [V5.0] 对应新经验公式 0 级需求 15 = 15 + 8*0
constexpr int MIN_ATK = 3;
constexpr int MAX_ATK = 7;
constexpr int STARTMEDICINE = 3;
constexpr int OTHERMEDICINE = 0;


#endif
