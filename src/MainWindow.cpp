
#include "MainWindow.h"

#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#define ID_IP          1001
#define ID_PORT        1002
#define ID_CONNECT     1003
#define ID_DISCONNECT  1004
#define ID_PACKETS     1005
#define ID_SEND        1006
#define ID_STOP        1007
#define ID_CLEAR       1008
#define ID_LOG         1009
#define ID_STATUS      1010

#define WM_LOG_MESSAGE (WM_APP + 10)

static const wchar_t WINDOW_CLASS[] =
    L"ShaiyaPacketToolWindow";

static WNDPROC g_oldEditProc = nullptr;

static LRESULT CALLBACK PacketEditProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == VK_RETURN)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
            return CallWindowProcW(
                g_oldEditProc, hwnd, msg, wParam, lParam);

        PostMessageW(
            GetParent(hwnd), WM_COMMAND,
            MAKEWPARAM(ID_SEND, BN_CLICKED), 0);

        return 0;
    }

    return CallWindowProcW(
        g_oldEditProc, hwnd, msg, wParam, lParam);
}

MainWindow::MainWindow() {}

MainWindow::~MainWindow()
{
    DisconnectFromServer();
    WSACleanup();
}

void MainWindow::AddLog(const std::wstring& message)
{
    auto* text = new std::wstring(message);

    if (!PostMessageW(hwnd, WM_LOG_MESSAGE, 0,
                      reinterpret_cast<LPARAM>(text)))
        delete text;
}

void MainWindow::UpdateStatus(const std::wstring& message)
{
    SetWindowTextW(statusLabel, message.c_str());
}

void MainWindow::CreateControls()
{
    CreateWindowW(L"STATIC", L"Server IP:",
        WS_CHILD | WS_VISIBLE,
        15, 15, 75, 22, hwnd, nullptr, nullptr, nullptr);

    ipInput = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"127.0.0.1",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        90, 12, 180, 25, hwnd,
        (HMENU)ID_IP, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Port:",
        WS_CHILD | WS_VISIBLE,
        290, 15, 45, 22, hwnd, nullptr, nullptr, nullptr);

    portInput = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"30800",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        335, 12, 80, 25, hwnd,
        (HMENU)ID_PORT, nullptr, nullptr);

    connectButton = CreateWindowW(
        L"BUTTON", L"Connect",
        WS_CHILD | WS_VISIBLE,
        435, 10, 100, 30, hwnd,
        (HMENU)ID_CONNECT, nullptr, nullptr);

    disconnectButton = CreateWindowW(
        L"BUTTON", L"Disconnect",
        WS_CHILD | WS_VISIBLE,
        545, 10, 110, 30, hwnd,
        (HMENU)ID_DISCONNECT, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"HEX Packet Editor",
        WS_CHILD | WS_VISIBLE,
        15, 55, 200, 22, hwnd, nullptr, nullptr, nullptr);

    packetEditor = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL |
        ES_WANTRETURN,
        15, 80, 640, 180, hwnd,
        (HMENU)ID_PACKETS, nullptr, nullptr);

    g_oldEditProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(
            packetEditor, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(PacketEditProc)));

    sendButton = CreateWindowW(
        L"BUTTON", L"Send All (Enter)",
        WS_CHILD | WS_VISIBLE,
        15, 270, 145, 32, hwnd,
        (HMENU)ID_SEND, nullptr, nullptr);

        CreateWindowW(
        L"BUTTON", L"Stop Queue",
        WS_CHILD | WS_VISIBLE,
        165, 270, 110, 32,
        hwnd,
        (HMENU)ID_STOP,
        nullptr,
        nullptr
        );

    CreateWindowW(L"STATIC", L"Interval: 50 ms",
        WS_CHILD | WS_VISIBLE,
        175, 278, 150, 22, hwnd, nullptr, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Traffic Monitor",
        WS_CHILD | WS_VISIBLE,
        15, 320, 200, 22, hwnd, nullptr, nullptr, nullptr);

    trafficLog = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL |
        ES_READONLY,
        15, 345, 640, 220, hwnd,
        (HMENU)ID_LOG, nullptr, nullptr);

    statusLabel = CreateWindowW(
        L"STATIC", L"Disconnected",
        WS_CHILD | WS_VISIBLE,
        15, 580, 640, 25, hwnd,
        (HMENU)ID_STATUS, nullptr, nullptr);

    clearButton = CreateWindowW(
        L"BUTTON", L"Clear Log",
        WS_CHILD | WS_VISIBLE,
        545, 575, 110, 30, hwnd,
        (HMENU)ID_CLEAR, nullptr, nullptr);
}

void MainWindow::ConnectToServer()
{
    if (connected)
        return;

    wchar_t ip[128] = {};
    wchar_t portText[16] = {};

    GetWindowTextW(ipInput, ip, 128);
    GetWindowTextW(portInput, portText, 16);

    int port = _wtoi(portText);

    if (port < 1 || port > 65535)
    {
        UpdateStatus(L"Invalid port.");
        return;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (s == INVALID_SOCKET)
    {
        UpdateStatus(L"Socket creation failed.");
        return;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(
        static_cast<u_short>(port));

    if (InetPtonW(AF_INET, ip, &address.sin_addr) != 1)
    {
        closesocket(s);
        UpdateStatus(L"Invalid IPv4 address.");
        return;
    }

    if (connect(s,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) == SOCKET_ERROR)
    {
        closesocket(s);
        UpdateStatus(L"Connection failed.");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        socketHandle = s;
    }

    connected = true;

    AddLog(L"TCP connection established.");
    UpdateStatus(L"Connected.");

    receiveThread = std::thread(
        &MainWindow::ReceiveLoop, this);
}

void MainWindow::DisconnectFromServer()
{
    StopPacketQueue();

    SOCKET s = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(socketMutex);

        s = socketHandle;

        if (s != INVALID_SOCKET)
            shutdown(s, SD_BOTH);

        socketHandle = INVALID_SOCKET;
    }

    connected = false;

    if (receiveThread.joinable())
        receiveThread.join();

    if (s != INVALID_SOCKET)
        closesocket(s);

    UpdateStatus(L"Disconnected.");
}

void MainWindow::ReceiveLoop()
{
    char buffer[4096];

    while (connected)
    {
        SOCKET s;

        {
            std::lock_guard<std::mutex> lock(socketMutex);
            s = socketHandle;
        }

        if (s == INVALID_SOCKET)
            break;

        int received = recv(s, buffer, sizeof(buffer), 0);

        if (received <= 0)
            break;

        std::wstringstream output;

        output << L"RECV [" << received << L" bytes]: ";

        for (int i = 0; i < received; i++)
        {
            wchar_t hex[4];
            swprintf_s(hex, L"%02X ",
                static_cast<unsigned char>(buffer[i]));
            output << hex;
        }

        AddLog(output.str());
    }

    connected = false;
    AddLog(L"Receive loop ended.");
}

void MainWindow::StartPacketQueue()
{
    if (!connected || sending)
    {
        UpdateStatus(L"Connect first.");
        return;
    }

    int length = GetWindowTextLengthW(packetEditor);

    std::wstring content(length + 1, L'\0');

    GetWindowTextW(
        packetEditor, content.data(), length + 1);

    content.resize(length);

    packetQueue.clear();

    std::wistringstream lines(content);
    std::wstring line;

    while (std::getline(lines, line))
    {
        std::wistringstream words(line);
        std::wstring token;

        std::vector<unsigned char> packet;
        bool valid = true;

        while (words >> token)
        {
            if (token.size() != 2)
            {
                valid = false;
                break;
            }

            try
            {
                size_t position = 0;
                unsigned long value =
                    std::stoul(token, &position, 16);

                if (position != token.size() || value > 255)
                {
                    valid = false;
                    break;
                }

                packet.push_back(
                    static_cast<unsigned char>(value));
            }
            catch (...)
            {
                valid = false;
                break;
            }
        }

        if (!valid)
        {
            UpdateStatus(L"Invalid HEX line.");
            packetQueue.clear();
            return;
        }

        if (!packet.empty())
            packetQueue.push_back(packet);
    }

    if (packetQueue.empty())
    {
        UpdateStatus(L"No valid packets.");
        return;
    }

    if (sendThread.joinable())
        sendThread.join();

    sending = true;

    sendThread = std::thread(
        &MainWindow::SendLoop, this);
}


void MainWindow::SendLoop()
{
    for (size_t i = 0; i < packetQueue.size(); ++i)
    {
        if (!connected || !sending)
            break;

        SOCKET currentSocket;

        {
            std::lock_guard<std::mutex> lock(socketMutex);
            currentSocket = socketHandle;
        }

        if (currentSocket == INVALID_SOCKET)
            break;

        const auto& packet = packetQueue[i];

        size_t totalSent = 0;
        bool success = true;

        while (totalSent < packet.size())
        {
            int result = send(
                currentSocket,
                reinterpret_cast<const char*>(
                    packet.data() + totalSent),
                static_cast<int>(packet.size() - totalSent),
                0
            );

            if (result == SOCKET_ERROR || result == 0)
            {
                success = false;
                break;
            }

            totalSent += static_cast<size_t>(result);
        }

        if (!success)
        {
            AddLog(L"TCP send failed.");
            connected = false;
            break;
        }

        std::wstringstream output;
        output << L"SENT [" << packet.size() << L" bytes]: ";

        for (unsigned char byte : packet)
        {
            wchar_t hex[4];
            swprintf_s(hex, L"%02X ", byte);
            output << hex;
        }

        AddLog(output.str());

        std::this_thread::sleep_for(
            std::chrono::milliseconds(sendIntervalMs)
        );
    }

    sending = false;
    AddLog(L"Packet queue completed.");
}


void MainWindow::StopPacketQueue()
{
    sending = false;

    if (sendThread.joinable())
        sendThread.join();
}

LRESULT CALLBACK MainWindow::WindowProc(
    HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    MainWindow* self =
        reinterpret_cast<MainWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        auto* create =
            reinterpret_cast<CREATESTRUCTW*>(lParam);

        self = static_cast<MainWindow*>(
            create->lpCreateParams);

        self->hwnd = hwnd;

        SetWindowLongPtrW(
            hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self));
    }

    if (self)
        return self->HandleMessage(
            message, wParam, lParam);

    return DefWindowProcW(
        hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(
    UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateControls();
        return 0;

    case WM_LOG_MESSAGE:
    {
        auto* messageText =
            reinterpret_cast<std::wstring*>(lParam);

        if (messageText)
        {
            int length = GetWindowTextLengthW(trafficLog);

            SendMessageW(
                trafficLog, EM_SETSEL,
                length, length);

            SendMessageW(
                trafficLog, EM_REPLACESEL,
                FALSE,
                reinterpret_cast<LPARAM>(
                    (messageText->append(L"\r\n")).c_str()));

            delete messageText;
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_CONNECT:
            ConnectToServer();
            return 0;

        case ID_DISCONNECT:
            DisconnectFromServer();
            return 0;

        case ID_SEND:
            StartPacketQueue();
            return 0;

        case ID_STOP:
            StopPacketQueue();
            return 0;

        case ID_CLEAR:
            SetWindowTextW(trafficLog, L"");
            return 0;
        }
        break;

    case WM_CLOSE:
        DisconnectFromServer();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(
        hwnd, message, wParam, lParam);
}

int MainWindow::Run(
    HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassW(&wc);

    HWND window = CreateWindowExW(
        0,
        WINDOW_CLASS,
        L"Shaiya Packet Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 680,
        nullptr, nullptr,
        hInstance, this);

    if (!window)
        return 1;

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

int RunApplication(
    HINSTANCE hInstance, int nCmdShow)
{
    WSADATA data = {};

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return 1;

    MainWindow application;

    return application.Run(hInstance, nCmdShow);
}
