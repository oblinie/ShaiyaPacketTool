#include <winsock2.h>
#include <windows.h>

#include "MainWindow.h"

static HANDLE g_threadHandle = nullptr;
static DWORD g_threadId = 0;
static HANDLE g_readyEvent = nullptr;

static DWORD WINAPI ApplicationThread(LPVOID parameter)
{
    HINSTANCE instance = static_cast<HINSTANCE>(parameter);

    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    if (g_readyEvent)
        SetEvent(g_readyEvent);

    return static_cast<DWORD>(RunApplication(instance, SW_SHOW));
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

    DWORD waitResult = WaitForSingleObject(g_readyEvent, 5000);

    CloseHandle(g_readyEvent);
    g_readyEvent = nullptr;

    if (waitResult != WAIT_OBJECT_0)
    {
        PostThreadMessageW(g_threadId, WM_QUIT, 0, 0);
        WaitForSingleObject(g_threadHandle, 5000);

        CloseHandle(g_threadHandle);
        g_threadHandle = nullptr;
        g_threadId = 0;

        return FALSE;
    }

    return TRUE;
}

extern "C" __declspec(dllexport)
BOOL WINAPI ShaiyaPacketTool_Stop()
{
    if (!g_threadHandle)
        return TRUE;

    if (!PostThreadMessageW(g_threadId, WM_QUIT, 0, 0))
        return FALSE;

    DWORD waitResult = WaitForSingleObject(g_threadHandle, 5000);

    if (waitResult != WAIT_OBJECT_0)
        return FALSE;

    CloseHandle(g_threadHandle);
    g_threadHandle = nullptr;
    g_threadId = 0;

    return TRUE;
}
