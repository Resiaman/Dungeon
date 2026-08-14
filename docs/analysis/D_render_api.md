# 美术资源表现接口设计

> 定位：本文档是 C 报告（架构质量评估与分级升级建议）的配套设计，回答"纯逻辑层（P2-2 GameCore）抽出来后，表现层如何接"。目标是为 Dongeon 后续图形化（Unity 2.5D 重制）做准备：**一套渲染/输入/资源抽象接口，Console 与 Unity 共用同一份规则**，迁移风险从"重写"降为"翻译"（呼应 C 报告 ③ 3.2-1）。
>
> 本文档只定义接口与数据流，不约束任何具体渲染技术。

---

## 1. 表现层与逻辑层边界

### 1.1 一句话边界

> **逻辑层回答"发生了什么"，表现层回答"怎么呈现"。**

C 报告 ② P2-2 指出：领域逻辑（回合结算、伤害公式、掉落 roll、升级曲线、怪物筛选）目前与控制台 I/O 纠缠，Unity 端无法复用；必须抽出平台无关的 **GameCore**（纯函数/纯对象，无 `std::cout`/`delay`/输入），现有 UI 只做"渲染 + 读输入 → 调 GameCore → 渲染结果"。本文档的整套接口，就是这条边界的形式化落地。

### 1.2 GameCore 输出什么（事件，而非画面）

GameCore 对外只产出一类东西：**游戏事件序列**（“什么发生了”）。例如：

| 事件 | 含义 | 示例 |
|---|---|---|
| 战斗结算 | 攻击/伤害/暴击/治疗/命中判定结果 | `DamageEvent{target: 史莱姆, amount: 7, isCrit: false}` |
| 数值变化 | 玩家或怪物的 HP/MP/攻防/经验增减 | `StatChangeEvent{stat: hp, delta: -7}` |
| 物品事件 | 掉落/获得/使用/穿戴 | `DropEvent{itemId: 10001, qty: 2}` |
| 状态转移 | 升级/死亡/逃跑/Boss 遭遇 | `LevelUpEvent{level: 5, gains: {...}}` |
| 文本消息 | 系统提示、规则说明 | `MessageEvent{"装备了铁剑"}` |

### 1.3 表现层消费什么

表现层消费两类输入：

1. **事件序列**——按顺序逐个"呈现"（打印一行 / 播放飘字 / 触发特效动画）；
2. **最小视图数据**——渲染一个画面所需的只读快照（当前场景名、战斗双方状态条、菜单选项列表等）。

节奏控制（`delay`）**属于表现层职责**。C 报告 ① 1.2-2 已指出 `UIConfig::delay` 是横切关注点，散落于 Player/Monster/Medicine/FightLogic 多处——抽逻辑层后这些 `delay` 全部回归表现层，由渲染器统一决定"这一条事件动画播多久、停顿多久"。

### 1.4 边界规则（三条红线）

1. **GameCore 不碰 I/O**：无 `std::cout`、无 `delay`、无 `std::cin`、无 `system("chcp")`（对应 C 报告 ① 1.4 与 ③ 3.2 的三大障碍中的阻塞输入与进程副作用）；
2. **表现层不回写逻辑状态**：渲染器只读事件与视图快照，所有规则（伤害、掉落、升级）都在 GameCore 内结算；
3. **输入单向流入**：表现层收集输入 → 转成 GameCommand → 调 GameCore；逻辑层不直接读输入设备（见第 4 章）。

**现状 vs 目标职责对照**（依据 C 报告 ① 1.2）：

| 职责 | 现状位置 | 目标位置 |
|---|---|---|
| 伤害结算/掉落 roll/升级曲线 | FightLogic / Monster / Player | GameCore（纯逻辑） |
| 打印战斗文案 | 领域类内 `std::cout` | 表现层（事件驱动） |
| 节奏控制 `delay` | Player/Monster/Medicine/FightLogic 散落 | 表现层渲染器统一 |
| 读输入 `checkNumberInput` | UIConfig 直读 `std::cin` | IInputSource（见第 4 章） |
| 菜单/状态/背包展示 | dongeon.cpp 状态机 + UI 门面 | 表现层场景渲染 |

---

## 2. 渲染抽象接口定义

### 2.1 设计目标

- **单一契约**：Console 与 Unity 各实现一份，GameCore 零改动（呼应 C 报告 ③ 3.2-1"UI 变薄壳"）；
- **按呈现动作分方法**：场景、战斗、消息、效果是四类最基本的"画什么"，各自独立演进（Unity 端可分别挂 UI Canvas / 战斗演出 / 日志滚动 / ParticleSystem）；
- **方法参数为只读值对象**：不传裸指针、不传回调，渲染器无副作用于逻辑层。

### 2.2 接口定义

```cpp
// IGameRenderer.h —— 渲染抽象接口：表现层与逻辑层的唯一契约
#pragma once
#include <string>
#include <vector>

// ---------- 只读视图数据（值对象，逻辑层组装，渲染器只读） ----------

// 一个场景/界面的最小描述（菜单、背包、装备栏、状态面板……）
struct SceneView {
    std::string sceneId;                 // 场景标识，如 "main_menu" / "bag" / "equip"
    std::string title;                   // 界面标题
    std::vector<std::string> options;    // 可选项文本（菜单/列表项）
    std::vector<std::string> lines;      // 自由文本行（说明、属性列表）
};

// 一场战斗回合双方的状态快照
struct CombatView {
    std::string sceneId;                 // 固定为 "combat"
    struct Side {
        std::string name;                // 实体名（玩家/怪物）
        int hp = 0, hpMax = 0;           // 当前/上限 HP
        int mp = 0, mpMax = 0;           // 当前/上限 MP（图形化预留）
        int atk = 0, def = 0;            // 攻/防
        bool isBoss = false;             // Boss 标识（影响 UI 表现，如血条样式）
    } player, enemy;
    std::string turnHint;                // 当前回合提示（"你的回合"/"怪物行动"）
};

// 玩家全局数值面板（状态菜单 / 战斗内状态条复用）
struct PlayerStatus {
    int level = 0, exp = 0, expNeed = 0; // 等级/经验/升级所需
    int hp = 0, hpMax = 0, mp = 0, mpMax = 0;
    int atk = 0, def = 0;                // 含装备加成后的总攻防
};

// 消息分级：渲染器据此决定样式（颜色/字体/日志过滤）
enum class MessageKind { Info, Combat, Loot, Error, System };

// 表现效果类型：Console 用符号/颜色模拟，Unity 映射到特效/动画
enum class EffectType { DamageFloat, HealFloat, CritFlash, DropSpark, LevelUpBurst };

struct EffectArgs {                      // 效果参数（宽泛但只读）
    std::string target;                  // 作用实体（名字或 id）
    int amount = 0;                      // 数值（飘字显示量）
    std::string itemId;                  // 物品相关效果时使用
};

// ---------- 渲染器接口 ----------

class IGameRenderer {
public:
    virtual ~IGameRenderer() = default;

    // 渲染一个场景/界面（菜单、背包、装备栏、状态面板……）
    // 由 dongeon.cpp 状态机（图形化后由场景控制器）在进入/刷新界面时调用
    virtual void RenderScene(const SceneView& view) = 0;

    // 渲染一场战斗回合（双方状态条、行动序列）
    // 每回合开始时调用一次，展示双方当前状态
    virtual void RenderCombat(const CombatView& view) = 0;

    // 输出一条文本消息（战斗日志、系统提示、对话）
    // kind 决定样式：Combat→战斗日志区、Error→错误样式、Loot→掉落高亮……
    virtual void RenderMessage(const std::string& text, MessageKind kind) = 0;

    // 播放一个表现效果（伤害飘字、技能特效、掉落闪光、升级爆发）
    // Console 实现为带颜色的符号动画；Unity 实现为 UI 飘字/粒子/动画
    virtual void PlayEffect(EffectType type, const EffectArgs& args) = 0;

    // 渲染玩家数值面板（状态菜单完整版 / 战斗内精简版）
    virtual void RenderStatus(const PlayerStatus& status) = 0;

    // 刷新画面：Console 实现为一次完整重绘 + delay 节奏；
    // Unity 实现为空操作（由引擎每帧驱动），或在需要强制同步时调用
    virtual void Flush() = 0;
};
```

### 2.3 方法职责与调用时机

| 方法 | 调用方 | 时机 |
|---|---|---|
| `RenderScene` | 主循环状态机 / 场景控制器 | 进入或刷新一个菜单/界面时 |
| `RenderCombat` | GameCore 战斗回调后的表现层 | 每回合开始（含 Boss 遭遇开场） |
| `RenderMessage` | 事件消费循环 | 每条 `MessageEvent` |
| `PlayEffect` | 事件消费循环 | 伤害/治疗/掉落/升级等事件 |
| `RenderStatus` | 状态菜单 / 战斗状态条 | 数值变更后的刷新 |
| `Flush` | 表现层每帧/每回合末尾 | 一帧或一回合呈现完毕 |

---

## 3. 渲染命令/事件流设计

### 3.1 总览：结算 → 事件序列 → 逐条呈现

采用 **"GameCore 产出事件序列，表现层逐条消费"** 的推模式，而非逻辑层直接调渲染器。理由：

- GameCore 保持**纯函数**形态（C 报告 P2-2），`resolveTurn()` 可离线执行、可单测、可回放；
- 表现层自由决定节奏与呈现方式（Console 逐条打印+delay；Unity 可加动画插帧）；
- 事件序列可序列化为日志，支持"复现某场战斗"（呼应 C 报告 P2-1 确定性目标）。

```
玩家选择"攻击" ──▶ GameCore::resolveTurn() ──▶ vector<GameEvent>
                                                    │
        ┌───────────────────────────────────────────┤
        ▼                                           ▼
 ConsoleRenderer                         UnityRenderer
 逐条: RenderMessage / PlayEffect /       逐条: 飘字 + 日志滚动 + 血条
 状态条重绘 + delay 节奏                  + 演出协程
```

### 3.2 事件类型定义

```cpp
// GameEvent.h —— 逻辑层唯一的事件输出形态（值对象 + 类型字段）
struct GameEvent {
    enum class Type { Damage, Heal, StatChange, Drop, LevelUp, Message, CombatTurn };
    Type type = Type::Message;

    // 公共字段
    std::string text;                 // Message 事件的文本；其他事件的人类可读摘要
    std::string target;               // 作用实体名

    // 分类型字段（按 type 取用）
    int amount = 0;                   // Damage/Heal 的数值
    bool isCrit = false;              // Damage 的暴击标记
    MessageKind kind = MessageKind::Info;      // Message 的分级
    std::string stat;                 // StatChange 的属性名（"hp"/"exp"/"atk"…）
    int delta = 0;                    // StatChange 的变化量（含符号）
    std::string itemId;               // Drop 的物品模板 id
    int qty = 0;                      // Drop 的数量
};
```

### 3.3 完整链路示例：伤害飘字

```cpp
// GameCore 侧（纯逻辑，无 I/O）：
std::vector<GameEvent> FightLogic::resolveTurn() {
    std::vector<GameEvent> events;
    int dmg = ComputeDamage(attacker, defender);          // 纯公式
    defender.hp -= dmg;
    events.push_back({GameEvent::Type::Damage, /*text=*/"史莱姆 受到 7 点伤害",
                      /*target=*/"史莱姆", /*amount=*/7, /*isCrit=*/false, ...});
    events.push_back({GameEvent::Type::StatChange, ..., /*stat=*/"hp", /*delta=*/-7, ...});
    return events;                                        // 表现层拿到的就是这份序列
}

// 表现层侧（消费循环）：
for (const auto& ev : renderer.PollEvents()) {            // 或 GameCore 回调返回的 vector
    switch (ev.type) {
        case GameEvent::Type::Damage:
            renderer.PlayEffect(EffectType::DamageFloat, {ev.target, ev.amount, ""});
            renderer.RenderMessage(ev.text, MessageKind::Combat);
            break;
        case GameEvent::Type::StatChange:
            renderer.RenderStatus(statusView);            // 血条/属性面板刷新
            break;
        // ...
    }
}
renderer.Flush();   // Console：重绘 + delay；Unity：无操作
```

### 3.4 战斗日志与数值变化：统一走事件通道

- **战斗日志** = `MessageEvent` 序列（`kind = Combat`），渲染器落滚动日志区；Console 直接逐行打印，Unity 落滚动 Text / RichText 高亮；
- **数值变化** = `StatChangeEvent` 序列，渲染器自行决定呈现：Console 打印"HP -7"；Unity 驱动血条插值 + 飘字（与 `DamageEvent` 的飘字去重由渲染器自行协调，接口层不关心）；
- **节奏**：Console 在每条事件后 `Flush()` 内做 delay；Unity 由演出协程控制，**同一个事件序列，两端呈现节奏完全可不同**——这正是第 1 章"表现层职责"的体现。

### 3.5 可回放与可测试收益

事件序列是无歧义的文本/值对象流，天然可：

1. 序列化为 JSON 战斗日志，bug 上报时可直接粘贴复现；
2. 用固定种子（C 报告 P2-1 的 IRandom）跑 `resolveTurn()`，断言事件序列与数值——把"战斗表现正确"从人肉看屏幕变成可断言的单测（P2-3）。

---

## 4. 输入抽象

### 4.1 问题背景

C 报告 ① 1.4-2：`UIConfig::checkNumberInput` 直读 `std::cin`（UIConfig.cpp:13），无输入流注入点，菜单/战斗流程无法脚本化驱动；③ 3.2-2：**std::cin 轮询在 Unity 事件驱动模型里不可用**——输入必须抽象为与逻辑层解耦（请求输入 → 回调/事件）。

### 4.2 统一输入接口

```cpp
// IInputSource.h —— 输入抽象：Console 键盘与图形化事件输入的统一入口
#pragma once
#include <string>
#include <optional>

// 与具体设备无关的按键码：Console 键盘与 Unity 的 KeyCode 在此对齐
enum class KeyCode { Up, Down, Left, Right, Confirm, Cancel, Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, None };

class IInputSource {
public:
    virtual ~IInputSource() = default;

    // 等待并返回下一个输入指令（阻塞版，Console 用）
    // Unity 版：从事件队列弹出一个（无事件时返回 std::nullopt，不阻塞）
    virtual std::optional<int> WaitNumber(int minVal, int maxVal) = 0;  // 数字选择：替代 checkNumberInput
    virtual std::optional<KeyCode> WaitKey() = 0;                        // 方向/确认/取消
    virtual void WaitAnyKey() = 0;                                       // "按任意键继续"
};
```

### 4.3 两个实现

| 实现 | 平台 | 说明 |
|---|---|---|
| `ConsoleInputSource` | Console | 包装 `std::cin`：`WaitNumber` 复用现有校验逻辑（非法输入提示 + 范围约束，替代 `checkNumberInput`，顺带修复 C 报告 P1-2 的"静默返回下限值"问题）；`WaitAnyKey` 用 `std::cin.get()` |
| `UnityInputSource` | Unity | 事件驱动：UI 按钮/键盘事件入队，`WaitNumber` 从队列取；无事件返回 `nullopt`，由表现层 `Flush()` 空转等下一帧——**不阻塞主线程** |

### 4.4 请求-响应模式（输入单向流入逻辑层）

逻辑层永远不主动读输入；流程固定为：

```
表现层 RenderScene(菜单) ──▶ 表现层 WaitNumber(1..N) ──▶ 输入转 GameCommand
                                                        │
                                                        ▼
                                           GameCore::HandleCommand(cmd)  ← 纯逻辑，无 I/O
```

- 菜单、背包、装备栏、战斗行为菜单（C 报告 ① 1.1 的 FightLogic 行为菜单）全部收敛为 `GameCommand` 枚举（如 `Attack / UseMedicine / Equip / Flee / AutoFight / Save`）；
- 这样同一份 GameCore 既可被 Console 数字键驱动，也可被 Unity 按钮/手柄驱动，**GameCore 对输入设备零感知**。

---

## 5. 资源加载抽象

### 5.1 美术资源目录约定

```
assets/
├── manifest.json          # 资源清单：id → 路径 映射（见 5.4）
├── sprites/               # 精灵/贴图（怪物、物品图标、UI 元素）
│   ├── monster_10001.png  # 资源 id 与 config JSON 的条目 id 对齐
│   └── item_20001.png
├── effects/               # 特效（飘字、闪光、爆发）
├── audio/                 # 音效/BGM（图形化预留）
└── fonts/                 # 字体（图形化预留）
```

### 5.2 ID 引用方式

资源按 **资源 id** 引用，且**与 config JSON 的条目 id 严格对齐**：

- 怪物 `monster.json` 条目 `id` → 精灵资源 `monster_<id>`（如 `monster_10001`）；
- 物品 `item.json` 条目 `id` → 图标资源 `item_<id>`（如 `item_20001`）；
- 效果按语义 id（`effect_damage_float` / `effect_levelup_burst`），与物品/怪物解耦。

这样 C 报告 ③ 3.1"数据驱动 JSON 原样保留"的资产，在图形化时直接由 id 关联到美术资源，**不引入第二套标识体系**。

### 5.3 与 config JSON 的衔接

config JSON 条目**可选**增加 `art` 字段（缺省走兜底，见 5.5）：

```json
{
  "id": 20001,
  "name": "铁剑",
  "type_2": "weapon",
  "atk": [5, 8],
  "art": {
    "sprite": "item_20001",
    "effect": "effect_equip_weapon"
  }
}
```

加载约定：`ConfigLoader`（C 报告 ① 1.1）保持只解析规则字段，`art` 由资源层读取；**解析失败仅告警并走兜底资源**——与 C 报告 P0-1"字段缺失兜底、不崩溃"的数据防线精神一致。

### 5.4 资源清单与缓存

`assets/manifest.json` 统一登记 `id → 路径`，资源层仿照 `ConfigLoader` 的静态缓存模式（C 报告 ② 2.2）：首次加载后缓存，`GetSprite(id)` / `GetEffect(id)` 命中缓存直接返回；`id` 不存在返回兜底资源并告警（对齐 `createItemById` 的"警告返回"约定，C 报告 ① 1.6）。

```cpp
// IResourceLoader.h —— 资源抽象：Console 用文本占位，Unity 用引擎资源
class IResourceLoader {
public:
    virtual ~IResourceLoader() = default;
    virtual bool HasArt(const std::string& artId) const = 0;
    virtual void* LoadSprite(const std::string& artId) = 0;   // Console: nullptr/占位; Unity: Sprite*
    virtual void* LoadEffect(const std::string& artId) = 0;   // Console: nullptr; Unity: ParticleSystem
};
```

### 5.5 缺省回退原则

任何资源缺失都**回退到占位资源并打印一条警告**，绝不阻塞游戏流程——图形化初期美术未就位时，玩法可先用纯文本/占位色块完整跑通（见第 6 章）。

---

## 6. Console 版最小实现方案

### 6.1 目标

用**最小可用的 `ConsoleRenderer`** 落地整套接口，替换现有 UI 薄壳：

- 实现 `IGameRenderer` + `ConsoleInputSource`，其他代码（GameCore、事件流）完全不变；
- 呈现方式：ANSI 颜色 + 文本符号模拟特效（`std::cout` + `system("chcp 65001 > nul")` 收敛到渲染器内部单点，呼应 C 报告 ① 1.4-3 的进程副作用问题）；
- 产出价值：**用 Console 端先验证接口与事件流设计是否正确，再迁移 Unity**（C 报告 ③ 3.3 第 3 步"跑通回归后整体迁 C#"）。

### 6.2 ConsoleRenderer 骨架

```cpp
// ConsoleRenderer.h —— 最小 Console 实现（约 100 行）
#include "IGameRenderer.h"
#include <iostream>

class ConsoleRenderer : public IGameRenderer {
public:
    void RenderScene(const SceneView& view) override {
        std::cout << "\n===== " << view.title << " =====\n";
        for (size_t i = 0; i < view.options.size(); ++i)
            std::cout << (i + 1) << ". " << view.options[i] << "\n";
    }

    void RenderCombat(const CombatView& view) override {
        std::cout << view.player.name << " HP:" << view.player.hp << "/" << view.player.hpMax
                  << "  vs  " << view.enemy.name;
        if (view.enemy.isBoss) std::cout << " [BOSS]";
        std::cout << " HP:" << view.enemy.hp << "/" << view.enemy.hpMax << "\n";
    }

    void RenderMessage(const std::string& text, MessageKind kind) override {
        // kind 决定颜色：Loot→绿、Error→红、Combat→白、System→灰
        std::cout << text << "\n";   // 颜色宏在此展开（ANSI 转义序列）
    }

    void PlayEffect(EffectType type, const EffectArgs& args) override {
        switch (type) {
            case EffectType::DamageFloat:  // 伤害飘字：红色 -N
                std::cout << "\x1b[31m-" << args.amount << "\x1b[0m ("
                          << args.target << ")\n"; break;
            case EffectType::HealFloat:    // 治疗飘字：绿色 +N
                std::cout << "\x1b[32m+" << args.amount << "\x1b[0m ("
                          << args.target << ")\n"; break;
            case EffectType::LevelUpBurst:
                std::cout << "\x1b[33m★ 升级！\x1b[0m\n"; break;
            default: break;                // 其余效果 Console 无表现
        }
    }

    void RenderStatus(const PlayerStatus& status) override {
        std::cout << "Lv." << status.level << " HP:" << status.hp << "/" << status.hpMax
                  << " 攻:" << status.atk << " 防:" << status.def
                  << " EXP:" << status.exp << "/" << status.expNeed << "\n";
    }

    void Flush() override {
        // 节奏控制单点：所有 delay 收敛于此（替代散落全库的 UIConfig::delay）
        UIConfig::delay(50);
    }
};
```

### 6.3 呈现方案对照

| 事件 | Console 呈现 | Unity 呈现（未来） |
|---|---|---|
| 伤害 | 红色飘字 + 战斗日志一行 | 血条插值 + UI 飘字 + 受击动画 |
| 治疗 | 绿色飘字 | 同左 + 治愈粒子 |
| 掉落 | 绿色日志 + 物品名 | 掉落闪光 + 物品图标入包动画 |
| 升级 | 黄色横幅 | 全屏爆发特效 + 音效 |
| 战斗日志 | 逐行滚动打印 | 滚动 Text 区（RichText 着色） |

### 6.4 落地步骤（小步替换，随时可回退）

1. **先接战斗事件**：`FightLogic` 抽成 `GameCore::resolveTurn()` 返回事件序列，`ConsoleRenderer` 消费——替换最核心的战斗 I/O（C 报告 ① 1.1 中最纠缠的部分）；
2. **再接输入**：`ConsoleInputSource` 替换 `checkNumberInput` 调用点（菜单/背包/装备/战斗行为菜单），顺带修复 P1-2 的非法输入问题；
3. **最后收口**：`delay` 全部收敛到 `Flush()`，`system("chcp")` 移入渲染器构造单点；
4. 每步跑通现有冒烟流程（`test_*.py` 恢复为可复现资产，呼应 C 报告 P1-3）后继续下一步。

---

## 7. 与 Unity 迁移衔接

### 7.1 对应 C 报告 ③ 3.2 的三处重构点

本文档的接口设计，逐一消化 C 报告"必须重构的部分"：

| C 报告 ③ 3.2 重构点 | 本文档的解法 |
|---|---|
| ① UI 与逻辑耦合（delay/cout 散落领域类） | 第 1 章边界 + 第 2 章 `IGameRenderer`：I/O 全部收敛到表现层，GameCore 纯逻辑 |
| ② 阻塞式输入模型（std::cin 轮询） | 第 4 章 `IInputSource`：请求-响应模式，Unity 事件队列实现不阻塞 |
| ③ 全局随机 gen | 沿用 C 报告 P2-1 的 IRandom 注入（本设计的事件序列在固定种子下可回放） |
| ④ 静态缓存单例 | 资源层（5.4）用实例化 Service + 依赖注入，与 ConfigLoader 迁移方案一致 |

### 7.2 组件迁移映射表

| 本设计组件（C++） | Console 实现 | Unity 实现（C#） |
|---|---|---|
| `GameCore`（纯逻辑） | 原样复用 | 原样移植/翻译（C 报告 ③ 3.3：同一份规则） |
| `IGameRenderer` | `ConsoleRenderer` | `UnityRenderer`：UI Canvas（场景/菜单）+ 战斗演出协程 + 飘字/粒子 + 滚动日志 |
| `IInputSource` | `ConsoleInputSource` | `UnityInputSource`：UI 事件 + Input 轮询 → `KeyCode`/`GameCommand` |
| `IResourceLoader` | 文本占位实现 | Unity Addressables/Resources：`manifest.json` → Sprite/Animator/ParticleSystem |

### 7.3 迁移顺序（呼应 C 报告 ③ 3.3 清单）

1. **前置**：落地 C 报告 P0-1/P0-2（配置健壮性 + 原子存档）、P2-2（抽 GameCore）——本接口依赖纯逻辑层先存在；
2. **Console 验证**：按第 6 章小步落地 `ConsoleRenderer`，跑通回归——**在控制台端验证接口设计**；
3. **同测迁移**：落地 P2-1/P2-3（确定性随机、单测），战斗事件序列与数值断言可复现；
4. **引擎替换**：只替换两个实现类（`UnityRenderer` + `UnityInputSource`），GameCore 与事件流原样带入——UI 层从"重写"变为"翻译"（C 报告 ③ 3.2-1 的迁移风险结论）；
5. **美术接入**：`assets/manifest.json` 从占位资源逐步替换为真实精灵/特效，缺省回退原则保证任何时刻可运行。

> 一句话核心设计：**GameCore 只产出"事件序列"这一种输出，渲染（IGameRenderer）、输入（IInputSource）、资源（IResourceLoader）三套抽象全部按"接口 + 双实现（Console/Unity）"组织，使图形化迁移从"重写"降为"替换两个实现类"。**
