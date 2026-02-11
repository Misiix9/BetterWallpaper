#pragma once
#ifdef _WIN32
#include <windows.h>
#include <string>
#include <memory>
#include "../../core/ipc/IIPCClient.hpp"
namespace bwp::gui {
class WindowsMainWindow {
public:
    WindowsMainWindow(std::unique_ptr<bwp::ipc::IIPCClient> ipcClient);
    ~WindowsMainWindow();
    bool init(HINSTANCE hInstance, int nCmdShow);
    int run();
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void createControls(HWND hwnd);
    void checkIPCStatus();
    HWND m_hwnd = NULL;
    HWND m_btnNext = NULL;
    HWND m_btnPrev = NULL;
    HWND m_btnPause = NULL;
    HWND m_chkMute = NULL;
    HWND m_lblStatus = NULL;
    std::unique_ptr<bwp::ipc::IIPCClient> m_ipcClient;
    HINSTANCE m_hInstance = NULL;
};
}  
#endif
