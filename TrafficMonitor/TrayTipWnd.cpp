#include "stdafx.h"
#include "TrayTipWnd.h"

BEGIN_MESSAGE_MAP(CTrayTipWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CTrayTipWnd::AdjustSize()
{
    if (GetSafeHwnd() == NULL)
        return;
    //按文本计算所需大小
    CClientDC dc(this);
    CGdiObject* pOldFont = dc.SelectStockObject(DEFAULT_GUI_FONT);
    CRect rect(0, 0, 320, 40);      //最大宽度320
    dc.DrawText(m_text, &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);
    dc.SelectObject(pOldFont);
    int width = rect.Width() + 24;      //左右各留12像素边距
    int height = rect.Height() + 20;    //上下各留10像素边距
    CRect wnd_rect;
    GetWindowRect(&wnd_rect);
    SetWindowPos(NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CTrayTipWnd::Show(const CString& text, CPoint anchor_top_center)
{
    if (GetSafeHwnd() == NULL)
    {
        if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
                ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1)),
            _T("TrafficMonitorTrayTip"), WS_POPUP,
            CRect(0, 0, 10, 10), NULL, 0))
            return FALSE;
    }
    m_text = text;
    AdjustSize();
    CRect wnd_rect;
    GetWindowRect(&wnd_rect);
    //气泡显示在锚点上方并水平居中，同时保证不出屏幕
    int x = anchor_top_center.x - wnd_rect.Width() / 2;
    int y = anchor_top_center.y - wnd_rect.Height() - 8;
    CRect work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    if (x < work_area.left)
        x = work_area.left;
    if (x + wnd_rect.Width() > work_area.right)
        x = work_area.right - wnd_rect.Width();
    if (y < work_area.top)
        y = work_area.top;
    SetWindowPos(&wndTopMost, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    return TRUE;
}

void CTrayTipWnd::UpdateText(const CString& text)
{
    if (!IsShown() || text == m_text)
        return;
    m_text = text;
    AdjustSize();
    Invalidate(FALSE);
}

void CTrayTipWnd::Hide()
{
    if (GetSafeHwnd() != NULL)
        ShowWindow(SW_HIDE);
}

void CTrayTipWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);
    //先填充背景，否则刷新文本时新旧内容会叠加（Invalidate(FALSE)不擦除背景）
    CBrush brush(RGB(250, 250, 250));
    dc.FillRect(rect, &brush);
    //浅灰边框
    CPen pen(PS_SOLID, 1, RGB(200, 200, 200));
    dc.SelectObject(&pen);
    dc.SelectStockObject(NULL_BRUSH);
    dc.Rectangle(rect);
    //文本
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(48, 48, 48));
    CGdiObject* pOldFont = dc.SelectStockObject(DEFAULT_GUI_FONT);
    CRect text_rect = rect;
    text_rect.DeflateRect(12, 10);
    dc.DrawText(m_text, &text_rect, DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);
    dc.SelectObject(pOldFont);
}
