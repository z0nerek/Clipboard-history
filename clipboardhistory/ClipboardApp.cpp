#include "ClipboardApp.h"
#include <vector>
#include "resource.h"

static WNDPROC g_OriginalEditWndProc = nullptr;

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            ShowWindow(GetParent(hwnd), SW_HIDE);
            return 0;
        }
        if ((wParam == 'A' || wParam == 'a') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            return 0;
        }
    }
    if (msg == WM_CHAR) {
        if (wParam == 1 || wParam == 27) {
            return 0;
        }
    }
    return CallWindowProc(g_OriginalEditWndProc, hwnd, msg, wParam, lParam);
}

bool SaveHBitmapToBMP(HBITMAP hBmp, const std::wstring& filePath) {
    BITMAP bmp;
    GetObject(hBmp, sizeof(BITMAP), &bmp);

    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    DWORD bmpSize = ((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight;

    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);

    std::vector<BYTE> pixelData(bmpSize);
    GetDIBits(hDC, hBmp, 0, (UINT)bmp.bmHeight, pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    SelectObject(hDC, hOldBmp);
    DeleteDC(hDC);

    BITMAPFILEHEADER bmfh = { 0 };
    bmfh.bfType = 0x4D42;
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfh.bfSize = bmfh.bfOffBits + (DWORD)pixelData.size();

    std::ofstream file(filePath, std::ios::binary);
    if (!file) return false;

    file.write(reinterpret_cast<const char*>(&bmfh), sizeof(bmfh));
    file.write(reinterpret_cast<const char*>(&bi), sizeof(bi));
    file.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());

    return true;
}

bool AreFilesIdentical(const std::wstring& file1, const std::wstring& file2) {
    std::ifstream f1(file1, std::ios::binary | std::ios::ate);
    std::ifstream f2(file2, std::ios::binary | std::ios::ate);
    if (!f1.is_open() || !f2.is_open()) return false;
    if (f1.tellg() != f2.tellg()) return false;
    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);
    return std::equal(std::istreambuf_iterator<char>(f1), std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>());
}

ClipboardApp::ClipboardApp(HINSTANCE hInstance)
    : m_hInstance(hInstance), m_hwnd(NULL), m_hListBox(NULL), m_hSearchBox(NULL),
    m_hImagePreviewBox(NULL), m_hDarkBrush(NULL), m_hCurrentPreviewBmp(NULL) {
}

ClipboardApp::~ClipboardApp() {
    SaveHistory();
    RemoveTrayIcon();
    RemoveClipboardFormatListener(m_hwnd);
    UnregisterHotKey(m_hwnd, 1);
    if (m_hDarkBrush) DeleteObject(m_hDarkBrush);
    if (m_hCurrentPreviewBmp) DeleteObject(m_hCurrentPreviewBmp);
}

void ClipboardApp::RegisterStartup() {
    HKEY hKey;
    const char* path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        RegSetValueExA(hKey, "LightClipboardHistory", 0, REG_SZ, (BYTE*)exePath, (DWORD)(strlen(exePath) + 1));
        RegCloseKey(hKey);
    }
}

bool ClipboardApp::Initialize() {
    m_hDarkBrush = CreateSolidBrush(RGB(32, 32, 32));

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = ClipboardApp::WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = "ClipboardHistoryClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassA(&wc);

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int startW = 400;
    int startH = 300;
    int startX = workArea.right - startW - 10;
    int startY = workArea.bottom - startH - 10;

    m_hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName, "Clipboard",
        WS_POPUP, startX, startY, startW, startH, NULL, NULL, m_hInstance, this);
    if (!m_hwnd) return false;

    int darkMode = 1;
    DwmSetWindowAttribute(m_hwnd, 20, &darkMode, sizeof(darkMode));
    int cornerPreference = 2;
    DwmSetWindowAttribute(m_hwnd, 33, &cornerPreference, sizeof(cornerPreference));
    int backdropType = 3;
    DwmSetWindowAttribute(m_hwnd, 38, &backdropType, sizeof(backdropType));

    // Kontrolki startują w trybie tekstowym (po lewej stronie okna o szerokości 400)
    m_hSearchBox = CreateWindowExA(0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        10, 10, 380, 20, m_hwnd, (HMENU)2, m_hInstance, NULL);

    g_OriginalEditWndProc = (WNDPROC)SetWindowLongPtr(m_hSearchBox, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    m_hListBox = CreateWindowExA(0, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_WANTKEYBOARDINPUT,
        10, 40, 380, 250, m_hwnd, (HMENU)1, m_hInstance, NULL);

    // Podgląd obrazu po LEWEJ stronie okna (gdy okno rozszerzy się do 680)
    m_hImagePreviewBox = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER,
        10, 10, 270, 280, m_hwnd, (HMENU)3, m_hInstance, NULL);

    RegisterHotKey(m_hwnd, 1, MOD_CONTROL | MOD_NOREPEAT, 0x42);
    AddClipboardFormatListener(m_hwnd);

    AddTrayIcon();
    LoadHistory();
    FilterList();
    UpdateImagePreview();

    CheckFirstRunAndAutostart();

    return true;
}

std::wstring ClipboardApp::GetImagesFolderPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\ClipboardHistory_Images";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }
    return L".";
}

void ClipboardApp::OnClipboardUpdate() {
    if (m_ignoreNextClipboardUpdate) {
        m_ignoreNextClipboardUpdate = false;
        m_lastClipboardSequence = GetClipboardSequenceNumber();
        return;
    }

    DWORD currentSeq = GetClipboardSequenceNumber();
    if (currentSeq == m_lastClipboardSequence) return;
    m_lastClipboardSequence = currentSeq;

    if (!OpenClipboard(m_hwnd)) return;

    if (IsClipboardFormatAvailable(CF_BITMAP)) {
        HBITMAP hBmp = (HBITMAP)GetClipboardData(CF_BITMAP);
        if (hBmp) {
            BITMAP bmpInfo;
            GetObject(hBmp, sizeof(BITMAP), &bmpInfo);

            wchar_t timeBuf[64];
            swprintf_s(timeBuf, L"img_%lld.bmp", GetTickCount64());
            std::wstring imgPath = GetImagesFolderPath() + L"\\" + timeBuf;

            if (SaveHBitmapToBMP(hBmp, imgPath)) {
                auto it = m_history.end();
                for (auto i = m_history.begin(); i != m_history.end(); ++i) {
                    if (i->type == Type_Image && AreFilesIdentical(i->imagePath, imgPath)) {
                        it = i;
                        break;
                    }
                }

                if (it != m_history.end()) {
                    DeleteFileW(imgPath.c_str());
                    if (it != m_history.begin()) {
                        bool wasPinned = it->isPinned;
                        std::wstring desc = it->text;
                        std::wstring path = it->imagePath;
                        m_history.erase(it);
                        m_history.push_front({ Type_Image, desc, path, wasPinned });
                        std::stable_sort(m_history.begin(), m_history.end(), [](const ClipboardItem& a, const ClipboardItem& b) {
                            return a.isPinned > b.isPinned;
                            });
                        FilterList();
                        SaveHistory();
                    }
                }
                else {
                    wchar_t desc[128];
                    swprintf_s(desc, L"[IMAGE] %dx%d px", bmpInfo.bmWidth, bmpInfo.bmHeight);
                    std::wstring itemDesc(desc);

                    m_history.push_front({ Type_Image, itemDesc, imgPath, false });
                    if (m_history.size() > MAX_HISTORY) {
                        auto lastUnpinned = std::find_if(m_history.rbegin(), m_history.rend(), [](const ClipboardItem& item) {
                            return !item.isPinned;
                            });
                        if (lastUnpinned != m_history.rend()) {
                            m_history.erase(std::next(lastUnpinned).base());
                        }
                        else {
                            m_history.pop_back();
                        }
                    }
                    std::stable_sort(m_history.begin(), m_history.end(), [](const ClipboardItem& a, const ClipboardItem& b) {
                        return a.isPinned > b.isPinned;
                        });
                    FilterList();
                    SaveHistory();
                }
            }
        }
        CloseClipboard();
        return;
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* text = static_cast<wchar_t*>(GlobalLock(hData));
        if (text) {
            std::wstring newText(text);
            GlobalUnlock(hData);
            CloseClipboard();

            auto it = std::find_if(m_history.begin(), m_history.end(), [&newText](const ClipboardItem& item) {
                return item.type == Type_Text && item.text == newText;
                });

            if (m_history.empty() || m_history.front().type != Type_Text || m_history.front().text != newText) {
                if (it != m_history.end()) {
                    bool wasPinned = it->isPinned;
                    m_history.erase(it);
                    m_history.push_front({ Type_Text, newText, L"", wasPinned });
                }
                else {
                    m_history.push_front({ Type_Text, newText, L"", false });
                    if (m_history.size() > MAX_HISTORY) {
                        auto lastUnpinned = std::find_if(m_history.rbegin(), m_history.rend(), [](const ClipboardItem& item) {
                            return !item.isPinned;
                            });
                        if (lastUnpinned != m_history.rend()) {
                            m_history.erase(std::next(lastUnpinned).base());
                        }
                        else {
                            m_history.pop_back();
                        }
                    }
                }

                std::stable_sort(m_history.begin(), m_history.end(), [](const ClipboardItem& a, const ClipboardItem& b) {
                    return a.isPinned > b.isPinned;
                    });

                FilterList();
                SaveHistory();
            }
            return;
        }
    }
    CloseClipboard();
}

void ClipboardApp::UpdateImagePreview() {
    if (m_hCurrentPreviewBmp) {
        DeleteObject(m_hCurrentPreviewBmp);
        m_hCurrentPreviewBmp = NULL;
    }

    int index = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
    bool showImage = false;

    if (index != LB_ERR && index < (int)m_filteredIndices.size()) {
        size_t histIndex = m_filteredIndices[index];
        const auto& item = m_history[histIndex];

        if (item.type == Type_Image && !item.imagePath.empty()) {
            showImage = true;
            m_hCurrentPreviewBmp = (HBITMAP)LoadImageW(NULL, item.imagePath.c_str(), IMAGE_BITMAP, 260, 270, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            SendMessage(m_hImagePreviewBox, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hCurrentPreviewBmp);
        }
    }

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int margin = 10;
    int windowH = 300;

    if (showImage) {
        ::ShowWindow(m_hImagePreviewBox, SW_SHOW);
        int newWidth = 680;
        int newX = workArea.right - newWidth - margin; // Okno rośnie w lewo, prawa strona sztywno przykuta do krawędzi ekranu

        SetWindowPos(m_hwnd, HWND_TOPMOST, newX, workArea.bottom - windowH - margin, newWidth, windowH, SWP_NOZORDER | SWP_NOACTIVATE);

        // Wyszukiwarka i lista przesuwają się na prawą stronę okna (X = 290)
        SetWindowPos(m_hSearchBox, NULL, 290, 10, 380, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(m_hListBox, NULL, 290, 40, 380, 250, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else {
        SendMessage(m_hImagePreviewBox, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
        ::ShowWindow(m_hImagePreviewBox, SW_HIDE);
        int newWidth = 400;
        int newX = workArea.right - newWidth - margin; // Okno kurczy się do prawej krawędzi

        SetWindowPos(m_hwnd, HWND_TOPMOST, newX, workArea.bottom - windowH - margin, newWidth, windowH, SWP_NOZORDER | SWP_NOACTIVATE);

        // Wyszukiwarka i lista wracają na lewą stronę kompaktowego okna (X = 10)
        SetWindowPos(m_hSearchBox, NULL, 10, 10, 380, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(m_hListBox, NULL, 10, 40, 380, 250, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void ClipboardApp::ShowWindow() {
    ::ShowWindow(m_hwnd, SW_RESTORE);
    ::ShowWindow(m_hwnd, SW_SHOW);

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int windowW = 400;
    int windowH = 300;
    int margin = 10;

    int x = workArea.right - windowW - margin;
    int y = workArea.bottom - windowH - margin;

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, windowW, windowH, SWP_SHOWWINDOW);
    SetForegroundWindow(m_hwnd);

    SetWindowTextA(m_hSearchBox, "");
    SetFocus(m_hSearchBox);
    UpdateImagePreview();
}

void ClipboardApp::HideWindow() {
    ::ShowWindow(m_hwnd, SW_HIDE);
}

LRESULT CALLBACK ClipboardApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ClipboardApp* app = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<ClipboardApp*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        app = reinterpret_cast<ClipboardApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) return app->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ClipboardApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLIPBOARDUPDATE:
        OnClipboardUpdate();
        return 0;
    case WM_HOTKEY:
        if (wParam == 1) ShowWindow();
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            HideWindow();
        }
        return 0;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            ShowTrayMenu();
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            HideWindow();
            return 0;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            DestroyWindow(hwnd);
            return 0;
        }

        if (LOWORD(wParam) == ID_TRAY_CLEAR) {
            m_history.erase(std::remove_if(m_history.begin(), m_history.end(), [](const ClipboardItem& item) {
                return !item.isPinned;
                }), m_history.end());
            SaveHistory();
            FilterList();
            UpdateImagePreview();
            return 0;
        }

        if (LOWORD(wParam) == 2 && HIWORD(wParam) == EN_CHANGE) {
            FilterList();
            UpdateImagePreview();
            return 0;
        }

        if (LOWORD(wParam) == 1 && (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_DBLCLK)) {
            UpdateImagePreview();

            if (HIWORD(wParam) == LBN_DBLCLK) {
                int index = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR && index < (int)m_filteredIndices.size()) {
                    size_t histIndex = m_filteredIndices[index];
                    const auto& item = m_history[histIndex];

                    m_ignoreNextClipboardUpdate = true;

                    if (OpenClipboard(hwnd)) {
                        EmptyClipboard();

                        if (item.type == Type_Image && !item.imagePath.empty()) {
                            std::ifstream imgFile(item.imagePath, std::ios::binary);
                            if (imgFile) {
                                BITMAPFILEHEADER bmfh;
                                imgFile.read(reinterpret_cast<char*>(&bmfh), sizeof(bmfh));

                                imgFile.seekg(0, std::ios::end);
                                size_t fileSize = (size_t)imgFile.tellg();
                                size_t dibSize = fileSize - sizeof(BITMAPFILEHEADER);

                                imgFile.seekg(sizeof(BITMAPFILEHEADER), std::ios::beg);

                                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
                                if (hMem) {
                                    void* pMem = GlobalLock(hMem);
                                    imgFile.read(static_cast<char*>(pMem), dibSize);
                                    GlobalUnlock(hMem);

                                    SetClipboardData(CF_DIB, hMem);
                                }
                            }
                        }
                        else {
                            size_t memSize = (item.text.length() + 1) * sizeof(wchar_t);
                            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, memSize);
                            if (hMem) {
                                wchar_t* lockedMem = static_cast<wchar_t*>(GlobalLock(hMem));
                                if (lockedMem) {
                                    memcpy(lockedMem, item.text.c_str(), memSize);
                                    GlobalUnlock(hMem);
                                    SetClipboardData(CF_UNICODETEXT, hMem);
                                }
                            }
                        }
                        CloseClipboard();
                    }
                    HideWindow();
                }
            }
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_VKEYTOITEM:
    {
        int vk = LOWORD(wParam);
        int index = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);

        if (vk == VK_ESCAPE) {
            HideWindow();
            return -2;
        }

        UpdateImagePreview();

        if (vk == VK_INSERT) {
            if (index != LB_ERR && index < (int)m_filteredIndices.size()) {
                size_t histIndex = m_filteredIndices[index];
                std::wstring itemText = m_history[histIndex].text;

                auto it = std::find_if(m_history.begin(), m_history.end(), [&itemText](const ClipboardItem& item) {
                    return item.text == itemText;
                    });

                if (it != m_history.end()) {
                    it->isPinned = !it->isPinned;
                }

                std::stable_sort(m_history.begin(), m_history.end(), [](const ClipboardItem& a, const ClipboardItem& b) {
                    return a.isPinned > b.isPinned;
                    });

                FilterList();
                SaveHistory();
                UpdateImagePreview();

                for (size_t i = 0; i < m_filteredIndices.size(); ++i) {
                    if (m_history[m_filteredIndices[i]].text == itemText) {
                        SendMessage(m_hListBox, LB_SETCURSEL, (WPARAM)i, 0);
                        break;
                    }
                }
            }
            return -2;
        }

        if (vk == VK_DELETE) {
            if (index != LB_ERR && index < (int)m_filteredIndices.size()) {
                size_t histIndex = m_filteredIndices[index];

                if (m_history[histIndex].type == Type_Image && !m_history[histIndex].imagePath.empty()) {
                    DeleteFileW(m_history[histIndex].imagePath.c_str());
                }

                m_history.erase(m_history.begin() + histIndex);

                FilterList();

                if (index >= (int)m_filteredIndices.size() && !m_filteredIndices.empty()) {
                    index = (int)m_filteredIndices.size() - 1;
                }
                if (!m_filteredIndices.empty()) {
                    SendMessage(m_hListBox, LB_SETCURSEL, index, 0);
                }
                SaveHistory();
                UpdateImagePreview();
            }
            return -2;
        }
        return -1;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(230, 230, 230));
        SetBkColor(hdc, RGB(32, 32, 32));
        return (LRESULT)m_hDarkBrush;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ClipboardApp::Run() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void ClipboardApp::AddTrayIcon() {
    memset(&m_nid, 0, sizeof(NOTIFYICONDATAA));
    m_nid.cbSize = sizeof(NOTIFYICONDATAA);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    strcpy_s(m_nid.szTip, "Schowek (Ctrl+B)");

    m_nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));

    if (!m_nid.hIcon) {
        m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    Shell_NotifyIconA(NIM_ADD, &m_nid);
}

void ClipboardApp::RemoveTrayIcon() {
    Shell_NotifyIconA(NIM_DELETE, &m_nid);
}

void ClipboardApp::ShowTrayMenu() {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_CLEAR, "Clear History (Keep Pins)");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hwnd, NULL);
    DestroyMenu(hMenu);
}

void ClipboardApp::FilterList() {
    wchar_t query[256];
    GetWindowTextW(m_hSearchBox, query, 256);
    std::wstring q(query);
    std::transform(q.begin(), q.end(), q.begin(), ::towlower);

    SendMessage(m_hListBox, LB_RESETCONTENT, 0, 0);
    m_filteredIndices.clear();

    for (size_t i = 0; i < m_history.size(); ++i) {
        const auto& item = m_history[i];
        std::wstring lowerItem = item.text;
        std::transform(lowerItem.begin(), lowerItem.end(), lowerItem.begin(), ::towlower);

        if (q.empty() || lowerItem.find(q) != std::wstring::npos) {
            m_filteredIndices.push_back(i);

            std::wstring preview = (item.isPinned ? L"[PIN] " : L"   ") + item.text;
            std::replace(preview.begin(), preview.end(), L'\n', L' ');
            std::replace(preview.begin(), preview.end(), L'\r', L' ');
            if (preview.length() > 52) preview = preview.substr(0, 49) + L"...";

            SendMessageW(m_hListBox, LB_ADDSTRING, 0, (LPARAM)preview.c_str());
        }
    }
}

std::wstring ClipboardApp::GetHistoryFilePath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::wstring(path) + L"\\ClipboardHistory.dat";
    }
    return L"ClipboardHistory.dat";
}

void ClipboardApp::SaveHistory() {
    std::ofstream file(GetHistoryFilePath(), std::ios::binary);
    if (!file) return;

    size_t count = m_history.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& item : m_history) {
        file.write(reinterpret_cast<const char*>(&item.type), sizeof(item.type));
        file.write(reinterpret_cast<const char*>(&item.isPinned), sizeof(item.isPinned));

        size_t textLen = item.text.length();
        file.write(reinterpret_cast<const char*>(&textLen), sizeof(textLen));
        file.write(reinterpret_cast<const char*>(item.text.data()), textLen * sizeof(wchar_t));

        size_t pathLen = item.imagePath.length();
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        file.write(reinterpret_cast<const char*>(item.imagePath.data()), pathLen * sizeof(wchar_t));
    }
}

void ClipboardApp::LoadHistory() {
    std::ifstream file(GetHistoryFilePath(), std::ios::binary);
    if (!file) return;

    size_t count = 0;
    if (!file.read(reinterpret_cast<char*>(&count), sizeof(count))) return;
    if (count > MAX_HISTORY) return;

    m_history.clear();
    for (size_t i = 0; i < count; ++i) {
        ItemType type = Type_Text;
        if (!file.read(reinterpret_cast<char*>(&type), sizeof(type))) break;

        bool isPinned = false;
        if (!file.read(reinterpret_cast<char*>(&isPinned), sizeof(isPinned))) break;

        size_t textLen = 0;
        if (!file.read(reinterpret_cast<char*>(&textLen), sizeof(textLen))) break;
        if (textLen > 50000) break;

        std::wstring text(textLen, L'\0');
        if (!file.read(reinterpret_cast<char*>(&text[0]), textLen * sizeof(wchar_t))) break;

        size_t pathLen = 0;
        if (!file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) break;
        if (pathLen > 500) break;

        std::wstring imagePath(pathLen, L'\0');
        if (!file.read(reinterpret_cast<char*>(&imagePath[0]), pathLen * sizeof(wchar_t))) break;

        m_history.push_back({ type, text, imagePath, isPinned });
    }

    std::stable_sort(m_history.begin(), m_history.end(), [](const ClipboardItem& a, const ClipboardItem& b) {
        return a.isPinned > b.isPinned;
        });
}

void ClipboardApp::CheckFirstRunAndAutostart() {
    HKEY hKey;
    DWORD disposition;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\LightClipboardHistory", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        if (disposition == REG_CREATED_NEW_KEY) {
            int result = MessageBoxA(NULL,
                "Would you like the clipboard history to automatically start in the background every time Windows boots?",
                "First Run Setup",
                MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);

            if (result == IDYES) {
                RegisterStartup();
            }
        }
        RegCloseKey(hKey);
    }
}