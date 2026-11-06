#pragma once
#include "PluginInterface.h"

//显示自定义主机名的插件：从程序运行目录的 插件-主机名.txt 读取内容显示
class CPluginHostName : public ITMPlugin
{
private:
    CPluginHostName();

public:
    static CPluginHostName& Instance();

    // 通过 ITMPlugin 继承
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnInitialize(ITrafficMonitor* pApp) override;

private:
    ITrafficMonitor* m_app{};

    static CPluginHostName m_instance;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
