#include "ClipboardApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {



    ClipboardApp app(hInstance);
    if (!app.Initialize()) {
        return -1;
    }

    app.Run();

    return 0;
}