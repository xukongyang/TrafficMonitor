// 插件 DLL 加载集成测试：验证导出函数、显示项、文件读取与热更新
// 用法: test_plugin_load.exe <PluginHostName.dll 路径>
#include <cstdio>
#include <string>
#include <Windows.h>
#include "../../include/PluginInterface.h"

static int g_failed = 0;

#define CHECK_TRUE(name, cond) \
    do { \
        if (cond) \
            std::printf("[PASS] %s\n", name); \
        else { \
            std::printf("[FAIL] %s\n", name); \
            ++g_failed; \
        } \
    } while (0)

// UTF-8 字面量转宽字符串（源文件为 UTF-8 编码）
static std::wstring FromUtf8(const char* s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0)
        return std::wstring();
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &result[0], len);
    return result;
}

static bool WriteFileSimple(const std::wstring& path, const std::string& content)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    WriteFile(hFile, content.data(), static_cast<DWORD>(content.size()), &written, nullptr);
    CloseHandle(hFile);
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: test_plugin_load.exe <dll path>\n");
        return 2;
    }
    //测试exe所在目录，GetDataFilePath 基于该目录
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::wstring exe_dir(exe_path);
    exe_dir = exe_dir.substr(0, exe_dir.find_last_of(L"\\/") + 1);
    const std::wstring data_file = exe_dir + FromUtf8("插件-主机名.txt");

    HMODULE dll = LoadLibraryW(std::wstring(argv[1], argv[1] + strlen(argv[1])).c_str());
    CHECK_TRUE("load_dll", dll != nullptr);
    if (dll == nullptr)
    {
        std::printf("%d test(s) FAILED\n", g_failed);
        return 1;
    }
    typedef ITMPlugin* (*GetInstanceFn)();
    GetInstanceFn get_instance = reinterpret_cast<GetInstanceFn>(GetProcAddress(dll, "TMPluginGetInstance"));
    CHECK_TRUE("export_TMPluginGetInstance", get_instance != nullptr);
    if (get_instance == nullptr)
        return 1;
    ITMPlugin* plugin = get_instance();
    CHECK_TRUE("plugin_instance_not_null", plugin != nullptr);
    CHECK_TRUE("info_name", std::wstring(plugin->GetInfo(ITMPlugin::TMI_NAME)) == FromUtf8("主机名"));
    CHECK_TRUE("item_0_not_null", plugin->GetItem(0) != nullptr);
    CHECK_TRUE("item_1_null", plugin->GetItem(1) == nullptr);
    IPluginItem* item = plugin->GetItem(0);
    if (item == nullptr)
    {
        std::printf("[FAIL] item_0_not_null, abort further checks\n");
        return 1;
    }
    CHECK_TRUE("item_id", std::wstring(item->GetItemId()) == L"Hn7kQ2xd");
    CHECK_TRUE("item_label", std::wstring(item->GetItemLableText()) == FromUtf8("主机名"));
    CHECK_TRUE("sample_text", std::wstring(item->GetItemValueSampleText()) == L"MY-COMPUTER-NAME");

    //OnInitialize 初次加载：文件不存在时后备系统主机名
    DeleteFileW(data_file.c_str());
    plugin->OnInitialize(nullptr);
    wchar_t sys_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sys_len = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(sys_name, &sys_len);
    CHECK_TRUE("fallback_to_system_host_name",
        std::wstring(item->GetItemValueText()) == std::wstring(sys_name, sys_len));

    //写入数据文件后热更新生效
    CHECK_TRUE("write_data_file", WriteFileSimple(data_file, "\xEF\xBB\xBF" "TEST-HOST-A\nsecond line"));
    plugin->DataRequired();
    CHECK_TRUE("hot_reload_a", std::wstring(item->GetItemValueText()) == FromUtf8("TEST-HOST-A"));

    //修改文件内容后再次热更新
    Sleep(50);      //确保文件最后写入时间变化
    CHECK_TRUE("rewrite_data_file", WriteFileSimple(data_file, "\xEF\xBB\xBF" "TEST-HOST-B"));
    plugin->DataRequired();
    CHECK_TRUE("hot_reload_b", std::wstring(item->GetItemValueText()) == FromUtf8("TEST-HOST-B"));

    //删除文件后回到后备系统主机名
    DeleteFileW(data_file.c_str());
    plugin->DataRequired();
    CHECK_TRUE("fallback_after_delete",
        std::wstring(item->GetItemValueText()) == std::wstring(sys_name, sys_len));

    //文件被其他程序瞬时独占时保持旧值，锁释放后自动恢复（OneDrive/杀毒扫描场景）
    CHECK_TRUE("rewrite_for_lock_test", WriteFileSimple(data_file, "\xEF\xBB\xBF" "TEST-LOCK-A\n"));
    plugin->DataRequired();
    CHECK_TRUE("load_lock_a", std::wstring(item->GetItemValueText()) == FromUtf8("TEST-LOCK-A"));
    {
        //重写内容为 B（不同长度保证文件标识变化）后用独占句柄锁住文件（不允许读），
        //模拟杀毒/同步软件瞬时锁定
        Sleep(50);
        CHECK_TRUE("rewrite_for_lock_b", WriteFileSimple(data_file, "\xEF\xBB\xBF" "TEST-LOCK-BB"));
        HANDLE hLock = CreateFileW(data_file.c_str(), GENERIC_READ, 0, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        CHECK_TRUE("lock_file_opened", hLock != INVALID_HANDLE_VALUE);
        plugin->DataRequired();
        CHECK_TRUE("keep_old_value_while_locked",
            std::wstring(item->GetItemValueText()) == FromUtf8("TEST-LOCK-A"));
        if (hLock != INVALID_HANDLE_VALUE)
            CloseHandle(hLock);
        plugin->DataRequired();
        CHECK_TRUE("recover_after_unlock",
            std::wstring(item->GetItemValueText()) == FromUtf8("TEST-LOCK-BB"));
    }

    FreeLibrary(dll);
    if (g_failed > 0)
    {
        std::printf("%d test(s) FAILED\n", g_failed);
        return 1;
    }
    std::printf("\nall tests passed\n");
    return 0;
}
