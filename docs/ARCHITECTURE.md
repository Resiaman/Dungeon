# Dongeon 架构分析

> 本文档分析 Dongeon V5.0 的系统架构与核心设计决策，作为本项目开发经验的技术沉淀。

## 一、整体架构

```
┌─────────────────────────────────────────────────────┐
│                  dongeon.cpp（主循环）                 │
│   菜单状态机：战斗/休息/状态/背包/装备/存档/退出          │
└────────────────────────┬────────────────────────────┘
                         │
        ┌────────────────┼─────────────────┐
        ▼                ▼                 ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────────┐
│  Battle/     │ │  Player/     │ │  Config/         │
│  FightLogic  │ │  Player      │ │  ConfigLoader    │
│  战斗状态机   │ │  PlayerBag   │ │  saveManager     │
│              │ │  PlayerEquip │ │  UIConfig        │
└──────┬───────┘ └──────┬───────┘ │  GameUIConfig    │
       │                │         └────────┬─────────┘
       ▼                ▼                  ▼
┌─────────────────────────────────────────────────────┐
│                  Item/ 物品体系                       │
│   Item（抽象基类）← Medicine / Equipment（多态）       │
│   Monster（含 Boss 标志 + 掉落表）                     │
└─────────────────────────────────────────────────────┘
```

**分层原则**：UI（Config/UI*）→ 逻辑（Battle/Player/Monster）→ 数据（ConfigLoader + JSON）。上层只依赖下层接口，模块间通过类对象协作，不直接操作全局状态。

## 二、数据驱动设计（核心）

### 2.1 配置与代码分离

所有游戏内容（怪物、物品、掉落、Boss）保存在 `config/*.json`，代码只负责"如何解析"和"如何运行"，不负责"内容是什么"。

**收益**：
- 调平衡只改 JSON，不重新编译、不动逻辑代码
- 加新怪物/物品 = 加一条 JSON 记录，零代码改动
- 内容与逻辑解耦，为后续工具化（如配置编辑器）留了空间

### 2.2 加载缓存 + 工厂模式

```cpp
// ConfigLoader 持有静态缓存，首次解析后复用
static std::vector<Monster> s_monsters;
static bool s_monstersLoaded;
static std::unordered_map<int, ItemFactoryData> s_itemFactoryData;
```

- **缓存**：`loadMonsters()` 首次读文件解析，之后直接返回缓存，避免重复 I/O 与解析
- **工厂**：`createItemById(id, qty)` 按物品模板 id 构造实例（区分药水/装备），掉落、初始物品共用一套构造入口——**一处定义，多处复用**，杜绝"各写各的构造"造成的维护灾难

### 2.3 掉落表 = 权重池

```json
"drops": [ {"itemId": 10001, "weight": 85}, {"itemId": 20001, "weight": 8}, ... ]
```

实现：`roll = random(1, totalWeight)`，累加权重直到 `roll ≤ 累计值` 命中。

- 概率 = 单项权重 / 总权重（85/100 = 85% 掉药水）
- 调权重即调概率，数学直观、可配置
- Boss 掉落 3 次 roll（独立抽取），保证"专属武器 70% 权重"下必掉概率达 97.3%

## 三、物品体系：多态 + 所有权管理

### 3.1 继承结构

```
Item（抽象基类）
├── virtual use(Player&) = 0        # 行为差异
├── virtual getType() = 0           # 类型信息
├── virtual toJson()                # 序列化
├── virtual getQuantity()/setQuantity()/isStackable()   # 堆叠能力
├── Medicine（药水）：可堆叠，use 恢复 HP
└── Equipment（装备）：不可堆叠，use 穿戴到装备栏
```

**为什么用多态而非 if-else**：背包持有 `vector<unique_ptr<Item>>`，遍历时统一调 `use()`，具体行为由动态分派决定——新增物品类型只需新增派生类，背包/战斗代码零改动（**开闭原则**）。

### 3.2 所有权管理：unique_ptr

- 背包、装备栏均用 `unique_ptr` 持有物品——**所有权唯一且明确**
- 穿戴装备 = 转移所有权（`std::move` 从背包到装备栏），杜绝裸指针的悬垂/泄漏风险
- `changeEuipType()` 负责 `dynamic_cast` 向下转型 + `release()` 安全转移，失败返回 `nullptr` 而非崩溃

### 3.3 序列化：静态工厂分发

```cpp
static std::unique_ptr<Item> Item::fromJson(nlohmann::json &j);
// 按 JSON 中 "ItemType" 字段分发到 Medicine/Equipment 构造
```

**关键坑**：虚函数无法在静态上下文中多态调用，所以反序列化不能用"基类虚函数"，必须**静态工厂 + 类型字段分发**。这是项目踩过坑后的修正（代码中留有 `【f】` 注释记录）。

## 四、玩家系统：背包与装备栏

### 4.1 背包（PlayerBag）

- `vector<unique_ptr<Item>>` + 数量堆叠：同 ID 可堆叠物品自动合并数量
- **显示/使用分离**：`showItem(Type)` 返回"可见物品的真实下标映射"，玩家输入序号 → 映射到真实索引 → 使用。避免用户输入与容器索引直接耦合（增删物品后索引漂移的隐患）
- 数量归零的物品由 `removeItem()` 统一清理（erase-remove 惯用法）

### 4.2 装备栏（PlayerEquipment）

- `array<unique_ptr<Equipment>, EquipmentType::count>`——**槽位与枚举一一对应**，天然防越界
- 穿戴三态：空槽直接穿 / 有装备则询问更换（交换所有权）/ 卸下回背包
- 属性加成实时增减：穿戴 `playerGain`、卸下 `takeOffGain`，**构筑强度由当前装备决定**，换装即时生效

## 五、Boss 机制（V5.0 新增）

**最小侵入式扩展**——在不推翻现有框架的前提下长出全新玩法：

1. `Monster` 加一个 `bool isBoss` 标志（JSON 可选字段，缺省 false）
2. 战斗入口把怪物池拆成"普通池 + Boss 池"，10% 概率遭遇 Boss
3. Boss 专属掉落：3 次 roll + 专属武器高权重

一个标志位 + 概率判断，就完成了"普通刷怪 → 惊喜 Boss"的玩法升级。**这种"在现有系统上叠加最小变更"的思路，是游戏迭代最常用的手法**——每次只加一个小概念，验证好玩再继续。

## 六、存档系统（V4.1）

- 全量 JSON 序列化：玩家属性 + 背包 + 装备栏 → `save/savegame.json`
- 每个类自管 `toJson()/fromJson()`，父类组合调用子类——职责单一、可独立测试
- 加载容错：字段缺失用默认值兜底，物品解析失败跳过并警告，不整体崩溃

## 七、设计模式小结

| 模式 | 应用位置 | 解决的问题 |
|---|---|---|
| 工厂方法 | ConfigLoader::createItemById | 统一物品构造入口 |
| 单例（静态缓存） | ConfigLoader 静态成员 | 避免重复解析配置 |
| 多态 | Item → Medicine/Equipment | 行为差异、开闭原则 |
| 策略（回调式） | FightLogic 怪物筛选 lambda | 可复用的筛选规则 |
| 外观 | UIConfig / GameUIConfig | 统一输入校验、延迟、UI 交互 |

## 八、已知局限与未来方向

1. **数值平衡靠人工**：掉落概率、怪物强度目前是静态权重 + 经验设计，未来可引入"随玩家等级动态修正"（如稀有度随等级提升）
2. **无地图概念**：当前是纯菜单战斗，没有移动/探索层——若要升级为真正的"地牢"，需引入地图与寻路
3. **存档单文件**：无多存档槽、无自动存档（战斗中途退出会丢进度）
4. **装备词条单一**：目前只有攻/防数值，无随机词条——这是"构筑驱动"体验的关键扩展方向

这些局限为 Unity 重制版（Dongeon 的架构资产迁移）划出了清晰的演进路线。
