#include "framework.h"
#include "HostNameFile.h"
#include <vector>

namespace
{
    const wchar_t DATA_FILE_NAME[] = L"插件-主机名.txt";

    //去除字符串首尾的空白字符（ASCII空白及全角空格）
    void TrimString(std::wstring& str)
    {
        size_t begin = 0;
        while (begin < str.size() && (iswspace(str[begin]) || str[begin] == 0x3000))
            ++begin;
        size_t end = str.size();
        while (end > begin && (iswspace(str[end - 1]) || str[end - 1] == 0x3000))
            --end;
        str = str.substr(begin, end - begin);
    }

    //截取第一行（到第一个\r或\n之前的部分）
    void KeepFirstLine(std::wstring& str)
    {
        size_t pos = str.find_first_of(L"\r\n");
        if (pos != std::wstring::npos)
            str = str.substr(0, pos);
    }

    //多字节字符串转宽字符；utf8_strict为true时对UTF-8做严格校验，转换失败返回空字符串
    std::wstring MultiByteToWide(const char* str, int length, bool utf8_strict)
    {
        UINT code_page = utf8_strict ? CP_UTF8 : CP_ACP;
        DWORD flags = utf8_strict ? MB_ERR_INVALID_CHARS : 0;
        int len = MultiByteToWideChar(code_page, flags, str, length, nullptr, 0);
        if (len <= 0)
            return std::wstring();
        std::wstring result(len, L'\0');
        MultiByteToWideChar(code_page, flags, str, length, &result[0], len);
        return result;
    }
}

namespace HostNameFile
{
    std::wstring GetDataFilePath()
    {
        wchar_t module_path[MAX_PATH];
        GetModuleFileNameW(nullptr, module_path, MAX_PATH);     //nullptr为主程序exe的路径，插件dll位于plugins子目录
        std::wstring path(module_path);
        size_t pos = path.find_last_of(L"\\/");
        if (pos == std::wstring::npos)
            return DATA_FILE_NAME;
        return path.substr(0, pos + 1) + DATA_FILE_NAME;
    }

    std::wstring ParseHostNameFromBytes(const unsigned char* data, size_t size)
    {
        if (data == nullptr || size == 0)
            return std::wstring();

        std::wstring wide_str;
        if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        {
            //UTF-8 BOM
            wide_str = MultiByteToWide(reinterpret_cast<const char*>(data + 3), static_cast<int>(size - 3), false);
        }
        else if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
        {
            //UTF-16LE BOM，宽字节直接取出
            if ((size - 2) % 2 != 0)
                return std::wstring();
            wide_str.assign(reinterpret_cast<const wchar_t*>(data + 2), (size - 2) / 2);
        }
        else
        {
            //无BOM：先按UTF-8严格校验，非法UTF-8再按系统代码页(ANSI)处理
            wide_str = MultiByteToWide(reinterpret_cast<const char*>(data), static_cast<int>(size), true);
            if (wide_str.empty())
                wide_str = MultiByteToWide(reinterpret_cast<const char*>(data), static_cast<int>(size), false);
        }

        KeepFirstLine(wide_str);
        TrimString(wide_str);
        return wide_str;
    }

    std::wstring ReadHostNameFromFile(const std::wstring& path)
    {
        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return std::wstring();
        std::wstring result;
        LARGE_INTEGER file_size{};
        if (GetFileSizeEx(hFile, &file_size) && file_size.QuadPart > 0 && file_size.QuadPart <= 1024 * 1024)
        {
            std::vector<unsigned char> buffer(static_cast<size_t>(file_size.QuadPart));
            DWORD bytes_read = 0;
            if (ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr) && bytes_read > 0)
                result = ParseHostNameFromBytes(buffer.data(), bytes_read);
        }
        CloseHandle(hFile);
        return result;
    }

    std::wstring GetSystemHostName()
    {
        wchar_t host_name[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(host_name, &size))
            return std::wstring(host_name, size);
        return std::wstring();
    }

    bool GetFileStamp(const std::wstring& path, ULONGLONG& stamp)
    {
        WIN32_FILE_ATTRIBUTE_DATA attr{};
        if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attr))
            return false;
        ULONGLONG write_time = (static_cast<ULONGLONG>(attr.ftLastWriteTime.dwHighDateTime) << 32)
            | attr.ftLastWriteTime.dwLowDateTime;
        ULONGLONG file_size = (static_cast<ULONGLONG>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
        stamp = write_time ^ file_size;
        return true;
    }
}
