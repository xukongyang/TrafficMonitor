#include "framework.h"
#include "PluginHostName.h"
#include "HostNameFile.h"

CPluginHostName CPluginHostName::m_instance;

CPluginHostName::CPluginHostName()
{
}

CPluginHostName& CPluginHostName::Instance()
{
    return m_instance;
}

IPluginItem* CPluginHostName::GetItem(int index)
{
    switch (index)
    {
    case 0:
        return &m_item;
    default:
        break;
    }
    return nullptr;
}

void CPluginHostName::LoadHostName()
{
    const std::wstring path = HostNameFile::GetDataFilePath();
    ULONGLONG stamp = 0;
    bool stamp_valid = HostNameFile::GetFileStamp(path, stamp);
    //文件未变化时（首次加载除外）不重复读取
    if (m_initialized && m_stamp_valid == stamp_valid && (!stamp_valid || m_file_stamp == stamp))
        return;

    std::wstring value = HostNameFile::ReadHostNameFromFile(path);
    if (value.empty())
    {
        if (stamp_valid)
            return;     //文件存在但读取失败（如被其他程序临时独占）：保留旧值与旧文件标识，下一秒重试
        value = HostNameFile::GetSystemHostName();      //文件不存在或内容为空时后备为系统主机名
    }
    m_initialized = true;
    m_file_stamp = stamp;
    m_stamp_valid = stamp_valid;
    m_host_name = value;
}

void CPluginHostName::DataRequired()
{
    LoadHostName();
}

const wchar_t* CPluginHostName::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"主机名";
    case TMI_DESCRIPTION:
        return L"显示自定义主机名（读取程序运行目录的 插件-主机名.txt）";
    case TMI_AUTHOR:
        return L"yangjian";
    case TMI_COPYRIGHT:
        return L"Copyright (C) by yangjian 2026";
    case TMI_VERSION:
        return L"1.0";
    case TMI_URL:
        return L"https://github.com/zhongyang219/TrafficMonitor";
    default:
        break;
    }
    return L"";
}

void CPluginHostName::OnInitialize(ITrafficMonitor* pApp)
{
    m_app = pApp;
    LoadHostName();
}

ITMPlugin* TMPluginGetInstance()
{
    return &CPluginHostName::Instance();
}
