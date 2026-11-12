#include <windows.h>
#include <iostream>
#include <string>

using StartFunction = BOOL(WINAPI*)(HINSTANCE);
using StopFunction  = BOOL(WINAPI*)();

int wmain()
{
    std::wcout << L"ShaiyaPacketTool TestLoader\n";
    std::wcout << L"DLL yukleniyor...\n";

    HMODULE dll = LoadLibraryW(L"ShaiyaPacketTool.dll");

    if (!dll)
    {
        std::wcerr << L"DLL yuklenemedi. Windows hata kodu: "
                   << GetLastError() << L"\n";
        std::wcout << L"Devam etmek icin Enter'a basin...";
        std::wstring input;
        std::getline(std::wcin, input);
        return 1;
    }

    auto start = reinterpret_cast<StartFunction>(
        GetProcAddress(dll, "ShaiyaPacketTool_Start")
    );

    auto stop = reinterpret_cast<StopFunction>(
        GetProcAddress(dll, "ShaiyaPacketTool_Stop")
    );

    if (!start || !stop)
    {
        std::wcerr << L"DLL icindeki baslatma/durdurma fonksiyonlari bulunamadi.\n";
        FreeLibrary(dll);
        return 2;
    }

    if (!start(reinterpret_cast<HINSTANCE>(dll)))
    {
        std::wcerr << L"DLL arayuzu baslatilamadi.\n";
        FreeLibrary(dll);
        return 3;
    }

    std::wcout << L"DLL arayuzu baslatildi.\n";
    std::wcout << L"Test bitince bu pencereye donup Enter'a basin.\n";

    std::wstring input;
    std::getline(std::wcin, input);

    if (!stop())
    {
        std::wcerr << L"DLL durdurulamadi. Guvenlik icin DLL serbest birakilmadi.\n";
        return 4;
    }

    FreeLibrary(dll);

    std::wcout << L"Test tamamlandi. DLL kapatildi.\n";
    return 0;
}
