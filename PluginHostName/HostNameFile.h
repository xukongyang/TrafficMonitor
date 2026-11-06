#pragma once
#include <string>
#include <Windows.h>

//主机名文件的读取与解析
namespace HostNameFile
{
    //获取数据文件的完整路径（exe所在目录 + "插件-主机名.txt"）
    std::wstring GetDataFilePath();

    /**
     * @brief   从文件的原始字节解析出主机名
     * @param   data 文件内容字节，可为空指针
     * @param   size 字节数
     * @return  解析出的主机名（第一行并去除首尾空白）；编码无法识别或内容为空时返回空字符串
     * @detail  编码识别顺序：UTF-8 BOM -> UTF-16LE BOM -> 无BOM时先按UTF-8严格校验，失败再按ANSI(系统代码页)
     */
    std::wstring ParseHostNameFromBytes(const unsigned char* data, size_t size);

    //读取指定路径的数据文件并解析主机名；文件不存在或读取失败时返回空字符串
    std::wstring ReadHostNameFromFile(const std::wstring& path);

    //获取系统主机名（GetComputerNameW）
    std::wstring GetSystemHostName();

    /**
     * @brief   获取文件的标识（大小+最后写入时间打包），用于检测文件是否变化
     * @param   path 文件路径
     * @param   stamp[out] 打包后的文件标识
     * @return  文件存在时返回true并写入stamp；文件不存在返回false
     */
    bool GetFileStamp(const std::wstring& path, ULONGLONG& stamp);
}
