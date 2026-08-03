# Dongeon（控制台地牢 RPG）

中文控制台回合制地牢冒险小游戏。数据驱动设计：怪物/物品配置在 `config/*.json`，运行时存档写入 `save/savegame.json`。

## 构建

要求：CMake ≥ 3.20、支持 C++17 的编译器（Windows 推荐 MinGW-w64 g++）。

```bash
# 配置 + 构建
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build

# 运行（注意：必须在项目根目录下启动，程序使用相对路径读取 config/ 与 save/）
./build/dongeon.exe
```

VS Code 使用：安装 CMake Tools 插件后打开本目录即可自动识别 `CMakeLists.txt`。

## 版本历史

- V4.1 (2026/6/15)：存档系统（JSON 序列化）
- V3.0 (2026/6/9)：装备系统、掉落表、工厂化管理模式、UI 封装重构
- V2.3 (2026/5/24)：战斗平衡调整、药水分类使用、物品类型强枚举
- V2.2 (2026/5/23)：等级限制的怪物刷新机制

详见 `LogContent/updataLog.txt`。
