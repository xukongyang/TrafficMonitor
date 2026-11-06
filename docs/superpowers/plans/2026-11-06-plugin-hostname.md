# TrafficMonitor 主机名插件（PluginHostName）实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新建 TrafficMonitor 插件，从程序运行目录的 `插件-主机名.txt` 读取主机名并作为浮动窗口显示项显示。

**Architecture:** 纯 Win32 的 Unicode DLL（无 MFC），实现 `ITMPlugin` + `IPluginItem`（参考 `PluginDemo`）。核心解析逻辑独立成 `HostNameFile` 模块，用独立 C++ 测试程序先行验证（TDD）。

**Tech Stack:** C++ (MSVC v143)、Win32 API、TrafficMonitor 插件接口（`include/PluginInterface.h`，API v8）

**Spec:** `docs/superpowers/specs/2026-11-06-plugin-hostname-design.md`

## Global Constraints

- 插件接口版本 8；不重写 `GetAPIVersion()`
- Unicode 字符集；工具集 v143；`WindowsTargetPlatformVersion` 10.0；不使用 MFC
- 所有新源文件保存为 **UTF-8 with BOM**
- `GetItemId()` 仅字母数字：`Hn7kQ2xd`
- 文件名固定为 `插件-主机名.txt`，位于 exe 所在目录
- 文件不存在/空 → 后备 `GetComputerNameW`，静默无提示
- 辅助脚本/测试程序用 C++ 或 Go，不用 Python
- msbuild 路径：`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`

## Review Focus

1. **编码识别**：记事本默认 ANSI（GBK 中文机器）和 UTF-8 都必须能解析 → Task 2 测试用例 2/3
2. **多行/空白**：文件含换行或首尾空白时显示干净的第一行 → Task 2 测试用例 6/7/8
3. **热更新**：改文件不重启，约 1 秒内生效（`DataRequired` 修改时间检测）→ Task 4 手动验证
4. **显示顺序**：插件项默认排最后，需用户在"显示设置→设置显示项目顺序"上移一次 → Task 4 使用说明写明
5. **位数匹配**：x64 主程序必须配 x64 dll，x86 配 Win32 dll；位数不符时主程序静默不加载 → Task 4 同时产出两种位数并写入说明

---

### Task 1: 工程骨架（vcxproj + sln 注册 + 可编译的空插件）

**Files:**
- Create: `PluginHostName/PluginHostName.vcxproj`（以 `PluginDemo/PluginDemo.vcxproj` 为蓝本）
- Create: `PluginHostName/PluginHostName.vcxproj.filters`
- Create: `PluginHostName/framework.h`（仅 `#include <Windows.h>` 等系统头，无 MFC）
- Create: `PluginHostName/PluginHostName.h` / `PluginHostName/PluginHostName.cpp`
- Modify: `TrafficMonitor.sln`（仿照 PluginDemo 条目注册新项目）

**Interfaces:**
- Consumes: `include/PluginInterface.h`（`ITMPlugin`、`IPluginItem`、`TMPluginGetInstance` 导出约定）
- Produces: 编译产物 `Bin\<Platform>\<Configuration>\plugins\PluginHostName.dll`，导出 `ITMPlugin* TMPluginGetInstance()`；后续任务在此骨架上填充 `GetItem`/`DataRequired`

- [ ] **Step 1: 创建工程文件** — vcxproj 要点：`ConfigurationType=DynamicLibrary`、`CharacterSet=Unicode`、`PlatformToolset=v143`、无 MFC（不设 UseOfMfc 或 false）、`OutDir` 两种分支照抄 PluginDemo（ARM64EC → `$(SolutionDir)Bin\$(Configuration)\plugins\`，其余 → `$(SolutionDir)Bin\$(Platform)\$(Configuration)\plugins\`）、平台 Debug/Release × Win32/x64/ARM64EC
- [ ] **Step 2: 注册到 TrafficMonitor.sln** — 新 GUID（如 `{A7B3E5C1-2D4F-46E8-9B0C-1D2E3F4A5B6C}`），ProjectConfigurationPlatforms 段仿照 PluginDemo 的映射（x86→Win32，x64→x64，ARM64EC→ARM64EC）
- [ ] **Step 3: 编译单项目验证骨架** — 运行：`MSBuild.exe TrafficMonitor.sln /t:PluginHostName /p:Configuration=Release /p:Platform=x64`，Expected: 构建成功，`Bin\x64\Release\plugins\PluginHostName.dll` 存在
- [ ] **Step 4: Commit** — `git add PluginHostName TrafficMonitor.sln docs && git commit -m "PluginHostName 插件工程骨架"`

### Task 2: HostNameFile 核心解析逻辑（TDD）

**Files:**
- Create: `PluginHostName/HostNameFile.h` / `PluginHostName/HostNameFile.cpp`
- Test: `PluginHostName/tests/test_hostname_file.cpp`

**Interfaces:**
- Consumes: Win32 `GetModuleFileNameW`/`GetComputerNameW`/`MultiByteToWideChar`
- Produces（Task 3 依赖，签名固定）:
  - `std::wstring HostNameFile::GetDataFilePath()` — exe 目录 + `插件-主机名.txt`
  - `std::wstring HostNameFile::ParseHostNameFromBytes(const unsigned char* data, size_t size)` — 编码识别 + 取首行 + 修剪；空输入/空结果返回 `L""`
  - `std::wstring HostNameFile::ReadHostNameFromFile(const std::wstring& path)` — 读文件后调 `ParseHostNameFromBytes`；失败返回 `L""`
  - `std::wstring HostNameFile::GetSystemHostName()` — `GetComputerNameW` 封装
  - `bool HostNameFile::GetFileStamp(const std::wstring& path, ULONGLONG& stamp)` — 文件大小+最后写入时间打包为 stamp；文件不存在返回 false

- [ ] **Step 1: 写失败测试 `tests/test_hostname_file.cpp`** — 独立 `main`，用断言宏（失败打印用例名并返回 1）。用例（原始字节直接用十六进制数组构造，避免源码编码干扰）：
  1. UTF-8 BOM `EF BB BF "MY-PC\nSECOND\n"` → `L"MY-PC"`
  2. UTF-8 无 BOM `"DESKTOP-ABC123"` → `L"DESKTOP-ABC123"`
  3. GBK ANSI 字节 `D6 F7 BB FA 30 30 31`（"主机001"）→ `L"主机001"`
  4. UTF-16LE BOM `FF FE` + `"SRV-01"` 的 UTF-16LE 字节 → `L"SRV-01"`
  5. 空/仅 BOM → `L""`
  6. 纯空白 `"\r\n  \n"` → `L""`
  7. 首尾空白 `"  pc1 \r\n"` → `L"pc1"`
  8. `GetDataFilePath()`：以 `TrafficMonitor.exe` 实际路径（用测试 exe 自身路径模拟）末尾拼接校验——直接断言返回值以调用 exe 所在目录结尾且以 `插件-主机名.txt` 结尾
- [ ] **Step 2: 编译运行测试验证失败** — 运行：`cmd /c "vcvars64.bat && cl /nologo /EHsc /utf-8 tests\test_hostname_file.cpp HostNameFile.cpp /Fe:tests\test.exe"`，Expected: 编译失败（HostNameFile.h 不存在）或断言失败
- [ ] **Step 3: 实现 HostNameFile.cpp** — 解析顺序：UTF-8 BOM → 按 CP_UTF8；UTF-16LE BOM → 按 LE 宽字节直取（奇数字节视为失败）；无 BOM → 先 `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)` 尝试，失败再按 `CP_ACP`；随后截取第一行（`\r`/`\n` 前的部分）、`TrimString` 首尾空白（ASCII + 全角空格）
- [ ] **Step 4: 编译运行测试验证通过** — 同 Step 2 命令，Expected: 全部用例通过（退出码 0）
- [ ] **Step 5: Commit** — `git add PluginHostName && git commit -m "PluginHostName 核心解析逻辑及测试"`

### Task 3: 显示项与插件主体接入

**Files:**
- Create: `PluginHostName/HostNameItem.h` / `PluginHostName/HostNameItem.cpp`
- Modify: `PluginHostName/PluginHostName.h` / `PluginHostName/PluginHostName.cpp`
- Modify: `PluginHostName/PluginHostName.vcxproj`（把 HostNameFile/HostNameItem 加入编译）

**Interfaces:**
- Consumes: Task 2 的全部 `HostNameFile` 函数；`IPluginItem`（`GetItemName/GetItemId/GetItemLableText/GetItemValueText/GetItemValueSampleText`）、`ITMPlugin`（`GetItem/DataRequired/GetInfo/OnInitialize`）
- Produces: 完整插件行为——`GetItem(0)` 返回显示项，其余返回 `nullptr`；`GetItemValueText()` 返回缓存的主机名

- [ ] **Step 1: 实现 `CHostNameItem : IPluginItem`** — `GetItemName()`/`GetItemLableText()` → `L"主机名"`；`GetItemId()` → `L"Hn7kQ2xd"`；`GetItemValueText()` → 缓存值（`CPluginHostName` 单例持有的 `std::wstring`，经 `c_str()` 返回）；`GetItemValueSampleText()` → `L"MY-COMPUTER-NAME"`
- [ ] **Step 2: 实现 `CPluginHostName : ITMPlugin`** — `OnInitialize`：初次 `ReadHostNameFromFile`，为空则 `GetSystemHostName()`；`DataRequired`：`GetFileStamp` 比对，stamp 变化时重读并按同样后备规则更新缓存；`GetInfo`：NAME=`主机名`、DESCRIPTION=`显示自定义主机名（读取程序运行目录的 插件-主机名.txt）`、AUTHOR=`yangjian`、VERSION=`1.0`、URL=`https://github.com/zhongyang219/TrafficMonitor`、COPYRIGHT 照例式；导出 `TMPluginGetInstance()`（`extern "C"` + `__declspec(dllexport)`，返回静态单例；无 MFC 故不需要 `AFX_MANAGE_STATE`）
- [ ] **Step 3: 编译验证** — Task 1 Step 3 的 msbuild 命令，Expected: 构建成功；`dumpbin /exports` 可见 `TMPluginGetInstance`
- [ ] **Step 4: Commit** — `git add PluginHostName && git commit -m "PluginHostName 显示项与插件主体实现"`

### Task 4: 端到端验证与使用说明

**Files:**
- Create: `PluginHostName/README.md`（使用说明）
- Create: `Bin\x64\Release\插件-主机名.txt`（验证用临时文件，验证后删除）
- Build: Release|x64 与 Release|Win32 两个位数

**Interfaces:**
- Consumes: Task 3 的完整 dll；主程序 `Bin\x64\Release\TrafficMonitor.exe`（需先构建主程序）
- Produces: 可交付的 dll + 使用说明

- [ ] **Step 1: 构建主程序与插件** — 运行：`MSBuild.exe TrafficMonitor.sln /p:Configuration=Release /p:Platform=x64` 及 `/p:Platform=x86`（`/t:TrafficMonitor;PluginHostName` 限定目标），Expected: `Bin\x64\Release\TrafficMonitor.exe` 与两个 `plugins\PluginHostName.dll`（x64/Win32）生成
- [ ] **Step 2: 功能验证** — 在 `Bin\x64\Release\` 写入 UTF-8 BOM 的 `插件-主机名.txt`（内容如 `TEST-HOST-01`），启动 TrafficMonitor.exe，用 PowerShell 截屏保存 PNG 后读图确认：浮动窗口出现"主机名：TEST-HOST-01"（若未在最上面一行，先在显示设置中把插件项上移到首位再确认）；随后修改文件内容为 `TEST-HOST-02`，截屏确认约 1 秒内刷新；删除 `插件-主机名.txt`，截屏确认后备显示系统主机名
- [ ] **Step 3: 删除验证用临时文件**
- [ ] **Step 4: 写使用说明 `PluginHostName/README.md`** — 内容：功能简介；文件格式（名称固定 `插件-主机名.txt`、位置为 exe 目录、编码自动识别、仅取第一行）；后备行为；部署步骤（按主程序位数选 x86/x64 dll 放入 `plugins` 子目录）；"显示到最上面一行"的主程序设置步骤（显示设置→设置显示项目顺序→上移）；热更新说明
- [ ] **Step 5: Commit** — `git add PluginHostName && git commit -m "PluginHostName 使用说明"`
