
#pragma once

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#define ID_SERVER_IP       1001
#define ID_SERVER_PORT     1002

#define ID_CONNECT         1003
#define ID_DISCONNECT      1004

#define ID_PACKET_EDITOR   1005
#define ID_SEND_QUEUE      1006
#define ID_STOP_QUEUE      1007
#define ID_CLEAR_EDITOR    1008

#define ID_TRAFFIC_LOG     1009
#define ID_CLEAR_LOG       1010

#define ID_STATUS          1011
#define ID_INTERVAL        1012

#define WM_APP_LOG         (WM_APP + 10)
#define WM_APP_STATUS      (WM_APP + 11)

class MainWindow
{
public:

    MainWindow();
    ~MainWindow();

    int Run(HINSTANCE instance, int showCommand);

private:

    HWND window = nullptr;

    HWND serverIpInput = nullptr;
    HWND serverPortInput = nullptr;

    HWND packetEditor = nullptr;
    HWND trafficLog = nullptr;

    HWND statusLabel = nullptr;
    HWND intervalInput = nullptr;

    HWND connectButton = nullptr;
    HWND disconnectButton = nullptr;
    HWND sendButton = nullptr;
    HWND stopButton = nullptr;

    SOCKET socketHandle = INVALID_SOCKET;

    std::atomic<bool> connected{ false };
    std::atomic<bool> running{ false };

    std::thread receiveThread;
    std::thread queueThread;

    std::mutex socketMutex;
    std::mutex logMutex;

    std::vector<std::vector<unsigned char>> packetQueue;

    unsigned int sendIntervalMs = 50;

    void CreateControls();

    void ConnectToServer();
    void DisconnectFromServer();

    void StartQueue();
    void StopQueue();

    void ReceiveLoop();
    void QueueLoop();

    void AddLog(const std::wstring& message);
    void SetStatus(const std::wstring& message);

    bool ParseHexLine(
        const std::wstring& line,
        std::vector<unsigned char>& output
    );

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    LRESULT HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );
};

int RunApplication(
    HINSTANCE instance,
    int showCommand
);
