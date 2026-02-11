#ifdef _WIN32
#include "WindowsMainWindow.hpp"
#include "../../core/utils/Logger.hpp"
namespace bwp::gui {
WindowsMainWindow::WindowsMainWindow(std::unique_ptr<bwp::ipc::IIPCClient> ipcClient)
    : m_ipcClient(std::move(ipcClient)) {
}
WindowsMainWindow::~WindowsMainWindow() {
}
bool WindowsMainWindow::init(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;
    const wchar_t CLASS_NAME[] = L"BetterWallpaperWindowClass";
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    m_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"BetterWallpaper Control",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,
        NULL,
        NULL,
        hInstance,
        this
    );
    if (m_hwnd == NULL) {
        return false;
    }
    createControls(m_hwnd);
    ShowWindow(m_hwnd, nCmdShow);
    checkIPCStatus();
    return true;
}
int WindowsMainWindow::run() {
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
void WindowsMainWindow::createControls(HWND hwnd) {
    int y = 20;
    int margin = 20;
    int btnWidth = 100;
    int btnHeight = 30;
    m_lblStatus = CreateWindowW(L"STATIC", L"Connecting to Daemon...",
        WS_CHILD | WS_VISIBLE, 
        margin, y, 300, 20, hwnd, NULL, m_hInstance, NULL);
    y += 40;
    m_btnPrev = CreateWindowW(L"BUTTON", L"Previous",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        margin, y, btnWidth, btnHeight, hwnd, (HMENU)101, m_hInstance, NULL);
    m_btnNext = CreateWindowW(L"BUTTON", L"Next",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        margin + btnWidth + 10, y, btnWidth, btnHeight, hwnd, (HMENU)102, m_hInstance, NULL);
    y += 40;
    m_btnPause = CreateWindowW(L"BUTTON", L"Pause/Resume",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        margin, y, btnWidth * 2 + 10, btnHeight, hwnd, (HMENU)103, m_hInstance, NULL);
    y += 40;
    m_chkMute = CreateWindowW(L"BUTTON", L"Mute Wallpaper",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin, y, 200, 20, hwnd, (HMENU)104, m_hInstance, NULL);
}
void WindowsMainWindow::checkIPCStatus() {
    if (m_ipcClient->connect()) {
        SetWindowTextW(m_lblStatus, L"Connected to Daemon");
    } else {
        SetWindowTextW(m_lblStatus, L"Daemon not found (IPC failed)");
    }
}
LRESULT CALLBACK WindowsMainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowsMainWindow* pThis = NULL;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (WindowsMainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (WindowsMainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) {
        switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case 101:  
                pThis->m_ipcClient->previousWallpaper("primary");
                break;
            case 102:  
                pThis->m_ipcClient->nextWallpaper("primary");
                break;
            case 103:  
                pThis->m_ipcClient->pauseWallpaper("primary");
                break;
            case 104:  
                bool checked = IsDlgButtonChecked(hwnd, 104);
                pThis->m_ipcClient->setMuted("primary", checked);
                break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
}  
#endif
