# PluginHostName 主机名插件

TrafficMonitor 插件：从程序运行目录的 `插件-主机名.txt` 文件读取自定义主机名，作为显示项目显示在主窗口/任务栏窗口中。

## 功能

- 读取 TrafficMonitor.exe 所在目录下的 `插件-主机名.txt`（仅取第一行，自动去除首尾空白）
- 编码自动识别：UTF-8（BOM 可有可无）、UTF-16LE、ANSI（系统代码页，如中文系统的 GBK）
- **热更新**：修改文件内容后约 1 秒内自动刷新，无需重启程序
- 文件不存在或内容为空时，自动显示系统主机名（ComputerName）

## 部署

1. 选择与主程序位数一致的 dll：
   - 64 位 TrafficMonitor → `Bin\x64\Release\plugins\PluginHostName.dll`
   - 32 位 TrafficMonitor → `Bin\Release\plugins\PluginHostName.dll`（Win32 产物不带平台子目录，与 PluginDemo 一致）
2. 将 dll 放入主程序目录下的 `plugins` 子目录（没有则新建）
3. 位数不匹配时插件会被静默忽略，请核对 TrafficMonitor.exe 的位数
4. 重启 TrafficMonitor，在"插件管理"中可以看到"主机名"插件

## 在浮动窗口显示（皮肤配置）

主窗口使用皮肤布局，插件项目需要在皮肤文件中声明才会显示。编辑所用皮肤的 `skin.xml`：

1. 在 `<plugin_map>` 段中添加映射（`hostname` 为自定义节点名，值为插件项 ID `Hn7kQ2xd`）：

   ```xml
   <plugin_map>
       <hostname>Hn7kQ2xd</hostname>
   </plugin_map>
   ```

2. 在 `<layout_l>`/`<layout_s>` 中添加布局条目。放在第一个即显示在最上面一行：

   ```xml
   <layout_l width="232" height="107">
       <hostname x="14" y="1" width="106" align="0" show="1"/>
       <cpu x="14" y="27" width="106" align="0" show="1"/>
       ...
   </layout_l>
   ```

3. **在 `<display_text>` 段中为各项目声明显示文本（含"主机名"）**。实测（TrafficMonitor v3.x）
   皮肤若无此段，各项目会只显示数值、没有"主机名: "等标签前缀：

   ```xml
   <display_text>
       <hostname>主机名: </hostname>
       <up>上传: </up>
       <down>下载: </down>
       <cpu>CPU: </cpu>
       <memory>内存: </memory>
   </display_text>
   ```

修改皮肤后需要重新加载：切换到其他皮肤再切回，或重启程序。

> 注意：**ini 格式的旧皮肤（skin.ini）不支持插件项目**，只有 xml 皮肤（skin.xml）可以在
> 主窗口布局插件项。背景图沿用同名约定（`background.bmp` / `background_l.bmp`）。

## 配置文件编码说明（已知坑）

`config.ini` 是 **UTF-8 with BOM** 编码，皮肤名以 UTF-8 写入。用其他编码（如 GBK）手工
编辑 `skin_selected` 会导致程序读出乱码、回退到第一个皮肤。请通过程序界面（右键 → 皮肤）
切换皮肤，不要手工编辑该值。

## 在任务栏窗口显示（设置界面）

1. 右键任务栏窗口 → 显示设置 → 设置显示项目顺序
2. 勾选"主机名"，用"上移"调整到需要的顺序（可移到最上面）

## 数据文件格式

- 文件名固定为 `插件-主机名.txt`，放在 TrafficMonitor.exe 所在目录
- 内容只有第一行有效，首尾空白自动去除
- 修改后约 1 秒自动生效；删除文件后显示系统主机名

## 从源码编译

- 使用 Visual Studio 2022（v143 工具集）打开 `TrafficMonitor.sln`
- 编译 `PluginHostName` 项目（Debug/Release × Win32/x64/ARM64EC）
- 产物自动输出到 `Bin\x64\<配置>\plugins\`（x64/ARM64EC）与 `Bin\<配置>\plugins\`（Win32，不带平台子目录）
- 单元测试：`cmd /c PluginHostName\tests\run_test.bat`（核心解析 11 用例 + DLL 集成 15 用例）
