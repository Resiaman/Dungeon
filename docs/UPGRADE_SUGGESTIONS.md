# Dongeon 修改升级建议文档

> 生成日期：2026-08 ｜ 分析基线：Dongeon V5.0（commit bc46fe6 / 414625a）
> 分析方法：组长通读全部源码（约 1800 行）+ 3 路并行专项分析（数值平衡 A / Bug 复核 B / 架构升级 C）交叉验证
> 子报告：`docs/analysis/A_balance.md`、`docs/analysis/B_bugs.md`、`docs/analysis/C_architecture.md`、`docs/analysis/D_render_api.md`（附可复现证据脚本 `simulate_balance.py`、`smoke_test_console.py`）

## 〇、总体结论（TL;DR）

**Dongeon V5.0 是一份"设计思想优秀但工程质量尚未配套"的可迁移架构资产**——数据驱动、多态物品、unique_ptr 所有权、Boss 最小侵入式扩展等设计决策值得原样带进 Unity（2.5D 搜打撤 + 基地建设 + 小队战术）；当前真正的风险不在架构方向，而在三处工程短板：

1. **数据无防线**：坏配置（形状错误/拼错）→ 启动即崩（C P0-1）
2. **存档会丢**：单文件非原子写入，写一半断电 = 全部进度丢失（C P0-2）
3. **最终 Boss 数值断崖**：深渊之王解锁等级 48 时真实装备胜率 0%（A-2），叠加"无自动存档"的死亡全丢惩罚，体验挫败感最强

**最高优先级的 5 件事**（按顺序）：
| # | 事项 | 来源 | 工作量 |
|---|---|---|---|
| 1 | 配置解析异常防护 + 形状校验 | C P0-1 | 中 |
| 2 | 存档原子写入（.tmp + rename + .bak） | C P0-2 | 小 |
| 3 | 背包索引族 API 防护（先校验后 move） | C P0-3 / B D-1 | 小 |
| 4 | 最终 Boss 数值下调 15~20% 或加等级建议提示 | A-2 | 小 |
| 5 | 存档多槽位 + 自动存档 + 版本号 | C P1-1 | 中 |

---

## 一、项目现状速览

| 维度 | 现状 |
|---|---|
| 版本 | V5.0（数值体系重构 + Boss 系统） |
| 规模 | 源码约 1800 行（14 cpp + 13 h，不含第三方 json.hpp） |
| 内容 | 30 怪物（23 普通 + 7 Boss）、27 物品模板（6 药水 + 21 装备） |
| 工程 | git + CMake（MinGW/Ninja）就绪，构建可过 |
| 测试 | 无自动化单元测试；冒烟测试脚本已补入 docs/analysis/ |
| 已知修复 | B1 怪物等级过滤、B2 等级上限（commit cdd2941）——复核均 ✅ 有效 |

---

## 二、三大专项分析摘要

### 2.1 数值平衡分析（子任务 A → A_balance.md）

- **升级曲线合理不肝**：0→60 累计 25,473 exp，约 330 只怪满级；二段曲线"前快后慢"数值成立
- **等级窗口无断档**：0~60 级每级 3~8 普通怪 + 0~2 Boss，过滤规则（[minLevel, maxLevel+3]）连通 ✓
- **⚠️ 普通怪全段无挑战**：10/10 场景纯攻击胜率 100%，期望回合仅 2.4~6.4——"构筑驱动"理念的落差
- **⚠️ 最终 Boss 数值断崖**：深渊之王 Lv48 解锁，真实装备胜率 **0%**、毕业装 17.9%，需刷到 Lv55 才稳过
- **⚠️ 新手期过度保护**：Lv0 第一战受伤仅 2.8~8.6/25，V5.0 STARTHP 20→25 修正过头
- 掉落设计自洽 ✓：普通怪 85/8/7 权重（0.15 件装备/只），Boss 3 连 roll 专属武器必掉率 97.3%（复核无误）

### 2.2 Bug 复核与代码质量（子任务 B → B_bugs.md）

- **13 项作者声明修复复核：10 项有效 ✅、2 项部分有效 ⚠️、1 项带体验残留**
- ⚠️ 部分有效：`equipItem` 先 erase 后校验 → 物品静默丢失竞态（D-1）；ConfigLoader 只校验存在性不校验形状 → 坏配置崩溃（D-13 = C P0-1）
- 新发现缺陷 11 项：高 2（装备丢失 / 存档非原子）、中 4、低 5

### 2.3 架构评估与升级路线（子任务 C → C_architecture.md）

- 名义分层实为"扁平服务混合"：领域类直接 include UI 层（delay）与数据层（ConfigLoader），对 Unity 迁移是主要障碍
- 无测试代码：全局随机 gen、阻塞 std::cin、静态缓存不可重置 → 结构性不可测
- 命名一致性：Equipslot/changeEuipType/idMappiing 等拼写错误 + 枚举字符串大小写不对称（潜在静默 bug）
- 共 15 条建议：P0×3、P1×6、P2×6，迁移路线 = 先打牢数据/存档根基 → 数值工具化 → 抽纯逻辑层 → 可测性 → 引擎侧

---

## 三、合并后的升级建议清单（组长汇总）

> 合并去重 A/B/C 三报告，按"影响 × 成本"排优先。每条含来源（报告编号）便于回溯。

### P0 必修（崩溃 / 数据丢失 / 存档损坏）

**【P0-1】配置解析整体缺异常防护与形状校验 → 坏配置即崩溃** [C P0-1 / B D-13]
- 位置：src/Config/ConfigLoader.cpp:33,50-51,123；src/Item/Item.cpp:67-70
- 改动：loadMonsters/loadInitialItems 入口 try/catch（nlohmann::exception + std::exception）；atk 形状、type_2 枚举、weight>0 显式校验；stringToItEnum/stringToEqEnum 改"失败返回哨兵 + 告警"
- 工作量：中

**【P0-2】存档单文件非原子写入 → 崩溃即全量丢失** [C P0-2 / B D-2]
- 位置：src/Config/saveManager.cpp:31-39
- 改动：写 `savegame.json.tmp` → close → `std::rename` 原子替换；保留 `.bak`；保存后回读校验
- 工作量：小

**【P0-3】背包索引族 API 缺统一越界/类型防护 → 公共接口可致 UB 或丢物品** [C P0-3 / B D-1]
- 位置：src/Player/PlayerBag.cpp:35-37（useItem 裸索引）、:124-138（equipItem 先 erase 后校验）
- 改动：裸索引版加 `if (index >= bag.size()) return;`；equipItem 先 dynamic_cast 成功再 move/erase
- 工作量：小

### P1 推荐（体验 / 平衡 / 健壮性）

**【P1-1】存档多槽位 + 自动存档 + 版本号字段** [C P1-1]
- 位置：src/Config/saveManager.h；dongeon.cpp:104-107
- 改动：`save/slot1.json...` 多槽位；战斗胜利/升级/进地牢前/回城里程碑自动保存；顶层 `"version": 1` + 迁移钩子
- 工作量：中

**【P1-2】最终 Boss 数值断崖修正** [A-2]
- 位置：config/monster.json（深渊之王 hp 1600 / atk 85~105）
- 改动：数值下调 15~20%（hp→1300~1350、atk→72~92），使 Lv48~50 毕业装胜率回到 25~40%；或在遭遇提示加等级建议
- 依据：Lv48 真实装胜率 0%、毕业装 17.9% → 需刷到 55 级，且叠加死亡全丢惩罚
- 工作量：小

**【P1-3】普通怪难度梯度与战斗节奏** [A-1 / C P1-4]
- 位置：config/monster.json + src/Battle/FightLogic.cpp
- 改动：① 每区域加 1~2 个"精英怪"（同区 ×1.5~2，掉率高）作梯度；② 行为菜单加"自动战斗"；③ 可选"回车跳过 delay"
- 工作量：小-中

**【P1-4】平衡工具化：配置校验脚本 + 数值模拟器入库** [C P1-3，证据已生成]
- 位置：仓库级（新增 tools/）
- 改动：① `tools/validate_config.py`（引用完整性/形状/窗口连通/重复 id，接入 CI 或 pre-commit）；② `tools/simulate.py`（蒙特卡洛胜率/掉落期望/药水经济，本分析脚本 `docs/analysis/simulate_balance.py` 可直接迁移）
- 工作量：中

**【P1-5】输入体验与反馈改进** [C P1-2 / B D-5]
- 位置：src/Config/UIConfig.cpp:8-24
- 改动：超限改报错回菜单（不返回魔法值 f）；提示剩余次数与合法范围
- 工作量：小

**【P1-6】穿戴/升级的数值反馈可见化** [C P1-5]
- 位置：src/Player/PlayerEquipment.cpp:102-112；src/Player/Player.cpp:50-81
- 改动：playerGain/takeOffGain 打印"攻击 +x/+y、防御 +z"；levelUp 打印属性增量
- 工作量：小

**【P1-7】命名与一致性清理（迁移前必做）** [C P1-6 / B D-8/D-9/D-11]
- 位置：PlayerEquipment.h:16,27,32；GameUIConfig.cpp:27,56,66；Item.cpp:45-70；Medicine.cpp 全文；CMakeLists.txt:2
- 改动：Equipslot/changeEuipType/idMappiing 全局重命名；枚举字符串统一小写；getType 改 const 引用；修正文案与版本号
- 工作量：中

**【P1-8】满级经验与空池提示修正** [B D-3/D-4]
- 位置：src/Player/Player.cpp:82-89；src/Battle/FightLogic.cpp:126
- 改动：满级 exp 封顶或显示 MAX；空池提示区分"无普通怪但可能遇 Boss / 无怪请升级"
- 工作量：小

### P2 可选（架构 / 工程化 / 面向 Unity 迁移）

**【P2-1】可测试性改造：确定性随机 + 可注入输入 + 缓存可重置** [C P2-1]
- 位置：Config.h:19-24；UIConfig.cpp:8-24；ConfigLoader.h:23-38；dongeon.cpp:16
- 改动：`class IRandom` 注入；checkNumberInput 接收 `std::istream&`；resetForTests()；chcp 移入平台封装单点
- 工作量：中-大

**【P2-2】纯逻辑层抽取（Unity 迁移的决定性前置）** [C P2-2]
- 位置：FightLogic.cpp / Player.cpp / Monster.cpp（UI 耦合点）
- 改动：抽出平台无关 GameCore（回合结算/伤害公式/掉落 roll/升级曲线/怪物筛选），UI 变薄壳
- 工作量：大

**【P2-3】单元测试框架** [C P2-3，前提 P2-1]
- 位置：tests/ + CMakeLists.txt enable_testing
- 改动：GoogleTest + CTest；首批：成长曲线、权重掉落（固定种子）、存档 roundtrip、背包堆叠、Boss 必掉率统计
- 工作量：中

**【P2-4】存档格式引用化 + 版本迁移** [C P2-4 / B D-6]
- 位置：Item.cpp:14-43；Player.cpp:113-136
- 改动：存档只存 `{id, quantity, slot}`，加载经 createItemById 还原；读档 clamp 数值完整性；version 字段 + 迁移钩子
- 工作量：中

**【P2-5】死代码与失效重载清理** [C P2-5 / B D-7/D-10]
- 位置：PlayerBag.cpp:141-168、:39-55；Equipment.cpp:14-16；Player.cpp:25-29；Medicine.cpp:30-32
- 改动：删除 5 个无调用方函数；showItem(ItemType::Equipment) 按真实语义修正或删除；清理注释旧代码块
- 工作量：小

**【P2-6】Player 上帝类拆分 / Config.h 解耦** [C P2-6]
- 位置：Player.h:12-46；Config.h:19-41
- 改动：Player 拆 Stats + Inventory 门面；常量与 Random 分离（constants.h / rng.h）；与 P2-2 可合并执行
- 工作量：中

**【P2-7】新手期难度回调（可选微调）** [A-3]
- 位置：Config.h:30（STARTHP 25）
- 改动：STARTHP 回调至 22~23 或区域 1 怪物伤害 +1~2，保留"打得过但会掉血"的紧张感
- 工作量：小

> 合并后共 **21 条**：P0×3、P1×8、P2×7（A 贡献 3 条、B 贡献 2 条、C 贡献 16 条，去重合并）。

---

## 四、验证证据

- 构建：`cmake --build build` → ✅ 成功（exit 0，2026-08-14 实测）
- 冒烟测试：`python docs/analysis/smoke_test_console.py` → ✅ **9/9 PASS**（脚本已修复：cwd 需指向项目根而非 build/，因游戏用相对路径读 config/）
- 数值复算：`python docs/analysis/simulate_balance.py` → ✅ 升级曲线/等级窗口/胜率/掉落期望全部输出，seed=20260814 可复现

---

## 五、遗留问题与后续建议

1. **本次未改动任何源码/配置**——本文件为纯建议文档，所有 P0 必修项需用户批准后按"改一项验一项"推进；
2. **迁移主线**：按 C 报告 ③3.3 的 5 步顺序（数据/存档根基 → 数值工具化 → 纯逻辑层 + 命名清理 → 可测性 + 单测 → 引擎侧），Unity 侧从第一天就带 version 存档与引用式物品；
3. **建议排期**：P0 三项可在 1 个会话内完成（工作量小-中且互相独立）；P1-1/P1-2 建议紧随 P0；P2 系列按迁移节奏分批；
4. **数据侧后续观察**：若引入 A-1 精英怪/动态修正，需同步重跑 `simulate_balance.py` 验证梯度，避免数值回归。

---

## 六、美术资源表现接口设计（为后续图形化做准备）

完整设计见子报告 [`docs/analysis/D_render_api.md`](analysis/D_render_api.md)（476 行）。

**核心设计一句话**：GameCore 只产出**游戏事件序列**这一种输出；渲染（`IGameRenderer`）、输入（`IInputSource`）、资源（`IResourceLoader`）三套抽象按"接口 + Console/Unity 双实现"组织——图形化迁移从"重写"降为"替换两个实现类"。

要点摘录：
- **边界三条红线**：GameCore 不碰 I/O（无 cout/delay/cin/chcp）；表现层不回写逻辑状态；输入单向流入（表现层收集 → GameCommand → GameCore）
- **IGameRenderer**：按呈现动作分方法（RenderScene/RenderCombat/RenderMessage/PlayEffect），参数为只读值对象（SceneView/CombatView 等）
- **节奏控制回归表现层**：散落各处的 `UIConfig::delay` 全部由渲染器统一决定
- **输入抽象**：`IInputSource` 统一 std::cin 与图形化事件输入
- **资源抽象**：`IResourceLoader` 按 ID 加载（美术资源目录约定 + config JSON 衔接）
- **落地顺序**：依赖 C 报告 P2-2（纯逻辑层抽取）先行；Console 版先实现最小 `ConsoleRenderer`
