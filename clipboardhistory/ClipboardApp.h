#pragma once

#include <windows.h>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <shellapi.h>
#include <fstream>
#include <ShlObj.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_CLEAR 1001
#define ID_TRAY_EXIT 1002

enum ItemType {
    Type_Text,
    Type_Image
};

struct ClipboardItem {
    ItemType type;
    std::wstring text;
    std::wstring imagePath;
    bool isPinned = false;
};

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
    std::wstring GetImagesFolderPath();
    void UpdateImagePreview();

    NOTIFYICONDATAA m_nid;
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    HWND m_hListBox;
    HWND m_hSearchBox;
    HWND m_hImagePreviewBox;
    HBRUSH m_hDarkBrush;
    HBITMAP m_hCurrentPreviewBmp;

    DWORD m_lastClipboardSequence = 0;
    bool m_ignoreNextClipboardUpdate = false;

    std::deque<ClipboardItem> m_history;
    std::vector<size_t> m_filteredIndices;
    const size_t MAX_HISTORY = 20;
};