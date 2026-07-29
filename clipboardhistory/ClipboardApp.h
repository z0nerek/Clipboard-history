#pragma once
#include <windows.h>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <shellapi.h>
#include <fstream>
#include <ShlObj.h>
#include <dwmapi.h> // Dodane: API zarządzania wyglądem okien

#pragma comment(lib, "dwmapi.lib") // Automatyczne linkowanie biblioteki DWM

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

class ClipboardApp {
public:
    ClipboardApp(HINSTANCE hInstance);
    ~ClipboardApp();

    bool Initialize();
    void Run();
    static void RegisterStartup();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnClipboardUpdate();
    void ShowWindow();
    void HideWindow();
    void CheckFirstRunAndAutostart();

    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();

    void FilterList();
    void SaveHistory();
    void LoadHistory();
    std::wstring GetHistoryFilePath();

    NOTIFYICONDATAA m_nid;
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    HWND m_hListBox;
    HWND m_hSearchBox; 

    HBRUSH m_hDarkBrush;

    std::deque<std::wstring> m_history;
    std::vector<std::wstring> m_filteredItems; 
    const size_t MAX_HISTORY = 20;
};