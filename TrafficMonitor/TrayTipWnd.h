#pragma once

//托盘图标点击弹出的统计信息气泡（无边框置顶小窗，由程序控制显示/隐藏，不随鼠标移开自动关闭）
class CTrayTipWnd : public CWnd
{
public:
    //在锚点（托盘图标）上方居中显示气泡
    BOOL Show(const CString& text, CPoint anchor_top_center);
    void UpdateText(const CString& text);       //更新显示的文本（内容变化时调用）
    void Hide();                                //隐藏气泡
    bool IsShown() const { return GetSafeHwnd() != NULL; }

protected:
    afx_msg void OnPaint();
    void AdjustSize();                          //根据文本计算并调整窗口大小
    CString m_text;

    DECLARE_MESSAGE_MAP()
};
