// HostNameFile 核心解析逻辑的独立测试程序
// 编译: cl /nologo /EHsc /utf-8 tests\test_hostname_file.cpp HostNameFile.cpp /Fe:tests\test_hostname_file.exe
#include <cstdio>
#include <string>
#include <Windows.h>
#include "../HostNameFile.h"

static int g_failed = 0;

#define CHECK(name, actual, expected) \
    do { \
        if ((actual) == (expected)) \
            std::printf("[PASS] %s\n", name); \
        else { \
            std::printf("[FAIL] %s: expected=[%ls] actual=[%ls]\n", name, \
                (expected).c_str(), (actual).c_str()); \
            ++g_failed; \
        } \
    } while (0)

// 源文件为 UTF-8 编码（cl 加 /utf-8），用此函数把 UTF-8 字面量转为期望的宽字符串
static std::wstring FromUtf8(const char* s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0)
        return std::wstring();
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &result[0], len);
    return result;
}

int main()
{
    // 用例1: UTF-8 BOM + 多行，取第一行
    {
        const unsigned char data[] = { 0xEF, 0xBB, 0xBF, 'M', 'Y', '-', 'P', 'C', '\n', 'S', 'E', 'C', 'O', 'N', 'D', '\n' };
        CHECK("utf8_bom_multi_line", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("MY-PC"));
    }
    // 用例2: UTF-8 无 BOM
    {
        const unsigned char data[] = { 'D', 'E', 'S', 'K', 'T', 'O', 'P', '-', 'A', 'B', 'C', '1', '2', '3' };
        CHECK("utf8_no_bom", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("DESKTOP-ABC123"));
    }
    // 用例3: GBK ANSI 字节 D6 F7 BB FA = "主机"（GBK 编码的 UTF-8 字面量做期望值）
    {
        const unsigned char data[] = { 0xD6, 0xF7, 0xBB, 0xFA, '0', '0', '1' };
        CHECK("gbk_ansi", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("主机001"));
    }
    // 用例4: UTF-16LE BOM + "SRV-01"
    {
        const unsigned char data[] = { 0xFF, 0xFE, 'S', 0, 'R', 0, 'V', 0, '-', 0, '0', 0, '1', 0 };
        CHECK("utf16le_bom", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("SRV-01"));
    }
    // 用例4b: UTF-16BE BOM（记事本"Unicode big endian"）+ "BE-SRV"
    {
        const unsigned char data[] = { 0xFE, 0xFF, 0, 'B', 0, 'E', 0, '-', 0, 'S', 0, 'R', 0, 'V' };
        CHECK("utf16be_bom", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("BE-SRV"));
    }
    // 用例5: 空/仅 BOM
    {
        CHECK("empty_input", HostNameFile::ParseHostNameFromBytes(nullptr, 0), std::wstring());
        const unsigned char bom_only[] = { 0xEF, 0xBB, 0xBF };
        CHECK("bom_only", HostNameFile::ParseHostNameFromBytes(bom_only, sizeof(bom_only)), std::wstring());
    }
    // 用例6: 纯空白
    {
        const unsigned char data[] = { '\r', '\n', ' ', ' ', '\n' };
        CHECK("whitespace_only", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), std::wstring());
    }
    // 用例7: 首尾空白修剪
    {
        const unsigned char data[] = { ' ', ' ', 'p', 'c', '1', ' ', '\r', '\n' };
        CHECK("trim", HostNameFile::ParseHostNameFromBytes(data, sizeof(data)), FromUtf8("pc1"));
    }
    // 用例8: GetDataFilePath 返回 exe 目录 + "插件-主机名.txt"
    {
        std::wstring path = HostNameFile::GetDataFilePath();
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        std::wstring dir(exe_path);
        size_t pos = dir.find_last_of(L"\\/");
        dir = dir.substr(0, pos + 1);
        const std::wstring expect = dir + FromUtf8("插件-主机名.txt");
        CHECK("data_file_path", path, expect);
    }
    // 用例9: GetFileStamp 对不存在的文件返回 false
    {
        ULONGLONG stamp = 0;
        std::wstring not_exist = HostNameFile::GetDataFilePath() + L"__not_exist_file__.txt";
        if (!HostNameFile::GetFileStamp(not_exist, stamp))
            std::printf("[PASS] file_stamp_not_exist\n");
        else {
            std::printf("[FAIL] file_stamp_not_exist: should return false\n");
            ++g_failed;
        }
    }
    // 用例10: ReadHostNameFromFile 读不存在的文件返回空
    {
        std::wstring not_exist = HostNameFile::GetDataFilePath() + L"__not_exist_file__.txt";
        if (HostNameFile::ReadHostNameFromFile(not_exist).empty())
            std::printf("[PASS] read_not_exist\n");
        else {
            std::printf("[FAIL] read_not_exist: should return empty\n");
            ++g_failed;
        }
    }

    if (g_failed > 0)
    {
        std::printf("\n%d test(s) FAILED\n", g_failed);
        return 1;
    }
    std::printf("\nall tests passed\n");
    return 0;
}
