#pragma once
#include "PluginInterface.h"
#include "HostNameItem.h"
#include <string>

//显示自定义主机名的插件：从程序运行目录的 插件-主机名.txt 读取内容显示
class CPluginHostName : public ITMPlugin
{
private:
    CPluginHostName();

public:
    static CPluginHostName& Instance();

    //获取当前显示的主机名
    const std::wstring& GetHostName() const { return m_host_name; }

    // 通过 ITMPlugin 继承
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnInitialize(ITrafficMonitor* pApp) override;

private:
    //读取数据文件更新主机名；文件不存在或内容为空时使用系统主机名。
    //内部通过文件标识检测变化，文件未变化时不重复读取
    void LoadHostName();

    CHostNameItem m_item;
    ITrafficMonitor* m_app{};
    std::wstring m_host_name;       //当前显示的主机名
    ULONGLONG m_file_stamp{};       //数据文件标识（大小+最后写入时间）
    bool m_stamp_valid{};           //数据文件是否存在
    bool m_initialized{};           //是否已经完成首次加载

    static CPluginHostName m_instance;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
