# 任务栏统计气泡点击切换 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 任务栏窗口统计信息气泡改为左键点击切换显示/隐藏（移开鼠标不关闭），替代悬停自动弹出。

**Architecture:** 把 `CTaskBarDlg` 现有的 `CToolTipCtrl` 切到 Win32 tracking 模式（`TTF_TRACK`+`TTF_ABSOLUTE`），显隐完全由程序控制；新增显示状态成员与 toggle/hide 方法；移除 hover 的 `RelayEvent` 路径；收起点（右键菜单、定时器菜单检查）统一改用新隐藏方法。

**Tech Stack:** C++/MFC（主程序 `TrafficMonitor.sln`，完整版 Release x64）

**Spec:** `docs/superpowers/specs/2026-11-20-taskbar-tip-click-toggle-design.md`

## Global Constraints

- 仅改 `TrafficMonitor/TaskBarDlg.h/.cpp`（以及必要时的 `TaskBarDlg.h` 成员声明）
- 不引入新窗口类；气泡样式不变（`TTS_ALWAYSTIP`、`SetMaxTipWidth(600)` 保留）
- `show_tool_tip`（任务栏"显示鼠标提示"设置）为总开关：false 时点击不弹且强制收起
- 双击动作保持不变；双击引发的气泡状态闪烁为已确认可接受的取舍
- 悬停自动弹出取消（用户要求的交互）
- 测试策略：本计划为 MFC UI 交互改造，无可注入纯逻辑，**以编译成功 + 真机验收清单代替自动化测试**（与用户约定）
- 构建：完整版 `TrafficMonitor.sln` Release x64（构建方式：本机 MFC 组件或云端 CI，由用户选择；代码实现不依赖构建环境）

## Review Focus

1. **tracking 模式的定位**：`TTM_TRACKPOSITION` 用屏幕坐标，任务栏在屏幕底部时气泡应出现在任务栏上方且不出屏（y<0 时 clamp）→ Task 2 验收项
2. **插件区域点击**：点在插件显示项上时插件 `OnMouseEvent(MT_LCLICKED)` 返回 0（未处理）时仍应切换气泡 → Task 2 验收项
3. **show_tool_tip=false**：点击无反应且已显示的气泡收起 → Task 2 验收项
4. **右键菜单/定时器菜单检查处的收起**：原 `Pop()` 调用点（`OnRButtonUp`、`OnTimer` 的 `m_menu_popuped` 分支）必须改为新的隐藏方法，否则 tracking 气泡在菜单打开时仍残留 → Task 1 代码步骤显式覆盖
5. **双击冲突**：双击前两次单击使气泡状态翻转两次回到原状（视觉闪烁）——已确认接受，实现不做双击判定延迟

---

### Task 1: tracking 模式改造（状态、toggle、收起点）

**Files:**
- Modify: `TrafficMonitor/TaskBarDlg.h`
- Modify: `TrafficMonitor/TaskBarDlg.cpp`（初始化约 1048 行、`SetToolTipsTopMost` 约 955 行、`UpdateToolTips` 约 962 行、`PreTranslateMessage` 约 1180 行、`OnRButtonUp` 约 1086 行、`OnTimer` 约 1253 行、`OnLButtonUp` 约 1447 行）

**Interfaces:**
- Consumes: 现有 `m_tool_tips`（`CToolTipCtrl`）、`GetMouseTipsInfo()`、`theApp.m_taskbar_data.show_tool_tip`
- Produces: `bool m_tip_shown` 成员；`void ShowMouseTip(CPoint point)` / `void HideMouseTip()` / `void ToggleMouseTip(CPoint point)` 私有方法；`OnLButtonUp` 的切换入口

- [ ] **Step 1: 头文件** — `TaskBarDlg.h`：加成员 `bool m_tip_shown{ false };`；声明三个私有方法（紧邻现有 `SetToolTipsTopMost()`/`UpdateToolTips()` 声明）
- [ ] **Step 2: 初始化改 tracking 模式** — OnInitDialog 的 tooltip 初始化：`AddTool(this, _T(""), NULL, 0, TTF_TRACK | TTF_ABSOLUTE)`（替代无参 flags 版本）；`SetToolTipsTopMost()` 保留
- [ ] **Step 3: 实现 Show/Hide/Toggle** —
  `ShowMouseTip(point)`：内容取 `GetMouseTipsInfo()`（更新到 tool）；`ClientToScreen` 后气泡左上角定位在 `(screen.x + 8, screen.y - 160)`（160 为气泡估高，clamp 到 `y >= 0`）；`m_tool_tips.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti)` 后 `SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(x, y))`；`m_tip_shown = true`
  `HideMouseTip()`：`TTM_TRACKACTIVATE FALSE`；`m_tip_shown = false`
  `ToggleMouseTip(point)`：`m_tip_shown ? HideMouseTip() : ShowMouseTip(point)`
  （`TOOLINFO ti` 需按 tooltip 内部 tool 填充：`GetToolInfo(ti, this, 0)` 取回后传址给 TTM_TRACKACTIVATE）
- [ ] **Step 4: 切换入口** — `OnLButtonUp`：插件分发逻辑保留；插件未消费（返回 0 或非插件区域）时调 `ToggleMouseTip(point)`；进入前检查：`show_tool_tip` 为 false 时仅调 `HideMouseTip()` 并返回
- [ ] **Step 5: 移除 hover 路径** — `PreTranslateMessage` 删除鼠标消息的 `m_tool_tips.RelayEvent(pMsg)` 分支（`WM_KEYDOWN` 插件键盘分发保留）
- [ ] **Step 6: 收起点统一** — `OnRButtonUp` 的 `m_tool_tips.Pop()` 与 `OnTimer` 的 `m_menu_popuped` 分支 `m_tool_tips.Pop()` 均改为 `HideMouseTip()`
- [ ] **Step 7: Commit** — `git add TrafficMonitor/TaskBarDlg.h TrafficMonitor/TaskBarDlg.cpp && git commit -m "任务栏统计气泡改为点击切换（tracking 模式）"`

### Task 2: 构建与真机验收

**Files:**
- Build: 完整版 `TrafficMonitor.sln` Release x64（`Bin\x64\Release\TrafficMonitor.exe`）

**Interfaces:**
- Consumes: Task 1 代码
- Produces: 可部署的主程序 exe

- [ ] **Step 1: 构建完整版主程序** — 方式按用户选择：本机（需先安装 VS "MFC for v143" 组件）`MSBuild.exe TrafficMonitor.sln -t:TrafficMonitor;OpenHardwareMonitorApi -p:Configuration=Release -p:Platform=x64`，或云端（改 CI 增加完整版 job 后 `gh workflow run` + `gh run download`）。Expected: 构建成功产出 exe
- [ ] **Step 2: 部署** — 用户退出 TrafficMonitor 后备份原 exe，复制新 exe 到 `C:\home\xukong\software\TrafficMonitor\`
- [ ] **Step 3: 真机验收清单**（用户配合确认）—
  1. 左键点击任务栏 → 气泡弹出且位置在任务栏上方
  2. 移开鼠标气泡不消失，数值每秒刷新
  3. 再点一次气泡关闭；循环可重复
  4. 点在插件显示项（主机名）上也能切换
  5. 右键菜单弹出时气泡收起
  6. 双击功能正常（气泡状态闪烁可接受）
  7. 设置关闭"显示鼠标提示"后点击无反应
- [ ] **Step 4: Commit（如有调整）** — `git commit -m "任务栏气泡点击切换真机验收调整"`
