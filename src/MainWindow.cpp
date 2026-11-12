
#include "MainWindow.h"

#include <windows.h>
#include <string>
#include <vector>

#define ID_EDIT       1001
#define ID_START      1002
#define ID_STOP       1003
#define ID_CLEAR      1004
#define ID_STATUS     1005

#define WM_START_QUEUE (WM_APP + 1)

static HWND g_edit = nullptr;
static HWND g_status = nullptr;
static std::vector<std::wstring> g_queue;
static size_t g_current = 0;
static bool g_running = false;

static const UINT TIMER_ID = 1;
static const UINT SEND_INTERVAL = 50;

static void UpdateStatus(const std::wstring& value)
{
    SetWindowTextW(g_status, value.c_str());
}

static void StartQueue(HWND hwnd)
{
    if (g_running)
        return;

    int length = GetWindowTextLengthW(g_edit);
    std::wstring content(length + 1, L'\0');

    GetWindowTextW(g_edit, content.data(), length + 1);
    content.resize(length);

    g_queue.clear();
    g_current = 0;

    size_t start = 0;

    while (start <= content.size())
    {
        size_t end = content.find(L'\n', start);

        if (end == std::wstring::npos)
            end = content.size();

        std::wstring line = content.substr(start, end - start);

        while (!line.empty() &&
               (line.back() == L'\r' || line.back() == L' '))
            line.pop_back();

        size_t first = line.find_first_not_of(L' ');

        if (first != std::wstring::npos)
            g_queue.push_back(line.substr(first));

        if (end == content.size())
            break;

        start = end + 1;
    }

    if (g_queue.empty())
    {
        UpdateStatus(L"Queue is empty.");
        return;
    }

    g_running = true;

    UpdateStatus(L"Starting local queue simulation...");

    SetTimer(hwnd, TIMER_ID, SEND_INTERVAL, nullptr);
}

static void StopQueue(HWND hwnd)
{
    KillTimer(hwnd, TIMER_ID);
    g_running = false;
    UpdateStatus(L"Queue stopped.");
}

static void ProcessQueue(HWND hwnd)
{
    if (!g_running)
        return;

    if (g_current >= g_queue.size())
    {
        StopQueue(hwnd);
        UpdateStatus(L"Simulation completed.");
        return;
    }

    std::wstring status =
        L"Processing line " +
        std::to_wstring(g_current + 1) +
        L" / " +
        std::to_wstring(g_queue.size());

    UpdateStatus(status);

    // Local simulation only.
    // No game connection or network packet transmission.

    g_current++;

    if (g_current >= g_queue.size())
    {
        KillTimer(hwnd, TIMER_ID);
        g_running = false;
        UpdateStatus(L"Simulation completed.");
    }
}

static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(
            L"STATIC", L"Packet Queue",
            WS_CHILD | WS_VISIBLE,
            15, 12, 200, 22,
            hwnd, nullptr, nullptr, nullptr);

        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE |
            WS_VSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_WANTRETURN,
            15, 40, 550, 260,
            hwnd,
            (HMENU)ID_EDIT,
            nullptr, nullptr);

        CreateWindowW(
            L"BUTTON", L"Start",
            WS_CHILD | WS_VISIBLE,
            15, 315, 100, 30,
            hwnd, (HMENU)ID_START, nullptr, nullptr);

        CreateWindowW(
            L"BUTTON", L"Stop",
            WS_CHILD | WS_VISIBLE,
            125, 315, 100, 30,
            hwnd, (HMENU)ID_STOP, nullptr, nullptr);

        CreateWindowW(
            L"BUTTON", L"Clear",
            WS_CHILD | WS_VISIBLE,
            235, 315, 100, 30,
            hwnd, (HMENU)ID_CLEAR, nullptr, nullptr);

        g_status = CreateWindowW(
            L"STATIC", L"Ready.",
            WS_CHILD | WS_VISIBLE,
            15, 365, 550, 25,
            hwnd, (HMENU)ID_STATUS, nullptr, nullptr);

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_START:
            StartQueue(hwnd);
            return 0;

        case ID_STOP:
            StopQueue(hwnd);
            return 0;

        case ID_CLEAR:
            StopQueue(hwnd);
            SetWindowTextW(g_edit, L"");
            g_queue.clear();
            g_current = 0;
            UpdateStatus(L"Cleared.");
            return 0;
        }
        break;

    case WM_TIMER:
        if (wParam == TIMER_ID)
            ProcessQueue(hwnd);
        return 0;

    case WM_CLOSE:
        StopQueue(hwnd);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int RunApplication(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ShaiyaPacketToolWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Shaiya Packet Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 470,
        nullptr, nullptr,
        hInstance, nullptr);

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
