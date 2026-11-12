
#include "MainWindow.h"

#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

static const wchar_t CLASS_NAME[] =
    L"ShaiyaPacketToolWindow";

MainWindow::MainWindow() {}

MainWindow::~MainWindow()
{
    DisconnectFromServer();
}

void MainWindow::AddLog(const std::wstring& message)
{
    if (!window) return;

    auto* text = new std::wstring(message);

    if (!PostMessageW(window, WM_APP_LOG, 0,
        reinterpret_cast<LPARAM>(text)))
        delete text;
}

void MainWindow::SetStatus(const std::wstring& message)
{
    if (!window) return;

    auto* text = new std::wstring(message);

    if (!PostMessageW(window, WM_APP_STATUS, 0,
        reinterpret_cast<LPARAM>(text)))
        delete text;
}

void MainWindow::CreateControls()
{
    CreateWindowW(L"STATIC", L"Server IP:",
        WS_CHILD | WS_VISIBLE,
        15, 15, 75, 22, window, nullptr, nullptr, nullptr);

    serverIpInput = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"127.0.0.1",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        90, 12, 190, 25, window,
        (HMENU)ID_SERVER_IP, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Port:",
        WS_CHILD | WS_VISIBLE,
        295, 15, 40, 22, window, nullptr, nullptr, nullptr);

    serverPortInput = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"30800",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        335, 12, 80, 25, window,
        (HMENU)ID_SERVER_PORT, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Connect",
        WS_CHILD | WS_VISIBLE,
        430, 10, 100, 30, window,
        (HMENU)ID_CONNECT, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Disconnect",
        WS_CHILD | WS_VISIBLE,
        540, 10, 110, 30, window,
        (HMENU)ID_DISCONNECT, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"HEX Packet Editor",
        WS_CHILD | WS_VISIBLE,
        15, 55, 200, 22, window, nullptr, nullptr, nullptr);

    packetEditor = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        15, 80, 640, 180, window,
        (HMENU)ID_PACKET_EDITOR, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Send All",
        WS_CHILD | WS_VISIBLE,
        15, 270, 110, 32, window,
        (HMENU)ID_SEND_QUEUE, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Stop Queue",
        WS_CHILD | WS_VISIBLE,
        135, 270, 110, 32, window,
        (HMENU)ID_STOP_QUEUE, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Interval (ms):",
        WS_CHILD | WS_VISIBLE,
        270, 278, 90, 22, window, nullptr, nullptr, nullptr);

    intervalInput = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"50",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        365, 273, 70, 25, window,
        (HMENU)ID_INTERVAL, nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Traffic Monitor",
        WS_CHILD | WS_VISIBLE,
        15, 320, 200, 22, window, nullptr, nullptr, nullptr);

    trafficLog = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        15, 345, 640, 220, window,
        (HMENU)ID_TRAFFIC_LOG, nullptr, nullptr);

    statusLabel = CreateWindowW(L"STATIC", L"Disconnected",
        WS_CHILD | WS_VISIBLE,
        15, 580, 400, 22, window,
        (HMENU)ID_STATUS, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Clear Log",
        WS_CHILD | WS_VISIBLE,
        540, 575, 110, 30, window,
        (HMENU)ID_CLEAR_LOG, nullptr, nullptr);
}

bool MainWindow::ParseHexLine(
    const std::wstring& line,
    std::vector<unsigned char>& output)
{
    output.clear();

    std::wistringstream stream(line);
    std::wstring token;

    while (stream >> token)
    {
        if (token.size() != 2)
            return false;

        try
        {
            size_t position = 0;
            unsigned long value = std::stoul(token, &position, 16);

            if (position != token.size() || value > 255)
                return false;

            output.push_back(
                static_cast<unsigned char>(value));
        }
        catch (...)
        {
            return false;
        }
    }

    return !output.empty();
}

void MainWindow::ConnectToServer()
{
    if (connected)
        return;

    wchar_t ip[128] = {};
    wchar_t portText[16] = {};

    GetWindowTextW(serverIpInput, ip, 128);
    GetWindowTextW(serverPortInput, portText, 16);

    int port = _wtoi(portText);

    if (port < 1 || port > 65535)
    {
        SetStatus(L"Invalid port.");
        return;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (s == INVALID_SOCKET)
    {
        SetStatus(L"Socket creation failed.");
        return;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(port));

    if (InetPtonW(AF_INET, ip, &address.sin_addr) != 1)
    {
        closesocket(s);
        SetStatus(L"Invalid IPv4 address.");
        return;
    }

    SetStatus(L"Connecting...");

    if (connect(s, reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) == SOCKET_ERROR)
    {
        closesocket(s);
        SetStatus(L"Connection failed.");
        AddLog(L"TCP connection failed.");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        socketHandle = s;
    }

    connected = true;

    AddLog(L"TCP connection established.");
    SetStatus(L"Connected.");

    receiveThread = std::thread(
        &MainWindow::ReceiveLoop, this);
}

void MainWindow::DisconnectFromServer()
{
    StopQueue();

    SOCKET s = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(socketMutex);

        s = socketHandle;

        if (s != INVALID_SOCKET)
            shutdown(s, SD_BOTH);
    }

    connected = false;

    if (receiveThread.joinable())
        receiveThread.join();

    {
        std::lock_guard<std::mutex> lock(socketMutex);

        if (socketHandle != INVALID_SOCKET)
        {
            closesocket(socketHandle);
            socketHandle = INVALID_SOCKET;
        }
    }

    SetStatus(L"Disconnected.");
    AddLog(L"Disconnected.");
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

        for (int i = 0; i < received; ++i)
        {
            wchar_t hex[4];
            swprintf_s(hex, L"%02X ",
                static_cast<unsigned char>(buffer[i]));
            output << hex;
        }

        AddLog(output.str());
    }

    connected = false;
    SetStatus(L"Connection closed.");
}

void MainWindow::StartQueue()
{
    if (!connected)
    {
        SetStatus(L"Connect to your test server first.");
        return;
    }

    if (running)
        return;

    int length = GetWindowTextLengthW(packetEditor);

    std::wstring content(length + 1, L'\0');

    GetWindowTextW(packetEditor, content.data(), length + 1);
    content.resize(length);

    packetQueue.clear();

    std::wistringstream lines(content);
    std::wstring line;

    while (std::getline(lines, line))
    {
        std::vector<unsigned char> packet;

        if (line.find_first_not_of(L" \t\r") == std::wstring::npos)
            continue;

        if (!ParseHexLine(line, packet))
        {
            SetStatus(L"Invalid HEX line. Queue not started.");
            packetQueue.clear();
            return;
        }

        packetQueue.push_back(packet);
    }

    if (packetQueue.empty())
    {
        SetStatus(L"Packet list is empty.");
        return;
    }

    wchar_t interval[16] = {};
    GetWindowTextW(intervalInput, interval, 16);

    int value = _wtoi(interval);

    if (value < 10 || value > 60000)
    {
        SetStatus(L"Interval must be 10-60000 ms.");
        return;
    }

    sendIntervalMs = static_cast<unsigned int>(value);

    running = true;

    queueThread = std::thread(
        &MainWindow::QueueLoop, this);

    SetStatus(L"Queue started.");
}

void MainWindow::QueueLoop()
{
    for (size_t i = 0; i < packetQueue.size(); ++i)
    {
        if (!running || !connected)
            break;

        SOCKET s;

        {
            std::lock_guard<std::mutex> lock(socketMutex);
            s = socketHandle;
        }

        if (s == INVALID_SOCKET)
            break;

        const auto& packet = packetQueue[i];

        size_t offset = 0;
        bool success = true;

        while (offset < packet.size() && running && connected)
        {
            int sent;

            {
                std::lock_guard<std::mutex> lock(socketMutex);

                if (socketHandle == INVALID_SOCKET)
                    break;

                sent = send(
                    socketHandle,
                    reinterpret_cast<const char*>(
                        packet.data() + offset),
                    static_cast<int>(packet.size() - offset),
                    0);
            }

            if (sent <= 0)
            {
                success = false;
                break;
            }

            offset += static_cast<size_t>(sent);
        }

        if (!success || offset != packet.size())
        {
            AddLog(L"TCP send failed.");
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

        if (i + 1 < packetQueue.size())
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(sendIntervalMs));
        }
    }

    running = false;
    SetStatus(L"Queue finished or stopped.");
}

void MainWindow::StopQueue()
{
    running = false;

    if (queueThread.joinable())
        queueThread.join();
}

LRESULT CALLBACK MainWindow::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    MainWindow* self = reinterpret_cast<MainWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);

        self = static_cast<MainWindow*>(create->lpCreateParams);
        self->window = hwnd;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self));
    }

    if (self)
        return self->HandleMessage(message, wParam, lParam);

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateControls();
        return 0;

    case WM_APP_LOG:
    case WM_APP_STATUS:
    {
        auto* text = reinterpret_cast<std::wstring*>(lParam);

        if (text)
        {
            if (message == WM_APP_STATUS)
            {
                SetWindowTextW(statusLabel, text->c_str());
            }
            else
            {
                int length = GetWindowTextLengthW(trafficLog);

                SendMessageW(trafficLog, EM_SETSEL, length, length);

                std::wstring line = *text + L"\r\n";

                SendMessageW(trafficLog, EM_REPLACESEL, FALSE,
                    reinterpret_cast<LPARAM>(line.c_str()));
            }

            delete text;
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

        case ID_SEND_QUEUE:
            StartQueue();
            return 0;

        case ID_STOP_QUEUE:
            StopQueue();
            return 0;

        case ID_CLEAR_LOG:
            SetWindowTextW(trafficLog, L"");
            return 0;
        }
        break;

    case WM_CLOSE:
        DisconnectFromServer();
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int MainWindow::Run(HINSTANCE instance, int showCommand)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Shaiya Packet Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 680,
        nullptr, nullptr, instance, this);

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG message = {};

    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

int RunApplication(HINSTANCE instance, int showCommand)
{
    WSADATA data = {};

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return 1;

    MainWindow application;

    int result = application.Run(instance, showCommand);

    WSACleanup();

    return result;
}
