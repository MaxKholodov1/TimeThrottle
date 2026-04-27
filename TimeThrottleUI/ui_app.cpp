#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

#include "ui_app.h"
#include "control_pipe_client.h"

namespace {
constexpr wchar_t kWindowClassName[] = L"TimeThrottleUIWindow";
constexpr wchar_t kWindowTitle[] = L"SYS://THROTTLE-CONSOLE";

constexpr COLORREF kBgColor = RGB(8, 10, 8);
constexpr COLORREF kPanelColor = RGB(3, 6, 3);
constexpr COLORREF kTextColor = RGB(72, 255, 110);
constexpr COLORREF kGridColor = RGB(18, 70, 30);

enum ControlId : int {
    IDC_MODE_OFF = 1001,
    IDC_MODE_NO_INTERNET = 1002,
    IDC_MODE_NO_MSG = 1003,
    IDC_MODE_NO_VIDEO = 1004,
    IDC_MODE_NO_SOCIAL_VIDEO = 1005,
    IDC_MODE_CUSTOM = 1006,
    IDC_STATUS = 1101,
    IDC_KILL = 1102,
    IDC_MINUTES = 1201,
    IDC_ADD_DOMAIN = 1301,
    IDC_DOMAIN_INPUT = 1302,
    IDC_LOG = 1401,
};

struct UiState {
    HWND hwnd = nullptr;
    HWND minutesInput = nullptr;
    HWND domainInput = nullptr;
    HWND logBox = nullptr;
    HFONT fontUi = nullptr;
    HFONT fontMono = nullptr;
    HBRUSH bgBrush = nullptr;
    HBRUSH panelBrush = nullptr;
};

UiState g_ui;

std::wstring ReadEditText(HWND hEdit) {
    const int len = GetWindowTextLengthW(hEdit);
    std::wstring out(static_cast<size_t>(len) + 1, L'\0');
    if (len > 0) {
        GetWindowTextW(hEdit, out.data(), len + 1);
        out.resize(static_cast<size_t>(len));
    } else {
        out.clear();
    }
    return out;
}

void AppendLog(const std::wstring& line) {
    if (!g_ui.logBox) return;

    const int oldLen = GetWindowTextLengthW(g_ui.logBox);
    SendMessageW(g_ui.logBox, EM_SETSEL, oldLen, oldLen);
    SendMessageW(g_ui.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
    SendMessageW(g_ui.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L"\r\n"));
}

void ApplyControlFont(HWND hwnd) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(g_ui.fontUi), TRUE);
}

void CreateControls(HWND hwnd) {
    HWND editMinutes = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"30",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        16, 20, 70, 24, hwnd, reinterpret_cast<HMENU>(IDC_MINUTES), nullptr, nullptr
    );
    g_ui.minutesInput = editMinutes;
    ApplyControlFont(editMinutes);

    CreateWindowW(L"STATIC", L"minutes",
                  WS_CHILD | WS_VISIBLE,
                  94, 24, 70, 20,
                  hwnd, nullptr, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"> ACCESS: OPERATOR",
                  WS_CHILD | WS_VISIBLE,
                  186, 24, 160, 20,
                  hwnd, nullptr, nullptr, nullptr);

    struct BtnDef { int id; const wchar_t* text; int x; int y; int w; };
    const BtnDef buttons[] = {
        {IDC_MODE_OFF, L"MODE OFF", 16, 80, 160},
        {IDC_MODE_NO_INTERNET, L"NO INTERNET", 186, 80, 160},
        {IDC_MODE_NO_MSG, L"NO MESSENGER", 16, 116, 160},
        {IDC_MODE_NO_VIDEO, L"NO VIDEO", 186, 116, 160},
        {IDC_MODE_NO_SOCIAL_VIDEO, L"NO SOCIAL+VIDEO", 16, 152, 160},
        {IDC_MODE_CUSTOM, L"CUSTOM", 186, 152, 160},
    };

    for (const auto& b : buttons) {
        HWND btn = CreateWindowW(L"BUTTON", b.text,
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                                 b.x, b.y, b.w, 30,
                                 hwnd, reinterpret_cast<HMENU>(b.id), nullptr, nullptr);
        ApplyControlFont(btn);
    }

    HWND statusBtn = CreateWindowW(L"BUTTON", L"MODE STATUS",
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                                   16, 192, 160, 30,
                                   hwnd, reinterpret_cast<HMENU>(IDC_STATUS), nullptr, nullptr);
    HWND killBtn = CreateWindowW(L"BUTTON", L"KILL CORE",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                                 186, 192, 160, 30,
                                 hwnd, reinterpret_cast<HMENU>(IDC_KILL), nullptr, nullptr);
    ApplyControlFont(statusBtn);
    ApplyControlFont(killBtn);

    HWND domainEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"example.com",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        16, 236, 240, 24, hwnd, reinterpret_cast<HMENU>(IDC_DOMAIN_INPUT), nullptr, nullptr
    );
    g_ui.domainInput = domainEdit;
    ApplyControlFont(domainEdit);

    HWND addDomainBtn = CreateWindowW(L"BUTTON", L"ADD DOMAIN",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                                      266, 236, 80, 24,
                                      hwnd, reinterpret_cast<HMENU>(IDC_ADD_DOMAIN), nullptr, nullptr);
    ApplyControlFont(addDomainBtn);

    HWND logBox = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
        16, 272, 330, 190, hwnd, reinterpret_cast<HMENU>(IDC_LOG), nullptr, nullptr
    );
    g_ui.logBox = logBox;
    SendMessageW(logBox, WM_SETFONT, reinterpret_cast<WPARAM>(g_ui.fontMono), TRUE);

    AppendLog(L"[BOOT] UI panel online");
    AppendLog(L"[BOOT] channel=TimeThrottleControlPipe");
    AppendLog(L"[BOOT] awaiting operator commands...");
}

std::wstring BuildModeCommand(const std::wstring& mode) {
    std::wstring mins = ReadEditText(g_ui.minutesInput);
    if (mins.empty()) {
        mins = L"0";
    }
    return L"mode " + mode + L" " + mins;
}

void HandleCommand(int id) {
    std::wstring cmd;
    switch (id) {
        case IDC_MODE_OFF: cmd = BuildModeCommand(L"off"); break;
        case IDC_MODE_NO_INTERNET: cmd = BuildModeCommand(L"no_internet"); break;
        case IDC_MODE_NO_MSG: cmd = BuildModeCommand(L"no_messenger_only"); break;
        case IDC_MODE_NO_VIDEO: cmd = BuildModeCommand(L"no_video_and_streaming"); break;
        case IDC_MODE_NO_SOCIAL_VIDEO: cmd = BuildModeCommand(L"no_social_media_and_no_video"); break;
        case IDC_MODE_CUSTOM: cmd = BuildModeCommand(L"custom"); break;
        case IDC_STATUS: cmd = L"mode_status"; break;
        case IDC_KILL: cmd = L"kill"; break;
        case IDC_ADD_DOMAIN: {
            const std::wstring domain = ReadEditText(g_ui.domainInput);
            if (domain.empty()) {
                AppendLog(L"[UI] domain is empty");
                return;
            }
            cmd = L"add " + domain;
            break;
        }
        default: return;
    }

    AppendLog(L"> " + cmd);
    AppendLog(SendCommandToCore(cmd));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_ui.hwnd = hwnd;
            g_ui.bgBrush = CreateSolidBrush(kBgColor);
            g_ui.panelBrush = CreateSolidBrush(kPanelColor);
            g_ui.fontUi = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      FIXED_PITCH, L"Lucida Console");
            g_ui.fontMono = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                        FIXED_PITCH, L"Terminal");
            CreateControls(hwnd);
            return 0;
        }
        case WM_COMMAND: {
            if (HIWORD(wParam) == BN_CLICKED) {
                HandleCommand(LOWORD(wParam));
            }
            return 0;
        }
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, kTextColor);
            SetBkColor(hdc, kBgColor);
            return reinterpret_cast<INT_PTR>(g_ui.bgBrush);
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, kTextColor);
            SetBkColor(hdc, kPanelColor);
            return reinterpret_cast<INT_PTR>(g_ui.panelBrush);
        }
        case WM_ERASEBKGND: {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            FillRect(reinterpret_cast<HDC>(wParam), &rc, g_ui.bgBrush);
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc{};
            GetClientRect(hwnd, &rc);

            HPEN gridPen = CreatePen(PS_SOLID, 1, kGridColor);
            HGDIOBJ oldPen = SelectObject(hdc, gridPen);
            for (int y = 0; y < rc.bottom; y += 4) {
                MoveToEx(hdc, 0, y, nullptr);
                LineTo(hdc, rc.right, y);
            }

            SelectObject(hdc, oldPen);
            DeleteObject(gridPen);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY: {
            if (g_ui.fontUi) DeleteObject(g_ui.fontUi);
            if (g_ui.fontMono) DeleteObject(g_ui.fontMono);
            if (g_ui.bgBrush) DeleteObject(g_ui.bgBrush);
            if (g_ui.panelBrush) DeleteObject(g_ui.panelBrush);
            PostQuitMessage(0);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
} // namespace

int RunUiApp(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0, kWindowClassName, kWindowTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 520,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
