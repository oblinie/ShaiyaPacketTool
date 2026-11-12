
#include <windows.h>
#include "MainWindow.h"

static HANDLE g_threadHandle = nullptr;
static DWORD g_threadId = 0;
static HANDLE g_readyEvent = nullptr;

static DWORD WINAPI ApplicationThread(LPVOID parameter)
{
    HINSTANCE instance = static_cast<HINSTANCE>(parameter);

    // Ensure that this thread has a message queue.
    MSG message;
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(g_readyEvent);

    int result = RunApplication(instance, SW_SHOW);

    return static_cast<DWORD>(result);
}

extern "C" __declspec(dllexport)
BOOL WINAPI ShaiyaPacketTool_Start(HINSTANCE instance)
{
    if (g_threadHandle != nullptr)
        return TRUE;

    g_readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (!g_readyEvent)
        return FALSE;

    g_threadHandle = CreateThread(
        nullptr,
        0,
        ApplicationThread,
        instance,
        0,
        &g_threadId
    );

    if (!g_threadHandle)
    {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
        return FALSE;
    }

    WaitForSingleObject(g_readyEvent, INFINITE);

    CloseHandle(g_readyEvent);
    g_readyEvent = nullptr;

    return TRUE;
}

extern "C" __declspec(dllexport)
BOOL WINAPI ShaiyaPacketTool_Stop()
{
    if (!g_threadHandle)
        return TRUE;

    PostThreadMessageW(g_threadId, WM_QUIT, 0, 0);

    WaitForSingleObject(g_threadHandle, 5000);

    CloseHandle(g_threadHandle);
    g_threadHandle = nullptr;
    g_threadId = 0;

    return TRUE;
}
