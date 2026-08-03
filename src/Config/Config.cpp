#include <chrono>
#include "Config/Config.h"

//std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
std::mt19937 gen(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));

int Random::range(int min, int max) {
    return std::uniform_int_distribution<>(min, max)(gen);
}


