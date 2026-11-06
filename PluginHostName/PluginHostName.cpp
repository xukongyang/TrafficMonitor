#include "framework.h"
#include "PluginHostName.h"

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
    return nullptr;
}

void CPluginHostName::DataRequired()
{
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
}

ITMPlugin* TMPluginGetInstance()
{
    return &CPluginHostName::Instance();
}
