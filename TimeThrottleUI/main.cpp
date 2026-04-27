#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui_app.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    return RunUiApp(hInstance, nCmdShow);
}
