# -*- coding: utf-8 -*-
"""
Dongeon V5.0 数值平衡分析脚本（A 报告证据）
分析项：
  1) 升级曲线（expToUp 二段公式）→ 各等级累计经验需求 / 对应击杀数
  2) 等级窗口过滤规则复核（[minLevel, maxLevel+3]）
  3) 战斗胜率蒙特卡洛（纯攻击 / 带药水两种策略，满血开局、不计升级回血=最坏情况）
  4) 掉落期望（普通怪 1 roll / Boss 3 roll）与 Boss 必掉率复核
用法: python docs/analysis/simulate_balance.py
"""
import json
import random
from collections import defaultdict

random.seed(20260814)

# ---------- 常量（来自 include/Config/Config.h 与 Player.cpp） ----------
STARTHP, STARTDEF, STARTLEVEL, STARTEXP = 25, 0, 0, 0
MAXLEVEL = 60
MIN_ATK, MAX_ATK = 3, 7
EXPTOUP = 15

# ---------- 配置数据 ----------
with open("config/monster.json", encoding="utf-8") as f:
    MONSTERS = json.load(f)["monsters"]
with open("config/item.json", encoding="utf-8") as f:
    ITEMS = json.load(f)
MEDS = {m["id"]: m["hp_restore"] for m in ITEMS["medicines"]}
WEP = {e["id"]: e for e in ITEMS["equipments"] if e["type_2"] == "weapon"}
ARM = {e["id"]: e for e in ITEMS["equipments"] if e["type_2"] == "armor"}


# ---------- 升级曲线 ----------
def exp_to_up(level: int) -> int:
    """升级到 level 后，下一级所需经验（level 为升级后等级）"""
    if level <= 39:
        return 15 + 8 * level
    return 15 + 8 * level + 3 * (level - 39) ** 2


def cumulative_exp(target_level: int) -> int:
    """从 0 级升到 target_level 累计所需经验"""
    total = 0
    lv = STARTLEVEL
    while lv < target_level:
        total += exp_to_up(lv + 1)
        lv += 1
    return total


def player_stats(level: int, eq_atk=(0, 0), eq_def=0):
    """按等级公式计算裸装玩家属性（不含升级回血模拟）"""
    hp = STARTHP
    atk_min, atk_max, d = MIN_ATK, MAX_ATK, STARTDEF
    lv = STARTLEVEL
    while lv < level:
        lv += 1
        hp_gain, atk_min_g, atk_max_g, def_g = 6, 1, 2, 1
        if lv >= 40:
            extra = lv - 39
            hp_gain += extra
            atk_min_g += int(extra * 0.1)
            atk_max_g += int(extra * 0.2)
            def_g += int(extra * 0.1)
        hp += hp_gain
        atk_min += atk_min_g
        atk_max += atk_max_g
        d += def_g
    return {"hp": hp, "atk": (atk_min + eq_atk[0], atk_max + eq_atk[1]), "def": d + eq_def}


# ---------- 等级窗口 ----------
def available(level: int, boss_only=False):
    out = []
    for m in MONSTERS:
        if m.get("isBoss", False) != boss_only:
            continue
        if level >= m["minLevel"] and m["maxLevel"] + 3 >= level:
            out.append(m)
    return out


# ---------- 战斗模拟 ----------
def fight(player, monster, potions=None, strategy="attack"):
    """返回 (胜负, 剩余hp, 耗药数)。potions: 药水恢复量列表(可重复使用)。"""
    m_hp = monster["hp"]
    m_atk = tuple(monster["atk"])
    p_hp = player["hp"]
    potions = list(potions or [])
    used = 0
    while p_hp > 0 and m_hp > 0:
        # 玩家先手
        dmg = random.randint(*player["atk"])
        m_hp -= dmg
        if m_hp <= 0:
            return True, p_hp, used
        # 策略：血量低且背包有药则吃药（消耗本回合）
        if strategy == "potion" and potions:
            if p_hp <= player["hp"] * 0.4:
                p_hp = min(player["hp"], p_hp + potions[0])
                used += 1
                potions.pop(0)
                continue
        # 怪物反击
        dmg = random.randint(*m_atk) - player["def"]
        if dmg > 0:
            p_hp -= dmg
    return p_hp > 0, p_hp, used


def win_rate(player, monster, trials=20000, **kw):
    wins = 0
    for _ in range(trials):
        win, _, _ = fight(player, monster, **kw)
        wins += win
    return wins / trials


# =====================================================================
print("=" * 72)
print("① 升级曲线")
print("=" * 72)
print(f"{'升级后等级':>8} {'下一级需求exp':>12} {'累计exp(0→L)':>14} {'该级属性':>22}")
for lv in [1, 5, 10, 15, 20, 25, 30, 35, 39, 40, 45, 50, 55, 60]:
    st = player_stats(lv)
    print(f"{lv:>8} {exp_to_up(lv):>12} {cumulative_exp(lv):>14} "
          f"HP{st['hp']} ATK{st['atk'][0]}~{st['atk'][1]} DEF{st['def']}")

total_exp_60 = cumulative_exp(60)
print(f"\n0→60 累计经验需求: {total_exp_60:,}")

# 击杀数估算：以各等级可打区域怪平均 exp 为分母
print("\n击杀数估算（按当前可打怪平均 exp）：")
for lv in [10, 20, 30, 40, 50, 60]:
    exp_at = cumulative_exp(lv)
    av = available(lv, boss_only=False)
    avg_exp = sum(m["exp"] for m in av) / len(av) if av else 0
    print(f"  0→{lv}级: 需求{exp_at:>7,} exp, 当前普通怪平均 {avg_exp:.0f} exp/只 → 约 {exp_at/max(avg_exp,1):.0f} 只")

# =====================================================================
print("\n" + "=" * 72)
print("② 等级窗口过滤规则复核（[minLevel, maxLevel+3]）")
print("=" * 72)
print(f"{'等级':>6} {'可打普通怪数':>10} {'可打Boss数':>9} {'普通怪等级跨度':>12}")
for lv in [0, 3, 8, 10, 16, 24, 32, 40, 48, 50, 55, 60]:
    av = available(lv, False)
    ab = available(lv, True)
    span = f"[{min(m['minLevel'] for m in av)},{max(m['maxLevel'] for m in av)}]" if av else "—"
    print(f"{lv:>6} {len(av):>10} {len(ab):>9} {span:>12}")

# 找断档：是否存在某等级没有任何普通怪可打
gaps = [lv for lv in range(0, 61) if not available(lv, False)]
print(f"\n无普通怪可打的等级（断档）: {gaps if gaps else '无，全部连通'}")

# =====================================================================
print("\n" + "=" * 72)
print("③ 战斗胜率蒙特卡洛（最坏情况：满血开局、不计升级回血）")
print("=" * 72)
# 关键等级 × 该等级窗口内代表性怪
# 装备模型：裸装 = (0,0)/0；区域标配 = 该区域普通怪武器+防具；Boss 战带 Boss 专属武器 = 毕业装（最乐观）
scenarios = [
    # (等级, 装备, 怪名, 说明)
    (0,  (0, 0), 0, "兔子",    "新手第一战(裸装)"),
    (0,  (0, 0), 0, "野猪",    "新手最硬普通怪(裸装)"),
    (3,  (1, 1), 1, "野猪王",  "区域1 Boss(匕首+厚衣)"),
    (8,  (3, 3), 2, "毒蛇",    "区域2(铁剑+皮甲)"),
    (10, (3, 3), 2, "猛虎",    "区域2最难(铁剑+皮甲)"),
    (16, (5, 5), 4, "灰熊",    "区域3(精钢剑+锁子甲)"),
    (24, (9, 9), 6, "遗迹守卫", "区域4(符文剑+板甲)"),
    (32, (13, 13), 9, "腐毒兽", "区域5(秘银剑+秘银甲)"),
    (40, (19, 19), 12, "深渊魔将", "区域6(龙牙剑+龙鳞甲)"),
    (48, (25, 25), 16, "混沌兽", "区域7(星辰剑+星辉甲)"),
    (48, (45, 45), 16, "深渊之王", "区域7 Boss(魔王裁决+星辉甲,毕业装)"),
    (48, (25, 25), 16, "深渊之王", "区域7 Boss(星辰剑+星辉甲,真实装)"),
    (55, (45, 45), 16, "深渊之王", "区域7 Boss(魔王裁决+星辉甲,Lv55)"),
    (60, (45, 45), 16, "深渊之王", "区域7 Boss(魔王裁决+星辉甲,Lv60)"),
]
for lv, eq_atk, eq_def, mname, note in scenarios:
    m = next(x for x in MONSTERS if x["name"] == mname)
    p = player_stats(lv, eq_atk, eq_def)
    wr_attack = win_rate(p, m, trials=20000, strategy="attack")
    # 药水策略：带 3 瓶当前区域药水
    med_key = "10001"
    if lv >= 8: med_key = "10002"
    if lv >= 16: med_key = "10003"
    if lv >= 24: med_key = "10004"
    if lv >= 32: med_key = "10005"
    if lv >= 40: med_key = "10006"
    potions = [MEDS[int(med_key)]] * 3
    wr_potion = win_rate(p, m, trials=20000, strategy="potion", potions=potions)
    print(f"  Lv{lv:<3} {mname:<8} {note:<20} 纯攻击 {wr_attack*100:5.1f}% | 带3药水 {wr_potion*100:5.1f}%")

# =====================================================================
print("\n" + "=" * 72)
print("④ 掉落期望与 Boss 必掉率")
print("=" * 72)
def drop_expect(drops, rolls=1):
    total = sum(d["weight"] for d in drops)
    exp = defaultdict(float)
    for _ in range(rolls):
        for d in drops:
            exp[d["itemId"]] += d["weight"] / total
    return dict(exp)

# 普通怪（85/8/7）与 Boss（70/20/10 × 3 rolls）
exp_normal = drop_expect([{"itemId": 10001, "weight": 85}, {"itemId": 20001, "weight": 8}, {"itemId": 30001, "weight": 7}])
exp_boss = drop_expect([{"itemId": 21001, "weight": 70}, {"itemId": 10001, "weight": 20}, {"itemId": 20001, "weight": 10}], rolls=3)
print(f"普通怪单次击杀期望: 药水 {exp_normal[10001]:.2f} 件 | 武器 {exp_normal[20001]:.2f} | 防具 {exp_normal[30001]:.2f}")
print(f"   → 平均 {exp_normal[20001]+exp_normal[30001]:.2f} 件装备/只")
boss_no_weapon = (1 - 0.7) ** 3
print(f"Boss 3 连 roll: 专属武器必掉率 = 1-0.3^3 = {1-boss_no_weapon:.4f} ({1-boss_no_weapon:.1%})  ← 复核 ARCHITECTURE 97.3%")

# 区域普通怪装备提升链条（武器攻击期望）
print("\n装备提升链（武器平均攻击 / 防具防御）：")
for eid, e in sorted(WEP.items(), key=lambda x: x[1]["atk"][1]):
    if eid < 20001 or eid > 21000:
        continue
    print(f"  {e['name']:<8} ID{eid} 攻击 {e['atk'][0]}~{e['atk'][1]} 均值 {(e['atk'][0]+e['atk'][1])/2:.1f}")

# 药水 vs 怪物伤害对比（区域1 兔子 atk1-3 vs 草药+10）
print("\n药水恢复 vs 当前区域怪物单回合平均伤害：")
for med_id, restore in sorted(MEDS.items()):
    print(f"  {med_id}: +{restore} HP")
