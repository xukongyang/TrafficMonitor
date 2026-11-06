#include "framework.h"
#include "HostNameItem.h"
#include "PluginHostName.h"

const wchar_t* CHostNameItem::GetItemName() const
{
    return L"主机名";
}

const wchar_t* CHostNameItem::GetItemId() const
{
    return L"Hn7kQ2xd";
}

const wchar_t* CHostNameItem::GetItemLableText() const
{
    return L"主机名";
}

const wchar_t* CHostNameItem::GetItemValueText() const
{
    return CPluginHostName::Instance().GetHostName().c_str();
}

const wchar_t* CHostNameItem::GetItemValueSampleText() const
{
    return L"MY-COMPUTER-NAME";
}
