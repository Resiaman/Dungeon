# -*- coding: utf-8 -*-
"""
Dongeon 控制台冒烟测试（B 报告/A 报告验证证据）
流程：启动 → 开始游戏 → 查看状态 → 进入战斗（遇敌后逃跑或打完）→ 保存 → 退出
用法: python docs/analysis/smoke_test_console.py [exe路径]
"""
import subprocess
import sys
import os

EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join("build", "dongeon.exe")

# 游戏用相对路径读 config/*.json，工作目录必须是项目根；
# exe 位于 build/ 下时，项目根 = exe 目录的上一级
_exe_dir = os.path.dirname(os.path.abspath(EXE))
CWD = os.path.dirname(_exe_dir) if os.path.basename(_exe_dir) == "build" else _exe_dir

# 交互输入序列：
# 1=开始游戏 → 3=查看状态 → 1=战斗 → (战斗中) 2=逃跑 → 若失败再 2 → 2=休息 → 6=保存 → 7=退出
# 逃跑 50% 概率，输入足够多的 2 保证成功
inputs = "1\n3\n1\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n2\n6\n7\n"

proc = subprocess.run(
    [EXE], input=inputs, capture_output=True, text=True,
    encoding="utf-8", errors="replace", cwd=CWD,
    timeout=60,
)

out = proc.stdout
checks = {
    "启动与菜单": "Welcome to Dongeon!" in out,
    "开始游戏": "开始游戏" in out,
    "状态查看": "Level:" in out and "Hp:" in out,
    "进入战斗": "进入战斗" in out,
    "战斗行为菜单": "选择你的行为" in out,
    "胜利或逃跑路径": ("你击败了" in out) or ("你逃跑了" in out) or ("逃跑失败" in out),
    "休息回血": ("你恢复了" in out) or ("状态绝佳" in out),
    "保存成功": "游戏已保存" in out,
    "正常退出": proc.returncode == 0,
}

print("=" * 56)
print(f"冒烟测试: {EXE}")
print(f"退出码: {proc.returncode}")
print("=" * 56)
all_pass = True
for name, ok in checks.items():
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
    all_pass = all_pass and ok

print("=" * 56)
print("总体:", "PASS" if all_pass else "FAIL")
sys.exit(0 if all_pass else 1)
