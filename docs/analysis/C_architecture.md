# Dongeon V5.0 架构质量评估与分级升级建议（C 报告）

> 分析范围：include/ + src/ 全部源码（约 1860 行）、config/*.json、CMakeLists.txt、docs/。
> 结论均指向具体 文件:行号。区分「作者设计意图」（30 怪/7 区域/二段曲线/Boss 等为刻意设计）与「可改进点」。

---

## ① 架构评估结论

### 1.1 规模与分层

- 总代码量约 1860 行：dongeon.cpp 116 行、src/ 约 1230 行、include/ 约 520 行；**无任何测试代码、无测试目录、无 CI**。
- 名义分层（docs/ARCHITECTURE.md:7-30）：dongeon.cpp（主循环+菜单状态机）→ Battle/FightLogic（战斗）→ Player（Player/PlayerBag/PlayerEquipment）→ Item（抽象基类+Medicine/Equipment 多态，unique_ptr 所有权）→ Monster → Config（ConfigLoader 静态缓存+工厂 / saveManager / UIConfig+GameUIConfig）。
- **头文件层无循环依赖**：Equipment.h 通过前向声明 Player/PlayerBag 切断了 Equipment.h→Player.h→PlayerEquipment.h→Equipment.h 的环（Equipment.h:12-15 的 [FIX] 注释），这是作者已修复过的成果，方向正确。
- 无上帝模块：最接近的是 Player（属性+背包+装备+序列化+UI 显示集于一身的门面类），但仅 166 行（Player.cpp），未失控。

### 1.2 依赖方向实况（文本依赖图）

```
dongeon.cpp ──→ Player / Monster / Medicine / Config / UIConfig / ConfigLoader / GameUIConfig / saveManager / FightLogic
  │
  ├─ FightLogic.cpp ──→ Player, Monster, Medicine, UIConfig, Config(常量+Random)
  ├─ Player.h ──→ Config.h(常量+Random+json.hpp), PlayerBag.h, PlayerEquipment.h
  │     ├─ PlayerBag.h ──→ Item.h / Equipment.h / Medicine.h
  │     ├─ PlayerEquipment.h ──→ Item.h / Equipment.h
  │     └─ Player.cpp ──→ UIConfig(UI), ConfigLoader(数据)      ← 逻辑层直接使用 UI 与数据层
  ├─ Monster.cpp ──→ UIConfig, ConfigLoader, PlayerBag, Medicine ← 同上
  ├─ ConfigLoader.h ──→ Monster.h, Item.h                        ← "数据层"依赖领域模型
  ├─ saveManager.h ──→ Player.h
  ├─ GameUIConfig.h ──→ Player.h
  └─ Item.h ──→ json.hpp（三方，经 Config.h 被全库传递包含）
```

关键结论（与作者自述的偏差）：

1. **「UI→逻辑→数据」的分层图是理想态，实为扁平服务混合**：领域类直接 include UI 层做 `UIConfig::delay()` 节奏控制（Player.cpp:9、Monster.cpp:8、PlayerEquipment.cpp:5），Player.cpp:8 又直接 include 数据层 ConfigLoader 在 reset() 里加载初始物品。UI / 逻辑 / 数据三者互相可达——规模小可接受，但对 Unity 迁移是主要障碍（见 ③）。
2. **`UIConfig::delay` 是横切关注点**：散落于 Player.cpp:14,21,27,44,53,66 / Monster.cpp:21,24,26 / Medicine.cpp:23,25 / PlayerEquipment.cpp:52,56,69,74 / FightLogic.cpp 多处——领域行为与 UI 节奏强耦合。
3. **Config.h 是杂物包**：常量 + `extern std::mt19937 gen` 全局随机 + Random 命名空间混在一处（Config.h:19-41），且 Player.h:7 传递包含 json.hpp，使三方头成为事实上的全局依赖。

### 1.3 命名与一致性

- 拼写错误：`Equipslot`（PlayerEquipment.h:16，GameUIConfig.cpp:12,13,41,47,67，dongeon.cpp:92）应为 equipSlot；`changeEuipType`（PlayerEquipment.h:27,41）与 `changeEuip`（PlayerEquipment.h:32,78）应为 changeEquipType/changeEquip；`idMappiing`（GameUIConfig.cpp:27,56,66）。
- 双签名：`getEquipIndex(EquipmentType)` / `getEquipIndex(std::string)`（PlayerEquipment.h:23-24），string 版失败返回 -1（注释"异常处理还没做"，PlayerEquipment.cpp:38）。
- 三重载 `showItem`（PlayerBag.h:23-28）语义不统一：`showItem(ItemType)` / `showItem(EquipmentType)` 是"打印+返回可见映射"，无参版仅打印；且 **`showItem(ItemType::Equipment)` 永远匹配不到任何物品**——Equipment::getType() 返回 "armor"/"weapon"（Item.cpp:31），而枚举字符串是 "Equipment"（Item.cpp:47）——静默失效的陷阱重载。
- 双签名 `useItem`（PlayerBag.h:19-20）：带映射版有越界检查（PlayerBag.cpp:26-33），裸索引版直接 `bag[index]->use(player)`（PlayerBag.cpp:35-37）无任何保护。
- 枚举字符串大小写不对称：`enumToString(ItemType)` 返回 "Medicine"/"Equipment"（Item.cpp:47-48），`stringToItEnum` 只接受小写（Item.cpp:62-63），序列化用全小写（Medicine.cpp:42、Equipment.cpp:27）——靠约定维持一致，是潜在静默 bug 源。
- `stringToItEnum` 抛错文案错误："Unknown equipment type" 应为 item type（Item.cpp:64）。
- `getType()` 按值返回 `const std::string`（Item.h:32），应为 const 引用。
- Medicine.cpp 全文多缩进 4 空格（Medicine.cpp:1 起），与全库不一致。
- 版本号失同步：CMakeLists.txt:2 `project(Dongeon VERSION 4.1)`，README/docs 已是 V5.0；PlayerBag.h:3 残留过时注释"//未适配CMake"。
- 数据隐藏：Item 的 `ID/name`（Item.h:23-24）、Monster 全部字段（Monster.h:19-25）为 public 可变状态。

### 1.4 可测试性（现状：不可测）

- 仓库内无测试代码；.gitignore:16-17 显示本地存在 `test_*.py` 冒烟脚本但**被排除分发**（本工作副本亦不存在）——测试资产"用过即弃"，DEVELOPMENT_JOURNEY.md:47 提到的"Python 自动化冒烟测试"没有成为可复现资产。
- 三大障碍（代码证据）：
  1. **全局随机**：`extern std::mt19937 gen`（Config.h:19）由 Config.cpp:5 用 steady_clock 播种，Random::range 直接消费——无法注入种子/替换 RNG，战斗与掉落不可确定性复现；
  2. **阻塞输入**：`UIConfig::checkNumberInput` 直读 `std::cin`（UIConfig.cpp:13），无输入流注入点，菜单/战斗流程无法脚本化驱动；
  3. **进程副作用 + 静态状态**：`system("chcp 65001 > nul")`（dongeon.cpp:16）；ConfigLoader 静态缓存 `s_monstersLoaded/s_itemsLoaded`（ConfigLoader.h:23-38）无重置钩子；存档路径硬编码相对路径（saveManager.h:12）——测试间无法隔离。
- 遗留"测试专用"接口：`removeByID/addItemByID`（PlayerBag.h:50-51），如今无调用方。

### 1.5 遗留 / 死代码（grep 证实无调用方）

- `PlayerBag::removeByID`（PlayerBag.cpp:141）、`PlayerBag::addItemByID`（PlayerBag.cpp:159）、`Equipment::dropEquipment`（Equipment.cpp:14，唯一调用者即 removeByID）、`Player::showBag`（Player.cpp:25）、`Medicine::showMedicine`（Medicine.cpp:30）。
- 静默失效重载：`showItem(ItemType::Equipment)`（PlayerBag.cpp:39）。
- 注释掉的旧代码块：Config.h:14-16（RNG v1）、Item.h:43、Medicine.h:40、Equipment.h:59（旧虚函数 fromJson）、Config.cpp:4、Medicine.cpp:48、dongeon.cpp:86（调试打印）。

### 1.6 数据驱动完整性（程序化校验结果）

- item.json：27 个模板、**无重复 id**；monster.json：30 怪（23 普通 + 7 Boss）；**全部 drops.itemId 均存在于 item.json ✓**；Boss 数量 7 ✓；普通怪等级窗口 [minLevel, maxLevel+3] 连通无断档 ✓。
- **当前数据是干净的，但健壮性完全依赖数据恰好正确**，加载器只校验"字段存在"不校验"形状"：
  - `data["monsters"]` 键缺失（ConfigLoader.cpp:33）、`item["atk"][0]` 形状不符（ConfigLoader.cpp:50-51,123）→ 抛未捕获 nlohmann 异常 → **启动即崩**；
  - `type_2` 拼错 → `stringToEqEnum` 抛异常（Item.cpp:67-70）→ 同样崩溃；
  - 运行时引用不存在 itemId → createItemById 打警告返回 nullptr（ConfigLoader.cpp:150），dropItem 静默无掉落（Monster.cpp:50-51）——体验问题。
- 其他数据层问题：模板 id 重复时后者静默覆盖（ConfigLoader.cpp:107,127）；缓存不按路径区分（第二次 loadMonsters 换路径仍返回旧缓存，ConfigLoader.cpp:18-21）；createItemById 依赖"先调用过 loadInitialItems"的时序约定（ConfigLoader.h:18 注释自认）。

### 1.7 已识别的逻辑健壮性问题

- saveGame 直接 ofstream 覆写单槽存档（saveManager.cpp:31-39），写一半断电/崩溃 → 存档损坏 = 全部进度丢失（docs/ARCHITECTURE.md:142 自认"战斗中途退出会丢进度"）。
- 存档无版本号、无结构校验（Player::fromJson 全部 j.value 兜底，Player.cpp:143-153），格式演进无法迁移。
- `PlayerBag::equipItem` 先 `bag.erase` 再校验 changeEuipType 返回值（PlayerBag.cpp:127-133）：dynamic_cast 失败时物品已被 erase+release → **静默丢失**（当前调用链 Equipment::use 保证是装备，不可达，但违反防御性编程）。
- `wear` 槽位被占时自动转 `changeEuip` 再次弹确认（PlayerEquipment.cpp:57-60），GameUIConfig case 3 也走 changeEuip——"二次确认"UI 流程重叠。
- `expEnough` 满级后经验永久累积不消耗（Player.cpp:85）——数值展示问题。
- 读档不校验 currentHp ≤ hp_UpperLimit（Player.cpp:143-144）——篡改档可产生"满血不死/无法回血"状态。

---

## ② 升级建议清单

### P0 必修（崩溃 / 数据丢失 / 存档损坏）

**【P0-1】配置解析整体缺异常防护与形状校验 → 坏配置即崩溃**
- 【位置】src/Config/ConfigLoader.cpp:33,50-51,123；src/Item/Item.cpp:67-70；入口 dongeon.cpp:19
- 【问题】任何"内容正确但形状错误"的配置（缺 monsters 键、atk 非二元数组、type_2 拼错、负权重）都会抛未捕获异常直接崩溃；而本项目的核心卖点恰是"改配置即改游戏"——数据防线缺失等于把根基暴露给手滑。
- 【改动思路】loadMonsters/loadInitialItems 入口包 try/catch（捕获 nlohmann::exception 与 std::exception，打印错误并跳过该条/返回空）；对 atk 形状、type_2 枚举、weight>0 做显式校验；stringToItEnum/stringToEqEnum 由抛异常改为"失败返回哨兵 + 调用方告警跳过"。与 P1-3 静态校验脚本双保险。
- 【工作量】中

**【P0-2】存档单文件非原子写入 → 崩溃即全量丢失**
- 【位置】src/Config/saveManager.cpp:31-39（ofstream 直接覆写）；dongeon.cpp:104-107（仅手动保存）
- 【问题】写文件中途进程被杀/断电 → savegame.json 半截 → loadGame 解析失败（saveManager.cpp:53-59 兜底为"开始新游戏"），玩家全部进度丢失；且单槽覆盖，误保存即覆盖旧档。
- 【改动思路】写 `savegame.json.tmp` → close → `std::rename` 原子替换；保留 `.bak` 上一版；保存后回读校验。多槽位/自动存档见 P1-1。
- 【工作量】小

**【P0-3】背包索引族 API 缺统一越界/类型防护 → 公共接口可致 UB 或丢物品**
- 【位置】src/Player/PlayerBag.cpp:35-37（useItem 裸索引直接 bag[index]）、:124-138（equipItem 先 erase 后校验 dynamic_cast）
- 【问题】裸索引版越界即 UB（当前唯一调用点 dongeon.cpp:85 由调用方保证边界，属"运气好"）；equipItem 在 changeEuipType 可能返回 nullptr 时物品已被 erase+release → 静默丢失。未来任何新调用点都可能踩中。
- 【改动思路】统一入口：删除裸索引版或加 `if (index >= bag.size()) return;`；equipItem 先 dynamic_cast 成功再 erase；行为与带映射版（PlayerBag.cpp:26-33）对齐。
- 【工作量】小

> 优先级：P0-1 > P0-2 > P0-3。前两条都是"数据出问题 = 游戏不可用"，对数据驱动项目是根基性风险。

### P1 推荐（体验改善 / 平衡工具化）

**【P1-1】存档多槽位 + 自动存档 + 版本号字段**
- 【位置】src/Config/saveManager.h:12-15；dongeon.cpp:104-107
- 【问题/机会】docs/ARCHITECTURE.md:142 自认"战斗中途退出会丢进度"；单槽覆盖 + 无版本字段，未来格式演进无法兼容。
- 【改动思路】save 目录按槽位 `save/slot1.json...`，加载菜单列出槽位；在"战斗胜利/升级/进地牢前/回城"里程碑自动保存；存档顶层加 `"version": 1`，加载按版本迁移或明确提示。
- 【工作量】中

**【P1-2】输入体验与反馈改进**
- 【位置】src/Config/UIConfig.cpp:8-24（checkNumberInput）
- 【问题】非法输入累计 50 次后**静默返回下限值 f**（UIConfig.cpp:15-18），玩家可能在不知情下误操作（如战斗误选"攻击"）；无剩余次数提示；越界输入无范围提示。
- 【改动思路】超限改为报错并回到当前菜单（不返回魔法值）；提示剩余次数与合法范围；可选"回车跳过 delay 动画"快捷键（delay 散落全库，见 ① 1.2-2）。
- 【工作量】小

**【P1-3】平衡工具化：配置校验脚本 + 数值模拟器入库**
- 【位置】仓库级（新增 tools/）；参照 .gitignore:16-17（test_*.py 曾被使用但被排除分发）
- 【问题/机会】本次校验显示数据恰好干净（无重复 id、掉落引用完整、窗口连通），但**没有任何自动化防线**——未来任何一次手改配置都可能引入断档/幽灵掉落/启动崩溃（P0-1）。DEVELOPMENT_JOURNEY.md:38,47 中"冒烟测试反推数值"的脚本未版本化，不可复现。
- 【改动思路】① `tools/validate_config.py`：字段存在性/类型/形状、drops.itemId 引用完整性、权重>0、isBoss 数量、等级窗口连通、重复 id——接入 CI 或 pre-commit；② `tools/simulate.py`：蒙特卡洛模拟（指定等级/装备下区域胜率、掉落期望、Boss 三连抽 70% 权重必掉率 97.3% 复算、药水消耗），把"数值靠实测反推"变成可重复流程。
- 【工作量】中

**【P1-4】战斗节奏改善：自动战斗/跳过动画**
- 【位置】src/Battle/FightLogic.cpp:21-22（战斗行为菜单）；:104（unlimitedFight 无限刷怪循环）
- 【问题/机会】回合制 + 全库 delay，重复刷怪节奏偏慢；P1-3 模拟器也需要"无 UI 批量模拟"能力。
- 【改动思路】行为菜单增加"自动战斗（直到死亡/逃跑/背包空）"；最优路径是先抽纯逻辑层（P2-2）再挂自动策略。
- 【工作量】小-中

**【P1-5】穿戴/升级的数值反馈可见化**
- 【位置】src/Player/PlayerEquipment.cpp:102-112（playerGain/takeOffGain 无输出）；src/Player/Player.cpp:50-81（levelUp 无属性明细）
- 【问题】换装/升级后玩家看不到攻防具体变化，"构筑驱动"的反馈闭环断了一环。
- 【改动思路】playerGain/takeOffGain 打印"攻击 +3/+5、防御 +2"；levelUp 打印各属性增量（含二段曲线 extra 部分）。
- 【工作量】小

**【P1-6】命名与一致性清理（迁移前必做，见 ③）**
- 【位置】include/Player/PlayerEquipment.h:16,27,32；src/Config/GameUIConfig.cpp:27,56,66；src/Item/Item.cpp:45-70；include/Item/Item.h:32；src/Item/Medicine.cpp 全文；CMakeLists.txt:2
- 【问题】Equipslot/changeEuipType/changeEuip/idMappiing 拼写错误；enumToString 与 stringToItEnum 大小写不对称（潜在静默 bug 源）；getType 按值返回；stringToItEnum 报错文案错误；Medicine.cpp 缩进不一致；CMake 版本号与 V5.0 失同步。
- 【改动思路】一次性全局重命名（ripgrep 批量替换）+ 枚举字符串统一小写规范；getType 改 const 引用；修正文案与版本号。建议排在 P2-2 纯逻辑层抽取之后（避免搬移时重命名冲突）。
- 【工作量】中

### P2 可选（架构 / 工程化 / Unity 迁移铺垫）

**【P2-1】可测试性改造：确定性随机 + 可注入输入 + 缓存可重置**
- 【位置】include/Config/Config.h:19-24（全局 gen）；src/Config/UIConfig.cpp:8-24（std::cin 直读）；src/Config/ConfigLoader.h:23-38（静态缓存）；dongeon.cpp:16（system("chcp")）
- 【问题/机会】三处障碍使 ① 1.4 的不可测成为结构性事实；不解决则 ② P2-3 无地基。
- 【改动思路】Random 改可注入（`class IRandom { virtual int range(int,int)=0; }` + 默认实现 + 测试用固定种子实现）；checkNumberInput 接收 `std::istream&`（默认 std::cin）；ConfigLoader 增加 resetForTests() 与按路径缓存；system("chcp") 移入平台封装单点。
- 【工作量】中-大

**【P2-2】纯逻辑层抽取（Unity 迁移的决定性前置，见 ③ 3.2）**
- 【位置】src/Battle/FightLogic.cpp、src/Player/Player.cpp、src/Monster/Monster.cpp（UI 耦合点：delay/std::cout）
- 【问题/机会】领域逻辑与控制台 I/O 纠缠，Unity 端无法复用；先抽逻辑、UI 变薄壳，控制台与引擎共用同一份规则，迁移风险从"重写"降为"翻译"。
- 【改动思路】抽出平台无关 GameCore：回合结算、伤害公式、掉落 roll、升级曲线、怪物筛选——纯函数/纯对象，无 std::cout/delay/输入；现有 UI 只做"渲染 + 读输入 → 调 GameCore → 渲染结果"。
- 【工作量】大

**【P2-3】单元测试框架**
- 【位置】仓库级（tests/ + CMakeLists.txt enable_testing）
- 【前提】P2-1 完成。
- 【改动思路】GoogleTest + CTest；首批用例：成长曲线公式（Player.cpp:50-81）、权重掉落（Monster.cpp:33-63，固定种子）、存档 roundtrip（toJson→fromJson 一致性）、背包堆叠/映射（PlayerBag.cpp:10-24）、Boss 三连抽必掉率统计。
- 【工作量】中

**【P2-4】存档格式引用化 + 版本迁移**
- 【位置】src/Item/Item.cpp:14-43（fromJson 全量快照）；src/Player/Player.cpp:113-136（toJson）
- 【问题】当前存档内嵌物品完整属性（atk/def/restore），配置演进（V5.1 调数值）后旧档物品与配置脱节形成"幽灵装备"；且存档自洽导致加载不校验配置引用。
- 【改动思路】存档只存 `{id, quantity, slot}`，加载时经 ConfigLoader::createItemById 还原属性（顺带获得配置引用校验）；配 version 字段与迁移钩子。
- 【工作量】中

**【P2-5】死代码与失效重载清理**
- 【位置】src/Player/PlayerBag.cpp:141-168（removeByID/addItemByID）；src/Item/Equipment.cpp:14-16（dropEquipment）；src/Player/Player.cpp:25-29（showBag）；src/Item/Medicine.cpp:30-32（showMedicine）；src/Player/PlayerBag.cpp:39-55（showItem(ItemType::Equipment) 永不匹配）
- 【问题/机会】5 个无调用方函数 + 1 个静默失效重载，增加阅读负担与误用面；① 1.5 注释块一并清理。
- 【改动思路】删除无调用方函数；showItem(ItemType) 重载改为按 itype 真实语义或删除，并在接口注释说明"装备请用 showItem(EquipmentType)"。
- 【工作量】小

**【P2-6】Player 上帝类拆分 / Config.h 解耦**
- 【位置】include/Player/Player.h:12-46；include/Config/Config.h:19-41
- 【问题/机会】Player 集属性/背包/装备/序列化/UI 于一身；Config.h 常量与 Random 混杂且传递包含 json.hpp。
- 【改动思路】Player 拆为 Stats（属性/等级/成长）+ Inventory 门面组合；常量与 Random 分离（constants.h / rng.h）。与 P2-2 重叠度高，可合并执行。
- 【工作量】中

> 共 15 条：P0×3、P1×6、P2×6，覆盖三个级别。

---

## ③ Unity 迁移衔接建议

### 3.1 值得保留的架构资产（直接迁移 / 低改造成本）

| 资产 | 现状 | 迁移方式 |
|---|---|---|
| 数据驱动 JSON（config/*.json） | 27 物品模板 / 30 怪+掉落表，已校验干净（① 1.6） | 原样保留，Unity 侧用 ScriptableObject 生成器或 JSON 加载 |
| 权重池掉落数学 | Monster::dropItem（Monster.cpp:33-63） | 抄成 C# 纯函数，Boss 三连 roll 照搬 |
| 物品多态 + 每类自管序列化 | Item→Medicine/Equipment；toJson/fromJson 静态工厂分发（Item.cpp:14-43） | C# abstract Item + JSON.NET；"静态工厂按类型字段分发"思想直接保留 |
| Boss 机制 | isBoss bool + 10% 遭遇 + 专属掉落（FightLogic.cpp:116-118） | 保留为数据字段 + 遭遇概率常量 |
| 二段成长曲线 | Player::levelUp（Player.cpp:50-81） | 纯公式，原样抄 C#，保留"厚积薄发"曲线参数 |
| 等级窗口筛选 lambda | unlimitedFight（FightLogic.cpp:108-111） | C# predicate，含"+3 容差窗口"规则 |
| 背包"显示/使用分离"映射模式 | showItem→映射数组→useItem（PlayerBag.cpp:39-73） | 模式本身正确，UI 层适配 |
| 装备栏与枚举槽位绑定 | array<unique_ptr<Equipment>, count>（PlayerEquipment.h:16） | C# 定长数组/List + enum 槽位 |

### 3.2 必须重构的部分（现状会成为迁移的坑）

1. **UI 与逻辑耦合**（① 1.2-2）：领域类里全是 std::cout / UIConfig::delay——Unity 渲染与节奏完全两套，迁移前必须先抽纯逻辑层（P2-2），否则等于重写。**最高优先级迁移前置项**。
2. **阻塞式输入模型**（UIConfig.cpp:13）：std::cin 轮询在 Unity 事件驱动模型里不可用——输入必须抽象为与逻辑层解耦（请求输入 → 回调/事件）。
3. **全局随机 gen**（Config.h:19）：Unity 端需要种子可复现以复刻数值、复现 bug——先做 P2-1 的 IRandom 注入。
4. **静态缓存单例**（ConfigLoader.h:23-38）：在 Unity 场景/生命周期管理下易出状态残留——迁移为实例化 Service / 依赖注入。
5. **unique_ptr 所有权纪律 → C# 引用语义**：C# 无 move 语义，迁移时靠代码纪律（装备转移明确"从容器 A 移出、加入 B"）防 GC 悬空引用；建议纯逻辑层就约束为"值语义装备结构体（id+槽位）"而非对象图。
6. **命名与枚举字符串不对称**（① 1.3）：迁移到 C# 前先清理（P1-6），否则把拼写错误与大小写约定带进新代码库。
7. **存档格式**（P2-4）：全量快照 + 无版本——Unity 端存档体系必须从第一天就有 version + 引用式条目，否则 2.5D 版本演进后旧档全废。

### 3.3 迁移前准备清单（建议顺序）

1. 落地 P0-1 / P0-2：配置健壮性 + 原子存档——把"数据即内容"的根基打牢；
2. 落地 P1-3：配置校验 + 数值模拟器——在控制台端把数值与掉落概率固化为可复算资产，Unity 直接引用同一份数值；
3. 落地 P2-2 + P1-6：抽纯逻辑层、命名清理——让控制台 UI 变成 GameCore 的薄壳，跑通回归后整体迁 C#；
4. 落地 P2-1 + P2-3：可注入随机/输入/缓存、单测——迁移期用同一组单测在 C++ 侧验证，C# 移植后对照；
5. 最后才是引擎侧：JSON → ScriptableObject 生成器、战斗表现层、搜打撤地图与小队系统（架构资产之外的新系统）。

---

## ④ 一句话总体判断

**Dongeon V5.0 是一份"设计思想优秀但工程质量尚未配套"的可迁移架构资产**——数据驱动、多态物品、unique_ptr 所有权、Boss 最小侵入式扩展等设计决策值得原样带进 Unity；当前真正的风险不在架构方向，而在"数据无防线、存档会丢、逻辑不可测"三处工程短板，恰好是迁移前最该补齐的部分。
