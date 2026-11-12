
#pragma once

#include <windows.h>
#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    HWND hwnd = nullptr;

    HWND ipInput = nullptr;
    HWND portInput = nullptr;
    HWND packetEditor = nullptr;
    HWND trafficLog = nullptr;
    HWND statusLabel = nullptr;

    HWND connectButton = nullptr;
    HWND disconnectButton = nullptr;
    HWND sendButton = nullptr;
    HWND clearButton = nullptr;

    SOCKET socketHandle = INVALID_SOCKET;

    std::atomic<bool> connected{ false };
    std::atomic<bool> sending{ false };

    std::thread receiveThread;
    std::thread sendThread;

    std::mutex socketMutex;
    std::mutex logMutex;

    std::vector<std::vector<unsigned char>> packetQueue;

    unsigned int sendIntervalMs = 50;

    void CreateControls();
    void ConnectToServer();
    void DisconnectFromServer();

    void StartPacketQueue();
    void StopPacketQueue();

    void ReceiveLoop();
    void SendLoop();

    void AddLog(const std::wstring& message);
    void UpdateStatus(const std::wstring& message);

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

int RunApplication(HINSTANCE hInstance, int nCmdShow);
