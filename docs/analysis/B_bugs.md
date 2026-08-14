# Dongeon V5.0 Bug 复核与代码质量报告（B 报告）

> 方法：组长通读全部源码（dongeon.cpp + src/ 14 文件 + include/ 13 头文件）+ 程序化校验 config 数据。
> 复核对象：代码中作者标注的 [FIX]/[FIX_J] 修复点 + git 提交 cdd2941 声明的 B1/B2，共 13 项；
> 另附通读中新发现的缺陷（含 C 报告交叉引用，不重复展开）。

---

## ① 13 项作者声明修复复核对照

| # | 声明修复（位置） | 复核结论 | 证据 | 残留问题 |
|---|---|---|---|---|
| 1 | B1 怪物等级过滤（cdd2941，FightLogic.cpp:108-111） | ✅ 有效 | 双条件 `level>=minLevel && maxLevel+3>=level` 正确实现"可打窗口"；A 报告②复核 0~60 级无断档 | 无 |
| 2 | B2 等级上限（cdd2941，Player.cpp:52-56,85） | ✅ 有效 | `if (level >= maxlevel) return;` + `while` 条件加 `level < maxlevel`，达上限不再升级、不扣经验 | 满级后 exp 永久累积不消耗（新发现 D-3） |
| 3 | [FIX] 等级上限检查（Player.cpp:51-56） | ✅ 有效 | 与 B2 同一修复，双保险一致 | 无 |
| 4 | [FIX] getEquipment 越界+类型防护（PlayerBag.cpp:113-122） | ✅ 有效 | `index >= bag.size()` 返回 nullptr；dynamic_cast 失败保护；release 后 erase 无泄漏 | 无 |
| 5 | [FIX] changeEuipType nullptr 检查（PlayerBag.cpp:129-132） | ⚠️ 部分有效 | 空指针不再解引用；但**先 `bag.erase` 后检查**，dynamic_cast 失败时物品已被 move 销毁 → 静默丢失（见新发现 D-1） | D-1 |
| 6 | [FIX] SLOT_TYPE 显式映射（GameUIConfig.cpp:23-25,52-54） | ✅ 有效 | `constexpr EquipmentType SLOT_TYPE[] = {armor, weapon}` 不依赖枚举内部顺序 | 无 |
| 7 | [FIX] 先检查 cancel 再减一（GameUIConfig.cpp:62-65） | ✅ 有效 | `rawChoice == cancel` 提前 break，避免映射越界 | 无 |
| 8 | [FIX] 删除多余 .reset()（GameUIConfig.cpp:66-68） | ✅ 有效 | changeEuip 后不再 reset，装备不被销毁 | 无 |
| 9 | [FIX] takeOff 空指针防御（PlayerEquipment.cpp:63-71） | ✅ 有效 | nullptr 直接返回，不再段错误 | 无 |
| 10 | [FIX] Equipment.h 头文件保护+循环切断（Equipment.h:1-15） | ✅ 有效 | guard 防止重复定义；前向声明 Player/PlayerBag 切断 Equipment↔Player 环；include 大小写统一（跨平台） | 无 |
| 11 | [FIX] 随机数封装（Config.h:19-24） | ✅ 有效 | `Random::range` 统一入口，外部不再直操作 gen | 全局 gen 不可注入种子（C 报告 P2-1，测试障碍） |
| 12 | [FIX] 空池越界保护（FightLogic.cpp:121-129） | ✅ 有效 | availableMons 空时打印提示并退出，避免 `uniform_int_distribution(0,-1)` UB | 提示语与 Boss 池为空场景共用，信息略误导（见 D-4） |
| 13 | [FIX_J] ConfigLoader 字段存在性校验（ConfigLoader.cpp:33-71,93-128） | ⚠️ 部分有效 | 怪物/物品缺字段会警告并跳过；但**只校验存在性不校验形状**：`item["atk"][0]` 形状不符、`data["monsters"]` 键缺失、`type_2` 拼错 → 抛未捕获 nlohmann 异常直接崩溃（与 C 报告 P0-1 一致） | C P0-1 |

**小结：13 项中 10 项有效 ✅、2 项部分有效 ⚠️（5 号、13 号）、1 项带体验残留（12 号）。作者在 V4.1→V5.0 期间的防御性修复方向正确，绝大多数落地干净。**

## ② 新发现缺陷（通读复核，按严重度）

### D-1（高）装备丢失竞态：equipItem 先销毁后校验（PlayerBag.cpp:124-138）
`std::move(bag[i])` 传入 `changeEuipType`（按值接收）→ dynamic_cast 失败时参数析构 = **物品销毁**；随后 `bag.erase(begin()+i)` 又把空槽删除 → 背包与物品同时消失。当前唯一调用链（Equipment::use）保证传入的是装备，不可达；但作为公共接口违反防御性编程，未来任何新调用点都会踩坑。
**修复**：先 `dynamic_cast` 成功再 move/erase，与 `getEquipment` 的防护模式（PlayerBag.cpp:113-122）对齐。

### D-2（高）存档写入非原子（saveManager.cpp:31-39）
`ofstream` 直接覆写 `savegame.json`，写一半断电/崩溃 → 文件半截 → 读档解析失败 → 兜底"开始新游戏" = **全部进度丢失**。与 C 报告 P0-2 一致，此处列为独立证据。
**修复**：写 `.tmp` → close → `std::rename` 原子替换 + 保留 `.bak`。

### D-3（中）满级经验永久累积（Player.cpp:82-89）
达到 maxlevel 后 `exp` 继续累加不消耗，状态栏显示不断膨胀的 exp/expToUp 比例，数值展示失真（玩家可能误以为"还能升级"）。
**修复**：满级时 `exp = min(exp, expToUp)` 或显示"MAX"。

### D-4（中）空池提示语误导（FightLogic.cpp:126）
玩家等级窗口内"只有 Boss 无普通怪"或"两者皆空"时统一打印"当前等级没有可挑战的怪物！"——实际此时 Boss 池可能非空（10% 遭遇逻辑在空普通怪时仍可遇 Boss，但先被 else 拦截）。窗口分析（A 报告②）显示当前数据下不会触发，但逻辑上该提示与实际情况不符。
**修复**：分场景提示（"普通怪无可用，但可能遭遇 Boss"/"该等级无怪，请升级"）。

### D-5（中）UIConfig 输入 50 次非法后静默返回下限（UIConfig.cpp:15-18）
返回 `f`（范围下限）会让玩家在不知情下执行错误操作（战斗中误选"攻击"）。与 C 报告 P1-2 一致。
**修复**：改为报错回到当前菜单 + 显示剩余次数与合法范围。

### D-6（中）读档不校验数值完整性（Player.cpp:139-165）
`fromJson` 对 `currentHp` 不校验 `≤ hp_UpperLimit`、对 `level` 不校验 `≤ maxlevel`、`atk` 不校验非负——篡改/损坏档可产生"满血不死""越级属性"状态。与 C 报告 1.7 一致。
**修复**：读档后统一 clamp + 版本号校验（联动 C P2-4）。

### D-7（低）showItem(ItemType::Equipment) 静默失效（PlayerBag.cpp:39-55）
`Equipment::getType()` 返回 "armor"/"weapon"，而 `enumToString(ItemType::Equipment)` 返回 "Equipment" → **该重载永远匹配不到任何物品**。当前无调用方（战斗用药水走 Medicine 分支），是陷阱重载。
**修复**：删除该重载或改为按真实语义过滤，接口注释说明"装备请用 showItem(EquipmentType)"。

### D-8（低）字符串枚举大小写不对称（Item.cpp:45-70）
`enumToString(ItemType)` 返回 "Medicine"/"Equipment"（首字母大写），`stringToItEnum` 只接受全小写；序列化用全小写靠约定维持一致——任一方向手写错大小写即静默失败（抛"Unknown equipment type"文案还错，应为 item type）。
**修复**：统一小写规范 + 修正报错文案 + `getType()` 改 const 引用返回。

### D-9（低）Medicine.cpp 全文缩进 4 空格（Medicine.cpp:1 起）
与全库 4 空格标准不一致，纯格式噪音，易被 linter 误伤。

### D-10（低）死代码与失效接口（grep 证实无调用方）
`PlayerBag::removeByID`（PlayerBag.cpp:141）、`addItemByID`（:159）、`Equipment::dropEquipment`（Equipment.cpp:14）、`Player::showBag`（Player.cpp:25）、`Medicine::showMedicine`（Medicine.cpp:30）——5 个无调用方函数 + 注释掉的旧代码块（Config.h:14-16、Item.h:43、Medicine.h:40、Equipment.h:59、dongeon.cpp:86），增加阅读负担。

### D-11（低）版本号失同步
CMakeLists.txt:2 `VERSION 4.1` vs 实际 V5.0；PlayerBag.h:3 残留"//未适配CMake"过时注释。

---

## ③ 与 C 报告的交叉引用（避免重复，仅列归属）

| 缺陷 | B 报告 | C 报告 |
|---|---|---|
| 配置形状校验缺失崩溃 | D-13 ⚠️（13号复核） | P0-1 |
| 存档非原子 | D-2 | P0-2 |
| 背包索引族防护 | D-1 + 复核4/5 | P0-3 |
| 输入 50 次静默 | D-5 | P1-2 |
| 命名/枚举清理 | D-8 | P1-6 |
| 可测试性三障碍 | 复核11 残留 | P2-1 |
| 纯逻辑层抽取 | —（B 无新增） | P2-2 |
| 存档引用化/版本 | D-6 | P2-4 |
| 死代码清理 | D-10 | P2-5 |

> 分工说明：B 报告负责"逐项验证作者声明修复是否落地 + 通读新发现"，C 报告负责"架构级评估与迁移路线"，两者结论一致处已标注交叉引用，合并清单以 UPGRADE_SUGGESTIONS.md 为准。

---

## ④ B 侧结论

1. **作者工程素养在线**：13 项声明修复 10 项完全落地，防御式编程意识（nullptr/越界/空池）贯穿 V5.0，值得肯定；
2. **剩余 2 项"部分有效"是真正的雷**：D-1 装备静默丢失（虽当前不可达）、D-13 配置崩溃（数据驱动项目的根基）——均与 C 报告 P0 级结论共振；
3. **中低优先级的体验/一致性缺陷（D-3~D-11）共 9 项**，多为"当前不触发但违反防御性编程"或"纯质量债"，适合与 C 报告 P1/P2 合并排期。
